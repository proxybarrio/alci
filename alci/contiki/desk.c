/*
 * desk: start the Contiki desktop built for the screen of this machine
 * (task D7). Asks the video BIOS what is fitted and runs ctk-vga (VGA),
 * ctk-mcga (MCGA), ctk-ega (EGA), ctk-vga2 or ctk-ega2 (VGA, MCGA or EGA
 * on a monochrome monitor), ctk-herc (Hercules), ctk-att (AT&T 6300,
 * Olivetti M24: an Olivetti BIOS) or ctk (CGA, also LCD and the rest),
 * passing on its arguments.
 * DESK=cga|cga4|ega|vga|mcga|herc|att|ega2|vga2 chooses by hand.
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
static unsigned short
equipment(void)
{
  unsigned short ax;
  __asm__ volatile ("int $0x11" : "=a" (ax) : : "memory", "cc");
  return ax;
}
static unsigned char
inb(unsigned short port)
{
  unsigned char v;
  __asm__ volatile ("inb %%dx,%%al" : "=a" (v) : "d" (port));
  return v;
}

/* Hercules: bit 7 of the status port changes with the vertical retrace;
   on an MDA it stays put */
static int
hercules(void)
{
  unsigned char a = inb(0x3ba) & 0x80;
  unsigned short i;
  for(i = 0; i < 32768; ++i) {
    if((inb(0x3ba) & 0x80) != a) {
      return 1;
    }
  }
  return 0;
}

/* AT&T 6300, Olivetti M24 and kin: an Olivetti BIOS, which has mode 40h
   (640x400); found by its name in the ROM, not by trying the mode, which
   another BIOS may not take well */
static int
att(void)
{
  const char __far *p = (const char __far *)0xF000C000UL;
  unsigned short i;
  for(i = 0; i < 0x4000 - 8; ++i) {
    if(p[i] == 'O' && p[i + 1] == 'L' && p[i + 2] == 'I' && p[i + 3] == 'V' &&
       p[i + 4] == 'E' && p[i + 5] == 'T' && p[i + 6] == 'T' && p[i + 7] == 'I') {
      return 1;
    }
  }
  return 0;
}

static const char *
detect(void)
{
  /* VGA BIOS: function 1Ah returns AL=1Ah and the active display in BL */
  if((bios10_ax(0x1a00) & 0xff) == 0x1a) {
    unsigned char d = bios10(0x1a00, 0) & 0xff;
    if(d == 8) {
      return "vga";
    }
    if(d == 7 || d == 0x0b) {
      return "vga2";                    /* VGA or MCGA, monochrome monitor */
    }
    if(d == 0x0c) {
      return "mcga";
    }
    if(d == 4) {
      return "ega";
    }
    if(d == 5) {
      return "ega2";
    }
    if(d == 2) {
      return att() ? "att" : "cga";
    }
    if(d == 1) {
      return hercules() ? "herc" : "mda";
    }
  }
  /* EGA BIOS: function 12h, BL=10h returns the memory size in BL and
     BH=1 for a monochrome monitor */
  {
    unsigned short bx = bios10(0x1200, 0x0010);
    if((bx & 0xff) != 0x10) {
      return (bx >> 8) == 1 ? "ega2" : "ega";
    }
  }
  /* the equipment word says 80x25 monochrome: MDA or Hercules */
  if((equipment() & 0x30) == 0x30) {
    return hercules() ? "herc" : "mda";
  }
  return att() ? "att" : "cga";
}

int
main(int argc, char **argv)
{
  static char path[32];
  const char *t = getenv("DESK");
  if(t == NULL) {
    t = detect();
  }
  if(strcmp(t, "mda") == 0) {
    write(2, "desk: an MDA has no graphics\n", 29);
    return 1;
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
