/*
 * memlater: wait, then report the memory a program could still get (D2
 * benchmark scripts, alci/tests/guibench). Started before a desktop that
 * leaves too little memory to start meminfo, it is already loaded when
 * the desktop is up: it takes main memory in blocks until none is left
 * and prints "free <K> largest <K>" (all of it, and the largest block,
 * the most a program's code or data can have), then gives it back by
 * exiting. memlater SECONDS [PIDFILE]: with PIDFILE, it then kills
 * (SIGKILL) the process whose pid is written there, for a desktop that
 * leaves no memory to start kill.
 * Copyright (c) 2026 the Alci contributors; GPL-2.0 like the tree.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <signal.h>
#include <fcntl.h>

int
main(int argc, char **argv)
{
  int s = argc > 1 ? atoi(argv[1]) : 60;
  unsigned int paras = 4096, largest = 0;
  unsigned long total = 0;
  unsigned short seg;

  while(s-- > 0)
    usleep(1000000L);
  while(paras >= 16) {
    if(_fmemalloc(paras, &seg) == 0) {
      total += paras;
      if(paras > largest)
        largest = paras;
    } else {
      paras >>= 1;
    }
  }
  printf("free %luK largest %uK\n", total >> 6, largest >> 6);
  fflush(stdout);
  if(argc > 2) {
    char b[8];
    int fd = open(argv[2], O_RDONLY), n = 0;
    if(fd >= 0)
      n = read(fd, b, sizeof(b) - 1);
    if(n > 0) {
      b[n] = 0;
      kill(atoi(b), SIGKILL);
    }
  }
  return 0;
}
