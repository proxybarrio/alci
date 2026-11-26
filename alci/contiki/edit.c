/*
 * Text editor for the CTK desktop on ELKS/Alci (task D7).
 *
 * One window as large as the screen: Open, Save and New, the file's name
 * and the cursor's line and column, then the text. The text is kept in a
 * gap buffer (the gap always at the cursor); lines end
 * with a newline and tabs stop every eight columns. The buffer is taken from
 * far memory when the editor starts (60 KB, or less if the machine is
 * short) and given back when it closes, so the desktop's own 64 KB of data
 * are not touched. Each line on the
 * screen is a label that is drawn again only when it changes; the cursor
 * is the character with its top bit set (drawn reversed by ctk-gfx.c).
 *
 * Keys: arrows, Home, End, PgUp, PgDn, Backspace, Delete, Enter; Ctrl-O
 * opens, Ctrl-S saves, Ctrl-N starts again. A click in the text puts the
 * cursor there.
 *
 * Laid out as the programs of Contiki 1.x (one process and its window),
 * after Contiki's editor sketch (apps/editor.c, Adam Dunkels), which kept
 * a line per label and the cursor in the top bit.
 *
 * Copyright (c) 2026, the Alci contributors. BSD 3-clause licence, the
 * terms of Contiki 1.x.
 */
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>

#include "ek.h"
#include "ctk.h"
#include "ctk-mouse.h"
#include "loader.h"
#include "arg.h"
#include "gfx.h"
#include "filedlg.h"

#define TW (GFX_SW - 2)                 /* the text: columns */
#define TH (GFX_SH - 5)                 /* and lines */
#define TABW 8

EK_EVENTHANDLER(edit_eventhandler, ev, data);
EK_PROCESS(p, "Editor", EK_PRIO_NORMAL, edit_eventhandler, NULL, NULL);
static ek_id_t id = EK_ID_NONE;

static struct ctk_window window;
static struct ctk_button openbutton = {CTK_BUTTON(0, 0, 4, "Open")};
static struct ctk_button savebutton = {CTK_BUTTON(7, 0, 4, "Save")};
static struct ctk_button newbutton = {CTK_BUTTON(14, 0, 3, "New")};
#define STATUS_X 19
static char statustext[TW - STATUS_X];
static struct ctk_label statuslabel =
  {CTK_LABEL(STATUS_X, 0, TW - STATUS_X, 1, statustext)};
static char shown[TH][TW];              /* the lines as on the screen */
static struct ctk_label linelabels[TH];
static char line[TW];

static char __far *buf;                 /* the text, in far memory */
static unsigned short bufsize;
static unsigned short gs, ge;           /* the gap: the cursor is at gs */
static char chunk[256];                 /* files go through it */
static unsigned short top;              /* first line on the screen */
static unsigned short topline;          /* its number, from 0 */
static unsigned short left;             /* first column on the screen */
static unsigned short want;             /* column kept going up and down */
static unsigned char modified;
static char filename[FILEDLG_NAMELEN + 1];
static char *message;                   /* shown once in the status */
static unsigned char dialog;            /* 1: open, 2: save */

/*---------------------------------------------------------------------------*/
/* The gap buffer */
#define LEN() (bufsize - (ge - gs))
static char
at(unsigned short p)
{
  return p < gs ? buf[p] : buf[p + (ge - gs)];
}
static void
gap_to(unsigned short p)
{
  unsigned short n;
  if(p < gs) {
    n = gs - p;
    while(n--) {
      buf[--ge] = buf[--gs];
    }
  } else if(p > gs) {
    n = p - gs;
    while(n--) {
      buf[gs++] = buf[ge++];
    }
  }
}
static unsigned short
line_start(unsigned short p)
{
  while(p > 0 && at(p - 1) != '\n') {
    --p;
  }
  return p;
}
static unsigned short
line_end(unsigned short p)
{
  unsigned short n = LEN();
  while(p < n && at(p) != '\n') {
    ++p;
  }
  return p;
}
static unsigned short
next_col(unsigned short c, char ch)
{
  return ch == '\t' ? (c / TABW + 1) * TABW : c + 1;
}
/* the column of position p */
static unsigned short
col_of(unsigned short p)
{
  unsigned short q = line_start(p), c = 0;
  for(; q < p; ++q) {
    c = next_col(c, at(q));
  }
  return c;
}
/* the position at column col of the line starting at ls, or its end */
static unsigned short
pos_at_col(unsigned short ls, unsigned short col)
{
  unsigned short n = LEN(), c = 0, c2;
  char ch;
  while(ls < n && (ch = at(ls)) != '\n') {
    c2 = next_col(c, ch);
    if(c2 > col) {
      break;
    }
    c = c2;
    ++ls;
  }
  return ls;
}
/* newlines between a and b */
static unsigned short
count_lines(unsigned short a, unsigned short b)
{
  unsigned short k = 0;
  for(; a < b; ++a) {
    if(at(a) == '\n') {
      ++k;
    }
  }
  return k;
}

/*---------------------------------------------------------------------------*/
/* The screen */
static void
put_num(char *o, unsigned short v)
{
  char d[6];
  unsigned char k = 0;
  do {
    d[k++] = '0' + v % 10;
    v /= 10;
  } while(v);
  while(k) {
    *o++ = d[--k];
  }
  *o = 0;
}
static void
show_status(unsigned short row, unsigned short col)
{
  char t[sizeof(statustext) + 12], *o;
  unsigned char n;
  memset(statustext, ' ', sizeof(statustext));
  if(message != NULL) {
    strcpy(t, message);
    message = NULL;
  } else {
    strcpy(t, filename[0] ? filename : "(new)");
    if(modified) {
      strcat(t, "*");
    }
    strcat(t, " ");
    o = t + strlen(t);
    put_num(o, row + 1);
    strcat(t, ":");
    o = t + strlen(t);
    put_num(o, col + 1);
  }
  n = strlen(t);
  if(n > sizeof(statustext)) {
    /* keep the end: the position and the end of the name */
    memcpy(statustext, t + n - sizeof(statustext), sizeof(statustext));
  } else {
    memcpy(statustext, t, n);
  }
  CTK_WIDGET_REDRAW(&statuslabel);
}

/* the cursor in view, then the lines that changed drawn again */
static void
show(void)
{
  unsigned short cl = line_start(gs), cc = col_of(gs), r, p, n, c, q;
  unsigned char more;
  char ch;

  if(cl < top) {
    topline -= count_lines(cl, top);
    top = cl;
  } else {
    r = count_lines(top, cl);
    while(r >= TH) {
      top = line_end(top) + 1;
      ++topline;
      --r;
    }
  }
  if(cc < left) {
    left = cc < TABW ? 0 : cc - TABW;
  } else if(cc >= left + TW) {
    left = cc - TW + TABW;
  }

  p = top;
  n = LEN();
  more = 1;
  for(r = 0; r < TH; ++r) {
    memset(line, ' ', TW);
    if(more) {
      c = 0;
      for(q = p; q < n && (ch = at(q)) != '\n'; ++q) {
        unsigned short c2 = next_col(c, ch);
        if(c >= left && c < left + TW) {
          line[c - left] = (ch >= ' ' && ch < 0x7f) ? ch : (ch == '\t' ? ' ' : '?');
        }
        c = c2;
      }
      if(cl == p && cc >= left && cc < left + TW) {
        line[cc - left] |= 0x80;
      }
      more = q < n;
      p = q + 1;
    }
    if(memcmp(line, shown[r], TW) != 0) {
      memcpy(shown[r], line, TW);
      CTK_WIDGET_REDRAW(&linelabels[r]);
    }
  }
  show_status(topline + count_lines(top, cl), cc);
}

/*---------------------------------------------------------------------------*/
/* Editing */
static void
insert(char ch)
{
  if(gs == ge) {
    message = "Text full";
    return;
  }
  buf[gs++] = ch;
  modified = 1;
}
static void
backspace(void)
{
  if(gs == 0) {
    return;
  }
  --gs;
  modified = 1;
  if(gs < top) {                        /* the newline before the first line */
    top = line_start(gs);
    --topline;
  }
}
static void
delete_forward(void)
{
  if(ge < bufsize) {
    ++ge;
    modified = 1;
  }
}
static void
line_up(void)
{
  unsigned short ls = line_start(gs);
  if(ls > 0) {
    gap_to(pos_at_col(line_start(ls - 1), want));
  }
}
static void
line_down(void)
{
  unsigned short le = line_end(gs);
  if(le < LEN()) {
    gap_to(pos_at_col(le + 1, want));
  }
}
static void
start_over(void)
{
  gs = 0;
  ge = bufsize;
  top = topline = left = want = 0;
  modified = 0;
}

/*---------------------------------------------------------------------------*/
/* Files */
static void
load(char *name)
{
  int fd, k;
  long size;
  unsigned short q, i;
  fd = open(name, O_RDONLY);
  if(fd < 0) {
    message = "Cannot open";
    return;
  }
  start_over();
  /* the text at the end of the buffer, the gap before it */
  size = lseek(fd, 0L, SEEK_END);
  lseek(fd, 0L, SEEK_SET);
  if(size < 0) {
    size = 0;
  }
  if(size > bufsize) {
    size = bufsize;
    message = "Too long: cut";
  }
  ge = q = bufsize - (unsigned short)size;
  while(q < bufsize) {
    unsigned short want_ = bufsize - q < sizeof(chunk) ? bufsize - q : sizeof(chunk);
    k = read(fd, chunk, want_);
    if(k <= 0) {
      message = "Cannot read";
      break;
    }
    for(i = 0; i < (unsigned short)k; ++i) {
      buf[q + i] = chunk[i];
    }
    q += k;
  }
  if(q < bufsize) {                     /* short: what came, at the end */
    unsigned short got = q - ge;
    for(i = 0; i < got; ++i) {
      buf[bufsize - 1 - i] = buf[q - 1 - i];
    }
    ge = bufsize - got;
  }
  close(fd);
  strcpy(filename, name);
  memset(shown, 0, sizeof(shown));      /* every line drawn again */
}
/* n bytes from the buffer at p to the file */
static int
put(int fd, unsigned short p, unsigned short n)
{
  while(n) {
    unsigned short k = n < sizeof(chunk) ? n : sizeof(chunk), i;
    for(i = 0; i < k; ++i) {
      chunk[i] = buf[p + i];
    }
    if(write(fd, chunk, k) != (int)k) {
      return -1;
    }
    p += k;
    n -= k;
  }
  return 0;
}
static void
store(void)
{
  int fd;
  fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if(fd < 0) {
    message = "Cannot save";
    return;
  }
  if(put(fd, 0, gs) < 0 || put(fd, ge, bufsize - ge) < 0) {
    message = "Disk full?";
  } else {
    modified = 0;
    message = "Saved";
  }
  close(fd);
}
static void
save(void)
{
  if(filename[0] == 0) {
    dialog = 2;
    filedlg_open("Save as", "Save", filename);
  } else {
    store();
  }
}

/*---------------------------------------------------------------------------*/
static void
key(ctk_arch_key_t c)
{
  unsigned char r;
  switch(c) {
  case CH_CURS_LEFT:
    if(gs > 0) {
      gap_to(gs - 1);
    }
    want = col_of(gs);
    break;
  case CH_CURS_RIGHT:
    if(gs < LEN()) {
      gap_to(gs + 1);
    }
    want = col_of(gs);
    break;
  case CH_CURS_UP:
    line_up();
    break;
  case CH_CURS_DOWN:
    line_down();
    break;
  case CH_PGUP:
    for(r = 1; r < TH; ++r) {
      line_up();
    }
    break;
  case CH_PGDN:
    for(r = 1; r < TH; ++r) {
      line_down();
    }
    break;
  case CH_HOME:
    gap_to(line_start(gs));
    want = 0;
    break;
  case CH_END:
    gap_to(line_end(gs));
    want = col_of(gs);
    break;
  case CH_ENTER:
    insert('\n');
    want = 0;
    break;
  case CH_DEL:
    backspace();
    want = col_of(gs);
    break;
  case CH_DELFWD:
    delete_forward();
    break;
  case 0x0f:                            /* Ctrl-O */
    dialog = 1;
    filedlg_open("Open a file", "Open", filename);
    return;
  case 0x13:                            /* Ctrl-S */
    save();
    break;
  case 0x0e:                            /* Ctrl-N */
    start_over();
    filename[0] = 0;
    break;
  default:
    if(ctk_arch_isprint(c)) {
      insert(c);
      want = col_of(gs);
    }
    break;
  }
  show();
}

/* a click in the text: the cursor there */
static void
click(void)
{
  int r = (int)ctk_mouse_ytoc(ctk_mouse_y()) - (window.y + 2) - 1;
  int x = (int)ctk_mouse_xtoc(ctk_mouse_x()) - (window.x + 1);
  unsigned short p = top, n = LEN();
  if(r < 0 || r >= TH || x < 0 || x >= TW) {
    return;
  }
  while(r-- > 0) {
    unsigned short e = line_end(p);
    if(e >= n) {
      break;
    }
    p = e + 1;
  }
  want = left + x;
  gap_to(pos_at_col(p, want));
  show();
}

static void
edit_quit(void)
{
  fmemfree(buf);
  buf = NULL;
  ek_exit();
  id = EK_ID_NONE;
  LOADER_UNLOAD();
}

EK_EVENTHANDLER(edit_eventhandler, ev, data)
{
  unsigned char r;
  r = filedlg_event(ev, data);
  if(r != FILEDLG_NOTMINE) {
    if(r == FILEDLG_OK) {
      if(dialog == 1) {
        load(filename);
      } else {
        store();
      }
    }
    if(r != FILEDLG_BUSY) {
      dialog = 0;
      show();
    }
    return;
  }
  if(ev == EK_EVENT_INIT) {
    ctk_window_new(&window, TW, TH + 1, "Editor");
    CTK_WIDGET_ADD(&window, &openbutton);
    CTK_WIDGET_ADD(&window, &savebutton);
    CTK_WIDGET_ADD(&window, &newbutton);
    CTK_WIDGET_ADD(&window, &statuslabel);
    for(r = 0; r < TH; ++r) {
      CTK_LABEL_NEW(&linelabels[r], 0, r + 1, TW, 1, shown[r]);
      CTK_WIDGET_ADD(&window, &linelabels[r]);
    }
    memset(shown, ' ', sizeof(shown));
    if(filename[0]) {
      load(filename);
    }
    show();
    ctk_window_open(&window);
  } else if(ev == ctk_signal_keypress) {
    key((ctk_arch_key_t)(unsigned short)data);
  } else if(ev == ctk_signal_pointer_button) {
    if(data != 0) {
      click();
    }
  } else if(ev == ctk_signal_widget_activate) {
    CTK_WIDGET_FOCUS(&window, NULL);    /* Enter goes back to the text */
    if(data == (ek_data_t)&openbutton) {
      key(0x0f);
    } else if(data == (ek_data_t)&savebutton) {
      key(0x13);
    } else if(data == (ek_data_t)&newbutton) {
      key(0x0e);
    }
  } else if(ev == ctk_signal_window_close && data == (ek_data_t)&window) {
    edit_quit();
  } else if(ev == EK_EVENT_REQUEST_EXIT) {
    ctk_window_close(&window);
    edit_quit();
  }
}

/* arg: a file to open, or nothing */
LOADER_INIT_FUNC(edit_init, arg)
{
  static const unsigned short sizes[] = {61440U, 32768U, 16384U, 4096U};
  unsigned char i;
  if(id == EK_ID_NONE) {
    for(i = 0; i < sizeof(sizes) / sizeof(sizes[0]) && buf == NULL; ++i) {
      buf = fmemalloc(sizes[i]);
      bufsize = sizes[i];
    }
    if(buf == NULL) {
      arg_free(arg);
      return;                           /* no memory: no editor */
    }
    filename[0] = 0;
    if(arg != NULL) {
      strncpy(filename, arg, FILEDLG_NAMELEN);
      filename[FILEDLG_NAMELEN] = 0;
    }
    start_over();
    id = ek_start(&p);
  }
  arg_free(arg);
}
