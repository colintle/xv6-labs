#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  printf("User Program: Number of free pages -> %d\n", freepages());
  exit(0);
}
