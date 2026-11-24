/*
 * CTK drawing layer for ELKS/Alci, one source for every screen type (task D7):
 * CGA 640x200 two colours, CGA 320x200 four colours, EGA 640x350, VGA 640x480
 * and MCGA 320x200. One program is built per screen type (gfx.h).
 *
 * The screen is a grid of cells, 8 pixels wide and GFX_CH high, as in the
 * Commodore 64 "hires" driver of Contiki 1.x (contiki-c64/ctk/ctk-hires.c,
 * Copyright (c) 2002-2004, Adam Dunkels, BSD 3-clause), whose structure this
 * file keeps. Each cell is composed in a small buffer of colour indices, then
 * written to video memory by the routine of the screen type. Two styles:
 * on two-colour screens the finish comes from shape (striped title bar with
 * the title on a plain plate, close box on the left, solid shadow, corners
 * cut by one pixel); on colour screens a two-tone or graded title bar with a
 * light line after the title, a stepped or soft shadow and an etched look.
 *
 * Copyright (c) 2002-2004, Adam Dunkels (original C64 driver).
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions of the
 * BSD 3-clause licence in the Contiki 1.x sources are met.
 */

#include "ctk.h"
#include "ctk-draw.h"
#include "gfx.h"
#include "font8x8.h"
#if defined(GFX_CGA)
#include "font-bar.h"
#endif

#include <string.h>

#define SW GFX_SW
#define SH GFX_SH
#define CH GFX_CH

unsigned char ctk_draw_windowborder_height = 1;
unsigned char ctk_draw_windowborder_width = 1;
unsigned char ctk_draw_windowtitle_height = 1;

/*---------------------------------------------------------------------------*/
/* Colours of each screen type. Roles: paper and ink of windows, title bars
   (top and bottom half, active and inactive), light line on the bar, frame,
   shadow, etched lines, focus ring, links. */
#if defined(GFX_CGA)
#define MONO 1
enum { C_PAPER = 0, C_INK = 1 };
#define C_EDGE_A C_INK
#define C_EDGE_I C_INK
#elif defined(GFX_CGA4)
/* palette 1, high intensity: 0 black, 1 cyan, 2 magenta, 3 white */
enum { C_PAPER = 0, C_INK = 3, C_BAR_AT = 2, C_BAR_AB = 2, C_BAR_IT = 0,
       C_BAR_IB = 0, C_HL = 3, C_EDGE_A = 1, C_EDGE_I = 1, C_SH1 = 2,
       C_SH2 = 2, C_ETCH0 = 1, C_ETCH1 = 0, C_RING = 1, C_TITLE_A = 3,
       C_TITLE_I = 3, C_LINK = 1, C_STATUS = 0, C_DIM = 1 };
#elif defined(GFX_EGA)
/* the 16 colours of the EGA icon set (icons/out/ega640x350-16col.ais) */
enum { C_INK = 0, C_PAPER = 1, C_HL = 2, C_BAR_AT = 3, C_BAR_AB = 4,
       C_BAR_IT = 1, C_BAR_IB = 6, C_EDGE_A = 15, C_EDGE_I = 7, C_SH1 = 7,
       C_SH2 = 6, C_ETCH0 = 7, C_ETCH1 = 1, C_RING = 2, C_TITLE_A = 1,
       C_TITLE_I = 0, C_LINK = 4, C_STATUS = 1, C_DIM = 7, C_GREY = 6 };
static const unsigned char pal16[16][3] = {
  {0,0,0},{255,255,255},{170,170,255},{85,85,255},{0,0,255},{255,255,0},
  {170,170,170},{85,85,85},{255,255,170},{0,255,0},{85,170,255},{0,255,255},
  {255,0,255},{255,170,0},{85,85,170},{0,0,85}};
#elif defined(GFX_VGA)
/* the 16 colours of the VGA icon set (icons/out/vga640-16col.ais) */
enum { C_INK = 0, C_PAPER = 1, C_HL = 3, C_BAR_AT = 14, C_BAR_AB = 6,
       C_BAR_IT = 1, C_BAR_IB = 15, C_EDGE_A = 6, C_EDGE_I = 7, C_SH1 = 7,
       C_SH2 = 15, C_ETCH0 = 15, C_ETCH1 = 1, C_RING = 4, C_TITLE_A = 1,
       C_TITLE_I = 0, C_LINK = 14, C_STATUS = 1, C_DIM = 7, C_GREY = 15,
       C_TINT = 3 };
static const unsigned char pal16[16][3] = {
  {0,0,0},{250,251,252},{251,248,222},{208,214,249},{160,168,226},
  {92,123,190},{12,40,110},{58,64,80},{252,230,148},{245,205,30},
  {110,236,20},{190,248,135},{222,80,164},{240,200,232},{25,110,236},
  {170,176,192}};
#elif defined(GFX_MCGA)
/* own palette: fixed colours, graded bars (8 rows), a graded desktop with
   two darker copies for the soft shadow, icons from 200 */
enum { C_INK = 2, C_PAPER = 1, C_HL = 3, C_EDGE_A = 4, C_EDGE_I = 5,
       C_ETCH0 = 6, C_ETCH1 = 1, C_RING = 7, C_TITLE_A = 1, C_TITLE_I = 2,
       C_LINK = 8, C_STATUS = 9, C_DIM = 6, C_SHBASE = 10,
       C_BAR_A = 16, C_BAR_I = 24, C_DESK = 32, C_DESK_SH1 = 64,
       C_DESK_SH2 = 96, DESK_STEPS = 32 };
#define C_SH1 C_DESK_SH1
#define C_SH2 C_DESK_SH2
#define C_BAR_AT (C_BAR_A + 1)
#define C_BAR_AB (C_BAR_A + 6)
#define C_BAR_IT (C_BAR_I + 1)
#define C_BAR_IB (C_BAR_I + 6)
static const unsigned char fixed[11][3] = {
  {0,0,0},{255,255,255},{27,42,68},{185,210,245},{29,53,97},{142,155,176},
  {154,168,189},{169,196,238},{29,82,168},{247,249,252},{26,40,64}};
static const unsigned char bar_a[4][3] = {{74,130,214},{47,102,190},{29,82,168},{17,63,135}};
static const unsigned char bar_i[4][3] = {{244,246,250},{230,234,241},{219,225,234},{207,214,225}};
static const unsigned char desk_rgb[2][3] = {{238,242,248},{206,216,232}};
#endif

#if defined(MONO)
#define C_SEL_FG C_PAPER
#define C_SEPLINE C_INK
#else
#define C_SEL_FG C_TITLE_A
#define C_SEPLINE C_DIM
#endif

/* Shadow width and height in pixels, inside the window's own border cells */
#if defined(MONO)
#define SHX 3
#define SHY 1
#elif CH == 8
#define SHX 3
#define SHY 2
#else
#define SHX 4
#define SHY 4
#endif
#define FX 3                    /* the frame runs in pixel column 3 of the border cells */

/* Lines along the active title bar on CGA, as on the approved trial of
   2026-11-23: the frame, three thin lines and the line under the bar at
   the same distance, five lines in nine pixel rows. The frame goes in the
   last pixel row of the cell row above the title (TOP_OUTSIDE), so that
   the bar keeps rows 0-7: lines on rows 1, 3 and 5, the line under the bar
   on row 7, the title on its plate in rows 0-6. */
#define TOP_OUTSIDE 1
#define BAR_SEP 7
static const unsigned char bar_lines[8] = {0, 1, 0, 1, 0, 1, 0, 0};

/*---------------------------------------------------------------------------*/
/* Low level: BIOS, ports, video memory */
static void
bios10(unsigned short ax, unsigned short bx)
{
  __asm__ volatile ("push %%bp\n\tint $0x10\n\tpop %%bp"
                    : "+a" (ax), "+b" (bx)
                    : : "cx", "dx", "si", "di", "memory", "cc");
}
static void
outb(unsigned short port, unsigned char v)
{
  __asm__ volatile ("outb %%al,%%dx" : : "a" (v), "d" (port));
}

#if defined(GFX_CGA) || defined(GFX_CGA4)
#define VRAM ((unsigned char __far *)0xB8000000UL)
#else
#define VRAM ((volatile unsigned char __far *)0xA0000000UL)
#endif

/* The cell being composed: one colour index per pixel */
static unsigned char pix[CH][8];

#if defined(GFX_CGA)
/* Two colours: cells are also kept as 8 bytes, one per row, 1 = black ink,
   so that the frequent cells (desktop, text, buttons) skip the pixel buffer */
static unsigned char mb[8];

static void
blit_bytes(unsigned char cx, unsigned char cy, const unsigned char *b)
{
  unsigned char __far *p;
  if(cx >= SW || cy >= SH) {
    return;
  }
  p = VRAM + (unsigned short)cy * 320 + cx;
  p[0]      = ~b[0];
  p[0x2000] = ~b[1];
  p[80]     = ~b[2];
  p[0x2050] = ~b[3];
  p[160]    = ~b[4];
  p[0x20a0] = ~b[5];
  p[240]    = ~b[6];
  p[0x20f0] = ~b[7];
}
/* the pixel buffer packed into mb */
static void
pack(void)
{
  unsigned char r, c;
  for(r = 0; r < 8; ++r) {
    unsigned char b = 0;
    for(c = 0; c < 8; ++c) {
      b = (b << 1) | (pix[r][c] != C_PAPER);
    }
    mb[r] = b;
  }
}
#define blit_again(cx, cy) blit_bytes(cx, cy, mb)
#elif defined(GFX_CGA4)
/* four colours: the last cell written, packed, 2 bytes a row */
static unsigned char cb4[16];
static void
write4(unsigned char cx, unsigned char cy)
{
  unsigned char r;
  unsigned char __far *p;
  if(cx >= SW || cy >= SH) {
    return;
  }
  p = VRAM + (unsigned short)cy * 320 + cx * 2;
  (void)r;
  p[0] = cb4[0];        p[1] = cb4[1];
  p[0x2000] = cb4[2];   p[0x2001] = cb4[3];
  p[80] = cb4[4];       p[81] = cb4[5];
  p[0x2050] = cb4[6];   p[0x2051] = cb4[7];
  p[160] = cb4[8];      p[161] = cb4[9];
  p[0x20a0] = cb4[10];  p[0x20a1] = cb4[11];
  p[240] = cb4[12];     p[241] = cb4[13];
  p[0x20f0] = cb4[14];  p[0x20f1] = cb4[15];
}
#define blit_again(cx, cy) write4(cx, cy)
#else
#define blit_again(cx, cy) blit(cx, cy)
#endif

static void
blit(unsigned char cx, unsigned char cy)
{
  unsigned char r, c;
  if(cx >= SW || cy >= SH) {
    return;
  }
#if defined(GFX_CGA)
  (void)r; (void)c;
  pack();
  blit_bytes(cx, cy, mb);
#elif defined(GFX_CGA4)
  for(r = 0; r < 8; ++r) {
    const unsigned char *q = pix[r];
    cb4[2 * r] = (q[0] << 6) | (q[1] << 4) | (q[2] << 2) | q[3];
    cb4[2 * r + 1] = (q[4] << 6) | (q[5] << 4) | (q[6] << 2) | q[7];
  }
  (void)c;
  write4(cx, cy);
#elif defined(GFX_MCGA)
  {
    volatile unsigned char __far *p = VRAM + (unsigned short)cy * (8 * 320) + cx * 8;
    for(r = 0; r < 8; ++r, p += 320) {
      for(c = 0; c < 8; ++c) {
        p[c] = pix[r][c];
      }
    }
  }
#else
  /* EGA and VGA planes, write mode 2: for each colour of a pixel row, the
     bit mask selects its pixels and one write sets all four planes. */
  {
    volatile unsigned char __far *p = VRAM + (unsigned short)cy * (CH * 80) + cx;
    for(r = 0; r < CH; ++r, p += 80) {
      unsigned char done = 0;
      for(c = 0; c < 8; ++c) {
        unsigned char col, mask, k;
        if(done & (0x80 >> c)) {
          continue;
        }
        col = pix[r][c];
        mask = 0;
        for(k = c; k < 8; ++k) {
          if(pix[r][k] == col) {
            mask |= 0x80 >> k;
          }
        }
        done |= mask;
        outb(0x3ce, 8);
        outb(0x3cf, mask);
        (void)*p;                       /* load the latches */
        *p = col;
      }
    }
    outb(0x3ce, 8);
    outb(0x3cf, 0xff);
  }
#endif
}
/*---------------------------------------------------------------------------*/
/* Fonts: 8x8 for 8-pixel cells, the small bar font, the BIOS font otherwise */
#if CH > 8
static unsigned char bfont[96][CH];
static void
load_bios_font(void)
{
  unsigned short seg, off;
  unsigned char i, r;
  const unsigned char __far *f;
  __asm__ volatile ("push %%bp\n\tpush %%es\n\tint $0x10\n\tmov %%es,%%ax\n\t"
                    "mov %%bp,%%dx\n\tpop %%es\n\tpop %%bp"
                    : "=a" (seg), "=d" (off)
                    : "a" (0x1130), "b" (CH == 14 ? 0x0200 : 0x0600)
                    : "cx", "si", "di", "memory", "cc");
  f = (const unsigned char __far *)(((unsigned long)seg << 16) | off);
  for(i = 0; i < 96; ++i) {
    for(r = 0; r < CH; ++r) {
      bfont[i][r] = f[(unsigned short)(i + 32) * CH + r];
    }
  }
}
#endif

static unsigned char barfont;   /* 1: bar text (small font on 8-pixel cells) */

static const unsigned char *
glyph(unsigned char c)
{
  if(c < 32 || c >= 128) {
    c = '?';
  }
#if CH > 8
  return bfont[c - 32];
#else
#if defined(MONO)
  /* two colours: the small Clean 6x6 font in the bars, as on the approved
     trial; the 8x8 font in the windows */
  if(barfont) {
    return font_bar[c - 32];
  }
#endif
  return font8x8[c];
#endif
}
/*---------------------------------------------------------------------------*/
/* Composing a cell */
static unsigned char rowbg[CH];         /* background colour of each pixel row */

static void
rows_solid(unsigned char col)
{
  memset(rowbg, col, CH);
}
/* Bar rows: top half and bottom half, a light first row on graded bars */
static void
rows_bar(unsigned char active)
{
  unsigned char r;
  for(r = 0; r < CH; ++r) {
#if defined(MONO)
    rowbg[r] = C_PAPER;
#elif defined(GFX_MCGA)
    rowbg[r] = (active ? C_BAR_A : C_BAR_I) + r;
#else
    rowbg[r] = r < CH / 2 ? (active ? C_BAR_AT : C_BAR_IT) : (active ? C_BAR_AB : C_BAR_IB);
#endif
  }
}
static void
pix_rows(void)
{
  unsigned char r;
  for(r = 0; r < CH; ++r) {
    memset(pix[r], rowbg[r], 8);
  }
}
static void
pix_glyph(unsigned char c, unsigned char fg)
{
  const unsigned char *g = glyph(c);
  unsigned char r, x, rows = CH;
  for(r = 0; r < rows; ++r) {
    unsigned char b = g[r];
    for(x = 0; x < 8; ++x) {
      if(b & (0x80 >> x)) {
        pix[r][x] = fg;
      }
    }
  }
}
static void
pix_hline(unsigned char r, unsigned char x0, unsigned char x1, unsigned char col)
{
  for(; x0 <= x1; ++x0) {
    pix[r][x0] = col;
  }
}
static void
pix_vline(unsigned char x, unsigned char r0, unsigned char r1, unsigned char col)
{
  for(; r0 <= r1; ++r0) {
    pix[r0][x] = col;
  }
}
/*---------------------------------------------------------------------------*/
/* Desktop */
#if defined(MONO)
unsigned char ctk_cga_wallpaper;
static const char * const wallpaper_names[4] = {"dots", "pairs", "slash", "none"};
static const unsigned char wallpapers[4][8] = {
  {0x00,0x00,0x10,0x00,0x00,0x00,0x01,0x00},
  {0x00,0x00,0x18,0x00,0x00,0x00,0x81,0x00},
  {0x00,0x10,0x20,0x00,0x00,0x01,0x02,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};
static const unsigned char *wall;
#else
unsigned char ctk_cga_wallpaper;
static const char * const wallpaper_names[1] = {"default"};
#endif

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

/* x of the etched line beside the icon column, in pixels */
#define SEP_X ((SW - CTK_ICONX_FROM_RIGHT - 3) * 8 + 3)

static unsigned char desk_nosep;        /* 1: leave out the icon column line */

static unsigned char
desk(unsigned short X, unsigned short Y)
{
#if CH == 8 && !defined(MONO)
  if(Y == (SH - 1) * CH - 1) {
    return C_ETCH0;                     /* line over the status bar */
  }
#endif
  if(!desk_nosep && Y >= CH + CH / 2 && Y < (SH - 1) * CH - CH / 2) {
    if(X == SEP_X) {
#if defined(MONO)
      return C_INK;
#else
      return C_ETCH0;
#endif
    }
#if !defined(MONO)
    if(X == SEP_X + 1) {
      return C_ETCH1;
    }
#endif
  }
#if defined(MONO)
  return (wall[Y & 7] & (0x80 >> (X & 7))) ? C_INK : C_PAPER;
#elif defined(GFX_CGA4)
  return 0;
#elif defined(GFX_MCGA)
  return C_DESK + (unsigned char)(Y * DESK_STEPS / (SH * CH));
#else
  {
    /* white above, a fine checker of white and grey across the middle,
       grey below: the stepped shading of the 16-colour cards */
    unsigned char band = (unsigned char)(Y * 5 / (SH * CH));
    if(band < 2) {
      return C_PAPER;
    }
    if(band < 4) {
      return ((X ^ Y) & 1) ? C_GREY : C_PAPER;
    }
    return C_GREY;
  }
#endif
}
/* The desktop under a shadow: one or two steps darker */
static unsigned char
shade(unsigned short X, unsigned short Y, unsigned char step)
{
#if defined(MONO)
  return C_INK;
#elif defined(GFX_MCGA)
  unsigned char d = desk(X, Y);
  if(d < C_DESK || d >= C_DESK + DESK_STEPS) {
    d = C_DESK + DESK_STEPS - 1;
  }
  return d + (step == 1 ? C_DESK_SH1 - C_DESK : C_DESK_SH2 - C_DESK);
#elif defined(GFX_CGA4)
  return ((X ^ Y) & 1) ? C_SH1 : desk(X, Y);
#else
  return step == 1 ? C_SH1 : C_SH2;
#endif
}

static void
desk_cell(unsigned char cx, unsigned char cy)
{
  unsigned char r, c;
#if defined(MONO)
  /* the wallpaper bytes, and the icon column line where it runs */
  unsigned short Y = cy * CH;
  for(r = 0; r < 8; ++r) {
    mb[r] = (cy == SH - 2 && r == 7) ? 0xff : wall[r];   /* line over the status bar */
    if(cx == SEP_X / 8 && Y + r >= CH + CH / 2 && Y + r < (SH - 1) * CH - CH / 2) {
      mb[r] |= 0x80 >> (SEP_X & 7);
    }
  }
  (void)c;
  blit_bytes(cx, cy, mb);
#else
  unsigned short X = cx * 8, Y = cy * CH;
  if(cx == SEP_X / 8 && !desk_nosep) {
    for(r = 0; r < CH; ++r) {
      for(c = 0; c < 8; ++c) {
        pix[r][c] = desk(X + c, Y + r);
      }
    }
  } else {
    /* away from the icon column line the desktop changes only from row to
       row, and between even and odd pixels (the checker) */
    for(r = 0; r < CH; ++r) {
      unsigned char a = desk(X, Y + r), b = desk(X + 1, Y + r);
      for(c = 0; c < 8; c += 2) {
        pix[r][c] = a;
        pix[r][c + 1] = b;
      }
    }
  }
  blit(cx, cy);
#endif
}
/*---------------------------------------------------------------------------*/
/* Windows. Geometry of the window being drawn, in pixels: frame from L to R
   and from T to B, title bar under T, shadow to the right and below. */
static unsigned short gL, gR, gT, gB;
static unsigned char gx, gxr, gy, gyb, gactive, gkf_title, gkf_close;
static const char *gtitle;
static unsigned char gtitlelen;
static unsigned short gtx0;             /* first pixel of the title text */
static unsigned char gn;                /* letters shown */
static char gstr[48];                   /* "/// title", cut to the room there is */
#define ADV 8                           /* pixels a letter in titles */
static unsigned short ggloss0, ggloss1; /* light line on the bar */
static unsigned short gbox0, gbox1;     /* close box */

static void
set_geom(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
         unsigned char active, const char *title, unsigned char titlelen)
{
  gx = x; gxr = x + w + 1; gy = y; gyb = y + h + 1;
  gL = x * 8 + FX;
  gR = gxr * 8 + FX;
  gT = y * CH;
  gB = gyb * CH + CH - 1 - SHY;
  gactive = active;
  gtitle = title;
  gtitlelen = titlelen;
#if defined(MONO)
  /* close box on the left (cells x+1, x+2), title centred on a plate */
  gbox0 = (x + 1) * 8 + 2;
  gbox1 = (x + 2) * 8 + 5;
  {
    unsigned short lo = gbox1 + 10, hi = gR - 8, room = hi > lo ? (hi - lo) / ADV : 0;
    gn = titlelen < room ? titlelen : room;
    memcpy(gstr, title, gn);
    gtx0 = lo + ((hi - lo) - gn * ADV) / 2;
  }
#else
  /* "/// title" on the left, close box on the right (cells xr-2, xr-1) */
  gbox0 = (gxr - 2) * 8 + 2;
  gbox1 = (gxr - 1) * 8 + 4;
#if defined(GFX_CGA4)
  gtx0 = gL + 13;                       /* a few lines before the title */
#else
  gtx0 = gL + 5;
#endif
  {
    unsigned short room = gbox0 > gtx0 + 6 ? (gbox0 - 6 - gtx0) / ADV : 0;
#if defined(GFX_CGA4)
    unsigned char pre = 0;              /* the lines already mark the bar */
#else
    unsigned char pre = (titlelen + 4 <= room && titlelen + 4 < sizeof(gstr)) ? 4 : 0;
#endif
    memcpy(gstr, "/// ", pre);
    gn = titlelen + pre < room ? titlelen + pre : room;
    if(gn > sizeof(gstr)) {
      gn = sizeof(gstr);
    }
    memcpy(gstr + pre, title, gn > pre ? gn - pre : 0);
  }
  ggloss0 = gtx0 + gn * ADV + 3;
  ggloss1 = gbox0 - 4;
#endif
}

/* Colour of one pixel of the window frame, title row and shadow (not the
   title text). */
static unsigned char
wpx(unsigned short X, unsigned short Y)
{
  unsigned char edge = gactive ? C_EDGE_A : C_EDGE_I;
  if(X >= gL && X <= gR && Y >= gT && Y <= gB) {
    unsigned char ex = (X == gL || X == gR), ey = (Y == gT || Y == gB);
    if(ex && ey) {
      goto outside;                     /* corners cut by one pixel */
    }
#if CH == 8
    /* on 8-pixel cells the bar fills the top row too: the letters need all
       of rows 0-6, and nothing must run through them */
    if(Y == gT) {
      ex = 0;
      ey = 0;
      if(X != gL && X != gR) {
        goto bar;
      }
    }
#endif
#if defined(GFX_CGA4)
    /* double frame, thick outside and thin inside, as on the CGA theme P1 */
    if(Y >= gT + CH) {
      if(X <= gL + 1 || X >= gR - 1 || Y == gB) {
        return edge;
      }
      if(((X == gL + 3 || X == gR - 3) && Y <= gB - 2) ||
         (Y == gB - 2 && X >= gL + 3 && X <= gR - 3)) {
        return edge;
      }
      return C_PAPER;
    }
    if(ex) {
      goto bar;                         /* the bar covers the frame */
    }
#endif
    if(ex || ey) {
      return edge;
    }
    if(Y >= gT + CH) {
      return C_PAPER;
    }
bar:
    {
      unsigned char r = Y - gT;
#if defined(GFX_CGA4)
      if(r >= BAR_SEP) {
        unsigned char onplate = gn && X + 4 >= gtx0 && X <= gtx0 + gn * ADV + 3;
        if(X <= gL + 1 || X >= gR - 1 || (r == BAR_SEP && !(onplate && BAR_SEP < CH - 1))) {
          return edge;                  /* line under the bar */
        }
        return C_PAPER;
      }
#endif
      if(r == CH - 1) {
        return edge;                    /* line under the bar */
      }
#if defined(MONO)
      if(X >= gbox0 && X <= gbox1 && r >= 1 && r <= CH - 2 && gactive) {
        unsigned char bx = (X == gbox0 || X == gbox1), by = (r == 1 || r == CH - 2);
        if(gkf_close) {
          return (bx && by) ? C_PAPER : C_INK;
        }
        return (bx || by) && !(bx && by) ? C_INK : C_PAPER;
      }
      if(gactive && (r == 2 || r == 4) && X >= gbox1 + 4 && X <= gR - 3) {
        /* stripes right of the close box, except on the title plate */
        if(gn && X + 6 >= gtx0 && X < gtx0 + gn * ADV + 5) {
          return C_PAPER;
        }
        return C_INK;
      }
      return C_PAPER;
#else
      if(X >= gbox0 && X <= gbox1 && r >= 1 && r <= CH - 2) {
        unsigned char bx = (X == gbox0 || X == gbox1), by = (r == 1 || r == CH - 2);
        if(bx || by) {
          return gkf_close ? C_RING : (gactive ? C_HL : C_EDGE_I);
        }
        {
          /* the cross */
          short cx = (short)X - (short)((gbox0 + gbox1) / 2);
          short cy = (short)r - (short)((CH - 1) / 2);
          short a = (CH == 8) ? 2 : 3;
          if(cx >= -a && cx <= a && (cx == cy || cx == -cy || (CH > 8 && (cx == cy + 1 || cx == -cy + 1)))) {
            return gactive ? C_TITLE_A : C_INK;
          }
        }
        return gactive ? C_BAR_AB : C_PAPER;
      }
#if defined(GFX_CGA4)
      /* as on two colours: thin magenta lines along the active bar (rows
         1, 3, 5) on black, the title on a plain plate */
      {
        unsigned char onplate = gn && X + 4 >= gtx0 && X <= gtx0 + gn * ADV + 3;
        if(X <= gL + 1 || X >= gR - 1) {
          return edge;                  /* the double frame goes round the bar */
        }
        if(gactive && bar_lines[r] && !onplate && X >= gL + 3 && X <= gR - 3) {
          return C_BAR_AT;
        }
      }
      return C_PAPER;
#endif
      if(r == 1 && X >= ggloss0 && X <= ggloss1) {
        return gactive ? C_HL : C_ETCH1;
      }
#if defined(GFX_MCGA)
      return (gactive ? C_BAR_A : C_BAR_I) + r;
#else
      return r < CH / 2 ? (gactive ? C_BAR_AT : C_BAR_IT) : (gactive ? C_BAR_AB : C_BAR_IB);
#endif
#endif
    }
  }
outside:
#if defined(MONO)
  /* solid shadow: SHX pixels to the right, SHY rows below */
  if(X > gR && X <= gR + SHX && Y >= gT + SHY + 1 && Y <= gB + SHY) {
    return C_INK;
  }
  if(Y > gB && Y <= gB + SHY && X >= gL + SHX && X <= gR + SHX - 1) {
    return C_INK;
  }
#else
  if(X >= gL + 2 && X <= gR + SHX && Y >= gT + 2 && Y <= gB + SHY &&
     (X > gR || Y > gB)) {
    /* two steps: dark next to the window, lighter further out */
    if(X <= gR + SHX / 2 && Y <= gB + SHY / 2 && X >= gL + 1 + SHX / 2 && Y >= gT + 1 + SHY / 2) {
      return shade(X, Y, 1);
    }
    if(!(X == gR + SHX && Y == gB + SHY)) {
      return shade(X, Y, 2);
    }
  }
#endif
  return desk(X, Y);
}

#if defined(MONO)
/* Bits of a cell (most significant = leftmost) for the pixels X0..X0+7 that
   fall in [a, b] */
static unsigned char
span(unsigned short X0, unsigned short a, unsigned short b)
{
  unsigned char lo, hi;
  if(b < a || b < X0 || a > X0 + 7) {
    return 0;
  }
  lo = a > X0 ? a - X0 : 0;
  hi = b < X0 + 7 ? b - X0 : 7;
  return (0xff >> lo) & (unsigned char)(0xff << (7 - hi));
}
/* The window cell on two colours, a row at a time, into mb */
static void
wcell(unsigned char cx, unsigned char cy)
{
  unsigned char r;
  unsigned short X0 = cx * 8, Y0 = cy * CH;
  for(r = 0; r < 8; ++r) {
    unsigned short Y = Y0 + r;
    unsigned char v = (cy == SH - 2 && r == 7) ? 0xff : wall[r], in;
    if(cx == SEP_X / 8 && Y >= CH + CH / 2 && Y < (SH - 1) * CH - CH / 2) {
      v |= 0x80 >> (SEP_X & 7);
    }
    /* shadow */
    if(Y >= gT + SHY + 1 && Y <= gB + SHY) {
      v |= span(X0, gR + 1, gR + SHX);
    }
    if(Y > gB && Y <= gB + SHY) {
      v |= span(X0, gL + SHX, gR + SHX - 1);
    }
    if(Y >= gT && Y <= gB) {
      /* the title sits on a white plate that also opens the top line, so
         the 8x8 letters have rows 0-6 to themselves */
      unsigned char plate = gn ? span(X0, gtx0 - 4, gtx0 + gn * ADV + 3) : 0;
      if(Y == gB) {
        in = span(X0, gL + 1, gR - 1);  /* corners cut */
        v = (v & ~in) | in;
      } else {
        in = span(X0, gL, gR);
        v &= ~in;
        v |= span(X0, gL, gL) | span(X0, gR, gR);
        if(Y < gT + CH) {
          unsigned char rr = Y - gT;
          if(rr == BAR_SEP) {
            /* line under the bar; on row 6 the title plate opens it, as the
               letters reach that row */
            v |= span(X0, gL + 1, gR - 1) & (BAR_SEP < CH - 1 ? ~plate : 0xff);
          } else if(rr > BAR_SEP) {
            /* white row between the bar and the contents */
          } else if(gactive) {
            /* lines along the bar, with plates for the close box and the
               title */
            if(bar_lines[rr]) {
              v |= span(X0, gbox1 + 4, gR - 2) & ~plate;
            }
            if(rr == 1 || rr == CH - 2) {
              v |= span(X0, gbox0 + 1, gbox1 - 1);
            } else {
              v |= span(X0, gbox0, gbox0) | span(X0, gbox1, gbox1);
              if(gkf_close) {
                v |= span(X0, gbox0, gbox1);
              }
            }
          }
        }
      }
    }
    mb[r] = v;
  }
}
#define wcell_done() do { } while(0)
#else
static void
wcell(unsigned char cx, unsigned char cy)
{
  unsigned char r, c;
  unsigned short X = cx * 8, Y = cy * CH;
  for(r = 0; r < CH; ++r) {
    for(c = 0; c < 8; ++c) {
      pix[r][c] = wpx(X + c, Y + r);
    }
  }
}
#endif

/* One cell of the title row: frame, bar, then the title letters, placed
   by pixel (6 pixels a letter in the bar font) so that titles fit */
static void
title_cell(unsigned char cx)
{
  unsigned char c, r, fg;
  unsigned short X0 = cx * 8;
  wcell(cx, gy);
#if defined(MONO)
  fg = C_INK;
  if(gkf_title && gn) {
    /* focus on the title: the plate turns black */
    unsigned char m = span(X0, gtx0 > 5 ? gtx0 - 5 : 0, gtx0 + gn * ADV + 3);
    for(r = 1; r < CH - 1; ++r) {
      mb[r] |= m;
    }
    fg = C_PAPER;
  }
#else
  fg = gkf_title ? C_RING : (gactive ? C_TITLE_A : C_TITLE_I);
#endif
  barfont = 1;
  for(c = 0; c < 8; ++c) {
    unsigned short X = X0 + c;
    if(X >= gtx0 && X < gtx0 + gn * ADV) {
      unsigned char k = (X - gtx0) / ADV, col = (X - gtx0) % ADV;
      const unsigned char *g = glyph(gstr[k]);
      if(ADV == 6) {
        ++col;                          /* the bar font starts in column 1 */
      }
      for(r = 0; r < CH - 1; ++r) {
        if(g[r] & (0x80 >> col)) {
#if defined(MONO)
          if(fg == C_INK) {
            mb[r] |= 0x80 >> c;
          } else {
            mb[r] &= ~(0x80 >> c);
          }
#else
          pix[r][c] = fg;
#endif
        }
      }
    }
  }
  barfont = 0;
}

/* Cells of a window row that are alike (the middle of the title bar, of the
   lower border) are composed once and copied: kind of a title cell, 0 if it
   has something of its own */
static unsigned char
title_kind(unsigned char cx)
{
  unsigned short X0 = cx * 8, X1 = X0 + 7;
  if(X0 <= gL || X1 >= gR || cx == SEP_X / 8) {
    return 0;
  }
#if defined(MONO)
  if(X1 + 3 >= gbox0 && X0 <= gbox1 + 4) {
    return 0;
  }
  if(X0 < gbox0) {
    return 0;
  }
  if(gn && X1 + 6 >= gtx0 && X0 < gtx0 + gn * ADV + 5) {
    return 0;
  }
  return 1;
#else
  if(X1 >= gbox0 - 1 && X0 <= gbox1 + 1) {
    return 0;
  }
  if(X1 >= gtx0 && X0 < gtx0 + gn * ADV) {
    return 0;
  }
  if(X0 >= ggloss0 && X1 <= ggloss1) {
    return 1;
  }
  if(X1 < ggloss0 || X0 > ggloss1) {
    return 2;
  }
  return 0;
#endif
}
#if defined(MONO)
static unsigned char cache[2][8];
#define SAVE(k) memcpy(cache[k], mb, 8)
#define PUT(k, cx, cy) blit_bytes(cx, cy, cache[k])
#else
static unsigned char cache[2][CH][8];
#define SAVE(k) memcpy(cache[k], pix, sizeof(pix))
#define PUT(k, cx, cy) do { memcpy(pix, cache[k], sizeof(pix)); blit(cx, cy); } while(0)
#endif

#if defined(MONO) || defined(GFX_CGA4)
/* The top line of the window frame, in the last pixel row of the cell row
   above the title (written into video memory over what is there) */
static void
top_line(void)
{
  unsigned char i;
  unsigned char __far *row;
  if(gy == 0) {
    return;
  }
  row = VRAM + (unsigned short)(gy - 1) * 320 + 0x2000 + 3 * 80;     /* pixel row 7 */
  for(i = gx; i <= gxr; ++i) {
    unsigned short X0 = i * 8;
#if defined(MONO)
    unsigned char m = span(X0, gL + 1, gR - 1);
    row[i] &= ~m;                       /* ink is 0 on the screen */
#else
    unsigned char c;
    for(c = 0; c < 8; ++c) {
      unsigned short X = X0 + c;
      if(X >= gL + 1 && X <= gR - 1) {
        unsigned char __far *b = row + i * 2 + (c >> 2);
        unsigned char sh = 6 - 2 * (c & 3);
        *b = (*b & ~(3 << sh)) | ((gactive ? C_EDGE_A : C_EDGE_I) << sh);
      }
    }
#endif
  }
}
#endif

/* a composed window cell to the screen */
#if defined(MONO)
#define BLITW(cx, cy) blit_bytes(cx, cy, mb)
#else
#define BLITW(cx, cy) blit(cx, cy)
#endif

/* the title row from cell x0 to x1 */
static void
title_row(unsigned char x0, unsigned char x1)
{
  unsigned char i, k, have = 0;
  for(i = x0; i <= x1; ++i) {
    k = title_kind(i);
    if(k) {
      if(!(have & k)) {
        title_cell(i);
        SAVE(k - 1);
        have |= k;
      }
      PUT(k - 1, i, gy);
    } else {
      title_cell(i);
      BLITW(i, gy);
    }
  }
}

static void
draw_windowborders(unsigned char x, unsigned char y, unsigned char w,
                   unsigned char h, unsigned char clipy1,
                   unsigned char clipy2, unsigned char focus,
                   const char *title, unsigned char titlelen)
{
  unsigned char i, r;
  set_geom(x, y, w, h, (focus & (CTK_FOCUS_WINDOW | CTK_FOCUS_DIALOG)) != 0,
           title, titlelen);
  gkf_title = gkf_close = 0;
  if(y >= clipy1 && y < clipy2) {
    title_row(gx, gxr);
#if defined(MONO) || defined(GFX_CGA4)
    top_line();
#endif
  }
  /* the side cells of the middle rows are alike: compose each once and copy
     it down, again only where the desktop under the shadow changes */
  {
    unsigned char lpix[CH][8], rpix[CH][8], key = 255;
    for(r = y + 1; r < gyb; ++r) {
      if(r < clipy1 || r >= clipy2) {
        continue;
      }
#if defined(MONO) || defined(GFX_CGA4)
      if(key == 255 || r == 1 || r == 2 || r >= SH - 3 ||
         gx == SEP_X / 8 || gxr == SEP_X / 8) {
#else
      if(1) {
#endif
        key = 0;
        wcell(gx, r);
#if defined(MONO)
        memcpy(lpix[0], mb, 8);
#else
        memcpy(lpix, pix, sizeof(pix));
#endif
        wcell(gxr, r);
#if defined(MONO)
        memcpy(rpix[0], mb, 8);
#else
        memcpy(rpix, pix, sizeof(pix));
#endif
      }
#if defined(MONO)
      blit_bytes(gx, r, lpix[0]);
      blit_bytes(gxr, r, rpix[0]);
#else
      memcpy(pix, lpix, sizeof(pix));
      blit(gx, r);
      memcpy(pix, rpix, sizeof(pix));
      blit(gxr, r);
#endif
    }
  }
  if(gyb >= clipy1 && gyb < clipy2) {
    unsigned char have = 0;
    for(i = gx; i <= gxr; ++i) {
      unsigned short X0 = i * 8;
      if(X0 >= gL + SHX + 2 && X0 + 7 < gR && i != SEP_X / 8) {
        if(!have) {
          wcell(i, gyb);
          SAVE(0);
          have = 1;
        }
        PUT(0, i, gyb);
      } else {
        wcell(i, gyb);
        BLITW(i, gyb);
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
/* Text in windows */
static unsigned char cursx, cursy;
#define gotoxy(x, y) do { cursx = (x); cursy = (y); } while(0)

/* a letter on the row colours of rowbg, into mb (two colours) or pix */
static void
comp_text(unsigned char c, unsigned char fg)
{
#if defined(MONO)
  const unsigned char *g = c != ' ' ? glyph(c) : NULL;
  unsigned char r;
  for(r = 0; r < 8; ++r) {
    unsigned char v = rowbg[r] == C_INK ? 0xff : 0;
    if(g) {
      v = fg == C_INK ? (v | g[r]) : (v & ~g[r]);
    }
    mb[r] = v;
  }
#else
  pix_rows();
  if(c != ' ') {
    pix_glyph(c, fg);
  }
#endif
}
#if defined(MONO)
#define BLITC(cx, cy) blit_bytes(cx, cy, mb)
#else
#define BLITC(cx, cy) blit(cx, cy)
#endif
static void
text_cell(unsigned char c, unsigned char fg)
{
  comp_text(c, fg);
  BLITC(cursx++, cursy);
}
static void
text(const char *s, unsigned char len, unsigned char fg)
{
  while(len-- && *s) {
    text_cell((unsigned char)*s++, fg);
  }
}
static void
blank(unsigned char len)
{
  if(len == 0) {
    return;
  }
  text_cell(' ', C_INK);
  while(--len) {
    blit_again(cursx++, cursy);
  }
}
/*---------------------------------------------------------------------------*/
/* BIOS video mode, from user space: ELKS runs in real mode. */
static void
set_mode(unsigned char mode)
{
  bios10(mode, 0);
}

void
ctk_cga_exit(void)
{
  set_mode(3);
}

#if defined(GFX_MCGA) || defined(GFX_VGA)
static void
dac(unsigned char i, const unsigned char *rgb)
{
  outb(0x3c8, i);
  outb(0x3c9, rgb[0] >> 2);
  outb(0x3c9, rgb[1] >> 2);
  outb(0x3c9, rgb[2] >> 2);
}
#endif
#if defined(GFX_MCGA)
#define GFX_PALETTE_ONLY
#include "gfx-icons.h"
static void
mix(unsigned char *o, const unsigned char *a, const unsigned char *b,
    unsigned char t, unsigned char n)
{
  unsigned char k;
  for(k = 0; k < 3; ++k) {
    o[k] = (unsigned char)((a[k] * (unsigned short)(n - t) + b[k] * (unsigned short)t) / n);
  }
}
#endif

static void
palette(void)
{
#if defined(GFX_CGA4)
  bios10(0x0b00, 0x0101);       /* palette 1: cyan, magenta, white */
  bios10(0x0b00, 0x0010);       /* high intensity, black background */
#elif defined(GFX_EGA)
  unsigned char i, k, v;
  for(i = 0; i < 16; ++i) {
    /* EGA colour byte: bits 0-2 B G R at 2/3, bits 3-5 b g r at 1/3 */
    v = 0;
    for(k = 0; k < 3; ++k) {
      unsigned char c = pal16[i][2 - k];
      if(c >= 170) v |= 1 << k;
      if(c == 85 || c == 255) v |= 8 << k;
    }
    bios10(0x1000, ((unsigned short)v << 8) | i);
  }
#elif defined(GFX_VGA)
  unsigned char i;
  for(i = 0; i < 16; ++i) {
    bios10(0x1000, ((unsigned short)i << 8) | i);    /* register i -> DAC i */
    dac(i, pal16[i]);
  }
#elif defined(GFX_MCGA)
  unsigned char i, o[3], s1[3], s2[3];
  static const unsigned char black[3] = {0, 0, 0};
  for(i = 0; i < 11; ++i) {
    dac(i, fixed[i]);
  }
  for(i = 0; i < 8; ++i) {
    /* graded bars: rows 0-3 from colour 0 to 1, rows 4-7 from 2 to 3 */
    mix(o, bar_a[i < 4 ? 0 : 2], bar_a[i < 4 ? 1 : 3], i & 3, 3);
    dac(C_BAR_A + i, o);
    mix(o, bar_i[i < 4 ? 0 : 2], bar_i[i < 4 ? 1 : 3], i & 3, 3);
    dac(C_BAR_I + i, o);
  }
  for(i = 0; i < DESK_STEPS; ++i) {
    mix(o, desk_rgb[0], desk_rgb[1], i, DESK_STEPS - 1);
    dac(C_DESK + i, o);
    mix(s1, o, fixed[C_SHBASE], 9, 20);
    dac(C_DESK_SH1 + i, s1);
    mix(s2, o, fixed[C_SHBASE], 4, 20);
    dac(C_DESK_SH2 + i, s2);
  }
  (void)black;
  for(i = 0; i < GFX_ICON_PAL_N; ++i) {
    dac(GFX_ICON_PAL_BASE + i, gfx_icon_pal[i]);
  }
#endif
}

void
ctk_draw_init(void)
{
#if defined(MONO)
  if(ctk_cga_wallpaper >= 4) {
    ctk_cga_wallpaper = 0;
  }
  wall = wallpapers[ctk_cga_wallpaper];
  set_mode(6);
#elif defined(GFX_CGA4)
  set_mode(4);
#elif defined(GFX_EGA)
  set_mode(0x10);
#elif defined(GFX_VGA)
  set_mode(0x12);
#else
  set_mode(0x13);
#endif
#if defined(GFX_EGA) || defined(GFX_VGA)
  load_bios_font();
  outb(0x3ce, 5); outb(0x3cf, 2);       /* write mode 2 */
  outb(0x3ce, 3); outb(0x3cf, 0);       /* replace, no rotation */
  outb(0x3ce, 1); outb(0x3cf, 0);
  outb(0x3c4, 2); outb(0x3c5, 0x0f);    /* all planes */
#endif
  palette();
  ctk_draw_clear(0, SH);
}
/*---------------------------------------------------------------------------*/
/* Rounded corners of the screen (two-colour screens): rows of the cut, from
   the corner inwards, in pixels */
#if defined(MONO)
static const unsigned char corner_cut[3] = {5, 2, 1};
static void
round_corner(unsigned char left, unsigned char top)
{
  unsigned char r;
  for(r = 0; r < 3; ++r) {
    unsigned char k = corner_cut[r];
    mb[top ? r : 7 - r] |= left ? (unsigned char)(0xff << (8 - k)) : (0xff >> (8 - k));
  }
}
#endif
/*---------------------------------------------------------------------------*/
static void
status_line(void)
{
  unsigned char i;
  static const char left[] = " Alci";
  static const char right[] = GFX_LABEL " ";
  unsigned char rl = sizeof(right) - 1;
#if defined(MONO)
  rows_solid(C_PAPER);                  /* its line is drawn above it */
#elif CH == 8
  rows_solid(C_STATUS);                 /* its line is drawn above it (desk) */
#else
  rows_solid(C_STATUS);
  rowbg[0] = C_ETCH0;
  rowbg[1] = C_ETCH1;
#endif
  barfont = 1;
  for(i = 0; i < SW; ++i) {
    unsigned char c = ' ';
    if(i < sizeof(left) - 1) {
      c = left[i];
    } else if(i >= SW - rl) {
      c = right[i - (SW - rl)];
    }
    comp_text(c, C_INK);
#if defined(MONO)
    if(i == 0 || i == SW - 1) {
      round_corner(i == 0, 0);
    }
#endif
    BLITC(i, SH - 1);
  }
  barfont = 0;
}

static void
clear_line(unsigned char line)
{
  unsigned char i;
  if(line == SH - 1) {
    status_line();
    return;
  }
#if defined(MONO)
  {
    /* fast path: the wallpaper cell is the same everywhere but by the
       icon column line */
    unsigned char sepc = SEP_X / 8;
    desk_cell(0, line);
    for(i = 1; i < SW; ++i) {
      if(i != sepc) {
        blit_again(i, line);
      }
    }
    desk_cell(sepc, line);
  }
#else
  {
    /* every cell of a desktop row is alike but the one with the line */
    unsigned char sepc = SEP_X / 8;
    desk_cell(0, line);
    for(i = 1; i < SW; ++i) {
      if(i != sepc) {
        blit_again(i, line);
      }
    }
    desk_cell(sepc, line);
  }
#endif
}

void
ctk_draw_clear(unsigned char y1, unsigned char y2)
{
  unsigned char i;
  for(i = y1; i < y2; ++i) {
    clear_line(i);
  }
}
/*---------------------------------------------------------------------------*/
/* Buttons and text entries: a cap cell on each side of the text. The frame
   runs in column 4 of the left cap and column 3 of the right one. */
#if CH == 8
/* 8-pixel cells: buttons are rounded brackets, a filled pill when focused;
   text entries have side bars and a line under the text (as on the CGA
   theme P1), so no line runs through the letters */
static const unsigned char cap_b[2][8] = {
  {0x0e,0x18,0x30,0x30,0x30,0x30,0x18,0x0e}, {0x70,0x18,0x0c,0x0c,0x0c,0x0c,0x18,0x70}};
static const unsigned char cap_f[2][8] = {
  {0x0f,0x1f,0x3f,0x3f,0x3f,0x3f,0x1f,0x0f}, {0xf0,0xf8,0xfc,0xfc,0xfc,0xfc,0xf8,0xf0}};
static const unsigned char cap_e[2][8] = {
  {0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xff}, {0x03,0x03,0x03,0x03,0x03,0x03,0x03,0xff}};
#if defined(MONO)
static const unsigned char cap_ef[2][8] = {
  {0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xff}, {0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0xff}};
/* buttons as on the approved trial: a box with cut corners round the small
   font (rows 1-6), and a second line on each side when it has the focus */
static const unsigned char cap_box[2][8] = {
  {0x07,0x08,0x08,0x08,0x08,0x08,0x08,0x07}, {0xe0,0x10,0x10,0x10,0x10,0x10,0x10,0xe0}};
static const unsigned char cap_boxf[2][8] = {
  {0x07,0x28,0x28,0x28,0x28,0x28,0x28,0x07}, {0xe0,0x14,0x14,0x14,0x14,0x14,0x14,0xe0}};
#else
static void
cap8(unsigned char cx, unsigned char cy, unsigned char right,
     const unsigned char (*pat)[8], unsigned char col, unsigned char col2)
{
  unsigned char r, c;
  for(r = 0; r < 8; ++r) {
    for(c = 0; c < 8; ++c) {
      pix[r][c] = (pat[right][r] & (0x80 >> c)) ? (r < 4 ? col : col2) : C_PAPER;
    }
  }
  blit(cx, cy);
}
#endif
#endif

static void
cap(unsigned char cx, unsigned char cy, unsigned char right,
    unsigned char fill_t, unsigned char fill_b, unsigned char edge,
    unsigned char ring)
{
  unsigned char r, c, f = right ? 3 : 4;
#if CH == 8 && !defined(MONO)
  if(fill_t != C_PAPER) {
    cap8(cx, cy, right, cap_f, fill_t, fill_b);
  } else if(ring == 2) {
    cap8(cx, cy, right, cap_e, edge, edge);
  } else {
    cap8(cx, cy, right, cap_b, edge, edge);
  }
  return;
#endif
#if defined(MONO)
  /* brackets, a pill when focused; entries: side bars and a line under */
  {
    const unsigned char (*pat)[8] = ring == 5 ? cap_boxf : ring == 4 ? cap_box :
      ring == 3 ? cap_ef : ring == 2 ? cap_e : cap_b;
    (void)r; (void)c; (void)f; (void)fill_b; (void)edge;
    blit_bytes(cx, cy, pat[right]);
    return;
  }
#endif
  for(r = 0; r < CH; ++r) {
    for(c = 0; c < 8; ++c) {
      unsigned char in = right ? (c < f) : (c > f);
      pix[r][c] = in ? (r < CH / 2 ? fill_t : fill_b) : C_PAPER;
    }
    pix[r][f] = edge;
  }
  pix_hline(0, right ? 0 : f, right ? f : 7, edge);
  pix_hline(CH - 1, right ? 0 : f, right ? f : 7, edge);
  pix[0][f] = C_PAPER;                  /* cut corners */
  pix[CH - 1][f] = C_PAPER;
#if CH > 8
  if(ring) {
    /* focus ring outside the frame, on cells tall enough for it */
    unsigned char g = right ? f + 2 : f - 2;
    pix_vline(g, 2, CH - 3, C_RING);
  }
#endif
  (void)ring;
  blit(cx, cy);
}

static void
button(unsigned char x, unsigned char y, const char *s, unsigned char len,
       unsigned char kf)
{
#if defined(MONO)
  unsigned char t = C_PAPER, b = C_PAPER, fg = C_INK, e = C_INK;
#elif CH == 8
  unsigned char t = C_PAPER, b = C_PAPER, fg = C_INK, e = C_EDGE_I;
  if(kf) {
    t = C_BAR_AT; b = C_BAR_AB; fg = C_TITLE_A; e = C_EDGE_A;
  }
#else
  unsigned char t = C_BAR_IT, b = C_BAR_IB, fg = C_INK, e = C_EDGE_I;
  if(kf) {
    t = C_BAR_AT; b = C_BAR_AB; fg = C_TITLE_A; e = C_EDGE_A;
  }
#endif
#if defined(MONO)
  cap(x, y, 0, t, b, e, kf ? 5 : 4);
#else
  cap(x, y, 0, t, b, e, kf);
#endif
  gotoxy(x + 1, y);
#if defined(GFX_MCGA)
  {
    unsigned char r;
    for(r = 0; r < CH; ++r) {
      rowbg[r] = kf ? C_BAR_A + r : C_BAR_I + r;
    }
  }
#else
  {
    unsigned char r;
    for(r = 0; r < CH; ++r) {
      rowbg[r] = r < CH / 2 ? t : b;
    }
  }
#endif
#if defined(MONO)
  rows_solid(C_PAPER);
  rowbg[0] = rowbg[CH - 1] = C_INK;     /* the box, clear of the small font */
#elif CH == 8
  if(!kf) {
    rows_solid(C_PAPER);
  }
#else
  rowbg[0] = rowbg[CH - 1] = e;
#endif
  barfont = 1;
  text(s, len, fg);
  barfont = 0;
#if defined(MONO)
  cap(cursx, y, 1, t, b, e, kf ? 5 : 4);
#else
  cap(cursx, y, 1, t, b, e, kf);
#endif
  ++cursx;
}
/*---------------------------------------------------------------------------*/
/* Desktop icons */
static void
icon(struct ctk_widget *w, unsigned char xpos, unsigned char ypos,
     unsigned char clipy1, unsigned char clipy2, unsigned char sel)
{
  const struct gfx_icon *ic = (const struct gfx_icon *)w->widget.icon.bitmap;
  unsigned char i, j, r, c, len;
  unsigned short ox, oy;
  if(ic != NULL) {
    ox = xpos * 8 + (GFX_ICON_CW * 8 - ic->w) / 2;
    oy = ypos * CH + (GFX_ICON_RH * CH - ic->h) / 2;
    for(j = 0; j < GFX_ICON_RH; ++j) {
      unsigned char cy = ypos + j;
      if(cy < clipy1 || cy >= clipy2) {
        continue;
      }
      for(i = 0; i < GFX_ICON_CW; ++i) {
        unsigned short X0 = (xpos + i) * 8, Y0 = cy * CH;
        /* columns of this cell inside the drawing */
        short d0 = (short)X0 - (short)ox;
#if defined(MONO)
        /* two colours: the icon is stored as cells, ready to copy */
        (void)d0; (void)Y0;
        blit_bytes(xpos + i, cy, ic->px + ((unsigned short)j * GFX_ICON_CW + i) * 8);
        continue;
#endif
        for(r = 0; r < CH; ++r) {
          unsigned short Y = Y0 + r;
          const unsigned char *row = NULL;
          if(Y >= oy && Y < oy + ic->h) {
            row = ic->px + (Y - oy) * ic->w;
          }
#if defined(MONO)
          {
            /* icons on two colours are opaque: ink where the drawing has
               it, the wallpaper outside the drawing */
            unsigned char b = 0;
            for(c = 0; c < 8; ++c) {
              short d = d0 + c;
              b <<= 1;
              if(row && d >= 0 && d < (short)ic->w) {
                b |= row[d] == C_INK;
              } else {
                b |= (wall[r] >> (7 - c)) & 1;
              }
            }
            mb[r] = b;
          }
#else
          for(c = 0; c < 8; ++c) {
            short d = d0 + c;
            unsigned char v = 255;
            if(row && d >= 0 && d < (short)ic->w) {
              v = row[d];
            }
            pix[r][c] = v == 255 ? desk(X0 + c, Y) : v;
          }
#endif
        }
#if defined(MONO)
        blit_bytes(xpos + i, cy, mb);
#else
        blit(xpos + i, cy);
#endif
      }
    }
  }
  /* title under the icon, centred; when selected, in a pill. A long title
     keeps its first word ("About Contiki" -> "About"). */
  len = strlen(w->widget.icon.title);
  if(len > GFX_ICON_CW + 2) {
    for(i = 0; i < len && w->widget.icon.title[i] != ' '; ++i) ;
    len = i;
  }
  desk_nosep = 1;
  {
    unsigned char x = xpos, ty = ypos + GFX_ICON_RH;
    if(len < GFX_ICON_CW) {
      x = xpos + (GFX_ICON_CW - len) / 2;
    }
    if(x + len > SW) {
      x = SW - len;
    }
    if(ty < clipy1 || ty >= clipy2) {
      desk_nosep = 0;
      return;
    }
#if defined(MONO)
    /* on two colours the title sits on plain paper, inverted in a pill
       when selected */
    rows_solid(sel ? C_INK : C_PAPER);
    for(i = 0; i < len; ++i) {
      barfont = 1;
      comp_text(w->widget.icon.title[i], sel ? C_PAPER : C_INK);
      barfont = 0;
      if(sel && (i == 0 || i == len - 1)) {
        unsigned char bit = i == 0 ? 0x80 : 0x01;
        mb[0] &= ~bit;
        mb[7] &= ~bit;
      }
      blit_bytes(x + i, ty, mb);
    }
    (void)r; (void)c;
#else
    for(i = 0; i < len; ++i) {
      unsigned short X0 = (x + i) * 8, Y0 = ty * CH;
      for(r = 0; r < CH; ++r) {
        for(c = 0; c < 8; ++c) {
          pix[r][c] = sel ? C_INK : desk(X0 + c, Y0 + r);
        }
      }
#if !defined(MONO)
      if(sel) {
        for(r = 0; r < CH; ++r) {
#if defined(GFX_MCGA)
          memset(pix[r], C_BAR_A + r * 8 / CH, 8);
#else
          memset(pix[r], r < CH / 2 ? C_BAR_AT : C_BAR_AB, 8);
#endif
        }
      }
#endif
      if(sel && (i == 0 || i == len - 1)) {
        /* rounded ends */
        unsigned char k = i == 0 ? 0 : 7;
        pix[0][k] = desk(X0 + k, Y0);
        pix[CH - 1][k] = desk(X0 + k, Y0 + CH - 1);
      }
      pix_glyph(w->widget.icon.title[i], sel ? C_TITLE_A : C_INK);
      blit(x + i, ty);
    }
#endif
  }
  desk_nosep = 0;
}
/*---------------------------------------------------------------------------*/
static void
draw_widget(struct ctk_widget *w, unsigned char x, unsigned char y,
            unsigned char clipy1, unsigned char clipy2, unsigned char focus)
{
  unsigned char xpos = x + w->x, ypos = y + w->y;
  unsigned char yclipped = (ypos >= clipy1 && ypos < clipy2);
  unsigned char kf = (focus & CTK_FOCUS_WIDGET) != 0;
  unsigned char i, tmp, xscroll;
  char *s;

  switch(w->type) {
  case CTK_WIDGET_SEPARATOR:
    if(yclipped) {
      rows_solid(C_PAPER);
#if defined(MONO)
      pix_rows();
      for(i = 0; i < 8; i += 2) {
        pix[CH / 2][i] = C_INK;
      }
#else
      pix_rows();
      pix_hline(CH / 2 - 1, 0, 7, C_ETCH0);
      pix_hline(CH / 2, 0, 7, C_ETCH1);
#endif
      blit(xpos, ypos);
      for(i = 1; i < w->w; ++i) {
        blit_again(xpos + i, ypos);
      }
    }
    break;
  case CTK_WIDGET_LABEL:
    rows_solid(C_PAPER);
    s = w->widget.label.text;
    for(i = 0; i < w->h; ++i) {
      if(ypos >= clipy1 && ypos < clipy2) {
        gotoxy(xpos, ypos);
        text(s, w->w, C_INK);
        tmp = w->w - (cursx - xpos);
        if(tmp > 0 && tmp <= w->w) {
          blank(tmp);
        }
      }
      ++ypos;
      s += w->w;
    }
    break;
  case CTK_WIDGET_BUTTON:
    if(!yclipped) {
      break;
    }
    if(w->window != NULL &&
       (w == (struct ctk_widget *)&w->window->titlebutton ||
        w == (struct ctk_widget *)&w->window->closebutton)) {
      /* both live in the title row: draw it again with the focus mark */
      struct ctk_window *win = w->window;
      set_geom(win->x, win->y + 1, win->w, win->h,
               (focus & (CTK_FOCUS_WINDOW | CTK_FOCUS_DIALOG)) != 0,
               win->title, win->titlelen);
      gkf_title = kf && w == (struct ctk_widget *)&win->titlebutton;
      gkf_close = kf && w == (struct ctk_widget *)&win->closebutton;
      title_row(gx + 1, gxr - 1);
      gkf_title = gkf_close = 0;
      break;
    }
    button(xpos, ypos, w->widget.button.text, w->w, kf);
    break;
  case CTK_WIDGET_HYPERLINK:
    if(yclipped) {
      rows_solid(kf ? C_INK : C_PAPER);
#if !defined(MONO)
      if(kf) {
        rows_bar(1);
      }
#endif
      rowbg[CH - 1] = kf ? rowbg[CH - 1] :
#if defined(MONO)
        C_INK;
#else
        C_LINK;
#endif
      gotoxy(xpos, ypos);
#if defined(MONO)
      text(w->widget.hyperlink.text, w->w, kf ? C_PAPER : C_INK);
#else
      text(w->widget.hyperlink.text, w->w, kf ? C_TITLE_A : C_LINK);
#endif
    }
    break;
  case CTK_WIDGET_TEXTENTRY:
    if(yclipped) {
      unsigned char edit = w->widget.textentry.state == CTK_TEXTENTRY_EDIT;
      unsigned char e = kf ? C_EDGE_A : C_EDGE_I;
#if !defined(MONO)
      if(kf) {
        e = C_RING;
      }
#endif
      xscroll = 0;
      tmp = w->w - 1;
      if(w->widget.textentry.xpos >= tmp) {
        xscroll = w->widget.textentry.xpos - tmp;
      }
      s = w->widget.textentry.text + xscroll;
#if defined(MONO)
      cap(xpos, ypos, 0, C_PAPER, C_PAPER, e, kf ? 3 : 2);
#else
      cap(xpos, ypos, 0, C_PAPER, C_PAPER, e, CH == 8 ? 2 : 0);
#endif
      gotoxy(xpos + 1, ypos);
      for(i = 0; i < w->w; ++i) {
        unsigned char c = *s;
        unsigned char cur = edit && (i == w->widget.textentry.xpos - xscroll);
        rows_solid(cur ? C_INK : C_PAPER);
#if !defined(MONO)
        if(cur) {
          rows_bar(1);
        }
#endif
#if CH == 8
        rowbg[CH - 1] = e;              /* a line under the text */
#else
        rowbg[0] = rowbg[CH - 1] = e;
#endif
        text_cell(c == 0 ? ' ' : (unsigned char)c,
#if defined(MONO)
                  cur ? C_PAPER : C_INK
#else
                  cur ? C_TITLE_A : C_INK
#endif
                  );
        if(c) {
          ++s;
        }
      }
#if defined(MONO)
      cap(cursx, ypos, 1, C_PAPER, C_PAPER, e, kf ? 3 : 2);
#else
      cap(cursx, ypos, 1, C_PAPER, C_PAPER, e, CH == 8 ? 2 : 0);
#endif
    }
    break;
  case CTK_WIDGET_ICON:
    if(yclipped || ypos + GFX_ICON_RH >= clipy1) {
      icon(w, xpos, ypos, clipy1, clipy2, kf);
    }
    break;
  case CTK_WIDGET_BITMAP:
    {
      /* C64-style bitmap: 8x8 one-bit cells, ink on paper */
      const unsigned char *bm = w->widget.bitmap.bitmap;
      unsigned char j, r, c;
      for(i = 0; i < w->h; ++i, ++ypos) {
        for(j = 0; j < w->w; ++j, bm += 8) {
          if(ypos < clipy1 || ypos >= clipy2) {
            continue;
          }
          rows_solid(C_PAPER);
          pix_rows();
          for(r = 0; r < 8; ++r) {
            for(c = 0; c < 8; ++c) {
              if(bm[r] & (0x80 >> c)) {
                pix[r + (CH - 8) / 2][c] = C_INK;
              }
            }
          }
          blit(xpos + j, ypos);
        }
      }
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
  rows_solid(C_PAPER);
  for(i = window->y + 2; i < h; ++i) {
    if(i >= clipy1 && i <= clipy2) {
      gotoxy(window->x + 1, i);
      blank(window->w);
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

  draw_windowborders(x, y, dialog->w, dialog->h, 0, SH,
                     CTK_FOCUS_DIALOG, dialog->title, dialog->titlelen);
  rows_solid(C_PAPER);
  for(i = y1; i < y2; ++i) {
    gotoxy(x1, i);
    blank(dialog->w);
  }
  for(w = dialog->inactive; w != NULL; w = w->next) {
    draw_widget(w, x1, y1, 0, SH, CTK_FOCUS_DIALOG);
  }
  for(w = dialog->active; w != NULL; w = w->next) {
    wf = CTK_FOCUS_DIALOG;
    if(w == dialog->focused) {
      wf |= CTK_FOCUS_WIDGET;
    }
    draw_widget(w, x1, y1, 0, SH, wf);
  }
}
/*---------------------------------------------------------------------------*/
/* Menus: the bar in the top row, an open menu below its title */
static void
menubar_rows(void)
{
#if defined(MONO)
  rows_solid(C_PAPER);
  rowbg[CH - 1] = C_INK;
#else
  rows_bar(1);
#endif
}
static unsigned char
menubar_fg(void)
{
#if defined(MONO)
  return C_INK;
#else
  return C_TITLE_A;
#endif
}

static void
draw_menu(struct ctk_menu *m)
{
  unsigned char x, x2, y, i, len;

  /* the open title: inverted on two colours, light on colour screens */
#if defined(MONO)
  rows_solid(C_INK);
  rowbg[CH - 1] = C_INK;
#else
  rows_solid(C_HL);
#endif
  x = cursx;
  barfont = 1;
#if defined(MONO)
  text(m->title, m->titlelen, C_PAPER);
#elif defined(GFX_CGA4)
  text(m->title, m->titlelen, 0);       /* black on the white title */
#else
  text(m->title, m->titlelen, C_INK);
#endif
  text_cell(' ', C_INK);
  barfont = 0;
  x2 = cursx;
  if(x + CTK_CONF_MENUWIDTH > SW) {
    x = SW - CTK_CONF_MENUWIDTH;
  }
  for(y = 0; y < m->nitems; ++y) {
    unsigned char act = (y == m->active), last = (y == m->nitems - 1);
    const char *t = m->items[y].title;
    len = strlen(t);
    for(i = 0; i < CTK_CONF_MENUWIDTH; ++i) {
      unsigned char c = (i >= 1 && i - 1 < len) ? t[i - 1] : ' ';
      if(act) {
#if defined(MONO)
        rows_solid(C_INK);
#else
        rows_bar(1);
#endif
      } else {
        rows_solid(C_PAPER);
      }
      pix_rows();
      if(t[0] == '-') {
        pix_hline(CH / 2, 0, 7, C_SEPLINE);
      } else if(c != ' ') {
        pix_glyph(c, act ? C_SEL_FG : C_INK);
      }
      /* frame: sides and bottom */
      if(i == 0) {
        pix_vline(0, 0, CH - 1, C_EDGE_A);
      }
      if(i == CTK_CONF_MENUWIDTH - 1) {
        pix_vline(7, 0, CH - 1, C_EDGE_A);
      }
      if(last) {
        pix_hline(CH - 1, 0, 7, C_EDGE_A);
      }
      blit(x + i, y + 1);
    }
  }
  gotoxy(x2, 0);
  menubar_rows();
}

void
ctk_draw_menus(struct ctk_menus *menus)
{
  struct ctk_menu *m;
  unsigned char fg = menubar_fg();

  menubar_rows();
  gotoxy(0, 0);
  barfont = 1;
  text(" ///", 4, fg);
  text_cell(' ', fg);
  for(m = menus->menus->next; m != NULL; m = m->next) {
    if(m != menus->open) {
      text(m->title, m->titlelen, fg);
      text_cell(' ', fg);
    } else {
      barfont = 0;
      draw_menu(m);
      barfont = 1;
    }
  }
  {
    unsigned char dl = strlen(menus->desktopmenu->title) + 1;
    menubar_rows();
    while(cursx < SW - dl) {
      text_cell(' ', fg);
    }
  }
  if(menus->desktopmenu != menus->open) {
    text(menus->desktopmenu->title, menus->desktopmenu->titlelen, fg);
    text_cell(' ', fg);
  } else {
    barfont = 0;
    draw_menu(menus->desktopmenu);
  }
  barfont = 0;
#if defined(MONO)
  /* rounded top corners of the screen */
  menubar_rows();
  comp_text(' ', C_INK);
  round_corner(1, 1);
  blit_bytes(0, 0, mb);
  comp_text(' ', C_INK);
  round_corner(0, 1);
  blit_bytes(SW - 1, 0, mb);
#endif
}
/*---------------------------------------------------------------------------*/
unsigned char
ctk_draw_height(void)
{
  return SH;
}
unsigned char
ctk_draw_width(void)
{
  return SW;
}
