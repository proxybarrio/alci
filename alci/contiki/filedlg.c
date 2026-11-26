/*
 * File dialog for the CTK desktop on ELKS/Alci (task D7): the files of the
 * current directory in a list, a name to type, a button and Cancel.
 * Directories are listed first, with a "/"; choosing one goes into it.
 * Keys: arrows, PgUp, PgDn, Home, End to choose, letters to type a name,
 * Enter for the button, Esc for Cancel; with the mouse, a click chooses
 * and a second click on the same line takes it.
 *
 * It takes the place of Contiki's ctk-filedialog (lib/ctk-filedialog.c,
 * Adam Dunkels), which reads directories through the cfs service; this one
 * reads them with opendir() and keeps the dialog's look.
 *
 * Copyright (c) 2026, the Alci contributors. BSD 3-clause licence, the
 * terms of Contiki 1.x.
 */
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "ctk.h"
#include "ctk-mouse.h"
#include "filedlg.h"

#define FW 30                           /* the dialog, cells wide */
#define NL 12                           /* lines of the list */
#define MAXN 64                         /* names kept */
#define NAMEW 16                        /* a name, with its "/" and end */

static struct ctk_window dlg;
static char titletext[FW];
static struct ctk_label titlelabel = {CTK_LABEL(0, 0, FW, 1, titletext)};
static char rows[NL][FW];
static struct ctk_label rowlabels[NL];
static struct ctk_label namelabel = {CTK_LABEL(0, NL + 2, 5, 1, "Name")};
static char entrytext[FILEDLG_NAMELEN + 1];
static struct ctk_textentry entry =
  {CTK_TEXTENTRY(5, NL + 2, FW - 7, 1, entrytext, FILEDLG_NAMELEN)};
static struct ctk_button okbutton;
static struct ctk_button cancelbutton = {CTK_BUTTON(FW - 8, NL + 4, 6, "Cancel")};

static char names[MAXN][NAMEW];
static unsigned char nnames, sel, top, isopen;
static char *result;

/* the directory, directories first, then by name */
static void
read_dir(void)
{
  DIR *d;
  struct dirent *e;
  struct stat st;
  unsigned char i, j;
  char tmp[NAMEW];

  nnames = 0;
  d = opendir(".");
  if(d == NULL) {
    return;
  }
  while((e = readdir(d)) != NULL && nnames < MAXN) {
    if(strcmp(e->d_name, ".") == 0) {
      continue;
    }
    strncpy(names[nnames], e->d_name, NAMEW - 2);
    names[nnames][NAMEW - 2] = 0;
    if(stat(e->d_name, &st) == 0 && S_ISDIR(st.st_mode)) {
      strcat(names[nnames], "/");
    }
    ++nnames;
  }
  closedir(d);
  for(i = 1; i < nnames; ++i) {
    for(j = i; j > 0; --j) {
      char *a = names[j - 1], *b = names[j];
      unsigned char da = a[strlen(a) - 1] == '/', db = b[strlen(b) - 1] == '/';
      if(da > db || (da == db && strcmp(a, b) <= 0)) {
        break;
      }
      strcpy(tmp, a);
      strcpy(a, b);
      strcpy(b, tmp);
    }
  }
}

/* the list, the chosen line shown reversed */
static void
show_list(void)
{
  unsigned char r, k;
  for(r = 0; r < NL; ++r) {
    unsigned char n = top + r;
    char *o = rows[r];
    memset(o, ' ', FW);
    if(n < nnames) {
      memcpy(o + 1, names[n], strlen(names[n]));
      if(n == sel) {
        for(k = 0; k < FW; ++k) {
          o[k] |= 0x80;
        }
      }
    }
    CTK_WIDGET_REDRAW(&rowlabels[r]);
  }
}

/* the chosen line, kept in view, its name in the entry (files only) */
static void
choose(int n)
{
  if(nnames == 0) {
    return;
  }
  if(n < 0) {
    n = 0;
  } else if(n >= nnames) {
    n = nnames - 1;
  }
  sel = n;
  if(sel < top) {
    top = sel;
  } else if(sel >= top + NL) {
    top = sel - NL + 1;
  }
  if(names[sel][strlen(names[sel]) - 1] != '/') {
    strncpy(entrytext, names[sel], FILEDLG_NAMELEN);
    entry.xpos = strlen(entrytext);
    CTK_WIDGET_REDRAW(&entry);
  }
  show_list();
}

void
filedlg_open(char *title, char *button, char *name)
{
  unsigned char r;
  result = name;
  ctk_dialog_new(&dlg, FW, NL + 5);
  memset(titletext, ' ', FW);
  memcpy(titletext, title, strlen(title) < FW ? strlen(title) : FW);
  CTK_WIDGET_ADD(&dlg, &titlelabel);
  for(r = 0; r < NL; ++r) {
    CTK_LABEL_NEW(&rowlabels[r], 0, r + 1, FW, 1, rows[r]);
    CTK_WIDGET_ADD(&dlg, &rowlabels[r]);
  }
  CTK_WIDGET_ADD(&dlg, &namelabel);
  strncpy(entrytext, name, FILEDLG_NAMELEN);
  entrytext[FILEDLG_NAMELEN] = 0;
  entry.xpos = strlen(entrytext);
  entry.state = CTK_TEXTENTRY_NORMAL;
  CTK_WIDGET_ADD(&dlg, &entry);
  CTK_BUTTON_NEW(&okbutton, 0, NL + 4, strlen(button), button);
  CTK_WIDGET_ADD(&dlg, &okbutton);
  CTK_WIDGET_ADD(&dlg, &cancelbutton);
  read_dir();
  sel = top = 0;
  memset(rows, ' ', sizeof(rows));
  ctk_dialog_open(&dlg);
  isopen = 1;
  show_list();
}

static unsigned char
done(unsigned char how)
{
  ctk_dialog_close();
  isopen = 0;
  return how;
}

/* the button: a directory is entered, a file is the answer */
static unsigned char
take(void)
{
  struct stat st;
  char *name = entrytext;
  if(name[0] == 0 && nnames > 0) {
    name = names[sel];
  }
  if(name[0] == 0) {
    return FILEDLG_BUSY;
  }
  if(name[strlen(name) - 1] == '/' ||
     (stat(name, &st) == 0 && S_ISDIR(st.st_mode))) {
    char dir[FILEDLG_NAMELEN + 1];
    strncpy(dir, name, FILEDLG_NAMELEN);
    dir[FILEDLG_NAMELEN] = 0;
    if(chdir(dir) == 0) {
      read_dir();
      sel = top = 0;
      entrytext[0] = 0;
      entry.xpos = 0;
      CTK_WIDGET_REDRAW(&entry);
      show_list();
    }
    return FILEDLG_BUSY;
  }
  strncpy(result, name, FILEDLG_NAMELEN);
  result[FILEDLG_NAMELEN] = 0;
  return done(FILEDLG_OK);
}

unsigned char
filedlg_event(ek_event_t ev, ek_data_t data)
{
  if(!isopen) {
    return FILEDLG_NOTMINE;
  }
  if(ev == ctk_signal_widget_activate) {
    if(data == (ek_data_t)&cancelbutton) {
      return done(FILEDLG_CANCEL);
    }
    if(data == (ek_data_t)&okbutton || data == (ek_data_t)&entry) {
      return take();
    }
    return FILEDLG_BUSY;
  }
  if(ev == ctk_signal_keypress) {
    ctk_arch_key_t c = (ctk_arch_key_t)(unsigned short)data;
    switch(c) {
    case CH_CURS_UP: choose(sel - 1); break;
    case CH_CURS_DOWN: choose(sel + 1); break;
    case CH_PGUP: choose(sel - (NL - 1)); break;
    case CH_PGDN: choose(sel + (NL - 1)); break;
    case CH_HOME: choose(0); break;
    case CH_END: choose(nnames - 1); break;
    case CH_ENTER: return take();
    case CH_ESC: return done(FILEDLG_CANCEL);
    default:
      if(ctk_arch_isprint(c)) {
        /* typing starts a name in the entry */
        unsigned char l = strlen(entrytext);
        if(entry.state != CTK_TEXTENTRY_EDIT) {
          l = 0;
        }
        if(l < FILEDLG_NAMELEN) {
          entrytext[l++] = c;
          entrytext[l] = 0;
        }
        entry.xpos = l;
        entry.state = CTK_TEXTENTRY_EDIT;
        CTK_WIDGET_FOCUS(&dlg, &entry);
        CTK_WIDGET_REDRAW(&entry);
      }
      break;
    }
    return FILEDLG_BUSY;
  }
  if(ev == ctk_signal_pointer_button) {
    if(data != 0) {
      int r = (int)ctk_mouse_ytoc(ctk_mouse_y()) - (dlg.y + 2) - 1;
      int x = (int)ctk_mouse_xtoc(ctk_mouse_x()) - (dlg.x + 1);
      if(r >= 0 && r < NL && x >= 0 && x < FW && top + r < nnames) {
        if(top + r == sel) {
          return take();                /* a second click: take it */
        }
        choose(top + r);
      }
    }
    return FILEDLG_BUSY;
  }
  if(ev == ctk_signal_pointer_move) {
    return FILEDLG_BUSY;
  }
  return FILEDLG_NOTMINE;
}
