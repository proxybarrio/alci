/* textmode: back to the 80x25 text mode (BIOS mode 3) and give the console
   back (DCREL_KRAW, DCREL_GRAPH), for after a graphical program stopped by force
   (D2 benchmark scripts, alci/tests/guibench). Run it with the console as
   standard input. Copyright (c) 2026 the Alci contributors; GPL-2.0 like
   the rest of the tree. */
#include <sys/ioctl.h>
#include <linuxmt/ntty.h>

int
main(void)
{
  __asm__ volatile("push %%bp; mov $3, %%ax; int $0x10; pop %%bp"
                   : : : "ax", "bx", "cx", "dx", "si", "di", "memory");
  ioctl(0, DCREL_KRAW);              /* the raw keyboard too */
  ioctl(0, DCREL_GRAPH);
  return 0;
}
