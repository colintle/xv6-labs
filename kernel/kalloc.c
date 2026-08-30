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
} kmem[NCPU];

void
kinit()
{
  for (int i = 0; i < NCPU; i++){
    initlock(&kmem[i].lock, "kmem");
  }
  // initlock(&kmem.lock, "kmem");
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

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int id = cpuid();

  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);

  pop_off(); 
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void) {
  struct run *r = 0;

  push_off();
  int id = cpuid();

  // Try local CPU.
  acquire(&kmem[id].lock);

  r = kmem[id].freelist;
  if (r)
    kmem[id].freelist = r->next;

  release(&kmem[id].lock);

  // Local list empty: find a donor CPU and steal its ENTIRE freelist
  // in one O(1) pointer swap, instead of a fixed-size batch. Splicing
  // a whole list costs the same single assignment whether it holds 1
  // page or 30000, so this keeps both the number of remote lock
  // acquisitions AND the time each is held to a minimum -- unlike a
  // partial/half steal, which must walk N nodes to find the split
  // point while holding the donor's lock.
  if (r == 0) {
    for (int off = 1; off < NCPU; off++) {
      int i = (id + off) % NCPU;

      acquire(&kmem[i].lock);
      r = kmem[i].freelist;
      if (r)
        kmem[i].freelist = 0;
      release(&kmem[i].lock);

      if (r) {
        // Hand ourselves the first page, keep the rest locally.
        acquire(&kmem[id].lock);
        kmem[id].freelist = r->next;
        release(&kmem[id].lock);
        break;
      }
    }
  }

  pop_off();

  if (r)
    memset((char *)r, 5, PGSIZE);

  return (void *)r;
}
