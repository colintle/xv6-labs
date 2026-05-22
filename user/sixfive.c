#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int n;
  char buf[32];
  int valid = 0;
  int counter = 0;
  int fd;
  int int_buf;

  for (int i = 1; i < argc; i++) {

    fd = open(argv[i], 0);

    if (fd == -1) {
      fprintf(2, "sixfive: Fail to open file");
      exit(1);
    }

    while ((n = read(fd, buf, 1)) > 0) {
      if (strchr(" -\r\t\n./,", buf[0])) {
        if (valid && ((counter % 5 == 0) || (counter % 6 == 0))) {
          fprintf(2, "%d\n", counter);
        }
        counter = 0;
        valid = 0;
        continue;
      }
      // fprintf(2, "buffer: %s\n", buf);
      
      int_buf = atoi(&buf[0]);
      // Case 1: abc123
      // Case 2: 123abc
      if (counter == -1 || int_buf + '0' != buf[0]){
        counter = -1;
        continue;
      }
      // fprintf(2, "int_buf: %d\n", int_buf);
      counter = counter * 10 + int_buf; 
      valid = 1;
      // fprintf(2, "Counter: %d\n", counter);
    }
    if (valid && ((counter % 5 == 0) || (counter % 6 == 0))) {
      fprintf(2, "%d\n", counter);
    }
    counter = 0;
    valid = 0;
  }

  exit(0);
}
