/*
 * vwatch: when did the screen stop changing? (D2 benchmark scripts,
 * alci/tests/guibench)
 *
 * vwatch SEG SECONDS: every 50 ms, reads 256 bytes spread over 16 KB of
 * video memory at segment SEG (b800 for CGA, a000 for EGA and VGA); a
 * change is 3 of them or more different from the last reading (fewer
 * is a pointer drawn again); when there has been no change for SECONDS
 * after at least one, it prints "ready <ms>", the time of the last
 * change, in the same clock as stamp. Started in the background before a desktop, it gives the
 * desktop's start time whatever the desktop is, without asking it.
 *
 * Copyright (c) 2026 the Alci contributors; GPL-2.0 like the rest of
 * the tree.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

static unsigned long
ms(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec % 100000UL) * 1000UL + tv.tv_usec / 1000;
}

int
main(int argc, char **argv)
{
  unsigned int seg = argc > 1 ? (unsigned int)strtoul(argv[1], NULL, 16)
                              : 0xb800;
  unsigned long quiet = (argc > 2 ? atoi(argv[2]) : 3) * 1000UL;
  unsigned long last, now;
  static unsigned char old[256];
  unsigned int i, n;
  int changed = 0;
  const unsigned char __far *v =
    (const unsigned char __far *)((unsigned long)seg << 16);

  for(i = 0; i < 256; ++i)
    old[i] = v[i << 6];
  last = ms();
  for(;;) {
    usleep(50000);
    for(n = 0, i = 0; i < 256; ++i)
      if(v[i << 6] != old[i]) {
        old[i] = v[i << 6];
        ++n;
      }
    now = ms();
    if(n >= 3) {
      last = now;
      changed = 1;
    } else if(changed && now - last >= quiet) {
      break;
    }
  }
  printf("ready %lu\n", last);
  return 0;
}
