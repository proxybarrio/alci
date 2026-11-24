/*
 * desk: start the Contiki desktop built for the screen of this machine
 * (task D7). Asks the video BIOS what is fitted and runs ctk-vga (VGA),
 * ctk-mcga (MCGA), ctk-ega (EGA) or ctk (CGA, also LCD and the rest),
 * passing on its arguments. DESK=cga|cga4|ega|vga|mcga chooses by hand.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static unsigned short
bios10(unsigned short ax, unsigned short bx)
{
  __asm__ volatile ("push %%bp\n\tint $0x10\n\tpop %%bp"
                    : "+a" (ax), "+b" (bx)
                    : : "cx", "dx", "si", "di", "memory", "cc");
  return bx;
}
static unsigned short
bios10_ax(unsigned short ax)
{
  unsigned short bx = 0;
  __asm__ volatile ("push %%bp\n\tint $0x10\n\tpop %%bp"
                    : "+a" (ax), "+b" (bx)
                    : : "cx", "dx", "si", "di", "memory", "cc");
  return ax;
}

static const char *
detect(void)
{
  /* VGA BIOS: function 1Ah returns AL=1Ah and the active display in BL */
  if((bios10_ax(0x1a00) & 0xff) == 0x1a) {
    unsigned char d = bios10(0x1a00, 0) & 0xff;
    if(d == 7 || d == 8) {
      return "vga";
    }
    if(d == 0x0b || d == 0x0c) {
      return "mcga";
    }
    if(d == 4 || d == 5) {
      return "ega";
    }
    if(d == 2) {
      return "cga";
    }
  }
  /* EGA BIOS: function 12h, BL=10h returns the memory size in BL */
  if((bios10(0x1200, 0x0010) & 0xff) != 0x10) {
    return "ega";
  }
  return "cga";
}

int
main(int argc, char **argv)
{
  static char path[32];
  const char *t = getenv("DESK");
  if(t == NULL) {
    t = detect();
  }
  strcpy(path, "/bin/ctk");
  if(strcmp(t, "cga") != 0) {
    strcat(path, "-");
    strcat(path, t);
  }
  argv[0] = path;
  execv(path, argv);
  write(2, "desk: cannot run ", 17);
  write(2, path, strlen(path));
  write(2, "\n", 1);
  return 1;
}
