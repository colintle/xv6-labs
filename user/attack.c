#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

int
main(int argc, char *argv[])
{
  char *mem = sbrk(32 * 4096);
  // the source string offset is 67840
  // the copied string offset is 69648
  // will need to investigate why this is
  for(int i = 0; i < 32 * 4096 - 16; i++){
    if(mem[i] == 'T' &&
       mem[i + 1] == 'h' &&
       mem[i + 2] == 'i' &&
       mem[i + 3] == 's'){

      printf("%d\n", i);
      char *secret = mem + i + 16; 

      while(*secret != '\0'){
        printf("%c", *secret);
        secret++;
      }
      printf("\n");
    }
  }

  exit(0);
}
