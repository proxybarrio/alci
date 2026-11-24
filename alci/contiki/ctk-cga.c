/*
 * CTK drawing layer for ELKS on CGA 640x200, two colours (BIOS mode 6).
 * Prototype for Alci (task D7). The colour screens use ctk-gfx.c; this
 * layer keeps the Alci CGA theme P1 (theme/p1.py): double frame, black
 * title bar, dotted shadow, rounded buttons, the 8x8 font throughout.
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
#include "gfx.h"
#include "font8x8.h"

#include <string.h>

#define SCREEN_WIDTH  80
/* Desktop icons: the Alci CGA 640x200 set, 10x4 cells of 8x8 (icons/out/cga640-mono.h) */
#define ICON_W GFX_ICON_CW
#define ICON_H GFX_ICON_RH
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
/* Alci CGA theme (P1): drawn for 640x200, whose pixels are about 2.4 times
   taller than wide. Pieces from theme/p1.py: double window border with a
   dotted shadow, black striped title bar when focused, two-cell close box,
   outlined buttons and entries. 1 = black ink on white paper. */
#include "theme-p1.h"

/* Wallpaper: index into wallpapers[] (theme-p1.h), chosen before ctk_init
   (main.c: ctk -w name). The pieces that let the desktop show through, such
   as the shadows, are combined with it once at start-up. */
unsigned char ctk_cga_wallpaper;
static const unsigned char *t_desk;
#define DECLARE(n) static unsigned char u_##n[8];
THEME_THROUGH(DECLARE)

/* Index of a wallpaper by name, or 255. */
unsigned char
ctk_cga_wallpaper_find(const char *name)
{
  unsigned char i;
  for(i = 0; i < sizeof(wallpaper_names) / sizeof(wallpaper_names[0]); ++i) {
    if(strcmp(wallpaper_names[i], name) == 0) {
      return i;
    }
  }
  return 255;
}

static void
cga_theme(void)
{
  struct ctk_hires_theme *t = &ctk_hires_theme;
  unsigned char i;
  if(ctk_cga_wallpaper >= sizeof(wallpapers) / 8) {
    ctk_cga_wallpaper = 0;
  }
  t_desk = wallpapers[ctk_cga_wallpaper];
#define COMBINE(n) for(i = 0; i < 8; ++i) u_##n[i] = t_##n[i] | (t_##n##_d[i] & t_desk[i]);
  THEME_THROUGH(COMBINE)
  memcpy(t->ulcorner, u_ul, 8);
  memcpy(t->titlebar, t_tb, 8);
  memcpy(t->urcorner, u_ur, 8);
  memcpy(t->rightborder, u_r, 8);
  memcpy(t->lrcorner, u_lr, 8);
  memcpy(t->lowerborder, u_lo, 8);
  memcpy(t->llcorner, u_ll, 8);
  memcpy(t->leftborder, t_l, 8);
  memcpy(t->buttonleftpattern, t_bl, 8);
  memcpy(t->buttonrightpattern, t_br, 8);
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
  static const unsigned char line[8] = {0,0,0,0x55,0,0,0,0};
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
    cell(x, y, active ? t->ulcorner : u_ul_i, 1);
    cell(x + 1, y, active ? t_st : t_st_i, 1);
    gotoxy(x + 2, y);
    color = (0 << 4) | 1;               /* black ink on white */
    reversed = active;                  /* white title on the black bar */
    deco_top = deco_bot = 0;            /* no line through the letters */
    putc_cell(' ');
    for(i = 0; i < titlelen && cursx < xr - 4; ++i) {
      putc_cell((unsigned char)title[i]);
    }
    putc_cell(' ');
    deco_top = deco_bot = 0;
    reversed = 0;
    while(cursx < xr - 2) {
      cell(cursx++, y, active ? t->titlebar : t_tb_i, 1);
    }
    cell(xr - 2, y, active ? t_cl : t_cl_i, 1);
    cell(xr - 1, y, active ? t_cl2 : t_cl2_i, 1);
    cell(xr, y, active ? t->urcorner : u_ur_i, 1);
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
  if(line == SCREEN_HEIGHT - 2) {
    /* the line over the status bar, in the last pixel row above it */
    unsigned char d[8];
    memcpy(d, t_desk, 8);
    d[7] = 0xff;
    for(i = 0; i < SCREEN_WIDTH; ++i) {
      cell(i, line, d, 1);
    }
  } else {
    for(i = 0; i < SCREEN_WIDTH; ++i) {
      cell(i, line, t_desk, 1);
    }
  }
  if(line == SCREEN_HEIGHT - 1) {
    /* status line: white, with a line along its top edge */
    color = (0 << 4) | 1;
    reversed = 0;
    gotoxy(0, line);
    cputsn(" Alci", 60);
    cclear(SCREEN_WIDTH - cursx);
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
      /* Title and close buttons live in the title bar; the keyboard focus
         on one of them inverts it. */
      unsigned char act = (focus & (CTK_FOCUS_WINDOW | CTK_FOCUS_DIALOG)) != 0;
      unsigned char kf = (focus & CTK_FOCUS_WIDGET) != 0;
      color = (0 << 4) | 1;
      if(w == (struct ctk_widget *)&w->window->closebutton) {
        cell(xpos, ypos, act ? t_tb : t_tb_i, !kf);
        cell(xpos + 1, ypos, act ? t_cl : t_cl_i, !kf);
        cell(xpos + 2, ypos, act ? t_cl2 : t_cl2_i, !kf);
        break;
      }
      reversed = act ^ kf;
      deco_top = deco_bot = 0;
      cell(xpos, ypos, act ? t_st : t_st_i, !kf);
      gotoxy(xpos + 1, ypos);
      putc_cell(' ');
      cputsn(w->widget.button.text, w->w);
      putc_cell(' ');
      deco_top = deco_bot = 0;
      reversed = 0;
      break;
    }
    if(yclipped) {
      unsigned char kf = (focus & CTK_FOCUS_WIDGET) != 0;
      color = (0 << 4) | 1;
      gotoxy(xpos, ypos);
      cell(cursx++, ypos, kf ? t_bl_f : t_bl, 1);
      reversed = kf;                    /* focused: white text, black button */
      deco_top = deco_bot = 0;          /* brackets, no lines through the letters */
      cputsn(w->widget.button.text, w->w);
      deco_top = deco_bot = 0;
      reversed = 0;
      cell(cursx++, ypos, kf ? t_br_f : t_br, 1);
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
      {
        unsigned char kf = (focus & CTK_FOCUS_WIDGET) != 0;
        unsigned char edit = w->widget.textentry.state == CTK_TEXTENTRY_EDIT;
        color = (0 << 4) | 1;
        reversed = 0;
        gotoxy(xpos, ypos);
        cell(cursx++, ypos, kf ? t_el_f : t_el, 1);
        deco_top = 0;
        deco_bot = 0xff;                /* a line under the text */
        text += xscroll;
        for(i = 0; i < w->w; ++i) {
          c = *text;
          reversed = edit && (i == w->widget.textentry.xpos - xscroll);
          putc_cell(c == 0 ? ' ' : (unsigned char)c);
          if(c) {
            ++text;
          }
        }
        reversed = 0;
        deco_top = deco_bot = 0;
        cell(cursx++, ypos, kf ? t_er_f : t_er, 1);
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
      if(len < ICON_W) {
        x = xpos + (ICON_W - len) / 2;  /* title centred under the icon */
      }
      tmp = ypos + ICON_H;
      if(tmp < clipy2) {
        /* focus: only the title is inverted, the drawing stays as it is */
        color = (0 << 4) | 1;
        reversed = (focus & CTK_FOCUS_WIDGET) != 0;
        gotoxy(x, tmp);
        cputsn(w->widget.icon.title, len);
        reversed = 0;
      }
#if CTK_CONF_ICON_BITMAPS
      if(w->widget.icon.bitmap != NULL) {
        /* the icon of this screen type, stored as 8x8 cells (gfx.h) */
        bitmapptr = ((const struct gfx_icon *)w->widget.icon.bitmap)->px;
        for(i = 0; i < ICON_H; ++i) {
          if(ypos >= clipy1 && ypos < clipy2) {
            gotoxy(xpos, ypos);
            draw_bitmapline(ICON_W);
          } else {
            bitmapptr += ICON_W * 8;
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
  cell(0, 0, t_menul, 1);
  gotoxy(1, 0);
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
