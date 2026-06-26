// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  int refcount[(PHYSTOP - KERNBASE) / PGSIZE]; // Reference count for each page
} ref;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock, "ref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  uint64 pa_addr = (uint64)pa;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&ref.lock);
  // For pages never allocated (initially added to freelist), refcount is 0
  // Just add to freelist without decrementing
  if(ref.refcount[(pa_addr - KERNBASE) / PGSIZE] == 0) {
    release(&ref.lock);
  } else {
    // Decrement refcount and check if we should free
    ref.refcount[(pa_addr - KERNBASE) / PGSIZE]--;
    if(ref.refcount[(pa_addr - KERNBASE) / PGSIZE] > 0) {
      release(&ref.lock);
      return;  // still have references, don't free
    }
    release(&ref.lock);
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&ref.lock);
    ref.refcount[((uint64)r - KERNBASE) / PGSIZE] = 1;
    release(&ref.lock);
  }
  return (void*)r;
}

// Increment reference count for page pa.
void
krefcount_increment(void *pa)
{
  uint64 pa_addr = (uint64)pa;
  acquire(&ref.lock);
  if(pa_addr >= KERNBASE && pa_addr < PHYSTOP)
    ref.refcount[(pa_addr - KERNBASE) / PGSIZE]++;
  release(&ref.lock);
}
