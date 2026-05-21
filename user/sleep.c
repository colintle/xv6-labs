
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int seconds;

  if (argc != 2){
    fprintf(2, "Usage: sleep requires a single numeric value\n");
    exit(1);
  }
  seconds = atoi(argv[1]);
  pause(seconds * 10);
  exit(0);
}
