/*
 * Contiki 1.x desktop (CTK) on ELKS/Alci, CGA prototype (task D7).
 * Main loop, keyboard and clock for ELKS. Ctrl-Q quits.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#include "ek.h"
#include "ctk.h"
#include "ctk-draw.h"
#include "program-handler.h"
#include "clock.h"

#include "about-dsc.h"
#include "calc-dsc.h"

void ctk_cga_exit(void);

static struct termios saved;
static unsigned char keybuf[16];
static unsigned char khead, ktail;

/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (clock_time_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
/*---------------------------------------------------------------------------*/
static void
quit(void)
{
  ctk_cga_exit();
  tcsetattr(0, TCSANOW, &saved);
  exit(0);
}
/*---------------------------------------------------------------------------*/
static void
push(unsigned char c)
{
  unsigned char n = (khead + 1) & 15;
  if(n != ktail) {
    keybuf[khead] = c;
    khead = n;
  }
}
/*---------------------------------------------------------------------------*/
/* Read whatever the console has, without blocking longer than "ms".
   ELKS console: arrows are ESC [ A..D, F1..F10 are ESC a..j. */
static void
poll_keys(int ms)
{
  fd_set fds;
  struct timeval tv;
  unsigned char b[16];
  int n, i;

  FD_ZERO(&fds);
  FD_SET(0, &fds);
  tv.tv_sec = 0;
  tv.tv_usec = (long)ms * 1000;
  if(select(1, &fds, NULL, NULL, &tv) <= 0) {
    return;
  }
  n = read(0, b, sizeof(b));
  for(i = 0; i < n; ++i) {
    unsigned char c = b[i];
    if(c == 0x11) {                      /* Ctrl-Q */
      quit();
    }
    if(c == 0x1b && i + 1 < n) {
      unsigned char d = b[i + 1];
      if(d == '[' && i + 2 < n) {
        switch(b[i + 2]) {
        case 'A': push(CH_CURS_UP); break;
        case 'B': push(CH_CURS_DOWN); break;
        case 'C': push(CH_CURS_RIGHT); break;
        case 'D': push(CH_CURS_LEFT); break;
        }
        i += 2;
        continue;
      }
      if(d >= 'a' && d <= 'e') {
        push(CH_F1 + (d - 'a'));
        i += 1;
        continue;
      }
    }
    if(c == '\n') {
      c = '\r';
    }
    if(c == 0x7f) {
      c = CH_DEL;
    }
    push(c);
  }
}
/*---------------------------------------------------------------------------*/
unsigned char
ctk_arch_keyavail(void)
{
  if(khead == ktail) {
    poll_keys(0);
  }
  return khead != ktail;
}
/*---------------------------------------------------------------------------*/
ctk_arch_key_t
ctk_arch_getkey(void)
{
  unsigned char c;
  if(khead == ktail) {
    return 0;
  }
  c = keybuf[ktail];
  ktail = (ktail + 1) & 15;
  return c;
}
/*---------------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
  struct termios t;

  tcgetattr(0, &saved);
  t = saved;
  t.c_lflag &= ~(ICANON | ECHO | ISIG);
  t.c_iflag &= ~(ICRNL | IXON);
  t.c_cc[VMIN] = 0;
  t.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &t);

  ek_init();
  ctk_init();
  program_handler_init();
  program_handler_add(&about_dsc, "About", 1);
  program_handler_add(&calc_dsc, "Calculator", 1);

  for(;;) {
    if(ek_run() == 0 && khead == ktail) {
      poll_keys(50);                    /* idle: wait for a key, no busy loop */
    }
  }
  return 0;
}
