/*
 * =====================================================================================
 *
 *       Filename:  ioctl.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  08/27/17 14:18:42
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  linsheng.pan (), life_is_legend@163.com
 *   Organization:
 *
 * =====================================================================================
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "command.h"

int main(int argc, char* argv[]) {
  int fd = open("/dev/hello0", O_RDWR);
  if (fd < 0) {
    perror("open error:");
    return -1;
  }

  if (ioctl(fd, HELLO_RESET) < 0) {
    perror("error:");
    return -1;
  }

  int val = 10;

  if (ioctl(fd, HELLO_WRITE, &val) < 0) {
    perror("write error:");
    return -1;
  }

  val = 2;
  if (ioctl(fd, HELLO_READ, &val) < 0) {
    perror("read error");
    return -1;
  }

  printf("val = %d\n", val);

  return 0;
}
