/*
 * File manager for the CTK desktop on ELKS/Alci (task D7).
 *
 * One window as large as the screen: Open, Copy, Rename, Delete and New
 * directory, the directory's path, then its entries, directories first
 * (with a "/") and files with their size. The list is read into far
 * memory when the window opens and given back when it closes; the
 * directory is kept as a path of its own, so the desktop's current
 * directory (which the file dialog moves) is not used.
 *
 * Keys: arrows, PgUp, PgDn, Home, End to choose, a letter to go to the
 * first name starting with it, Enter to open (a directory is entered, a
 * file goes to the editor), Backspace for the directory above, Delete to
 * delete, Ctrl-C to copy, Ctrl-R to rename, Ctrl-N for a new directory.
 * With the mouse a click chooses, a second click opens.
 *
 * Copyright (c) 2026, the Alci contributors. BSD 3-clause licence, the
 * terms of Contiki 1.x.
 */
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <malloc.h>
#include <sys/stat.h>

#include "ek.h"
#include "ctk.h"
#include "ctk-mouse.h"
#include "loader.h"
#include "arg.h"
#include "gfx.h"
#include "filedlg.h"

#define TW (GFX_SW - 2)                 /* the window: columns */
#define TH (GFX_SH - 4)                 /* and lines */
#define NL (TH - 2)                     /* lines of the list */
#define MAXN 200                        /* entries kept */
#define NAMEW 31                        /* a name's characters */
#define SIZEW (TW >= 40 ? 9 : 7)        /* the size column */

struct entry {
  char name[NAMEW + 1];
  unsigned char dir;
  unsigned long size;
};

EK_EVENTHANDLER(files_eventhandler, ev, data);
EK_PROCESS(p, "Files", EK_PRIO_NORMAL, files_eventhandler, NULL, NULL);
static ek_id_t id = EK_ID_NONE;

static struct ctk_window window;
static struct ctk_button openbutton = {CTK_BUTTON(0, 0, 4, "Open")};
static struct ctk_button copybutton = {CTK_BUTTON(7, 0, 4, "Copy")};
static struct ctk_button renbutton = {CTK_BUTTON(14, 0, 3, "Ren")};
static struct ctk_button delbutton = {CTK_BUTTON(20, 0, 3, "Del")};
static struct ctk_button mkdirbutton = {CTK_BUTTON(26, 0, 5, "Mkdir")};
static char pathtext[TW];
static struct ctk_label pathlabel = {CTK_LABEL(0, 1, TW, 1, pathtext)};
static char rows[NL][TW];
static struct ctk_label rowlabels[NL];

/* the question for a name (copy, rename, new directory) or for deleting */
static struct ctk_window ask;
static char asktext[28];
static struct ctk_label asklabel = {CTK_LABEL(0, 0, 28, 1, asktext)};
static char answer[FILEDLG_NAMELEN + 1];
static struct ctk_textentry answerentry =
  {CTK_TEXTENTRY(0, 1, 26, 1, answer, FILEDLG_NAMELEN)};
static struct ctk_button okbutton = {CTK_BUTTON(0, 3, 2, "OK")};
static struct ctk_button cancelbutton = {CTK_BUTTON(20, 3, 6, "Cancel")};
static unsigned char asking;            /* 'c', 'r', 'm', 'd', or 0 */

static char path[FILEDLG_PATHLEN + 1];  /* the directory shown */
static struct entry __far *ents;        /* the entries, in far memory */
static unsigned char order[MAXN];       /* their order on the screen */
static unsigned char nents, sel, top;
static char *message;                   /* shown once instead of the path */
static char buf[256];                   /* copies go through it */

/*---------------------------------------------------------------------------*/
static void
far_name(char *d, unsigned char i)
{
  struct entry __far *e = &ents[order[i]];
  unsigned char k;
  for(k = 0; k <= NAMEW && (d[k] = e->name[k]) != 0; ++k) {
  }
}
/* the full path of entry i, or of a name, in d */
static void
full(char *d, const char *name)
{
  strcpy(d, path);
  if(d[strlen(d) - 1] != '/') {
    strcat(d, "/");
  }
  strncat(d, name, FILEDLG_PATHLEN - strlen(d));
}
static void
sel_path(char *d)
{
  char n[NAMEW + 1];
  far_name(n, sel);
  full(d, n);
}
/* directories first, then by name */
static int
before(unsigned char a, unsigned char b)
{
  struct entry __far *x = &ents[a];
  struct entry __far *y = &ents[b];
  unsigned char k;
  if(x->dir != y->dir) {
    return x->dir > y->dir;
  }
  for(k = 0; k < NAMEW && x->name[k] == y->name[k] && x->name[k]; ++k) {
  }
  return (unsigned char)x->name[k] < (unsigned char)y->name[k];
}

static void
read_dir(void)
{
  DIR *d;
  struct dirent *e;
  struct stat st;
  unsigned char i, j, t;
  char full_[FILEDLG_PATHLEN + NAMEW + 2];

  nents = sel = top = 0;
  d = opendir(path);
  if(d == NULL) {
    message = "Cannot read the directory";
    return;
  }
  while((e = readdir(d)) != NULL && nents < MAXN) {
    struct entry __far *n = &ents[nents];
    if(strcmp(e->d_name, ".") == 0 ||
       (strcmp(e->d_name, "..") == 0 && strcmp(path, "/") == 0)) {
      continue;
    }
    for(i = 0; i < NAMEW && e->d_name[i]; ++i) {
      n->name[i] = e->d_name[i];
    }
    n->name[i] = 0;
    full(full_, e->d_name);
    n->dir = stat(full_, &st) == 0 && S_ISDIR(st.st_mode);
    n->size = n->dir ? 0 : st.st_size;
    order[nents] = nents;
    ++nents;
  }
  closedir(d);
  for(i = 1; i < nents; ++i) {          /* insertion sort of the order */
    for(j = i; j > 0 && before(order[j], order[j - 1]); --j) {
      t = order[j];
      order[j] = order[j - 1];
      order[j - 1] = t;
    }
  }
}

/*---------------------------------------------------------------------------*/
/* The window */
static void
put_size(char *o, unsigned long v)
{
  char d[10];
  unsigned char k = 0;
  do {
    d[k++] = '0' + v % 10;
    v /= 10;
  } while(v && k < SIZEW);
  while(k) {
    *o++ = d[--k];
  }
}
static void
show(void)
{
  char line[TW];
  unsigned char r, k, n;
  /* the path, or a message for once */
  memset(line, ' ', TW);
  {
    const char *s = message != NULL ? message : path;
    n = strlen(s);
    if(n > TW) {                        /* the end of a long path */
      s += n - TW;
      n = TW;
    }
    memcpy(line, s, n);
  }
  message = NULL;
  if(memcmp(line, pathtext, TW) != 0) {
    memcpy(pathtext, line, TW);
    CTK_WIDGET_REDRAW(&pathlabel);
  }
  if(sel < top) {
    top = sel;
  } else if(sel >= top + NL) {
    top = sel - NL + 1;
  }
  for(r = 0; r < NL; ++r) {
    unsigned char i = top + r;
    memset(line, ' ', TW);
    if(i < nents) {
      struct entry __far *e = &ents[order[i]];
      unsigned char w = TW - SIZEW - 2;
      for(k = 0; k < w && e->name[k]; ++k) {
        line[1 + k] = e->name[k];
      }
      if(e->dir) {
        line[1 + k] = '/';
      } else {
        char s[10];
        memset(s, ' ', sizeof(s));
        put_size(s, e->size);
        /* right-aligned in the last column */
        for(n = 0; n < SIZEW && s[n] != ' '; ++n) {
        }
        memcpy(line + TW - 1 - n, s, n);
      }
      if(i == sel) {
        for(k = 0; k < TW; ++k) {
          line[k] |= 0x80;
        }
      }
    }
    if(memcmp(line, rows[r], TW) != 0) {
      memcpy(rows[r], line, TW);
      CTK_WIDGET_REDRAW(&rowlabels[r]);
    }
  }
}
static void
choose(int n)
{
  if(nents == 0) {
    return;
  }
  if(n < 0) {
    n = 0;
  } else if(n >= nents) {
    n = nents - 1;
  }
  sel = n;
  show();
}

/*---------------------------------------------------------------------------*/
/* The operations */
void edit_init(char *arg);
unsigned char edit_unsaved(void);
unsigned char edit_open(void);

static void
enter(const char *name)
{
  if(strcmp(name, "..") == 0) {
    char *s = strrchr(path, '/');
    if(s == path) {
      path[1] = 0;
    } else if(s != NULL) {
      *s = 0;
    }
  } else {
    char p2[FILEDLG_PATHLEN + 1];
    full(p2, name);
    strcpy(path, p2);
  }
  read_dir();
  show();
}
static void
open_sel(void)
{
  char n[NAMEW + 1], p2[FILEDLG_PATHLEN + 1];
  if(nents == 0) {
    return;
  }
  far_name(n, sel);
  if(ents[order[sel]].dir) {
    enter(n);
    return;
  }
  if(edit_open()) {
    message = "The editor is open: close it first";
  } else {
    full(p2, n);
    edit_init(p2);
  }
  show();
}
static unsigned char
copy_file(const char *from, const char *to)
{
  int in, out, k;
  struct stat st;
  in = open(from, O_RDONLY);
  if(in < 0) {
    return 0;
  }
  out = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if(out < 0) {
    close(in);
    return 0;
  }
  while((k = read(in, buf, sizeof(buf))) > 0) {
    if(write(out, buf, k) != k) {
      k = -1;
      break;
    }
  }
  close(in);
  close(out);
  if(k < 0) {
    unlink(to);
    return 0;
  }
  if(stat(from, &st) == 0) {
    chmod(to, st.st_mode & 07777);
  }
  return 1;
}
/* the answer to the question, then the list read again */
static void
act(void)
{
  char from[FILEDLG_PATHLEN + 1], to[FILEDLG_PATHLEN + NAMEW + 2];
  char n[NAMEW + 1];
  struct stat st;
  if(nents > 0) {
    far_name(n, sel);
    sel_path(from);
  }
  if(asking != 'd') {
    if(answer[0] == 0) {
      return;
    }
    if(answer[0] == '/') {
      strcpy(to, answer);
    } else {
      full(to, answer);
    }
    /* into a directory: under the same name */
    if(asking == 'c' && stat(to, &st) == 0 && S_ISDIR(st.st_mode)) {
      strcat(to, "/");
      strcat(to, n);
    }
  }
  switch(asking) {
  case 'c':
    message = copy_file(from, to) ? "Copied" : "Cannot copy";
    break;
  case 'r':
    message = rename(from, to) == 0 ? "Renamed" : "Cannot rename";
    break;
  case 'm':
    message = mkdir(to, 0755) == 0 ? "Directory made" : "Cannot make it";
    break;
  case 'd':
    if(ents[order[sel]].dir) {
      message = rmdir(from) == 0 ? "Deleted" : "Cannot delete (not empty?)";
    } else {
      message = unlink(from) == 0 ? "Deleted" : "Cannot delete";
    }
    break;
  }
  {
    char *m = message;
    unsigned char s = sel;
    read_dir();
    message = m;
    sel = s < nents ? s : (nents ? nents - 1 : 0);
  }
}
static void
question(unsigned char what)
{
  char n[NAMEW + 1];
  unsigned char k;
  if(what != 'm' && (nents == 0 || strcmp((far_name(n, sel), n), "..") == 0)) {
    return;
  }
  if(what == 'c' && ents[order[sel]].dir) {
    message = "Directories are not copied";
    show();
    return;
  }
  asking = what;
  ctk_dialog_new(&ask, 28, 4);
  memset(asktext, ' ', sizeof(asktext));
  {
    const char *q = what == 'c' ? "Copy to:" : what == 'r' ? "New name:" :
                    what == 'm' ? "New directory:" : "Delete ";
    memcpy(asktext, q, strlen(q));
    if(what == 'd') {
      k = strlen(n);
      if(k > sizeof(asktext) - 8) {
        k = sizeof(asktext) - 8;
      }
      memcpy(asktext + 7, n, k);
      asktext[7 + k] = '?';
    }
  }
  CTK_WIDGET_ADD(&ask, &asklabel);
  if(what != 'd') {
    strcpy(answer, what == 'r' ? n : "");  /* a new name starts from the old one */
    answerentry.xpos = strlen(answer);
    answerentry.state = CTK_TEXTENTRY_EDIT;
    CTK_WIDGET_ADD(&ask, &answerentry);
    CTK_WIDGET_FOCUS(&ask, &answerentry);
  }
  CTK_BUTTON_NEW(&okbutton, 0, 3, what == 'd' ? 6 : 2, what == 'd' ? "Delete" : "OK");
  CTK_WIDGET_ADD(&ask, &okbutton);
  CTK_WIDGET_ADD(&ask, &cancelbutton);
  if(what == 'd') {
    CTK_WIDGET_FOCUS(&ask, &cancelbutton);
  }
  ctk_dialog_open(&ask);
}
static void
answered(unsigned char yes)
{
  ctk_dialog_close();
  if(yes) {
    act();
  }
  asking = 0;
  show();
}

static void
key(ctk_arch_key_t c)
{
  unsigned char i;
  char n[NAMEW + 1];
  switch(c) {
  case CH_CURS_UP: choose(sel - 1); return;
  case CH_CURS_DOWN: choose(sel + 1); return;
  case CH_PGUP: choose(sel - (NL - 1)); return;
  case CH_PGDN: choose(sel + (NL - 1)); return;
  case CH_HOME: choose(0); return;
  case CH_END: choose(nents - 1); return;
  case CH_ENTER: open_sel(); return;
  case CH_DEL: enter(".."); return;
  case CH_DELFWD: question('d'); return;
  case 0x03: question('c'); return;     /* Ctrl-C */
  case 0x12: question('r'); return;     /* Ctrl-R */
  case 0x0e: question('m'); return;     /* Ctrl-N */
  default:
    if(ctk_arch_isprint(c)) {
      /* the next name starting with that letter */
      for(i = 1; i <= nents; ++i) {
        unsigned char j = (sel + i) % nents;
        far_name(n, j);
        if((n[0] | 0x20) == (c | 0x20)) {
          choose(j);
          return;
        }
      }
    }
  }
}

static void
click(void)
{
  int r = (int)ctk_mouse_ytoc(ctk_mouse_y()) - (window.y + 2) - 2;
  int x = (int)ctk_mouse_xtoc(ctk_mouse_x()) - (window.x + 1);
  if(r < 0 || r >= NL || x < 0 || x >= TW || top + r >= nents) {
    return;
  }
  if(top + r == sel) {
    open_sel();                         /* a second click */
  } else {
    choose(top + r);
  }
}

static void
files_quit(void)
{
  if(ents != NULL) {
    fmemfree(ents);
    ents = NULL;
  }
  ek_exit();
  id = EK_ID_NONE;
  LOADER_UNLOAD();
}

EK_EVENTHANDLER(files_eventhandler, ev, data)
{
  unsigned char r;
  if(asking) {
    if(ev == ctk_signal_widget_activate) {
      if(data == (ek_data_t)&cancelbutton) {
        answered(0);
      } else if(data == (ek_data_t)&okbutton || data == (ek_data_t)&answerentry) {
        answered(1);
      }
    } else if(ev == ctk_signal_keypress &&
              (ctk_arch_key_t)(unsigned short)data == CH_ESC) {
      answered(0);
    } else if(ev == ctk_signal_keypress &&
              (ctk_arch_key_t)(unsigned short)data == CH_ENTER) {
      answered(asking != 'd');          /* Enter deletes only from the button */
    }
    return;
  }
  if(ev == EK_EVENT_INIT) {
    ctk_window_new(&window, TW, TH, "Files");
    CTK_WIDGET_ADD(&window, &openbutton);
    CTK_WIDGET_ADD(&window, &copybutton);
    CTK_WIDGET_ADD(&window, &renbutton);
    CTK_WIDGET_ADD(&window, &delbutton);
    CTK_WIDGET_ADD(&window, &mkdirbutton);
    CTK_WIDGET_ADD(&window, &pathlabel);
    memset(pathtext, ' ', sizeof(pathtext));
    for(r = 0; r < NL; ++r) {
      CTK_LABEL_NEW(&rowlabels[r], 0, r + 2, TW, 1, rows[r]);
      CTK_WIDGET_ADD(&window, &rowlabels[r]);
    }
    memset(rows, ' ', sizeof(rows));
    read_dir();
    show();
    ctk_window_open(&window);
  } else if(ev == ctk_signal_keypress) {
    key((ctk_arch_key_t)(unsigned short)data);
  } else if(ev == ctk_signal_pointer_button) {
    if(data != 0) {
      click();
    }
  } else if(ev == ctk_signal_widget_activate) {
    CTK_WIDGET_FOCUS(&window, NULL);    /* the keys go back to the list */
    if(data == (ek_data_t)&openbutton) {
      open_sel();
    } else if(data == (ek_data_t)&copybutton) {
      question('c');
    } else if(data == (ek_data_t)&renbutton) {
      question('r');
    } else if(data == (ek_data_t)&delbutton) {
      question('d');
    } else if(data == (ek_data_t)&mkdirbutton) {
      question('m');
    }
  } else if(ev == ctk_signal_window_close && data == (ek_data_t)&window) {
    files_quit();
  } else if(ev == EK_EVENT_REQUEST_EXIT) {
    ctk_window_close(&window);
    files_quit();
  }
}

/* arg: the directory to show, or the current one */
LOADER_INIT_FUNC(files_init, arg)
{
  if(id == EK_ID_NONE) {
    ents = fmemalloc((unsigned long)MAXN * sizeof(struct entry));
    if(ents != NULL) {
      if(arg != NULL) {
        strncpy(path, arg, FILEDLG_PATHLEN);
        path[FILEDLG_PATHLEN] = 0;
      } else if(getcwd(path, sizeof(path)) == NULL) {
        strcpy(path, "/");
      }
      id = ek_start(&p);
    }
  }
  arg_free(arg);
}
