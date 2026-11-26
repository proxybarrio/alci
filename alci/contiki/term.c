/*
 * Terminal window for the CTK desktop on ELKS/Alci (task D7): a shell, or
 * any ELKS program, run on a pseudo-terminal (/dev/ptypN and /dev/ttypN)
 * in a window as large as the screen. The program is an ELKS process of
 * its own, scheduled by the kernel like any other; the desktop only reads
 * what it writes, when the event loop sees data (watch.h), and sends it
 * the keys.
 *
 * The screen understands what ELKS programs send to the console: CR, LF,
 * Backspace, Tab, and the ANSI sequences ESC [ n A B C D (cursor), H and f
 * (position), J and K (clear), m (ignored). TERM=ansi, COLUMNS and LINES
 * tell the program the window's size.
 *
 * Up to NTERM windows at once, each with its own pty and program (ELKS has
 * four ptys). A window's text is kept in far memory and the lines as drawn
 * in the heap, both taken when it opens and given back when it closes, so
 * a closed window costs only its labels. The pty is opened as telnetd does
 * (elkscmd/inet/telnetd.c); closing the window hangs the program up.
 *
 * Copyright (c) 2026, the Alci contributors. BSD 3-clause licence, the
 * terms of Contiki 1.x.
 */
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <malloc.h>
#include <sys/wait.h>

#include "ek.h"
#include "ctk.h"
#include "loader.h"
#include "arg.h"
#include "gfx.h"
#include "watch.h"
#include "mouse.h"

#define TW (GFX_SW - 2)                 /* a window: columns */
#define TH (GFX_SH - 4)                 /* and lines */
#define NTERM 3                         /* windows at once */

struct term {
  struct ctk_window window;
  struct ctk_label labels[TH];
  char title[8];                        /* "Shell 1" */
  char __far *screen;                   /* the text, TH lines of TW */
  char *shown;                          /* the lines as drawn, cursor included */
  unsigned char cx, cy;                 /* cursor */
  unsigned char esc, npar;              /* escape sequence: state, parameters */
  unsigned char par[2];
  int ptyfd;
  pid_t child;
  ek_id_t id;
  unsigned char used;
};
static struct term terms[NTERM];

EK_EVENTHANDLER(term_eventhandler, ev, data);
EK_PROCESS(p0, "Shell", EK_PRIO_NORMAL, term_eventhandler, NULL, &terms[0]);
EK_PROCESS(p1, "Shell", EK_PRIO_NORMAL, term_eventhandler, NULL, &terms[1]);
EK_PROCESS(p2, "Shell", EK_PRIO_NORMAL, term_eventhandler, NULL, &terms[2]);
static struct ek_proc * const procs[NTERM] = {&p0, &p1, &p2};
static ek_event_t ev_data;              /* a pty has something to read */
static char envcols[12], envlines[10];

#define CELL(t, y, x) ((t)->screen[(unsigned short)(y) * TW + (x)])

/*---------------------------------------------------------------------------*/
/* The screen */
static void
clear_line(struct term *t, unsigned char y, unsigned char from, unsigned char to)
{
  for(; from < to; ++from) {
    CELL(t, y, from) = ' ';
  }
}
static void
newline(struct term *t)
{
  unsigned short i;
  if(t->cy + 1 < TH) {
    ++t->cy;
  } else {
    for(i = 0; i < (TH - 1) * TW; ++i) {
      t->screen[i] = t->screen[i + TW];
    }
    clear_line(t, TH - 1, 0, TW);
  }
}
static unsigned char
param(struct term *t, unsigned char i, unsigned char dflt)
{
  return (i < t->npar && t->par[i]) ? t->par[i] : dflt;
}
static void
csi(struct term *t, char c)
{
  unsigned char n = param(t, 0, 1), y;
  switch(c) {
  case 'A': t->cy = t->cy >= n ? t->cy - n : 0; break;
  case 'B': t->cy = t->cy + n < TH ? t->cy + n : TH - 1; break;
  case 'C': t->cx = t->cx + n < TW ? t->cx + n : TW - 1; break;
  case 'D': t->cx = t->cx >= n ? t->cx - n : 0; break;
  case 'H':
  case 'f':
    t->cy = param(t, 0, 1) - 1;
    t->cx = param(t, 1, 1) - 1;
    if(t->cy >= TH) {
      t->cy = TH - 1;
    }
    if(t->cx >= TW) {
      t->cx = TW - 1;
    }
    break;
  case 'J':
    if(t->npar && t->par[0] == 2) {
      for(y = 0; y < TH; ++y) {
        clear_line(t, y, 0, TW);
      }
    } else if(!t->npar || t->par[0] == 0) {
      clear_line(t, t->cy, t->cx, TW);
      for(y = t->cy + 1; y < TH; ++y) {
        clear_line(t, y, 0, TW);
      }
    }
    break;
  case 'K':
    if(!t->npar || t->par[0] == 0) {
      clear_line(t, t->cy, t->cx, TW);
    } else if(t->par[0] == 1) {
      clear_line(t, t->cy, 0, t->cx + 1);
    } else {
      clear_line(t, t->cy, 0, TW);
    }
    break;
  }
}
static void
put(struct term *t, char c)
{
  if(t->esc == 1) {
    if(c == '[') {
      t->esc = 2;
      t->npar = 0;
      t->par[0] = t->par[1] = 0;
    } else {
      t->esc = 0;
    }
    return;
  }
  if(t->esc == 2) {
    if(c >= '0' && c <= '9') {
      if(t->npar == 0) {
        t->npar = 1;
      }
      if(t->npar <= 2) {
        t->par[t->npar - 1] = t->par[t->npar - 1] * 10 + (c - '0');
      }
    } else if(c == ';') {
      ++t->npar;
    } else if(c != '?') {
      csi(t, c);
      t->esc = 0;
    }
    return;
  }
  switch(c) {
  case 0x1b: t->esc = 1; break;
  case '\r': t->cx = 0; break;
  case '\n': newline(t); break;
  case '\b': if(t->cx > 0) --t->cx; break;
  case '\t':
    t->cx = (t->cx / 8 + 1) * 8;
    if(t->cx >= TW) {
      t->cx = TW - 1;
    }
    break;
  default:
    if((unsigned char)c >= ' ' && (unsigned char)c < 0x7f) {
      if(t->cx >= TW) {                 /* wrap */
        t->cx = 0;
        newline(t);
      }
      CELL(t, t->cy, t->cx++) = c;
    }
    break;
  }
}
/* the lines that changed drawn again, the cursor reversed */
static void
show(struct term *t)
{
  unsigned char y, x;
  char line[TW];
  for(y = 0; y < TH; ++y) {
    for(x = 0; x < TW; ++x) {
      line[x] = CELL(t, y, x);
    }
    if(y == t->cy && t->ptyfd >= 0) {
      line[t->cx < TW ? t->cx : TW - 1] |= 0x80;
    }
    if(memcmp(line, t->shown + y * TW, TW) != 0) {
      memcpy(t->shown + y * TW, line, TW);
      CTK_WIDGET_REDRAW(&t->labels[y]);
    }
  }
}
static void
say(struct term *t, const char *s)
{
  if(t->cx) {
    newline(t);
  }
  while(*s) {
    put(t, *s++);
  }
}

/*---------------------------------------------------------------------------*/
/* The program on the pty */
static void
start(struct term *t)
{
  int n, fd;
  char name[12];
  struct termios tio;

  for(n = 0; n < 4; ++n) {
    strcpy(name, "/dev/ptyp0");
    name[9] = '0' + n;
    if((t->ptyfd = open(name, O_RDWR)) >= 0) {
      break;
    }
  }
  if(t->ptyfd < 0) {
    return;
  }
  t->child = fork();
  if(t->child < 0) {
    close(t->ptyfd);
    t->ptyfd = -1;
    return;
  }
  if(t->child == 0) {
    char *sh[2];
    setsid();
    for(fd = 3; fd < 20; ++fd) {        /* the desktop's files: other ptys, the mouse */
      close(fd);
    }
    close(0);
    close(1);
    name[5] = 't';                      /* the slave side, /dev/ttypN */
    fd = open(name, O_RDWR);
    if(fd < 0) {
      _exit(1);
    }
    close(2);
    tcgetattr(fd, &tio);
    tio.c_lflag |= ISIG | ICANON | ECHO | ECHOE;
    tio.c_iflag |= ICRNL;
    tio.c_oflag |= OPOST | ONLCR;
    tcsetattr(fd, TCSANOW, &tio);
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
    if(fd > 2) {
      close(fd);
    }
    putenv("TERM=ansi");
    putenv(envcols);
    putenv(envlines);
    sh[0] = "/bin/sh";
    sh[1] = NULL;
    execv(sh[0], sh);
    _exit(1);
  }
  fcntl(t->ptyfd, F_SETFL, O_NONBLOCK);
  watch_fd(t->ptyfd, t->id, ev_data);
}
static void
stop(struct term *t)
{
  int st;
  if(t->ptyfd >= 0) {
    unwatch_fd(t->ptyfd);
    close(t->ptyfd);                    /* hangs the program up */
    t->ptyfd = -1;
  }
  if(t->child > 0) {
    kill(t->child, SIGKILL);
    waitpid(t->child, &st, 0);
    t->child = 0;
  }
}
/* what the program has written, then the window */
static void
read_pty(struct term *t)
{
  char b[64];
  int n, i, st, total = 0;
  while(total < 1024 && (n = read(t->ptyfd, b, sizeof(b))) > 0) {
    for(i = 0; i < n; ++i) {
      put(t, b[i]);
    }
    total += n;
  }
  if(n == 0 && waitpid(t->child, &st, WNOHANG) == t->child) {
    /* the program has ended (until the shell has opened its side of the
       pty, reading gives 0 too: only its exit counts) */
    t->child = 0;
    stop(t);
    say(t, "[ended]");
  } else {
    watch_fd(t->ptyfd, t->id, ev_data);
  }
  show(t);
}
static void
send(struct term *t, const char *s, unsigned char n)
{
  if(t->ptyfd >= 0) {
    write(t->ptyfd, s, n);
  }
}
static void
key(struct term *t, ctk_arch_key_t c)
{
  char k[4];
  k[0] = 0x1b;
  k[1] = '[';
  switch(c) {
  case CH_ENTER: send(t, "\r", 1); break;
  case CH_DEL: send(t, "\b", 1); break;
  case CH_DELFWD: send(t, "\x7f", 1); break;
  case CH_CURS_UP: k[2] = 'A'; send(t, k, 3); break;
  case CH_CURS_DOWN: k[2] = 'B'; send(t, k, 3); break;
  case CH_CURS_RIGHT: k[2] = 'C'; send(t, k, 3); break;
  case CH_CURS_LEFT: k[2] = 'D'; send(t, k, 3); break;
  case CH_HOME: k[2] = 'H'; send(t, k, 3); break;
  case CH_END: k[2] = 'F'; send(t, k, 3); break;
  case CH_PGUP: k[2] = '5'; k[3] = '~'; send(t, k, 4); break;
  case CH_PGDN: k[2] = '6'; k[3] = '~'; send(t, k, 4); break;
  default:
    if(c < 0x80) {
      k[0] = c;
      send(t, k, 1);
    }
    break;
  }
}

/*---------------------------------------------------------------------------*/
static void
term_quit(struct term *t)
{
  stop(t);
  fmemfree(t->screen);
  free(t->shown);
  t->screen = NULL;
  t->shown = NULL;
  t->used = 0;
  ek_exit();
  LOADER_UNLOAD();
}

EK_EVENTHANDLER(term_eventhandler, ev, data)
{
  struct term *t = (struct term *)EK_CURRENT()->procstate;
  unsigned char y;
  if(ev == EK_EVENT_INIT) {
    t->id = EK_CURRENT()->id;
    ctk_window_new(&t->window, TW, TH, t->title);
    for(y = 0; y < TH; ++y) {
      CTK_LABEL_NEW(&t->labels[y], 0, y, TW, 1, t->shown + y * TW);
      CTK_WIDGET_ADD(&t->window, &t->labels[y]);
      clear_line(t, y, 0, TW);
    }
    memset(t->shown, ' ', TH * TW);
    t->cx = t->cy = t->esc = 0;
    start(t);
    if(t->ptyfd < 0) {
      say(t, "No free pty");
    }
    show(t);
    ctk_window_open(&t->window);
  } else if(ev == ev_data) {
    if(t->ptyfd >= 0) {
      read_pty(t);
    }
  } else if(ev == ctk_signal_keypress) {
    key(t, (ctk_arch_key_t)(unsigned short)data);
  } else if(ev == ctk_signal_window_close && data == (ek_data_t)&t->window) {
    term_quit(t);
  } else if(ev == EK_EVENT_REQUEST_EXIT) {
    ctk_window_close(&t->window);
    term_quit(t);
  }
}

/* a new window, while there is one free and the memory for it */
LOADER_INIT_FUNC(term_init, arg)
{
  unsigned char i;
  struct term *t;
  arg_free(arg);
  for(i = 0; i < NTERM && terms[i].used; ++i) {
  }
  if(i == NTERM) {
    return;
  }
  t = &terms[i];
  t->screen = fmemalloc(TH * TW);
  t->shown = malloc(TH * TW);
  if(t->screen == NULL || t->shown == NULL) {
    if(t->screen != NULL) {
      fmemfree(t->screen);
    }
    free(t->shown);
    t->screen = NULL;
    t->shown = NULL;
    return;
  }
  if(ev_data == 0) {
    ev_data = ek_alloc_event();
  }
  if(envcols[0] == 0) {
    char *o;
    strcpy(envcols, "COLUMNS=");
    o = envcols + 8;
    if(TW >= 10) {
      *o++ = '0' + TW / 10;
    }
    *o++ = '0' + TW % 10;
    *o = 0;
    strcpy(envlines, "LINES=");
    o = envlines + 6;
    *o++ = '0' + TH / 10;
    *o++ = '0' + TH % 10;
    *o = 0;
  }
  strcpy(t->title, "Shell 1");
  t->title[6] = '1' + i;
  t->ptyfd = -1;
  t->used = 1;
  ek_start(procs[i]);
}
