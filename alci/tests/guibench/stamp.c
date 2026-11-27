/* stamp: print the time in ms (D2 benchmark scripts, alci/tests/guibench).
   stamp NAME prints "NAME <ms>". Copyright (c) 2026 the Alci contributors;
   GPL-2.0 like the rest of the tree. */
#include <stdio.h>
#include <sys/time.h>

int
main(int argc, char **argv)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  printf("%s %lu\n", argc > 1 ? argv[1] : "t",
         (tv.tv_sec % 100000UL) * 1000UL + tv.tv_usec / 1000);
  return 0;
}
