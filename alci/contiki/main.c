/*
 * Contiki 1.x desktop (CTK) on ELKS/Alci, CGA prototype (task D7).
 * Main loop, keyboard, mouse and clock for ELKS. Ctrl-Q quits.
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
#include "edit-dsc.h"
#include "gfx.h"
#include "gfx-icons.h"            /* Alci icons of this screen type (icons/tools/mkgfx.py) */
#include "ctk-mouse.h"
#include "mouse.h"

void ctk_cga_exit(void);
void calc_init(char *arg);
void about_init(char *arg);
void edit_init(char *arg);
extern struct ctk_icon about_icon, calc_icon, edit_icon;

/* Give a program's desktop icon the Alci drawing of the same name. */
static void
set_icon(struct ctk_icon *icon, const char *name)
{
  unsigned char i;
  /* the area the mouse finds it in: the drawing's cells and the title row
     under them (CTK counts both ends) */
  icon->w = GFX_ICON_CW - 1;
  icon->h = GFX_ICON_RH;
  for(i = 0; i < GFX_ICON_COUNT; ++i) {
    if(strcmp(gfx_icon_names[i], name) == 0) {
      icon->bitmap = (unsigned char *)&gfx_icons[i];
      return;
    }
  }
  icon->bitmap = NULL;                  /* title only */
}

extern unsigned char ctk_cga_wallpaper;
unsigned char ctk_cga_wallpaper_find(const char *name);

static struct termios saved;
static unsigned char keybuf[16];
static unsigned char khead, ktail;

/*---------------------------------------------------------------------------*/
/* Mouse (mouse.c): a Microsoft serial mouse on /dev/ttyS0 unless MOUSE_TYPE
   (ms, pc, ps2, none) and MOUSE_PORT say otherwise. The pointer appears with
   the first movement, so a port with no mouse changes nothing. */
void ctk_gfx_pointer(unsigned short x, unsigned short y);
void ctk_gfx_pointer_off(void);
#define MPW ((unsigned short)GFX_SW * 8)        /* screen, pixels */
#define MPH ((unsigned short)GFX_SH * GFX_CH)
/* the position in half pixels; a mouse count is a pixel across on 640
   pixel screens, half a pixel on 320; a pixel down on 200 lines, two on
   taller screens, about the same distance either way on the glass */
#if GFX_SW * 8 >= 640
#define MDX 2
#else
#define MDX 1
#endif
#if GFX_SH * GFX_CH > 200
#define MDY 2
#else
#define MDY 1
#endif
static int mfx = MPW, mfy = MPH;        /* half pixels: the middle of the screen */
static unsigned char mbut, mseen, mwant, mreal;   /* mreal: a mouse has moved */

/* The pointer from the keyboard, for a machine with no mouse: F6 or Alt-M
   turns it on and off; the arrows move it a cell at a time, Space or Enter
   click, Esc leaves. CTK finds things by cell, so a cell is enough. */
static unsigned char kmode;             /* arrows move the pointer */
static unsigned char kclick;            /* 1: press given, 2: seen by CTK */
static void
kmode_toggle(void)
{
  kmode = !kmode;
  if(kmode) {
    /* where the mouse is, or the middle of the screen, on a cell */
    mfx = ((mfx >> 4) << 4) + 6;        /* 3 pixels into the cell */
    mfy = (mfy / (2 * GFX_CH)) * 2 * GFX_CH + GFX_CH / 2 * 2;
    mseen = 1;
    mwant = 1;
  }
}
static void
kmode_move(int dx, int dy)
{
  mfx += dx * 16;                       /* half pixels: 8 pixels a cell */
  mfy += dy * 2 * GFX_CH;
  if(mfx < 0) {
    mfx += 16;
  } else if(mfx > 2 * MPW - 1) {
    mfx -= 16;
  }
  if(mfy < 0) {
    mfy += 2 * GFX_CH;
  } else if(mfy > 2 * MPH - 1) {
    mfy -= 2 * GFX_CH;
  }
  mwant = 1;
}

void
ctk_mouse_init(void)
{
  /* the port is opened once the desktop is on the screen (main): on a
     floppy, finding /dev/ttyS0 can take a second */
}
/* 0, 0 until the mouse first moves, where CTK starts it: no movement (and
   no redraw) for the desktop to act on before that */
unsigned short ctk_mouse_x(void) { return mseen ? mfx >> 1 : 0; }
unsigned short ctk_mouse_y(void) { return mseen ? mfy >> 1 : 0; }
unsigned char ctk_mouse_xtoc(unsigned short x) { return x >> 3; }
unsigned char ctk_mouse_ytoc(unsigned short y) { return y / GFX_CH; }
unsigned char
ctk_mouse_button(void)
{
  if(kclick == 1) {
    kclick = 2;                         /* CTK has seen the press */
  }
  return mbut;
}
void ctk_mouse_hide(void) { mwant = 0; }
void ctk_mouse_show(void) { mwant = 1; }

/* the packets waiting; stops at a change of the buttons, so that the
   desktop sees each press and each release */
static void
mouse_read(void)
{
  int dx, dy, dz, b;
  while(read_mouse(&dx, &dy, &dz, &b) == 1) {
    if(dx != 0 || dy != 0 || b != 0) {
      mseen = 1;
      mreal = 1;
    }
    mfx += dx * MDX;
    mfy += dy * MDY;
    if(mfx < 0) {
      mfx = 0;
    } else if(mfx > 2 * MPW - 1) {
      mfx = 2 * MPW - 1;
    }
    if(mfy < 0) {
      mfy = 0;
    } else if(mfy > 2 * MPH - 1) {
      mfy = 2 * MPH - 1;
    }
    if(b != mbut) {
      mbut = b;
      break;
    }
  }
}
/* the pointer drawn where the mouse is, once the desktop has drawn */
static void
mouse_sync(void)
{
  if(kclick == 2) {                     /* a click from the keyboard: release */
    kclick = 0;
    mbut = 0;
  }
  if(mseen && mwant && (mreal || kmode)) {
    ctk_gfx_pointer(mfx >> 1, mfy >> 1);
  } else {
    ctk_gfx_pointer_off();
  }
}

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
  close_mouse();
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
   ELKS console: arrows are ESC [ A..D, Home and End ESC [ H and F, PgUp and
   PgDn ESC [ 5 ~ and 6 ~, F1..F10 ESC a..j, Delete 0x7f, Backspace 0x08. */
static void
poll_keys(int ms)
{
  fd_set fds;
  struct timeval tv;
  unsigned char b[16];
  int n, i;

  FD_ZERO(&fds);
  FD_SET(0, &fds);
  if(mouse_fd >= 0) {
    FD_SET(mouse_fd, &fds);
  }
  tv.tv_sec = 0;
  tv.tv_usec = (long)ms * 1000;
  if(select(mouse_fd >= 0 ? mouse_fd + 1 : 1, &fds, NULL, NULL, &tv) <= 0) {
    return;
  }
  if(mouse_fd >= 0 && FD_ISSET(mouse_fd, &fds)) {
    mouse_read();
  }
  if(!FD_ISSET(0, &fds)) {
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
      if(d == '[' && i + 2 < n && kmode) {
        switch(b[i + 2]) {
        case 'A': kmode_move(0, -1); break;
        case 'B': kmode_move(0, 1); break;
        case 'C': kmode_move(1, 0); break;
        case 'D': kmode_move(-1, 0); break;
        }
        i += 2;
        continue;
      }
      if(d == 'f' || d == 'M') {        /* F6, Alt-M: the pointer from the keys */
        kmode_toggle();
        i += 1;
        continue;
      }
      if(d == '[' && i + 2 < n) {
        switch(b[i + 2]) {
        case 'A': push(CH_CURS_UP); break;
        case 'B': push(CH_CURS_DOWN); break;
        case 'C': push(CH_CURS_RIGHT); break;
        case 'D': push(CH_CURS_LEFT); break;
        case 'H': push(CH_HOME); break;
        case 'F': push(CH_END); break;
        case '5': push(CH_PGUP); break;
        case '6': push(CH_PGDN); break;
        }
        i += 2;
        if(i + 1 < n && b[i + 1] == '~') {    /* ESC [ 5 ~ and the like */
          ++i;
        }
        continue;
      }
      if(d >= 'a' && d <= 'e') {
        push(CH_F1 + (d - 'a'));
        i += 1;
        continue;
      }
    }
    if(kmode && c == 0x1b) {            /* Esc alone leaves */
      kmode = 0;
      continue;
    }
    if(kmode && (c == ' ' || c == '\r' || c == '\n')) {
      if(kclick == 0) {                 /* press now, release once CTK has seen it */
        mbut = BUTTON_L;
        kclick = 1;
      }
      continue;
    }
    if(c == '\n') {
      c = '\r';
    }
    if(c == 0x7f) {                     /* the Delete key on the console */
      c = CH_DELFWD;
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

  {
    /* wallpaper: ctk -w dots|pairs|slash|none, or CTK_WALLPAPER in the
       environment; dots by default, none on Hercules */
    const char *wp = getenv("CTK_WALLPAPER");
    if(argc > 2 && strcmp(argv[1], "-w") == 0) {
      wp = argv[2];
      argv += 2;
      argc -= 2;
    }
    if(wp != NULL) {
      unsigned char n = ctk_cga_wallpaper_find(wp);
      if(n == 255) {
        fprintf(stderr, "ctk: unknown wallpaper\n");
        return 1;
      }
      ctk_cga_wallpaper = n;
    }
  }
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
  set_icon(&about_icon, "about");
  set_icon(&calc_icon, "calculator");
  set_icon(&edit_icon, "editor");
  program_handler_add(&about_dsc, "About", 1);
  program_handler_add(&calc_dsc, "Calculator", 1);
  program_handler_add(&edit_dsc, "Editor", 1);

  if(argc > 1 && strcmp(argv[1], "-b") == 0) {
    /* Benchmark: timings in ms, printed after going back to text mode. */
    clock_time_t t[8];
    const char *keys = "2*21=";
    t[0] = clock_time();                /* desktop drawn by ctk_init */
    while(ek_run()) ;
    t[1] = clock_time();
    calc_init(NULL);                    /* open the calculator window */
    while(ek_run()) ;
    t[2] = clock_time();
    while(*keys) {                      /* five keys into the calculator */
      push((unsigned char)*keys++);
      while(ek_run() || khead != ktail) ;
    }
    t[3] = clock_time();
    about_init(NULL);                   /* open the About dialog */
    while(ek_run()) ;
    t[4] = clock_time();
    ctk_draw_clear(0, ctk_draw_height());   /* repaint the whole background */
    t[5] = clock_time();
    ctk_draw_clear(1, ctk_draw_height() - 1);   /* the desktop rows alone */
    t[6] = clock_time();
    ctk_draw_clear(ctk_draw_height() - 1, ctk_draw_height());   /* the status bar */
    t[7] = clock_time();
    ctk_cga_exit();
    tcsetattr(0, TCSANOW, &saved);
    printf("ctk ms: settle %lu calc %lu keys %lu\n", t[1] - t[0], t[2] - t[1], t[3] - t[2]);
    printf("ctk ms: dialog %lu background %lu\n", t[4] - t[3], t[5] - t[4]);
    printf("ctk ms: rows %lu status %lu\n", t[6] - t[5], t[7] - t[6]);
    return 0;
  }
  if(argc > 1 && strcmp(argv[1], "-d") == 0) {
    /* Demonstration: the calculator already open */
    while(ek_run()) ;
    calc_init(NULL);
  }
  if(argc > 1 && strcmp(argv[1], "-e") == 0) {
    /* the editor open, on a file if one is named */
    while(ek_run()) ;
    edit_init(argc > 2 ? argv[2] : NULL);
  }
  if(argc > 1 && strcmp(argv[1], "-q") == 0) {
    while(ek_run()) ;                   /* draw the desktop, then quit */
    quit();
  }

  while(ek_run()) ;                     /* the desktop first, then the mouse */
  open_mouse();                         /* no mouse: mouse_fd stays -1 */
  for(;;) {
    if(ek_run() == 0 && khead == ktail) {
      mouse_sync();
      poll_keys(50);                    /* idle: wait for a key or the mouse, no busy loop */
    }
  }
  return 0;
}
