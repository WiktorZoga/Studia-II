#include "csapp.h"

#define BUFFSIZE 4096

int main(int argc, char* argv[]) {
  int n;
	
  int fd;	// file descriptor

  char buf[BUFFSIZE];

  if (argc != 2)
	  app_error("no file specified");

  fd = open(argv[1], O_RDONLY);

  if (fd < 0)
	  app_error("open error");

  while ((n = read(fd, buf, BUFFSIZE)) > 0)
    if (write(STDOUT_FILENO, buf, n) != n)
      app_error("write error");

  if (n < 0)
    app_error("read error");

  close(fd);

  exit(0);
}
