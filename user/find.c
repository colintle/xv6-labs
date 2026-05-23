#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "user/user.h"

char *exec_argv[MAXARG];
int exec_argc = 0;

char *fmtname(char *path) {
  char *p;

  // Find first character after last slash.
  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  return p;
}

void find(char *path, char *file_pattern) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type) {
  case T_DEVICE:
  case T_FILE:
    return;
  case T_DIR:
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if (stat(buf, &st) < 0) {
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      if (st.type == T_FILE) {
        if (strcmp(fmtname(buf), file_pattern) == 0) {
          if (exec_argc == 0) {
            printf("%s\n", buf);
          }
          else {
            int pid = fork();
            if (pid == 0) {
              exec_argv[exec_argc] = buf;
              exec(exec_argv[0], exec_argv);
              exit(0);
            } else {
              wait(0);
            }
          }
        }
      }
      if (st.type == T_DIR) {
        if (strcmp(fmtname(buf), ".") == 0 || strcmp(fmtname(buf), "..") == 0) {
          continue;
        }
        find(buf, file_pattern);
      }
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[]) {

  if (argc > 3 && strcmp(argv[3], "-exec") == 0) {
    for (int i = 4; i < argc && exec_argc < MAXARG - 1; i++, exec_argc++) {
      exec_argv[exec_argc] = argv[i];
    }
  }
  find(argv[1], argv[2]);

  exit(0);
  return 0;
}
