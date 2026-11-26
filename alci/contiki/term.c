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
 * The pty is opened as telnetd does (elkscmd/inet/telnetd.c). Nothing of
 * this runs, and no memory is taken from the pty, until the window opens;
 * closing it hangs the program up.
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
#include <sys/wait.h>

#include "ek.h"
#include "ctk.h"
#include "loader.h"
#include "arg.h"
#include "gfx.h"
#include "watch.h"
#include "mouse.h"

#define TW (GFX_SW - 2)                 /* the window: columns */
#define TH (GFX_SH - 4)                 /* and lines */

EK_EVENTHANDLER(term_eventhandler, ev, data);
EK_PROCESS(p, "Terminal", EK_PRIO_NORMAL, term_eventhandler, NULL, NULL);
static ek_id_t id = EK_ID_NONE;
static ek_event_t ev_data;              /* the pty has something to read */

static struct ctk_window window;
static char screen[TH][TW];             /* the text */
static char shown[TH][TW];              /* the lines as drawn, cursor included */
static struct ctk_label labels[TH];
static unsigned char cx, cy;            /* cursor */
static unsigned char esc, npar;         /* escape sequence: state, parameters */
static unsigned char par[2];
static int ptyfd = -1;
static pid_t child;
static char envcols[12], envlines[10];

/*---------------------------------------------------------------------------*/
/* The screen */
static void
clear_line(unsigned char y, unsigned char from, unsigned char to)
{
  memset(&screen[y][from], ' ', to - from);
}
static void
newline(void)
{
  if(cy + 1 < TH) {
    ++cy;
  } else {
    memmove(screen[0], screen[1], (TH - 1) * TW);
    clear_line(TH - 1, 0, TW);
  }
}
static unsigned char
param(unsigned char i, unsigned char dflt)
{
  return (i < npar && par[i]) ? par[i] : dflt;
}
static void
csi(char c)
{
  unsigned char n = param(0, 1), y;
  switch(c) {
  case 'A': cy = cy >= n ? cy - n : 0; break;
  case 'B': cy = cy + n < TH ? cy + n : TH - 1; break;
  case 'C': cx = cx + n < TW ? cx + n : TW - 1; break;
  case 'D': cx = cx >= n ? cx - n : 0; break;
  case 'H':
  case 'f':
    cy = param(0, 1) - 1;
    cx = param(1, 1) - 1;
    if(cy >= TH) {
      cy = TH - 1;
    }
    if(cx >= TW) {
      cx = TW - 1;
    }
    break;
  case 'J':
    if(npar && par[0] == 2) {
      for(y = 0; y < TH; ++y) {
        clear_line(y, 0, TW);
      }
    } else if(!npar || par[0] == 0) {
      clear_line(cy, cx, TW);
      for(y = cy + 1; y < TH; ++y) {
        clear_line(y, 0, TW);
      }
    }
    break;
  case 'K':
    if(!npar || par[0] == 0) {
      clear_line(cy, cx, TW);
    } else if(par[0] == 1) {
      clear_line(cy, 0, cx + 1);
    } else {
      clear_line(cy, 0, TW);
    }
    break;
  }
}
static void
put(char c)
{
  if(esc == 1) {
    if(c == '[') {
      esc = 2;
      npar = 0;
      par[0] = par[1] = 0;
    } else {
      esc = 0;
    }
    return;
  }
  if(esc == 2) {
    if(c >= '0' && c <= '9') {
      if(npar == 0) {
        npar = 1;
      }
      if(npar <= 2) {
        par[npar - 1] = par[npar - 1] * 10 + (c - '0');
      }
    } else if(c == ';') {
      ++npar;
    } else if(c != '?') {
      csi(c);
      esc = 0;
    }
    return;
  }
  switch(c) {
  case 0x1b: esc = 1; break;
  case '\r': cx = 0; break;
  case '\n': newline(); break;
  case '\b': if(cx > 0) --cx; break;
  case '\t': cx = (cx / 8 + 1) * 8; if(cx >= TW) cx = TW - 1; break;
  default:
    if((unsigned char)c >= ' ' && (unsigned char)c < 0x7f) {
      if(cx >= TW) {                    /* wrap */
        cx = 0;
        newline();
      }
      screen[cy][cx++] = c;
    }
    break;
  }
}
/* the lines that changed drawn again, the cursor reversed */
static void
show(void)
{
  unsigned char y;
  char line[TW];
  for(y = 0; y < TH; ++y) {
    memcpy(line, screen[y], TW);
    if(y == cy && ptyfd >= 0) {
      line[cx < TW ? cx : TW - 1] |= 0x80;
    }
    if(memcmp(line, shown[y], TW) != 0) {
      memcpy(shown[y], line, TW);
      CTK_WIDGET_REDRAW(&labels[y]);
    }
  }
}

/*---------------------------------------------------------------------------*/
/* The program on the pty */
static void
start(void)
{
  int n, fd;
  char name[12];
  struct termios t;

  for(n = 0; n < 4; ++n) {
    strcpy(name, "/dev/ptyp0");
    name[9] = '0' + n;
    if((ptyfd = open(name, O_RDWR)) >= 0) {
      break;
    }
  }
  if(ptyfd < 0) {
    return;
  }
  child = fork();
  if(child < 0) {
    close(ptyfd);
    ptyfd = -1;
    return;
  }
  if(child == 0) {
    char *sh[2];
    setsid();
    close(ptyfd);
    if(mouse_fd >= 0) {
      close(mouse_fd);
    }
    close(0);
    close(1);
    name[5] = 't';                      /* the slave side, /dev/ttypN */
    fd = open(name, O_RDWR);
    if(fd < 0) {
      _exit(1);
    }
    close(2);
    tcgetattr(fd, &t);
    t.c_lflag |= ISIG | ICANON | ECHO | ECHOE;
    t.c_iflag |= ICRNL;
    t.c_oflag |= OPOST | ONLCR;
    tcsetattr(fd, TCSANOW, &t);
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
  fcntl(ptyfd, F_SETFL, O_NONBLOCK);
  watch_fd(ptyfd, id, ev_data);
}
static void
stop(void)
{
  int st;
  if(ptyfd >= 0) {
    unwatch_fd(ptyfd);
    close(ptyfd);                       /* hangs the program up */
    ptyfd = -1;
  }
  if(child > 0) {
    kill(child, SIGKILL);
    waitpid(child, &st, 0);
    child = 0;
  }
}
/* what the program has written, then the window */
static void
read_pty(void)
{
  char b[64];
  int n, i, st, total = 0;
  while(total < 1024 && (n = read(ptyfd, b, sizeof(b))) > 0) {
    for(i = 0; i < n; ++i) {
      put(b[i]);
    }
    total += n;
  }
  if(n == 0 && waitpid(child, &st, WNOHANG) == child) {
    /* the program has ended (until the shell has opened its side of the
       pty, reading gives 0 too: only its exit counts) */
    child = 0;
    stop();
    if(cx) {
      newline();
    }
    memcpy(screen[cy], "[ended]", 7);
  } else {
    watch_fd(ptyfd, id, ev_data);
  }
  show();
}
static void
send(const char *s, unsigned char n)
{
  if(ptyfd >= 0) {
    write(ptyfd, s, n);
  }
}
static void
key(ctk_arch_key_t c)
{
  char k[4];
  k[0] = 0x1b;
  k[1] = '[';
  switch(c) {
  case CH_ENTER: send("\r", 1); break;
  case CH_DEL: send("\b", 1); break;
  case CH_DELFWD: send("\x7f", 1); break;
  case CH_CURS_UP: k[2] = 'A'; send(k, 3); break;
  case CH_CURS_DOWN: k[2] = 'B'; send(k, 3); break;
  case CH_CURS_RIGHT: k[2] = 'C'; send(k, 3); break;
  case CH_CURS_LEFT: k[2] = 'D'; send(k, 3); break;
  case CH_HOME: k[2] = 'H'; send(k, 3); break;
  case CH_END: k[2] = 'F'; send(k, 3); break;
  case CH_PGUP: k[2] = '5'; k[3] = '~'; send(k, 4); break;
  case CH_PGDN: k[2] = '6'; k[3] = '~'; send(k, 4); break;
  default:
    if(c < 0x80) {
      k[0] = c;
      send(k, 1);
    }
    break;
  }
}

/*---------------------------------------------------------------------------*/
static void
term_quit(void)
{
  stop();
  ek_exit();
  id = EK_ID_NONE;
  LOADER_UNLOAD();
}

EK_EVENTHANDLER(term_eventhandler, ev, data)
{
  unsigned char y;
  if(ev == EK_EVENT_INIT) {
    ev_data = ek_alloc_event();
    ctk_window_new(&window, TW, TH, "Shell");
    for(y = 0; y < TH; ++y) {
      CTK_LABEL_NEW(&labels[y], 0, y, TW, 1, shown[y]);
      CTK_WIDGET_ADD(&window, &labels[y]);
      clear_line(y, 0, TW);
    }
    memset(shown, ' ', sizeof(shown));
    cx = cy = esc = 0;
    start();
    if(ptyfd < 0) {
      memcpy(screen[0], "No free pty", 11);
    }
    show();
    ctk_window_open(&window);
  } else if(ev == ev_data) {
    if(ptyfd >= 0) {
      read_pty();
    }
  } else if(ev == ctk_signal_keypress) {
    key((ctk_arch_key_t)(unsigned short)data);
  } else if(ev == ctk_signal_window_close && data == (ek_data_t)&window) {
    term_quit();
  } else if(ev == EK_EVENT_REQUEST_EXIT) {
    ctk_window_close(&window);
    term_quit();
  }
}

LOADER_INIT_FUNC(term_init, arg)
{
  arg_free(arg);
  if(id == EK_ID_NONE) {
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
    id = ek_start(&p);
  }
}
