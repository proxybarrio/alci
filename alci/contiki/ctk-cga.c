/*
 * CTK drawing layer for ELKS on CGA 640x200, two colours (BIOS mode 6).
 * Prototype for Alci (task D7).
 *
 * The structure follows the Commodore 64 "hires" driver of Contiki 1.x
 * (contiki-c64/ctk/ctk-hires.c and ctk-hires-asm.S, Copyright (c) 2002-2004,
 * Adam Dunkels, BSD 3-clause): the screen is a grid of 8x8 cells, window
 * borders, buttons and the desktop background use the 8x8 patterns of a
 * hires theme, and the C64 colours are mapped to "normal" or "reversed".
 * The 6502 assembly primitives are rewritten here in C for the CGA
 * interleaved frame buffer.
 *
 * Copyright (c) 2002-2004, Adam Dunkels (original C64 driver).
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions of the
 * BSD 3-clause licence in the Contiki 1.x sources are met.
 */

#include "ctk.h"
#include "ctk-draw.h"
#include "ctk-hires-theme.h"
#include "font8x8.h"

#include <string.h>

#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 25

unsigned char ctk_draw_windowborder_height = 1;
unsigned char ctk_draw_windowborder_width = 1;
unsigned char ctk_draw_windowtitle_height = 1;

static unsigned char cursx, cursy, reversed, underline;
static unsigned char color;             /* C64 colour byte: (fg << 4) | bg */
static const unsigned char *bitmapptr;

extern struct ctk_hires_theme ctk_hires_theme;

/* Brightness of the 16 C64 colours, to decide normal or reversed cells. */
static const unsigned char lum[16] =
  {0, 15, 4, 11, 6, 9, 3, 13, 6, 3, 9, 5, 8, 13, 8, 11};

#define VRAM ((unsigned char __far *)0xB8000000UL)

/*---------------------------------------------------------------------------*/
/* One 8x8 cell. "inv" set: white background, black ink (the normal look of
   windows); clear: pattern bits are drawn white on black. */
static void
cell(unsigned char cx, unsigned char cy, const unsigned char *pat,
     unsigned char inv)
{
  unsigned char __far *p;
  unsigned char x = inv ? 0xff : 0x00;
  unsigned short off;

  if(cx >= SCREEN_WIDTH || cy >= SCREEN_HEIGHT) {
    return;
  }
  off = (unsigned short)cy * 320 + cx;   /* 4 even scanlines of 80 bytes */
  p = VRAM + off;
  p[0]        = pat[0] ^ x;
  p[0x2000]   = pat[1] ^ x;
  p[80]       = pat[2] ^ x;
  p[0x2050]   = pat[3] ^ x;
  p[160]      = pat[4] ^ x;
  p[0x20a0]   = pat[5] ^ x;
  p[240]      = pat[6] ^ x;
  p[0x20f0]   = pat[7] ^ x;
}
/*---------------------------------------------------------------------------*/
/* Alci CGA theme: drawn for 640x200, whose pixels are about 2.4 times taller
   than wide. Vertical strokes are two pixels wide, horizontal ones one pixel
   tall. Windows: thin left and top edges, a thick right and lower edge that
   reads as a shadow, a striped title bar when focused. Buttons are rounded
   "pills". 1 = black ink on white paper. */
static const unsigned char cga_ul[8]  = {0x3f,0x7f,0xff,0xff,0xff,0xff,0xff,0xff};
static const unsigned char cga_tb[8]  = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
static const unsigned char cga_tbi[8] = {0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff};
static const unsigned char cga_uli[8] = {0x3f,0x60,0xc0,0xc0,0xc0,0xc0,0xc0,0xff};
static const unsigned char cga_ur[8]  = {0xf0,0xf8,0xfc,0xfc,0xff,0xff,0xff,0xff};
static const unsigned char cga_uri[8] = {0xf0,0x18,0x0c,0x0c,0x0f,0x0f,0x0f,0xff};
static const unsigned char cga_r[8]   = {0x0c,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f};
static const unsigned char cga_lr[8]  = {0x0f,0x0f,0x0f,0x0f,0x0f,0x1f,0xf3,0xff};
static const unsigned char cga_lo[8]  = {0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff};
static const unsigned char cga_ll[8]  = {0xc0,0xc0,0xc0,0xc0,0xc0,0x60,0x3f,0x0f};
static const unsigned char cga_l[8]   = {0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0};
static const unsigned char cga_bl[8]  = {0x07,0x0c,0x18,0x18,0x18,0x18,0x0c,0x07};
static const unsigned char cga_br[8]  = {0xe0,0x30,0x18,0x18,0x18,0x18,0x30,0xe0};
static const unsigned char cga_desk[8]= {0x88,0x00,0x22,0x00,0x88,0x00,0x22,0x00};

static void
cga_theme(void)
{
  struct ctk_hires_theme *t = &ctk_hires_theme;
  memcpy(t->ulcorner, cga_ul, 8);
  memcpy(t->titlebar, cga_tb, 8);
  memcpy(t->urcorner, cga_ur, 8);
  memcpy(t->rightborder, cga_r, 8);
  memcpy(t->lrcorner, cga_lr, 8);
  memcpy(t->lowerborder, cga_lo, 8);
  memcpy(t->llcorner, cga_ll, 8);
  memcpy(t->leftborder, cga_l, 8);
  memcpy(t->buttonleftpattern, cga_bl, 8);
  memcpy(t->buttonrightpattern, cga_br, 8);
}
/*---------------------------------------------------------------------------*/
/* Extra lines added to text cells: top and bottom rows, for title bars,
   buttons and the menu bar. */
static unsigned char deco_top, deco_bot;
/*---------------------------------------------------------------------------*/
/* Is the current colour drawn with black ink on white paper? */
static unsigned char
paper(void)
{
  unsigned char fg = color >> 4, bg = color & 15;
  unsigned char inv = lum[bg] >= lum[fg];
  return reversed ? !inv : inv;
}
/*---------------------------------------------------------------------------*/
static void
putc_cell(unsigned char c)
{
  unsigned char g[8];
  const unsigned char *f;

  if(c >= 128) {
    c = '?';
  }
  f = font8x8[c];
  if(underline || deco_top || deco_bot) {
    memcpy(g, f, 8);
    if(underline) {
      g[7] = 0xff;
    }
    g[0] |= deco_top;
    g[7] |= deco_bot;
    f = g;
  }
  cell(cursx, cursy, f, paper());
  ++cursx;
}
/*---------------------------------------------------------------------------*/
static void
cputsn(const char *s, unsigned char len)
{
  while(len-- && *s) {
    putc_cell((unsigned char)*s++);
  }
}
/*---------------------------------------------------------------------------*/
static void
cclear(unsigned char len)
{
  unsigned char blank[8] = {0,0,0,0,0,0,0,0};
  unsigned char inv = paper();
  blank[0] = deco_top;
  blank[7] = deco_bot;
  while(len--) {
    cell(cursx++, cursy, blank, inv);
  }
}
/*---------------------------------------------------------------------------*/
static void
chline(unsigned char len)
{
  static const unsigned char line[8] = {0,0,0,0xff,0,0,0,0};
  unsigned char inv = paper();
  while(len--) {
    cell(cursx++, cursy, line, inv);
  }
}
/*---------------------------------------------------------------------------*/
static void
cvline(unsigned char len)
{
  static const unsigned char line[8] = {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18};
  unsigned char inv = paper();
  while(len--) {
    cell(cursx, cursy++, line, inv);
  }
}
/*---------------------------------------------------------------------------*/
#define gotoxy(x, y) do { cursx = (x); cursy = (y); } while(0)
/*---------------------------------------------------------------------------*/
static void
draw_bitmapline(unsigned char len)
{
  unsigned char inv = paper();
  while(len--) {
    cell(cursx++, cursy, bitmapptr, inv);
    bitmapptr += 8;
  }
}
/*---------------------------------------------------------------------------*/
/* Window frame: title row with corners and title-bar pattern, side borders
   and lower border, clipped to [clipy1, clipy2) in screen rows. */
static void
draw_windowborders(unsigned char x, unsigned char y, unsigned char w,
                   unsigned char h, unsigned char clipy1,
                   unsigned char clipy2, unsigned char focus,
                   const char *title, unsigned char titlelen)
{
  unsigned char i, r, xr = x + w + 1, yb = y + h + 1;
  unsigned char active = (focus & (CTK_FOCUS_WINDOW | CTK_FOCUS_DIALOG)) != 0;
  const struct ctk_hires_theme *t = &ctk_hires_theme;

  if(y >= clipy1 && y < clipy2) {
    cell(x, y, active ? t->ulcorner : cga_uli, 1);
    gotoxy(x + 1, y);
    color = (0 << 4) | 1;               /* black ink on white */
    reversed = active;                  /* white title on the black bar */
    deco_top = active ? 0 : 0xff;
    deco_bot = active ? 0 : 0xff;
    putc_cell(' ');
    for(i = 0; i < titlelen && cursx < xr - 3; ++i) {
      putc_cell((unsigned char)title[i]);
    }
    putc_cell(' ');
    deco_top = deco_bot = 0;
    reversed = 0;
    while(cursx < xr) {
      cell(cursx++, y, active ? t->titlebar : cga_tbi, 1);
    }
    cell(xr, y, active ? t->urcorner : cga_uri, 1);
  }
  for(r = y + 1; r < yb; ++r) {
    if(r >= clipy1 && r < clipy2) {
      cell(x, r, t->leftborder, 1);
      cell(xr, r, t->rightborder, 1);
    }
  }
  if(yb >= clipy1 && yb < clipy2) {
    cell(x, yb, t->llcorner, 1);
    for(i = x + 1; i < xr; ++i) {
      cell(i, yb, t->lowerborder, 1);
    }
    cell(xr, yb, t->lrcorner, 1);
  }
}
/*---------------------------------------------------------------------------*/
static void
clear_line(unsigned char line)
{
  /* Light grey desktop: sparse dots drawn as ink on white. (The C64 theme
     pattern relies on per-row colours and turns into heavy stripes on a
     two-colour screen.) */
  unsigned char i;
  for(i = 0; i < SCREEN_WIDTH; ++i) {
    cell(i, line, cga_desk, 1);
  }
  if(line == SCREEN_HEIGHT - 1) {
    color = (0 << 4) | 1;
    reversed = 0;
    gotoxy(0, line);
    cputsn(" Alci ", 60);
  }
}
/*---------------------------------------------------------------------------*/
/* BIOS video mode, from user space: ELKS runs in real mode. */
static void
set_mode(unsigned char mode)
{
  __asm__ volatile ("push %%bp\n\tint $0x10\n\tpop %%bp"
                    : : "a" ((unsigned short)mode)
                    : "bx", "cx", "dx", "si", "di", "memory", "cc");
}
/*---------------------------------------------------------------------------*/
void
ctk_cga_exit(void)
{
  set_mode(3);
}
/*---------------------------------------------------------------------------*/
void
ctk_draw_init(void)
{
  cga_theme();
  set_mode(6);
  ctk_draw_clear(0, SCREEN_HEIGHT);
}
/*---------------------------------------------------------------------------*/
static void
draw_widget(struct ctk_widget *w, unsigned char x, unsigned char y,
            unsigned char clipy1, unsigned char clipy2, unsigned char focus)
{
  unsigned char xpos = x + w->x, ypos = y + w->y;
  unsigned char yclipped = (ypos >= clipy1 && ypos < clipy2);
  unsigned char i, len, tmp, xscroll;
  char *text;
  char c;

  switch(w->type) {
  case CTK_WIDGET_SEPARATOR:
    color = ctk_hires_theme.separatorcolors[focus];
    if(yclipped) {
      gotoxy(xpos, ypos);
      chline(w->w);
    }
    break;
  case CTK_WIDGET_LABEL:
    color = ctk_hires_theme.labelcolors[focus];
    text = w->widget.label.text;
    for(i = 0; i < w->h; ++i) {
      if(ypos >= clipy1 && ypos < clipy2) {
        gotoxy(xpos, ypos);
        cputsn(text, w->w);
        tmp = w->w - (cursx - xpos);
        if(tmp > 0 && tmp <= w->w) {
          cclear(tmp);
        }
      }
      ++ypos;
      text += w->w;
    }
    break;
  case CTK_WIDGET_BUTTON:
    if(yclipped && w->window != NULL &&
       (w == (struct ctk_widget *)&w->window->titlebutton ||
        w == (struct ctk_widget *)&w->window->closebutton)) {
      /* Title and close buttons live in the title bar: plain text, white
         on the black bar of the focused window, black on white otherwise;
         swapped when the keyboard focus is on them. */
      unsigned char act = (focus & (CTK_FOCUS_WINDOW | CTK_FOCUS_DIALOG)) != 0;
      color = (0 << 4) | 1;
      reversed = act ^ ((focus & CTK_FOCUS_WIDGET) != 0);
      deco_top = deco_bot = act ? 0 : 0xff;
      gotoxy(xpos, ypos);
      putc_cell(' ');
      cputsn(w->widget.button.text, w->w);
      putc_cell(' ');
      deco_top = deco_bot = 0;
      reversed = 0;
      break;
    }
    if(yclipped) {
      color = ctk_hires_theme.buttoncolors[focus];
      gotoxy(xpos, ypos);
      cell(cursx++, ypos, ctk_hires_theme.buttonleftpattern, paper());
      deco_top = deco_bot = 0xff;
      cputsn(w->widget.button.text, w->w);
      deco_top = deco_bot = 0;
      cell(cursx++, ypos, ctk_hires_theme.buttonrightpattern, paper());
    }
    break;
  case CTK_WIDGET_HYPERLINK:
    if(yclipped) {
      color = ctk_hires_theme.hyperlinkcolors[focus];
      underline = 1;
      gotoxy(xpos, ypos);
      cputsn(w->widget.button.text, w->w);
      underline = 0;
    }
    break;
  case CTK_WIDGET_TEXTENTRY:
    if(yclipped) {
      color = ctk_hires_theme.textentrycolors[focus];
      reversed = (focus & CTK_FOCUS_WIDGET) &&
        w->widget.textentry.state != CTK_TEXTENTRY_EDIT;
      xscroll = 0;
      tmp = w->w - 1;
      if(w->widget.textentry.xpos >= tmp) {
        xscroll = w->widget.textentry.xpos - tmp;
      }
      text = w->widget.textentry.text;
      if(w->widget.textentry.state == CTK_TEXTENTRY_EDIT) {
        reversed = 0;
        gotoxy(xpos, ypos);
        putc_cell('>');
        text += xscroll;
        for(i = 0; i < w->w; ++i) {
          c = *text;
          reversed = (i == w->widget.textentry.xpos - xscroll);
          putc_cell(c == 0 ? ' ' : (unsigned char)c);
          reversed = 0;
          if(c) {
            ++text;
          }
        }
        putc_cell('<');
      } else {
        gotoxy(xpos, ypos);
        putc_cell('|');
        cputsn(text, w->w);
        tmp = cursx - xpos - 1;
        if(tmp < w->w) {
          cclear(w->w - tmp);
        }
        putc_cell('|');
      }
    }
    reversed = 0;
    break;
  case CTK_WIDGET_ICON:
    if(yclipped) {
      color = ctk_hires_theme.iconcolors[focus];
      len = strlen(w->widget.icon.title);
      x = xpos;
      if(x + len >= SCREEN_WIDTH) {
        x = SCREEN_WIDTH - len;
      }
      tmp = ypos + 3;
      if(tmp < clipy2) {
        gotoxy(x, tmp);
        cputsn(w->widget.icon.title, len);
      }
#if CTK_CONF_ICON_BITMAPS
      if(w->widget.icon.bitmap != NULL) {
        bitmapptr = w->widget.icon.bitmap;
        for(i = 0; i < 3; ++i) {
          if(ypos >= clipy1 && ypos < clipy2) {
            gotoxy(xpos, ypos);
            draw_bitmapline(3);
          } else {
            bitmapptr += 3 * 8;
          }
          ++ypos;
        }
      }
#endif
    }
    break;
  case CTK_WIDGET_BITMAP:
    color = ctk_hires_theme.bitmapcolors[focus];
    bitmapptr = w->widget.bitmap.bitmap;
    for(i = 0; i < w->h; ++i) {
      if(ypos >= clipy1 && ypos < clipy2) {
        gotoxy(xpos, ypos);
        draw_bitmapline(w->w);
      } else {
        bitmapptr += w->w * 8;
      }
      ++ypos;
    }
    break;
  default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
void
ctk_draw_widget(struct ctk_widget *w, unsigned char focus,
                unsigned char clipy1, unsigned char clipy2)
{
  struct ctk_window *win = w->window;

  if(w == win->focused) {
    focus |= CTK_FOCUS_WIDGET;
  }
  draw_widget(w, win->x + 1, win->y + 2, clipy1, clipy2, focus);
}
/*---------------------------------------------------------------------------*/
void
ctk_draw_clear_window(struct ctk_window *window, unsigned char focus,
                      unsigned char clipy1, unsigned char clipy2)
{
  unsigned char i, h = window->y + 2 + window->h;

  color = ctk_hires_theme.windowcolors[focus];
  reversed = 0;
  for(i = window->y + 2; i < h; ++i) {
    if(i >= clipy1 && i <= clipy2) {
      gotoxy(window->x + 1, i);
      cclear(window->w);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
ctk_draw_window(struct ctk_window *window, unsigned char focus,
                unsigned char clipy1, unsigned char clipy2,
                unsigned char draw_borders)
{
  struct ctk_widget *w;
  unsigned char x = window->x, y = window->y + 1, wf;

  ++clipy2;
  if(clipy2 <= y) {
    return;
  }
  if(draw_borders) {
    draw_windowborders(x, y, window->w, window->h, clipy1, clipy2, focus,
                       window->title, window->titlelen);
  }
  for(w = window->inactive; w != NULL; w = w->next) {
    draw_widget(w, x + 1, y + 1, clipy1, clipy2, focus);
  }
  for(w = window->active; w != NULL; w = w->next) {
    wf = focus;
    if(w == window->focused) {
      wf |= CTK_FOCUS_WIDGET;
    }
    draw_widget(w, x + 1, y + 1, clipy1, clipy2, wf);
  }
}
/*---------------------------------------------------------------------------*/
void
ctk_draw_dialog(struct ctk_window *dialog)
{
  struct ctk_widget *w;
  unsigned char x = dialog->x, y = dialog->y + 1, i, wf;
  unsigned char x1 = x + 1, y1 = y + 1, y2 = y1 + dialog->h;

  draw_windowborders(x, y, dialog->w, dialog->h, 0, SCREEN_HEIGHT,
                     CTK_FOCUS_DIALOG, dialog->title, dialog->titlelen);
  color = ctk_hires_theme.windowcolors[CTK_FOCUS_DIALOG];
  reversed = 0;
  for(i = y1; i < y2; ++i) {
    gotoxy(x1, i);
    cclear(dialog->w);
  }
  for(w = dialog->inactive; w != NULL; w = w->next) {
    draw_widget(w, x1, y1, 0, SCREEN_HEIGHT, CTK_FOCUS_DIALOG);
  }
  for(w = dialog->active; w != NULL; w = w->next) {
    wf = CTK_FOCUS_DIALOG;
    if(w == dialog->focused) {
      wf |= CTK_FOCUS_WIDGET;
    }
    draw_widget(w, x1, y1, 0, SCREEN_HEIGHT, wf);
  }
}
/*---------------------------------------------------------------------------*/
void
ctk_draw_clear(unsigned char y1, unsigned char y2)
{
  unsigned char i;
  for(i = y1; i < y2; ++i) {
    clear_line(i);
  }
}
/*---------------------------------------------------------------------------*/
static void
draw_menu(struct ctk_menu *m)
{
  unsigned char x, x2, y;

  color = ctk_hires_theme.openmenucolor;
  reversed = 0;
  x = cursx;
  cputsn(m->title, m->titlelen);
  putc_cell(' ');
  x2 = cursx;
  if(x + CTK_CONF_MENUWIDTH > SCREEN_WIDTH) {
    x = SCREEN_WIDTH - CTK_CONF_MENUWIDTH;
  }
  deco_bot = 0;
  for(y = 0; y < m->nitems; ++y) {
    color = ctk_hires_theme.menucolor;
    reversed = (y == m->active);
    deco_bot = (y == m->nitems - 1) ? 0xff : 0;
    gotoxy(x, y + 1);
    if(m->items[y].title[0] == '-') {
      chline(CTK_CONF_MENUWIDTH);
    } else {
      cputsn(m->items[y].title, strlen(m->items[y].title));
    }
    if(cursx < x + CTK_CONF_MENUWIDTH) {
      cclear(x + CTK_CONF_MENUWIDTH - cursx);
    }
    reversed = 0;
  }
  gotoxy(x2, 0);
  color = ctk_hires_theme.menucolor;
  deco_bot = 0xff;
}
/*---------------------------------------------------------------------------*/
void
ctk_draw_menus(struct ctk_menus *menus)
{
  struct ctk_menu *m;

  color = ctk_hires_theme.menucolor;
  reversed = 0;
  deco_bot = 0xff;
  gotoxy(0, 0);
  putc_cell(' ');
  for(m = menus->menus->next; m != NULL; m = m->next) {
    if(m != menus->open) {
      cputsn(m->title, m->titlelen);
      putc_cell(' ');
    } else {
      draw_menu(m);
    }
  }
  {
    unsigned char dl = strlen(menus->desktopmenu->title) + 1;
    if(cursx < SCREEN_WIDTH - dl) {
      cclear(SCREEN_WIDTH - dl - cursx);
    }
  }
  if(menus->desktopmenu != menus->open) {
    cputsn(menus->desktopmenu->title, menus->desktopmenu->titlelen);
    putc_cell(' ');
  } else {
    draw_menu(menus->desktopmenu);
  }
  deco_bot = 0;
}
/*---------------------------------------------------------------------------*/
unsigned char
ctk_draw_height(void)
{
  return SCREEN_HEIGHT;
}
/*---------------------------------------------------------------------------*/
unsigned char
ctk_draw_width(void)
{
  return SCREEN_WIDTH;
}
