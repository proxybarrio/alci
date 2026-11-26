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
#if defined(GFX_CGA) || defined(GFX_CGA4)
#include "font-bar.h"
#endif
#if defined(GFX_MONO) && GFX_CH == 14
#include "font-tall14.h"
#elif defined(GFX_MONO) && GFX_CH == 16
#include "font-tall16.h"
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
#if defined(GFX_MONO)
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
#elif defined(GFX_TANDY)
/* the 16 standard colours (Tandy 1000 and PCjr keep them in mode 9), as on
   the theme trial: grey desktop, blue two-tone bar, white windows with a
   black frame, shadow in two greys */
enum { C_INK = 0, C_PAPER = 15, C_HL = 11, C_BAR_AT = 9, C_BAR_AB = 1,
       C_BAR_IT = 7, C_BAR_IB = 7, C_EDGE_A = 0, C_EDGE_I = 8, C_SH1 = 8,
       C_SH2 = 0, C_ETCH0 = 8, C_ETCH1 = 15, C_RING = 14, C_TITLE_A = 15,
       C_TITLE_I = 0, C_LINK = 1, C_STATUS = 15, C_DIM = 8, C_GREY = 7 };
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
#if defined(MONO) && CH == 8
#define SHX 3
#define SHY 1
#elif defined(MONO)
#define SHX 2
#define SHY 2
#elif CH == 8
#define SHX 3
#define SHY 2
#else
#define SHX 4
#define SHY 4
#endif
#define FX 3                    /* the frame runs in pixel column 3 of the border cells */

/* Lines along the active title bar on CGA, in one text row: thin lines at
   the same distance, one pixel row apart, as on the bars of the trials.
   CGA_BAR 2: the frame runs in the last pixel row of the cell row above the
     title (drawn over what is there; under the menu bar its line serves);
     the bar keeps rows 0-7: lines on rows 1, 3, 5, the line under the bar
     on row 7: five lines in all.
   CGA_BAR 1: all inside the title row: frame on row 0, lines on rows 2 and
     4, the line under the bar on row 6, a white row 7: four lines.
   The window layout of Contiki is not changed. */
#ifndef CGA4_BTN
#define CGA4_BTN 2                      /* four colours: 0 brackets, 1 box, 2 box and small font (chosen) */
#endif
#ifndef CGA_BAR
#define CGA_BAR 2
#endif
#define BAR_ABOVE 5                     /* (not used: kept for the record) */
#if defined(GFX_MONO) && GFX_CH > 8
/* Tall cells: all in the title row, as on the trials: frame on row 0, lines
   on every other row from 2 to CH-3, the line under the bar on row CH-1,
   the close box from row 2 to row CH-3 */
#undef CGA_BAR
#define CGA_BAR 1
#define BAR_SEP (GFX_CH - 1)
#define BOX_T 2
#define bar_lines(r) ((r) >= 2 && (r) <= GFX_CH - 3 && !((r) & 1))
#define BOX_B (GFX_CH - 3)
#elif CGA_BAR == 2
#define BAR_SEP 7
#define BOX_T 1
static const unsigned char bar_one[8] = {0, 1, 0, 1, 0, 1, 0, 0};
#define bar_lines(r) bar_one[r]
#else
#define BAR_SEP 6
#define BOX_T 1
static const unsigned char bar_one[8] = {0, 0, 1, 0, 1, 0, 0, 0};
#define bar_lines(r) bar_one[r]
#endif
#ifndef BOX_B
#define BOX_B (GFX_CH - 2)
#endif
static unsigned char gabove;            /* pixel rows of the bar above the title row */
#if defined(GFX_TANDY) || defined(GFX_EGA) || defined(GFX_VGA)
#define PL16 1                          /* 16 colours composed in four bit planes */
#endif

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
  /* "memory": video memory writes must not move across a register write */
  /* (v as a word: ia16-gcc may put a byte operand of "a" in AH) */
  __asm__ volatile ("outb %%al,%%dx" : : "a" ((unsigned short)v), "d" (port) : "memory");
}
#if defined(GFX_EGA) || defined(GFX_VGA)
/* index in the low byte, data in the high byte: both registers at once */
static void
outw(unsigned short port, unsigned short v)
{
  __asm__ volatile ("outw %%ax,%%dx" : : "a" (v), "d" (port) : "memory");
}
#endif

#if defined(GFX_CGA) || defined(GFX_CGA4) || defined(GFX_ATT) || defined(GFX_TANDY)
#define VRAM ((unsigned char __far *)0xB8000000UL)
#elif defined(GFX_HERC)
#define VRAM ((unsigned char __far *)0xB0000000UL)
#else
#define VRAM ((volatile unsigned char __far *)0xA0000000UL)
#endif

/* The cell being composed: one colour index per pixel */
static unsigned char pix[CH][8];

#if defined(MONO)
/* Two colours: cells are also kept as bytes, one per row, 1 = ink, so that
   the frequent cells (desktop, text, buttons) skip the pixel buffer */
static unsigned char mb[CH];
#if defined(GFX_INVERT)
#define INK(v) (v)                      /* ink lit, paper dark */
#else
#define INK(v) (~(v))                   /* black ink on white */
#endif

static void
blit_bytes(unsigned char cx, unsigned char cy, const unsigned char *b)
{
  unsigned char __far *p;
  if(cx >= SW || cy >= SH) {
    return;
  }
#if defined(GFX_CGA)
  p = VRAM + (unsigned short)cy * 320 + cx;
  p[0]      = ~b[0];
  p[0x2000] = ~b[1];
  p[80]     = ~b[2];
  p[0x2050] = ~b[3];
  p[160]    = ~b[4];
  p[0x20a0] = ~b[5];
  p[240]    = ~b[6];
  p[0x20f0] = ~b[7];
#elif defined(GFX_HERC)
  /* four banks of 8K: pixel row Y is in bank Y & 3, line Y >> 2 of 90
     bytes; a cell row of 14 starts on a line in bank 0 or bank 2 */
  {
    unsigned short Y0 = (unsigned short)cy * CH;
    p = VRAM + (Y0 >> 2) * 90 + cx;
    if(Y0 & 2) {
      p[0x4000] = b[0];
      p[0x6000] = b[1];
      p[0x005a] = b[2];
      p[0x205a] = b[3];
      p[0x405a] = b[4];
      p[0x605a] = b[5];
      p[0x00b4] = b[6];
      p[0x20b4] = b[7];
      p[0x40b4] = b[8];
      p[0x60b4] = b[9];
      p[0x010e] = b[10];
      p[0x210e] = b[11];
      p[0x410e] = b[12];
      p[0x610e] = b[13];
    } else {
      p[0x0000] = b[0];
      p[0x2000] = b[1];
      p[0x4000] = b[2];
      p[0x6000] = b[3];
      p[0x005a] = b[4];
      p[0x205a] = b[5];
      p[0x405a] = b[6];
      p[0x605a] = b[7];
      p[0x00b4] = b[8];
      p[0x20b4] = b[9];
      p[0x40b4] = b[10];
      p[0x60b4] = b[11];
      p[0x010e] = b[12];
      p[0x210e] = b[13];
    }
  }
#elif defined(GFX_ATT)
  /* four banks of 8K: pixel row Y in bank Y & 3, line Y >> 2 of 80 bytes;
     cell rows of 16 start in bank 0 */
  p = VRAM + (unsigned short)cy * (4 * 80) + cx;
    p[0x0000] = ~b[0];
    p[0x2000] = ~b[1];
    p[0x4000] = ~b[2];
    p[0x6000] = ~b[3];
    p[0x0050] = ~b[4];
    p[0x2050] = ~b[5];
    p[0x4050] = ~b[6];
    p[0x6050] = ~b[7];
    p[0x00a0] = ~b[8];
    p[0x20a0] = ~b[9];
    p[0x40a0] = ~b[10];
    p[0x60a0] = ~b[11];
    p[0x00f0] = ~b[12];
    p[0x20f0] = ~b[13];
    p[0x40f0] = ~b[14];
    p[0x60f0] = ~b[15];
#else
  /* EGA and VGA, one plane or all planes alike: rows of 80 bytes */
  p = VRAM + (unsigned short)cy * (CH * 80) + cx;
  p[0] = ~b[0];
  p[80] = ~b[1];
  p[160] = ~b[2];
  p[240] = ~b[3];
  p[320] = ~b[4];
  p[400] = ~b[5];
  p[480] = ~b[6];
  p[560] = ~b[7];
  p[640] = ~b[8];
  p[720] = ~b[9];
  p[800] = ~b[10];
  p[880] = ~b[11];
  p[960] = ~b[12];
  p[1040] = ~b[13];
#if CH == 16
  p[1120] = ~b[14];
  p[1200] = ~b[15];
#endif
#endif
}
/* the pixel buffer packed into mb */
static void
pack(void)
{
  unsigned char r, c;
  for(r = 0; r < CH; ++r) {
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
#if defined(GFX_TANDY4)
  /* four banks of 8K, lines of 160 bytes; two bytes for eight pixels: the
     low bit of each, then the high bit */
  p[0] = cb4[0];        p[1] = cb4[1];
  p[0x2000] = cb4[2];   p[0x2001] = cb4[3];
  p[0x4000] = cb4[4];   p[0x4001] = cb4[5];
  p[0x6000] = cb4[6];   p[0x6001] = cb4[7];
  p[160] = cb4[8];      p[161] = cb4[9];
  p[0x20a0] = cb4[10];  p[0x20a1] = cb4[11];
  p[0x40a0] = cb4[12];  p[0x40a1] = cb4[13];
  p[0x60a0] = cb4[14];  p[0x60a1] = cb4[15];
  return;
#endif
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
/* Cells composed as two bit planes a row (bit 1 and bit 0 of the colour,
   most significant bit = leftmost pixel), then spread into the 2-bit
   pixels of cb4: a byte operation does eight pixels */
static unsigned char P1[8], P0[8];
#define FILL(m, c) do { unsigned char m_ = (m); \
    p1 = ((c) & 2) ? (p1 | m_) : (p1 & ~m_); \
    p0 = ((c) & 1) ? (p0 | m_) : (p0 & ~m_); } while(0)
/* the planes into the 2-bit pixels of cb4 */
/* A byte of a plane spread over the even bits of a word, its two bytes
   swapped so that a word store puts the left pixels first. Spread bits
   stay inside their byte when shifted by one, so the same table serves the
   high plane, shifted. */
static const unsigned short spread16[256] = {
  0x0000, 0x0100, 0x0400, 0x0500, 0x1000, 0x1100, 0x1400, 0x1500,
  0x4000, 0x4100, 0x4400, 0x4500, 0x5000, 0x5100, 0x5400, 0x5500,
  0x0001, 0x0101, 0x0401, 0x0501, 0x1001, 0x1101, 0x1401, 0x1501,
  0x4001, 0x4101, 0x4401, 0x4501, 0x5001, 0x5101, 0x5401, 0x5501,
  0x0004, 0x0104, 0x0404, 0x0504, 0x1004, 0x1104, 0x1404, 0x1504,
  0x4004, 0x4104, 0x4404, 0x4504, 0x5004, 0x5104, 0x5404, 0x5504,
  0x0005, 0x0105, 0x0405, 0x0505, 0x1005, 0x1105, 0x1405, 0x1505,
  0x4005, 0x4105, 0x4405, 0x4505, 0x5005, 0x5105, 0x5405, 0x5505,
  0x0010, 0x0110, 0x0410, 0x0510, 0x1010, 0x1110, 0x1410, 0x1510,
  0x4010, 0x4110, 0x4410, 0x4510, 0x5010, 0x5110, 0x5410, 0x5510,
  0x0011, 0x0111, 0x0411, 0x0511, 0x1011, 0x1111, 0x1411, 0x1511,
  0x4011, 0x4111, 0x4411, 0x4511, 0x5011, 0x5111, 0x5411, 0x5511,
  0x0014, 0x0114, 0x0414, 0x0514, 0x1014, 0x1114, 0x1414, 0x1514,
  0x4014, 0x4114, 0x4414, 0x4514, 0x5014, 0x5114, 0x5414, 0x5514,
  0x0015, 0x0115, 0x0415, 0x0515, 0x1015, 0x1115, 0x1415, 0x1515,
  0x4015, 0x4115, 0x4415, 0x4515, 0x5015, 0x5115, 0x5415, 0x5515,
  0x0040, 0x0140, 0x0440, 0x0540, 0x1040, 0x1140, 0x1440, 0x1540,
  0x4040, 0x4140, 0x4440, 0x4540, 0x5040, 0x5140, 0x5440, 0x5540,
  0x0041, 0x0141, 0x0441, 0x0541, 0x1041, 0x1141, 0x1441, 0x1541,
  0x4041, 0x4141, 0x4441, 0x4541, 0x5041, 0x5141, 0x5441, 0x5541,
  0x0044, 0x0144, 0x0444, 0x0544, 0x1044, 0x1144, 0x1444, 0x1544,
  0x4044, 0x4144, 0x4444, 0x4544, 0x5044, 0x5144, 0x5444, 0x5544,
  0x0045, 0x0145, 0x0445, 0x0545, 0x1045, 0x1145, 0x1445, 0x1545,
  0x4045, 0x4145, 0x4445, 0x4545, 0x5045, 0x5145, 0x5445, 0x5545,
  0x0050, 0x0150, 0x0450, 0x0550, 0x1050, 0x1150, 0x1450, 0x1550,
  0x4050, 0x4150, 0x4450, 0x4550, 0x5050, 0x5150, 0x5450, 0x5550,
  0x0051, 0x0151, 0x0451, 0x0551, 0x1051, 0x1151, 0x1451, 0x1551,
  0x4051, 0x4151, 0x4451, 0x4551, 0x5051, 0x5151, 0x5451, 0x5551,
  0x0054, 0x0154, 0x0454, 0x0554, 0x1054, 0x1154, 0x1454, 0x1554,
  0x4054, 0x4154, 0x4454, 0x4554, 0x5054, 0x5154, 0x5454, 0x5554,
  0x0055, 0x0155, 0x0455, 0x0555, 0x1055, 0x1155, 0x1455, 0x1555,
  0x4055, 0x4155, 0x4455, 0x4555, 0x5055, 0x5155, 0x5455, 0x5555
};
#define PLANE_ROW(r) ((unsigned short *)cb4)[r] = \
  (spread16[P1[r]] << 1) | spread16[P0[r]]
static void
planes4(void)
{
#if defined(GFX_TANDY4)
  unsigned char r;
  for(r = 0; r < 8; ++r) {
    cb4[2 * r] = P0[r];                 /* video memory keeps the planes */
    cb4[2 * r + 1] = P1[r];
  }
  return;
#endif
  PLANE_ROW(0); PLANE_ROW(1); PLANE_ROW(2); PLANE_ROW(3);
  PLANE_ROW(4); PLANE_ROW(5); PLANE_ROW(6); PLANE_ROW(7);
}
#define blit_planes(cx, cy) do { planes4(); write4(cx, cy); } while(0)
#elif defined(GFX_TANDY)
/* 16 colours: cells composed as four bit planes a row (most significant
   bit = leftmost pixel), filled by masks, then packed into the 4-bit
   pixels of cb4 (4 bytes a row, left pixel in the high nibble) by tables */
static unsigned char cb4[32];
static unsigned char P0[8], P1[8], P2[8], P3[8];
#define FILL16(m, c) do { unsigned char m_ = (m); \
    q0 = ((c) & 1) ? (q0 | m_) : (q0 & ~m_); \
    q1 = ((c) & 2) ? (q1 | m_) : (q1 & ~m_); \
    q2 = ((c) & 4) ? (q2 | m_) : (q2 & ~m_); \
    q3 = ((c) & 8) ? (q3 | m_) : (q3 & ~m_); } while(0)
static void
write4(unsigned char cx, unsigned char cy)
{
  /* four banks of 8K, pixel row Y in bank Y & 3, line Y >> 2 of 160 bytes */
  unsigned short __far *p = (unsigned short __far *)(VRAM + (unsigned short)cy * 320 + cx * 4);
  const unsigned short *s = (const unsigned short *)cb4;
  if(cx >= SW || cy >= SH) {
    return;
  }
  p[0] = s[0];          p[1] = s[1];
  p[0x1000] = s[2];     p[0x1001] = s[3];
  p[0x2000] = s[4];     p[0x2001] = s[5];
  p[0x3000] = s[6];     p[0x3001] = s[7];
  p[80] = s[8];         p[81] = s[9];
  p[0x1050] = s[10];    p[0x1051] = s[11];
  p[0x2050] = s[12];    p[0x2051] = s[13];
  p[0x3050] = s[14];    p[0x3051] = s[15];
}
#define blit_again(cx, cy) write4(cx, cy)
/* a plane byte spread over the nibbles: pixels 0-3 and pixels 4-7, as the
   two words of a row */
static const unsigned short nib_hi[256] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
  0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
  0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000,
  0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000,
  0x1100, 0x1100, 0x1100, 0x1100, 0x1100, 0x1100, 0x1100, 0x1100,
  0x1100, 0x1100, 0x1100, 0x1100, 0x1100, 0x1100, 0x1100, 0x1100,
  0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
  0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
  0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
  0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
  0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 0x1001,
  0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 0x1001,
  0x1101, 0x1101, 0x1101, 0x1101, 0x1101, 0x1101, 0x1101, 0x1101,
  0x1101, 0x1101, 0x1101, 0x1101, 0x1101, 0x1101, 0x1101, 0x1101,
  0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
  0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
  0x0110, 0x0110, 0x0110, 0x0110, 0x0110, 0x0110, 0x0110, 0x0110,
  0x0110, 0x0110, 0x0110, 0x0110, 0x0110, 0x0110, 0x0110, 0x0110,
  0x1010, 0x1010, 0x1010, 0x1010, 0x1010, 0x1010, 0x1010, 0x1010,
  0x1010, 0x1010, 0x1010, 0x1010, 0x1010, 0x1010, 0x1010, 0x1010,
  0x1110, 0x1110, 0x1110, 0x1110, 0x1110, 0x1110, 0x1110, 0x1110,
  0x1110, 0x1110, 0x1110, 0x1110, 0x1110, 0x1110, 0x1110, 0x1110,
  0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011,
  0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011,
  0x0111, 0x0111, 0x0111, 0x0111, 0x0111, 0x0111, 0x0111, 0x0111,
  0x0111, 0x0111, 0x0111, 0x0111, 0x0111, 0x0111, 0x0111, 0x0111,
  0x1011, 0x1011, 0x1011, 0x1011, 0x1011, 0x1011, 0x1011, 0x1011,
  0x1011, 0x1011, 0x1011, 0x1011, 0x1011, 0x1011, 0x1011, 0x1011,
  0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111,
  0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111
};
static const unsigned short nib_lo[256] = {
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111,
  0x0000, 0x0100, 0x1000, 0x1100, 0x0001, 0x0101, 0x1001, 0x1101,
  0x0010, 0x0110, 0x1010, 0x1110, 0x0011, 0x0111, 0x1011, 0x1111
};
static void
planes4(void)
{
  unsigned char r;
  unsigned short *o = (unsigned short *)cb4;
  for(r = 0; r < 8; ++r) {
    unsigned char a = P0[r], b = P1[r], c = P2[r], d = P3[r];
    *o++ = nib_hi[a] | (nib_hi[b] << 1) | (nib_hi[c] << 2) | (nib_hi[d] << 3);
    *o++ = nib_lo[a] | (nib_lo[b] << 1) | (nib_lo[c] << 2) | (nib_lo[d] << 3);
  }
}
#define blit_planes(cx, cy) do { planes4(); write4(cx, cy); } while(0)
#elif defined(GFX_EGA) || defined(GFX_VGA)
/* 16 colours on EGA and VGA: cells composed as four bit planes a row
   (most significant bit = leftmost pixel), as the video memory keeps them,
   and written a plane at a time (map mask, write mode 0) */
static unsigned char cb4[4 * CH];
#define P0 (cb4)
#define P1 (cb4 + CH)
#define P2 (cb4 + 2 * CH)
#define P3 (cb4 + 3 * CH)
#define FILL16(m, c) do { unsigned char m_ = (m); \
    q0 = ((c) & 1) ? (q0 | m_) : (q0 & ~m_); \
    q1 = ((c) & 2) ? (q1 | m_) : (q1 & ~m_); \
    q2 = ((c) & 4) ? (q2 | m_) : (q2 & ~m_); \
    q3 = ((c) & 8) ? (q3 | m_) : (q3 & ~m_); } while(0)
#define PLANE_ROWS(p, s) do { \
    p[0] = s[0]; p[80] = s[1]; p[160] = s[2]; p[240] = s[3]; \
    p[320] = s[4]; p[400] = s[5]; p[480] = s[6]; p[560] = s[7]; \
    p[640] = s[8]; p[720] = s[9]; p[800] = s[10]; p[880] = s[11]; \
    p[960] = s[12]; p[1040] = s[13]; \
    if(CH == 16) { p[1120] = s[14]; p[1200] = s[15]; } } while(0)
static void
write4(unsigned char cx, unsigned char cy)
{
  /* the sequencer index stays on the map mask (2): one OUT a plane, then
     the CH bytes of the plane down the cell, a movsb and a step of a line
     each */
  unsigned short off = (unsigned short)cy * (CH * 80) + cx;
  const unsigned char *src = cb4;
  if(cx >= SW || cy >= SH) {
    return;
  }
  __asm__ volatile ("push %%es\n\t"
                    "mov $0xa000,%%ax\n\tmov %%ax,%%es\n\t"
                    "mov $0x3c5,%%dx\n\tcld\n\t"
                    "mov $1,%%al\n\tout %%al,%%dx\n\tmov %%bx,%%di\n\t"
                    ".rept %c[ch]\n\tmovsb\n\tadd $79,%%di\n\t.endr\n\t"
                    "mov $2,%%al\n\tout %%al,%%dx\n\tmov %%bx,%%di\n\t"
                    ".rept %c[ch]\n\tmovsb\n\tadd $79,%%di\n\t.endr\n\t"
                    "mov $4,%%al\n\tout %%al,%%dx\n\tmov %%bx,%%di\n\t"
                    ".rept %c[ch]\n\tmovsb\n\tadd $79,%%di\n\t.endr\n\t"
                    "mov $8,%%al\n\tout %%al,%%dx\n\tmov %%bx,%%di\n\t"
                    ".rept %c[ch]\n\tmovsb\n\tadd $79,%%di\n\t.endr\n\t"
                    "mov $15,%%al\n\tout %%al,%%dx\n\t"
                    "pop %%es"
                    : "+S" (src)
                    : "b" (off), [ch] "i" (CH)
                    : "ax", "dx", "di", "memory", "cc");
}
#define blit_again(cx, cy) write4(cx, cy)
#define planes4() do { } while(0)
/* n bytes of v from offset off of the video segment, in the planes enabled */
static void vset(unsigned short off, unsigned char v, unsigned short n);
/* a pixel row of n cells, the four plane bytes q the same in every cell:
   planes all on or all off by set/reset, the others a pass for each
   different byte */
static void
vrow(unsigned short off, const unsigned char *q, unsigned short n)
{
  unsigned char k, e = 0, sr = 0, done;
  for(k = 0; k < 4; ++k) {
    if(q[k] == 0 || q[k] == 0xff) {
      e |= 1 << k;
      if(q[k]) {
        sr |= 1 << k;
      }
    }
  }
  outw(0x3ce, (unsigned short)sr << 8);          /* set/reset */
  outw(0x3ce, ((unsigned short)e << 8) | 1);     /* enable set/reset */
  done = e;
  if(e == 0x0f) {
    vset(off, 0, n);
  }
  for(k = 0; k < 4; ++k) {
    if(!(done & (1 << k))) {
      unsigned char j, m = 1 << k;
      for(j = k + 1; j < 4; ++j) {
        if(!(done & (1 << j)) && q[j] == q[k]) {
          m |= 1 << j;
        }
      }
      outb(0x3c5, m | e);               /* the set/reset planes come along */
      vset(off, q[k], n);
      done |= m;
      e = 0;
    }
  }
  outw(0x3ce, 1);
  outb(0x3c5, 0x0f);
}
static void
vset(unsigned short off, unsigned char v, unsigned short n)
{
  __asm__ volatile ("push %%es\n\tmov %%dx,%%es\n\tcld\n\trep stosb\n\tpop %%es"
                    : "+D" (off), "+c" (n)
                    : "a" ((unsigned short)v), "d" (0xa000)
                    : "memory", "cc");
}
#define blit_planes(cx, cy) write4(cx, cy)
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
#if defined(MONO)
  (void)r; (void)c;
  pack();
  blit_bytes(cx, cy, mb);
#elif defined(GFX_TANDY4)
  for(r = 0; r < 8; ++r) {
    unsigned char lo = 0, hi = 0;
    for(c = 0; c < 8; ++c) {
      lo = (lo << 1) | (pix[r][c] & 1);
      hi = (hi << 1) | (pix[r][c] >> 1);
    }
    cb4[2 * r] = lo;
    cb4[2 * r + 1] = hi;
  }
  write4(cx, cy);
#elif defined(GFX_CGA4)
  for(r = 0; r < 8; ++r) {
    const unsigned char *q = pix[r];
    cb4[2 * r] = (q[0] << 6) | (q[1] << 4) | (q[2] << 2) | q[3];
    cb4[2 * r + 1] = (q[4] << 6) | (q[5] << 4) | (q[6] << 2) | q[7];
  }
  (void)c;
  write4(cx, cy);
#elif defined(GFX_TANDY)
  /* 4 bits a pixel, left pixel in the high nibble; four banks of 8K,
     pixel row Y in bank Y & 3, line Y >> 2 of 160 bytes */
  for(r = 0; r < 8; ++r) {
    const unsigned char *s = pix[r];
    unsigned char *o = cb4 + 4 * r;
    o[0] = (s[0] << 4) | s[1];
    o[1] = (s[2] << 4) | s[3];
    o[2] = (s[4] << 4) | s[5];
    o[3] = (s[6] << 4) | s[7];
  }
  (void)c;
  write4(cx, cy);
#elif defined(GFX_MCGA)
  {
    /* eight rows of eight bytes: four word moves and a step of a line */
    unsigned short off = (unsigned short)cy * (8 * 320) + cx * 8;
    const unsigned char *src = pix[0];
    (void)r; (void)c;
    __asm__ volatile ("push %%es\n\tmov $0xa000,%%ax\n\tmov %%ax,%%es\n\tcld\n\t"
                      ".rept 8\n\tmovsw\n\tmovsw\n\tmovsw\n\tmovsw\n\tadd $312,%%di\n\t.endr\n\t"
                      "pop %%es"
                      : "+S" (src), "+D" (off)
                      : : "ax", "memory", "cc");
  }
#else
  /* EGA and VGA: the pixels into the four planes */
  for(r = 0; r < CH; ++r) {
    const unsigned char *s = pix[r];
    unsigned char q0 = 0, q1 = 0, q2 = 0, q3 = 0, m = 0x80;
    for(c = 0; c < 8; ++c, m >>= 1) {
      unsigned char v = s[c];
      if(v & 1) {
        q0 |= m;
      }
      if(v & 2) {
        q1 |= m;
      }
      if(v & 4) {
        q2 |= m;
      }
      if(v & 8) {
        q3 |= m;
      }
    }
    P0[r] = q0; P1[r] = q1; P2[r] = q2; P3[r] = q3;
  }
  write4(cx, cy);
#endif
}
/*---------------------------------------------------------------------------*/
/* Fonts: 8x8 for 8-pixel cells, the small bar font, the BIOS font otherwise */
#if CH > 8 && !defined(MONO)
static unsigned char bfont[96][CH];
static void
load_bios_font(void)
{
  unsigned short seg, off, bx = CH == 14 ? 0x0200 : 0x0600;
  unsigned char i, r;
  const unsigned char __far *f;
  /* (BX is an output too: a video BIOS may change it) */
  __asm__ volatile ("push %%bp\n\tpush %%es\n\tint $0x10\n\tmov %%es,%%ax\n\t"
                    "mov %%bp,%%dx\n\tpop %%es\n\tpop %%bp"
                    : "=a" (seg), "=d" (off), "+b" (bx)
                    : "a" (0x1130)
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
#if CH > 8 && defined(MONO)
  /* tall cells on two colours: Clean Bold, one row lower in the bars */
  return font_tall[c - 32] + (barfont ? 0 : 1);
#elif CH > 8
  return bfont[c - 32];
#else
#if defined(MONO)
  /* two colours: the small Clean 6x6 font in the bars, as on the approved
     trial; the 8x8 font in the windows (four colours: 8x8 everywhere) */
  if(barfont) {
    return font_bar[c - 32];
  }
#elif defined(GFX_CGA4) && CGA4_BTN == 2
  if(barfont == 2) {
    return font_bar[c - 32];
  }
#endif
  return font8x8[c];
#endif
}
/*---------------------------------------------------------------------------*/
/* Composing a cell */
static unsigned char rowbg[CH];         /* background colour of each pixel row */
#if defined(GFX_EGA) || defined(GFX_VGA)
/* the text cell last composed: its row colours and colour, and as planes
   its background and the background xor the colour */
static unsigned char tc_bg[CH], tc_fg = 0xff, tc_bd[8 * CH];
#endif

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
#if defined(GFX_HERC)
/* Hercules: plain black by default; light dots on its narrow pixels read
   as a grey weave */
unsigned char ctk_cga_wallpaper = 3;
#else
unsigned char ctk_cga_wallpaper;
#endif
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
#define SEP_X ((SW - CTK_ICONX_FROM_RIGHT - GFX_SEP_GAP) * 8 + 3)

static unsigned char desk_nosep;        /* 1: leave out the icon column line */
#if defined(GFX_MCGA)
/* the colour of each pixel row of the graded desktop, worked out at start:
   C_DESK + Y * DESK_STEPS / (SH * CH) */
static unsigned char mdesk[SH * CH];
static void
mdesk_init(void)
{
  unsigned short y;
  for(y = 0; y < SH * CH; ++y) {
    mdesk[y] = C_DESK + (unsigned char)(y * DESK_STEPS / (SH * CH));
  }
}
#endif
/* Y * 5 / (SH * CH), the band of the 16-colour desktop, without dividing:
   band k starts at the first Y with Y * 5 >= k * SH * CH */
#define DESK_B(k) (((k) * SH * CH + 4) / 5)
#define DESK_BAND(Y) ((Y) < DESK_B(2) ? ((Y) < DESK_B(1) ? 0 : 1) : \
  (Y) < DESK_B(3) ? 2 : (Y) < DESK_B(4) ? 3 : 4)

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
  return mdesk[Y];
#elif defined(GFX_TANDY)
  (void)X; (void)Y;
  return C_GREY;
#else
  {
    /* white above, a fine checker of white and grey across the middle,
       grey below: the stepped shading of the 16-colour cards */
    unsigned char band = DESK_BAND(Y);
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
#if defined(GFX_EGA) || defined(GFX_VGA)
/* desk() for the 8 pixels of a cell row, as four plane bytes: white above,
   the checker of white and grey across the middle (grey on odd X ^ Y),
   grey below, and the etched line of the icon column */
/* the four rows of the desktop as plane bytes: white, grey, the checker
   on an even row and on an odd one (grey on odd X ^ Y), set at start */
static unsigned char dpat[4][4];
#define DESKQ(Y) (DESK_BAND(Y) < 2 ? 0 : DESK_BAND(Y) >= 4 ? 1 : 2 + ((Y) & 1))
static unsigned char deskrow[SH * CH][4];       /* dpat of each pixel row */
static unsigned short expw[16][4];      /* colour -> plane words, 0 or 0xffff */
static void
desk_patterns(void)
{
  unsigned char k, c;
  for(c = 0; c < 16; ++c) {
    for(k = 0; k < 4; ++k) {
      expw[c][k] = (c >> k) & 1 ? 0xffff : 0;
    }
  }
  for(k = 0; k < 4; ++k) {
    unsigned char w = (C_PAPER >> k) & 1 ? 0xff : 0, g = (C_GREY >> k) & 1 ? 0xff : 0;
    dpat[0][k] = w;
    dpat[1][k] = g;
    dpat[2][k] = (g & ~0xaa) | (w & 0xaa);      /* even Y: grey on odd X */
    dpat[3][k] = (g & ~0x55) | (w & 0x55);
  }
  {
    unsigned short y;
    for(y = 0; y < SH * CH; ++y) {
      memcpy(deskrow[y], dpat[DESKQ(y)], 4);
    }
  }
}
static void
desk16(unsigned char cx, unsigned short Y, unsigned char *q)
{
  const unsigned char *d = deskrow[Y];
  q[0] = d[0]; q[1] = d[1]; q[2] = d[2]; q[3] = d[3];
  if(cx == SEP_X / 8 && !desk_nosep && Y >= CH + CH / 2 && Y < (SH - 1) * CH - CH / 2) {
    unsigned char q0 = q[0], q1 = q[1], q2 = q[2], q3 = q[3];
    FILL16(0x80 >> (SEP_X & 7), C_ETCH0);
    if(cx == (SEP_X + 1) / 8) {
      FILL16(0x80 >> ((SEP_X + 1) & 7), C_ETCH1);
    }
    q[0] = q0; q[1] = q1; q[2] = q2; q[3] = q3;
  }
}
#endif
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
  for(r = 0; r < CH; ++r) {
    mb[r] = (cy == SH - 2 && r == CH - 1) ? 0xff : wall[(Y + r) & 7];   /* line over the status bar */
    if(cx == SEP_X / 8 && Y + r >= CH + CH / 2 && Y + r < (SH - 1) * CH - CH / 2) {
      mb[r] |= 0x80 >> (SEP_X & 7);
    }
  }
  (void)c;
  blit_bytes(cx, cy, mb);
#elif defined(GFX_TANDY)
  /* grey, with the icon column line (dark grey then white) and the dark
     line over the status bar */
  unsigned short Y = cy * CH;
  unsigned char m0 = 0, m1 = 0;
  (void)c;
  if(!desk_nosep) {
    m0 = cx == SEP_X / 8 ? 0x80 >> (SEP_X & 7) : 0;
    m1 = cx == (SEP_X + 1) / 8 ? 0x80 >> ((SEP_X + 1) & 7) : 0;
  }
  for(r = 0; r < 8; ++r) {
    unsigned char q0 = 0xff, q1 = 0xff, q2 = 0xff, q3 = 0;
    if(Y + r == (SH - 1) * CH - 1) {
      FILL16(0xff, C_ETCH0);
    } else if(Y + r >= CH + CH / 2 && Y + r < (SH - 1) * CH - CH / 2) {
      FILL16(m0, C_ETCH0);
      FILL16(m1, C_ETCH1);
    }
    P0[r] = q0; P1[r] = q1; P2[r] = q2; P3[r] = q3;
  }
  blit_planes(cx, cy);
#elif defined(GFX_CGA4)
  /* black, with the icon column line (cyan then black) and the cyan line
     over the status bar */
  unsigned short Y = cy * CH;
  unsigned char m = (cx == SEP_X / 8 && !desk_nosep) ? 0x80 >> (SEP_X & 7) : 0;
  (void)c;
  for(r = 0; r < 8; ++r) {
    unsigned char v = 0;
    if(Y + r == (SH - 1) * CH - 1) {
      v = 0xff;
    } else if(Y + r >= CH + CH / 2 && Y + r < (SH - 1) * CH - CH / 2) {
      v = m;
    }
    P1[r] = 0;
    P0[r] = v;
  }
  blit_planes(cx, cy);
#elif defined(GFX_EGA) || defined(GFX_VGA)
  unsigned char q[4];
  (void)c;
  for(r = 0; r < CH; ++r) {
    desk16(cx, cy * CH + r, q);
    P0[r] = q[0]; P1[r] = q[1]; P2[r] = q[2]; P3[r] = q[3];
  }
  blit_planes(cx, cy);
#elif defined(GFX_MCGA)
  /* one colour a row but by the icon column line */
  unsigned short X = cx * 8, Y = cy * CH;
  for(r = 0; r < CH; ++r) {
    unsigned short y = Y + r;
    memset(pix[r], desk(X, y), 8);
    if(cx == SEP_X / 8 && !desk_nosep && y >= CH + CH / 2 && y < (SH - 1) * CH - CH / 2) {
      pix[r][SEP_X & 7] = C_ETCH0;
      pix[r][(SEP_X + 1) & 7] = C_ETCH1;     /* (in the same cell) */
    }
  }
  (void)c;
  blit(cx, cy);
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
static unsigned char gfresh;            /* widgets drawn right after borders */
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
  gabove = (CGA_BAR == 2 && y >= 2) ? 1 : 0;
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
        if(X <= gL + 1 || X >= gR - 1 || (CGA_BAR == 1 && r == 0)) {
          return edge;                  /* the double frame goes round the bar */
        }
        if(gactive && bar_lines(r) && !onplate && X >= gL + 3 && X <= gR - 3) {
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
  if(X >= gL + 2 && X <= gR + SHX && Y + gabove >= gT + 2 && Y <= gB + SHY &&
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

#if defined(MONO) || defined(GFX_CGA4) || defined(PL16) || defined(GFX_MCGA)
/* Bits of a cell (most significant = leftmost) for the pixels X0..X0+7 that
   fall in [a, b] */
static const unsigned char span_lo[8] = {
  0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01
};
static const unsigned char span_hi[8] = {
  0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};
static unsigned char
span(unsigned short X0, unsigned short a, unsigned short b)
{
  unsigned char lo, hi;
  if(b < a || b < X0 || a > X0 + 7) {
    return 0;
  }
  lo = a > X0 ? a - X0 : 0;
  hi = b < X0 + 7 ? b - X0 : 7;
  return span_lo[lo] & span_hi[hi];
}
/* span() with the common cases, a cell wholly out of or inside [a, b], done
   in line */
#define SPAN(X0, a, b) ((b) < (X0) || (a) > (X0) + 7 ? 0 : \
  ((a) <= (X0) && (b) >= (X0) + 7 && (a) <= (b)) ? 0xff : span(X0, a, b))
#endif
#if defined(MONO)
/* Masks for the rows of a cell, as span() for the pixels: the top bit is
   row 0 */
#if CH == 8
typedef unsigned char rmask_t;
#define RSPAN(Y0, a, b) SPAN(Y0, a, b)
#define RBIT0 0x80
#else
typedef unsigned short rmask_t;
#define RBIT0 0x8000
static rmask_t
RSPAN(unsigned short Y0, unsigned short a, unsigned short b)
{
  unsigned char lo, hi;
  if(b < a || b < Y0 || a > Y0 + CH - 1) {
    return 0;
  }
  lo = a > Y0 ? a - Y0 : 0;
  hi = b < Y0 + CH - 1 ? b - Y0 : CH - 1;
  return (0xffffu >> lo) & (unsigned short)(0xffffu << (15 - hi));
}
#endif
/* The window cell on two colours, a row at a time, into mb. The masks
   along the cell do not change from row to row, so they are worked out
   once a cell. */
static void
wcell(unsigned char cx, unsigned char cy)
{
  unsigned char r;
  unsigned short X0 = cx * 8, Y0 = cy * CH;
  unsigned char m_shr, m_shb, plate, m_cut, m_in, m_edge, m_sep, m_lines,
    m_boxin, m_boxside, m_box, m_desk;
  rmask_t rm_desk, rm_shr, rm_shb, rm_cut, rm_win, bit;
  short d;
  m_shr = SPAN(X0, gR + 1, gR + SHX);
  m_shb = SPAN(X0, gL + SHX, gR + SHX - 1);
  m_cut = SPAN(X0, gL + 1, gR - 1);
  m_in = SPAN(X0, gL, gR);
  m_edge = SPAN(X0, gL, gL) | SPAN(X0, gR, gR);
  m_sep = m_lines = m_boxin = m_boxside = m_box = 0;
  if(Y0 <= gT + BAR_SEP && Y0 + CH - 1 >= gT) {
    /* the cell has rows of the title bar */
    plate = gn ? SPAN(X0, gtx0 - 4, gtx0 + gn * ADV + 3) : 0;
    m_sep = m_cut & (BAR_SEP < CH - 1 ? ~plate : 0xff);
    if(gactive) {
      m_lines = SPAN(X0, gbox1 + 4, gR - 2) & ~plate;
      m_boxin = SPAN(X0, gbox0 + 1, gbox1 - 1);
      m_boxside = SPAN(X0, gbox0, gbox0) | SPAN(X0, gbox1, gbox1);
      m_box = gkf_close ? SPAN(X0, gbox0, gbox1) : 0;
    }
  }
  m_desk = cx == SEP_X / 8 ? 0x80 >> (SEP_X & 7) : 0;
  rm_desk = m_desk ? RSPAN(Y0, CH + CH / 2, (SH - 1) * CH - CH / 2 - 1) : 0;
  rm_shr = RSPAN(Y0, gT + SHY + 1 - gabove, gB + SHY);
  rm_shb = RSPAN(Y0, gB + 1, gB + SHY);
  rm_cut = RSPAN(Y0, gB, gB) | (CGA_BAR == 1 ? RSPAN(Y0, gT, gT) : 0);
  rm_win = RSPAN(Y0, gT, gB) & ~rm_cut;
  d = (short)Y0 - (short)gT;            /* bar row of pixel row 0 */
  for(r = 0, bit = RBIT0; r < CH; ++r, bit >>= 1) {
    unsigned char v = wall[(Y0 + r) & 7];
    if(cy == SH - 2 && r == CH - 1) {
      v = 0xff;                         /* line over the status bar */
    }
    if(rm_desk & bit) {
      v |= m_desk;
    }
    /* shadow */
    if(rm_shr & bit) {
      v |= m_shr;
    }
    if(rm_shb & bit) {
      v |= m_shb;
    }
    if(rm_cut & bit) {
      v |= m_cut;                       /* corners cut */
    } else if(rm_win & bit) {
      short rr = d + r;
      v = (v & ~m_in) | m_edge;
      if(rr == BAR_SEP) {
        /* line under the bar; on 8-pixel cells with the frame inside the
           title row, the title plate opens it, as the letters reach it */
        v |= m_sep;
      } else if(rr < BAR_SEP && gactive) {
        /* lines along the bar, with plates for the close box and the
           title */
        if(bar_lines(rr)) {
          v |= m_lines;
        }
        if(rr == BOX_T || rr == BOX_B) {
          v |= m_boxin;
        } else if(rr > BOX_T && rr < BOX_B) {
          v |= m_boxside | m_box;
        }
      }
    }
    mb[r] = v;
  }
}
/* A cell of the title row between the sides of the frame: the frame when
   it is in this row, the bar rows, then the line under it; nothing of the
   desktop shows */
static void
bar_cell(unsigned short X0)
{
  unsigned char r, plate, m_lines = 0, m_boxin = 0, m_boxside = 0;
  plate = gn ? SPAN(X0, gtx0 - 4, gtx0 + gn * ADV + 3) : 0;
  if(gactive) {
    m_lines = SPAN(X0, gbox1 + 4, gR - 2) & ~plate;
    m_boxin = SPAN(X0, gbox0 + 1, gbox1 - 1);
    m_boxside = SPAN(X0, gbox0, gbox0) | SPAN(X0, gbox1, gbox1) |
      (gkf_close ? SPAN(X0, gbox0, gbox1) : 0);
  }
  for(r = 0; r < BAR_SEP; ++r) {
    unsigned char v = bar_lines(r) ? m_lines : 0;
    if(r == BOX_T || r == BOX_B) {
      v |= m_boxin;
    } else if(r > BOX_T && r < BOX_B) {
      v |= m_boxside;
    }
    mb[r] = v;
  }
  if(CGA_BAR == 1) {
    mb[0] = 0xff;                       /* the frame */
  }
  mb[BAR_SEP] = BAR_SEP < CH - 1 ? ~plate : 0xff;
  for(r = BAR_SEP + 1; r < CH; ++r) {
    mb[r] = 0;
  }
}
#define wcell_done() do { } while(0)
#elif defined(GFX_CGA4) && CGA_BAR == 2
/* Four colours: the window cell in the bit planes, filled by masks as on
   two colours */
static void
wcell(unsigned char cx, unsigned char cy)
{
  unsigned char r, p1, p0;
  unsigned short X0 = cx * 8, Y0 = cy * CH;
  unsigned char m_sep1, m_sep0, m_shr, m_shb, m_far, m_win, m_cut, m_e2, m_e3,
    m_e3row, plate = 0, m_lines = 0, m_box = 0, m_boxside = 0, boxcol, mid;
  m_sep1 = desk_nosep ? 0 : SPAN(X0, SEP_X, SEP_X);
  m_sep0 = desk_nosep ? 0 : SPAN(X0, SEP_X + 1, SEP_X + 1);
  m_shr = SPAN(X0, gR + 1, gR + SHX);
  m_shb = SPAN(X0, gL + 2, gR + SHX);
  m_far = SPAN(X0, gR + SHX, gR + SHX);
  m_win = SPAN(X0, gL, gR);
  m_cut = SPAN(X0, gL + 1, gR - 1);
  m_e2 = SPAN(X0, gL, gL + 1) | SPAN(X0, gR - 1, gR);
  m_e3 = SPAN(X0, gL + 3, gL + 3) | SPAN(X0, gR - 3, gR - 3);
  m_e3row = SPAN(X0, gL + 3, gR - 3);
  if(Y0 <= gT + CH - 1 && Y0 + 7 >= gT) {
    /* the cell has rows of the title bar */
    plate = gn ? SPAN(X0, gtx0 - 4, gtx0 + gn * ADV + 3) : 0;
    m_lines = gactive ? m_e3row & ~plate : 0;
    m_box = SPAN(X0, gbox0, gbox1);
    m_boxside = SPAN(X0, gbox0, gbox0) | SPAN(X0, gbox1, gbox1);
  }
  boxcol = gkf_close ? C_RING : (gactive ? C_HL : C_EDGE_I);
  mid = (gbox0 + gbox1) / 2 - X0;       /* cross centre, if in this cell */
  for(r = 0; r < 8; ++r) {
    unsigned short Y = Y0 + r;
    p1 = p0 = 0;
    if(Y == (SH - 1) * CH - 1) {
      p0 = 0xff;                        /* line over the status bar */
    }
    if(Y >= CH + CH / 2 && Y < (SH - 1) * CH - CH / 2) {
      FILL(m_sep1, C_ETCH0);
      FILL(m_sep0, C_ETCH1);
    }
    /* shadow: a checker of magenta over the desktop */
    if(Y + gabove >= gT + 2 && Y <= gB + SHY) {
      unsigned char sm = Y > gB ? m_shb : m_shr;
      if(Y == gB + SHY) {
        sm &= ~m_far;
      }
      FILL(sm & ((Y & 1) ? 0xaa : 0x55), C_SH1);
    }
    if(Y >= gT && Y <= gB) {
      unsigned char wm = (Y == gT || Y == gB) ? m_cut : m_win, rr = Y - gT;
      if(Y >= gT + CH) {
        /* double frame, thick outside and thin inside */
        if(Y == gB) {
          FILL(wm, C_EDGE_A);
        } else {
          FILL(wm, C_PAPER);
          FILL(m_e2, C_EDGE_A);
          if(Y <= gB - 2) {
            FILL(m_e3, C_EDGE_A);
          }
          if(Y == gB - 2) {
            FILL(m_e3row, C_EDGE_A);
          }
        }
      } else if(rr == CH - 1) {
        FILL(wm, C_EDGE_A);             /* line under the bar */
      } else {
        FILL(wm, C_PAPER);
        FILL(wm & m_e2, C_EDGE_A);
        if(bar_lines(rr)) {
          FILL(m_lines, C_BAR_AT);
        }
        if(m_box && rr >= 1 && rr <= CH - 2) {
          if(rr == 1 || rr == CH - 2) {
            FILL(m_box, boxcol);
          } else {
            /* the cross: pixels mid - d and mid + d, d = rr - 3 */
            signed char d = rr - (CH - 1) / 2, a = mid - d, b = mid + d;
            unsigned char cm = 0;
            if(a >= 0 && a < 8) {
              cm |= 0x80 >> a;
            }
            if(b >= 0 && b < 8) {
              cm |= 0x80 >> b;
            }
            FILL(m_box, gactive ? C_BAR_AB : C_PAPER);
            FILL(m_boxside, boxcol);
            FILL(cm & m_box, gactive ? C_TITLE_A : C_INK);
          }
        }
      }
    }
    P1[r] = p1;
    P0[r] = p0;
  }
}
#define wcell_done() do { } while(0)
#elif defined(GFX_TANDY)
/* 16 colours, the colour style of wpx() with masks: four planes a row */
static void
wcell(unsigned char cx, unsigned char cy)
{
  unsigned char r, q0, q1, q2, q3, edge = gactive ? C_EDGE_A : C_EDGE_I;
  unsigned short X0 = cx * 8, Y0 = cy * CH;
  unsigned char m_sep0, m_sep1, m_win, m_cut, m_ends, m_shr, m_shr1, m_shb,
    m_shb1, m_far, m_box, m_boxside, m_gloss, boxcol, top, bot;
  signed char mid;
  m_sep0 = desk_nosep ? 0 : SPAN(X0, SEP_X, SEP_X);
  m_sep1 = desk_nosep ? 0 : SPAN(X0, SEP_X + 1, SEP_X + 1);
  m_win = SPAN(X0, gL, gR);
  m_cut = SPAN(X0, gL + 1, gR - 1);
  m_ends = SPAN(X0, gL, gL) | SPAN(X0, gR, gR);
  m_shr = SPAN(X0, gR + 1, gR + SHX);
  m_shr1 = SPAN(X0, gR + 1, gR + SHX / 2);
  m_shb = SPAN(X0, gL + 2, gR + SHX);
  m_shb1 = SPAN(X0, gL + 2, gR + SHX / 2);
  m_far = SPAN(X0, gR + SHX, gR + SHX);
  m_box = SPAN(X0, gbox0, gbox1);
  m_boxside = SPAN(X0, gbox0, gbox0) | SPAN(X0, gbox1, gbox1);
  m_gloss = SPAN(X0, ggloss0, ggloss1);
  boxcol = gkf_close ? C_RING : (gactive ? C_HL : C_EDGE_I);
  mid = (signed char)((gbox0 + gbox1) / 2 - X0);
  top = gactive ? C_BAR_AT : C_BAR_IT;
  bot = gactive ? C_BAR_AB : C_BAR_IB;
  for(r = 0; r < 8; ++r) {
    unsigned short Y = Y0 + r;
    /* the desktop: grey, the icon column line, the line over the status bar */
    q0 = q1 = q2 = 0xff;                /* C_GREY = 7 */
    q3 = 0;
    if(Y == (SH - 1) * CH - 1) {
      FILL16(0xff, C_ETCH0);
    } else if(Y >= CH + CH / 2 && Y < (SH - 1) * CH - CH / 2) {
      FILL16(m_sep0, C_ETCH0);
      FILL16(m_sep1, C_ETCH1);
    }
    /* shadow in two steps: dark next to the window, lighter further out */
    if(Y + gabove >= gT + 2 && Y <= gB + SHY) {
      if(Y <= gB) {
        unsigned char in = (Y >= gT + 1 + SHY / 2) ? m_shr1 : 0;
        FILL16(m_shr & ~in, C_SH2);
        FILL16(in, C_SH1);
      } else {
        unsigned char in = (Y <= gB + SHY / 2) ? m_shb1 : 0;
        FILL16(m_shb & ~in & (Y == gB + SHY ? ~m_far : 0xff), C_SH2);
        FILL16(in, C_SH1);
      }
    }
    if(Y >= gT && Y <= gB) {
      unsigned char rr = Y - gT;
      if(Y == gB) {
        FILL16(m_cut, edge);
      } else if(rr >= CH) {
        FILL16(m_win, C_PAPER);
        FILL16(m_ends, edge);
      } else {
        unsigned char in = m_cut;
        if(rr != 0) {
          FILL16(m_ends, edge);
        }
        if(rr == CH - 1) {
          FILL16(in, edge);             /* line under the bar */
        } else {
          FILL16(in, rr < CH / 2 ? top : bot);
          if(rr == 1) {
            FILL16(in & m_gloss, gactive ? C_HL : C_ETCH1);
          }
          if(m_box && rr >= 1 && rr <= CH - 2) {
            if(rr == 1 || rr == CH - 2) {
              FILL16(m_box, boxcol);
            } else {
              signed char d = rr - (CH - 1) / 2, a = mid - d, b = mid + d;
              unsigned char cm = 0;
              if(a >= 0 && a < 8) {
                cm |= 0x80 >> a;
              }
              if(b >= 0 && b < 8) {
                cm |= 0x80 >> b;
              }
              FILL16(m_box, gactive ? C_BAR_AB : C_PAPER);
              FILL16(m_boxside, boxcol);
              FILL16(cm & m_box, gactive ? C_TITLE_A : C_INK);
            }
          }
        }
      }
    }
    P0[r] = q0; P1[r] = q1; P2[r] = q2; P3[r] = q3;
  }
}
#define wcell_done() do { } while(0)
#elif defined(GFX_MCGA)
/* bytes of a pixel row of pix where the mask has bits */
static void
fillm(unsigned char *row, unsigned char m, unsigned char col)
{
  unsigned char c;
  for(c = 0; m; ++c, m <<= 1) {
    if(m & 0x80) {
      row[c] = col;
    }
  }
}
/* wpx() for a whole cell, a row at a time, with masks: the desktop, the
   shadow in two steps (darker copies of the desktop colours), the frame
   with its corners cut, the graded bar with its light line and the close
   box */
static void
wcell(unsigned char cx, unsigned char cy)
{
  unsigned char r, edge = gactive ? C_EDGE_A : C_EDGE_I;
  unsigned short X0 = cx * 8, Y0 = cy * CH;
  unsigned char m_win, m_in, m_ends, m_shr, m_shb, m_s1x, m_far, m_box,
    m_boxside, m_boxin, m_gloss, m_sep0, m_sep1, boxcol, bar;
  short mid;
  m_win = SPAN(X0, gL, gR);
  m_in = SPAN(X0, gL + 1, gR - 1);
  m_ends = m_win & ~m_in;
  m_shr = SPAN(X0, gR + 1, gR + SHX);
  m_shb = SPAN(X0, gL + 2, gR + SHX);
  m_s1x = SPAN(X0, gL + 1 + SHX / 2, gR + SHX / 2);
  m_far = SPAN(X0, gR + SHX, gR + SHX);
  m_box = SPAN(X0, gbox0, gbox1);
  m_boxside = SPAN(X0, gbox0, gbox0) | SPAN(X0, gbox1, gbox1);
  m_boxin = SPAN(X0, gbox0 + 1, gbox1 - 1);
  m_gloss = SPAN(X0, ggloss0, ggloss1);
  m_sep0 = desk_nosep ? 0 : SPAN(X0, SEP_X, SEP_X);
  m_sep1 = desk_nosep ? 0 : SPAN(X0, SEP_X + 1, SEP_X + 1);
  boxcol = gkf_close ? C_RING : (gactive ? C_HL : C_EDGE_I);
  bar = gactive ? C_BAR_A : C_BAR_I;
  mid = (short)((gbox0 + gbox1) / 2) - (short)X0;
  for(r = 0; r < CH; ++r) {
    unsigned short Y = Y0 + r;
    unsigned char *row = pix[r], d = mdesk[Y], seprow;
    /* the desktop: the colour of the row, the line over the status bar,
       the icon column line */
    seprow = Y >= CH + CH / 2 && Y < (SH - 1) * CH - CH / 2;
    if(Y == (SH - 1) * CH - 1) {
      memset(row, C_ETCH0, 8);
    } else {
      memset(row, d, 8);
      if(seprow) {
        fillm(row, m_sep0, C_ETCH0);
        fillm(row, m_sep1, C_ETCH1);
      }
    }
    if(Y + gabove >= gT + 2 && Y <= gB + SHY) {
      unsigned char sm = Y <= gB ? m_shr : m_shb;
      unsigned char s1 = (Y >= gT + 1 + SHY / 2 && Y <= gB + SHY / 2) ? (sm & m_s1x) : 0;
      unsigned char s2 = sm & ~s1, sp;
      if(Y == gB + SHY) {
        s2 &= ~m_far;
      }
      /* shade(): one or two steps darker; where the desktop is not a
         desktop colour (the lines), the last desktop colour darkened */
      sp = (Y == (SH - 1) * CH - 1) ? 0xff : (seprow ? (m_sep0 | m_sep1) : 0);
      fillm(row, s2 & ~sp, d + (C_DESK_SH2 - C_DESK));
      fillm(row, s1 & ~sp, d + (C_DESK_SH1 - C_DESK));
      fillm(row, s2 & sp, C_DESK + DESK_STEPS - 1 + (C_DESK_SH2 - C_DESK));
      fillm(row, s1 & sp, C_DESK + DESK_STEPS - 1 + (C_DESK_SH1 - C_DESK));
    }
    if(Y >= gT && Y <= gB) {
      unsigned short rr = Y - gT;
      if(Y == gT) {
        /* on 8-pixel cells the bar fills the top row too (the close box
           starts at row 1) */
        fillm(row, m_in, bar);
      } else if(Y == gB) {
        fillm(row, m_in, edge);
      } else {
        fillm(row, m_ends, edge);
        if(rr >= CH) {
          fillm(row, m_in, C_PAPER);
        } else if(rr == CH - 1) {
          fillm(row, m_in, edge);       /* line under the bar */
        } else {
          fillm(row, m_in, bar + rr);
          if(rr == 1) {
            fillm(row, m_gloss, gactive ? C_HL : C_ETCH1);
          }
          if(m_box && rr >= 1 && rr <= CH - 2) {
            if(rr == 1 || rr == CH - 2) {
              fillm(row, m_box, boxcol);
            } else {
              short cyy = (short)rr - (short)((CH - 1) / 2), d1 = cyy, d2 = -cyy;
              unsigned char cm = 0;
              if(d1 >= -2 && d1 <= 2 && mid + d1 >= 0 && mid + d1 < 8) {
                cm |= 0x80 >> (mid + d1);
              }
              if(d2 >= -2 && d2 <= 2 && mid + d2 >= 0 && mid + d2 < 8) {
                cm |= 0x80 >> (mid + d2);
              }
              fillm(row, m_boxin, gactive ? C_BAR_AB : C_PAPER);
              fillm(row, m_boxside, boxcol);
              fillm(row, cm & m_box, gactive ? C_TITLE_A : C_INK);
            }
          }
        }
      }
    }
  }
}
#define wcell_done() do { } while(0)
#elif defined(GFX_EGA) || defined(GFX_VGA)
/* wpx() for a whole cell, a row at a time, with masks into the four
   planes: the desktop, the shadow in two steps, the frame with its corners
   cut, the bar in two halves with its light line and the close box */
static void
wcell(unsigned char cx, unsigned char cy)
{
  unsigned char r, q0, q1, q2, q3, q[4], edge = gactive ? C_EDGE_A : C_EDGE_I;
  unsigned short X0 = cx * 8, Y0 = cy * CH;
  unsigned char m_win, m_in, m_ends, m_shr, m_shb, m_s1x, m_far, m_box,
    m_boxside, m_boxin, m_gloss, boxcol, top, bot;
  short mid;
  m_win = SPAN(X0, gL, gR);
  m_in = SPAN(X0, gL + 1, gR - 1);
  m_ends = m_win & ~m_in;
  m_shr = SPAN(X0, gR + 1, gR + SHX);
  m_shb = SPAN(X0, gL + 2, gR + SHX);
  m_s1x = SPAN(X0, gL + 1 + SHX / 2, gR + SHX / 2);
  m_far = SPAN(X0, gR + SHX, gR + SHX);
  m_box = SPAN(X0, gbox0, gbox1);
  m_boxside = SPAN(X0, gbox0, gbox0) | SPAN(X0, gbox1, gbox1);
  m_boxin = SPAN(X0, gbox0 + 1, gbox1 - 1);
  m_gloss = SPAN(X0, ggloss0, ggloss1);
  boxcol = gkf_close ? C_RING : (gactive ? C_HL : C_EDGE_I);
  top = gactive ? C_BAR_AT : C_BAR_IT;
  bot = gactive ? C_BAR_AB : C_BAR_IB;
  mid = (short)((gbox0 + gbox1) / 2) - (short)X0;
  for(r = 0; r < CH; ++r) {
    unsigned short Y = Y0 + r;
    desk16(cx, Y, q);
    q0 = q[0]; q1 = q[1]; q2 = q[2]; q3 = q[3];
    if(Y + gabove >= gT + 2 && Y <= gB + SHY) {
      unsigned char sm = Y <= gB ? m_shr : m_shb;
      unsigned char s1 = (Y >= gT + 1 + SHY / 2 && Y <= gB + SHY / 2) ? (sm & m_s1x) : 0;
      unsigned char s2 = sm & ~s1;
      if(Y == gB + SHY) {
        s2 &= ~m_far;
      }
      FILL16(s2, C_SH2);
      FILL16(s1, C_SH1);
    }
    if(Y >= gT && Y <= gB) {
      unsigned short rr = Y - gT;
      if(Y == gT || Y == gB) {
        FILL16(m_in, edge);             /* the corners stay outside */
      } else {
        FILL16(m_ends, edge);
        if(rr >= CH) {
          FILL16(m_in, C_PAPER);
        } else if(rr == CH - 1) {
          FILL16(m_in, edge);           /* line under the bar */
        } else {
          FILL16(m_in, rr < CH / 2 ? top : bot);
          if(rr == 1) {
            FILL16(m_gloss, gactive ? C_HL : C_ETCH1);
          }
          if(m_box) {
            if(rr == 1 || rr == CH - 2) {
              FILL16(m_box, boxcol);
            } else {
              /* the cross: pixels mid + d for d = cy, -cy, cy + 1, -cy + 1
                 within 3 of the middle */
              short cyy = (short)rr - (short)((CH - 1) / 2), d[4];
              unsigned char k, cm = 0;
              d[0] = cyy; d[1] = -cyy; d[2] = cyy + 1; d[3] = 1 - cyy;
              for(k = 0; k < 4; ++k) {
                if(d[k] >= -3 && d[k] <= 3) {
                  short x = mid + d[k];
                  if(x >= 0 && x < 8) {
                    cm |= 0x80 >> x;
                  }
                }
              }
              FILL16(m_boxin, gactive ? C_BAR_AB : C_PAPER);
              FILL16(m_boxside, boxcol);
              FILL16(cm & m_box, gactive ? C_TITLE_A : C_INK);
            }
          }
        }
      }
    }
    P0[r] = q0; P1[r] = q1; P2[r] = q2; P3[r] = q3;
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
#if defined(MONO)
  if(cx > gx && cx < gxr) {
    bar_cell(X0);                       /* wholly inside the frame */
  } else
#endif
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
#if defined(MONO) || (defined(GFX_CGA4) && CGA_BAR == 2) || defined(PL16)
  {
    /* letters are 8 pixels wide: the cell takes bytes from at most two of
       them, shifted */
    short d = (short)X0 - (short)gtx0;
    unsigned char sh = d & 7;
    short k = (d + 8) / 8 - 1;          /* the letter under pixel X0 */
    const unsigned char *g0 = NULL, *g1 = NULL;
    (void)c;
    if(d <= -8 || d >= (short)(gn * ADV)) {
      return;
    }
    barfont = 1;
    if(k >= 0) {
      g0 = glyph(gstr[k]);
    }
    if(sh && k + 1 < gn) {
      g1 = glyph(gstr[k + 1]);
    }
    barfont = 0;
    for(r = 0; r < CH - 1; ++r) {
      unsigned char b = 0;
      if(g0) {
        b = g0[r] << sh;
      }
      if(g1) {
        b |= g1[r] >> (8 - sh);
      }
#if defined(MONO)
      if(fg == C_INK) {
        mb[r] |= b;
      } else {
        mb[r] &= ~b;
      }
#elif defined(PL16)
      {
        unsigned char q0 = P0[r], q1 = P1[r], q2 = P2[r], q3 = P3[r];
        FILL16(b, fg);
        P0[r] = q0; P1[r] = q1; P2[r] = q2; P3[r] = q3;
      }
#else
      {
        unsigned char p1 = P1[r], p0 = P0[r];
        FILL(b, fg);
        P1[r] = p1;
        P0[r] = p0;
      }
#endif
    }
  }
#else
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
          pix[r][c] = fg;
        }
      }
    }
  }
  barfont = 0;
#endif
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
static unsigned char cache[2][CH];
#define SAVE(k) memcpy(cache[k], mb, CH)
#define PUT(k, cx, cy) blit_bytes(cx, cy, cache[k])
#elif (defined(GFX_CGA4) && CGA_BAR == 2) || defined(PL16)
static unsigned char cache[2][sizeof(cb4)];
#define SAVE(k) do { planes4(); memcpy(cache[k], cb4, sizeof(cb4)); } while(0)
#define PUT(k, cx, cy) do { memcpy(cb4, cache[k], sizeof(cb4)); write4(cx, cy); } while(0)
#else
static unsigned char cache[2][CH][8];
#define SAVE(k) memcpy(cache[k], pix, sizeof(pix))
#define PUT(k, cx, cy) do { memcpy(pix, cache[k], sizeof(pix)); blit(cx, cy); } while(0)
#endif

#if defined(MONO) || defined(GFX_CGA4)
/* The part of the bar above the title row: the last gabove pixel rows of
   the cell row above, written into video memory over what is there */
static void
top_line(void)
{
  unsigned char i, k;
  if(gy == 0) {
    return;
  }
  for(k = 8 - gabove; k < 8; ++k) {
    /* k: pixel row in the cell row above; d: rows from the top of the bar */
    unsigned char d = k - (8 - gabove);
#if defined(GFX_TANDY4)
    unsigned char __far *row = VRAM + (unsigned short)(gy - 1) * 320 +
      ((unsigned short)(k & 3) << 13) + (k >> 2) * 160;
#else
    unsigned char __far *row = VRAM + (unsigned short)(gy - 1) * 320 +
      ((k & 1) ? 0x2000 : 0) + (k >> 1) * 80;
#endif
    unsigned char stripe = gactive && gabove == BAR_ABOVE && d == 2;
    unsigned char shadow = d >= SHY + 1;
    for(i = gx; i <= gxr; ++i) {
      unsigned short X0 = i * 8;
#if defined(MONO)
      unsigned char ink, area = SPAN(X0, gL, gR + (shadow ? SHX : 0));
      if(d == 0) {
        ink = SPAN(X0, gL + 1, gR - 1);           /* frame, corners cut */
        area = SPAN(X0, gL + 1, gR - 1);
      } else {
        ink = SPAN(X0, gL, gL) | SPAN(X0, gR, gR);
        if(stripe) {
          ink |= SPAN(X0, gL + 2, gR - 2);
        }
        if(shadow) {
          ink |= SPAN(X0, gR + 1, gR + SHX);
        }
      }
      row[i] = (row[i] | area) & ~ink;            /* ink is 0 on the screen */
#else
      unsigned char c;
      for(c = 0; c < 8; ++c) {
        unsigned short X = X0 + c;
        unsigned char col = 255, e = gactive ? C_EDGE_A : C_EDGE_I;
        if(d == 0) {
          if(X >= gL + 1 && X <= gR - 1) {
            col = e;
          }
        } else if(X >= gL && X <= gR) {
          col = (X <= gL + 1 || X >= gR - 1) ? e :
                (stripe && X >= gL + 3 && X <= gR - 3) ? C_BAR_AT : C_PAPER;
        } else if(shadow && X > gR && X <= gR + SHX) {
          if(((X ^ (k + (gy - 1) * 8)) & 1) == 0) {
            col = C_SH1;
          }
        }
        if(col != 255) {
#if defined(GFX_TANDY4)
          /* the low bit of the eight pixels, then the high bit */
          unsigned char __far *b = row + i * 2;
          unsigned char m = 0x80 >> c;
          b[0] = (col & 1) ? (b[0] | m) : (b[0] & ~m);
          b[1] = (col & 2) ? (b[1] | m) : (b[1] & ~m);
#else
          unsigned char __far *b = row + i * 2 + (c >> 2);
          unsigned char sh = 6 - 2 * (c & 3);
          *b = (*b & ~(3 << sh)) | (col << sh);
#endif
        }
      }
#endif
    }
  }
}
#endif

/* a composed window cell to the screen */
#if defined(MONO)
#define BLITW(cx, cy) blit_bytes(cx, cy, mb)
#elif (defined(GFX_CGA4) && CGA_BAR == 2) || defined(PL16)
#define BLITW(cx, cy) blit_planes(cx, cy)
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
         gx == SEP_X / 8 || gxr == SEP_X / 8 ||
         (CH & 7 && ((r * CH) & 7) != key)) {
        /* (cells of 14 rows meet the 8-row wallpaper at another row each
           time: key keeps the row it starts at) */
#else
      if(1) {
#endif
        key = (r * CH) & 7;
        wcell(gx, r);
#if defined(MONO)
        memcpy(lpix[0], mb, CH);
#elif (defined(GFX_CGA4) && CGA_BAR == 2) || defined(PL16)
        planes4();
        memcpy(lpix[0], cb4, sizeof(cb4));
#else
        memcpy(lpix, pix, sizeof(pix));
#endif
        wcell(gxr, r);
#if defined(MONO)
        memcpy(rpix[0], mb, CH);
#elif (defined(GFX_CGA4) && CGA_BAR == 2) || defined(PL16)
        planes4();
        memcpy(rpix[0], cb4, sizeof(cb4));
#else
        memcpy(rpix, pix, sizeof(pix));
#endif
      }
#if defined(MONO)
      blit_bytes(gx, r, lpix[0]);
      blit_bytes(gxr, r, rpix[0]);
#elif (defined(GFX_CGA4) && CGA_BAR == 2) || defined(PL16)
      memcpy(cb4, lpix[0], sizeof(cb4));
      write4(gx, r);
      memcpy(cb4, rpix[0], sizeof(cb4));
      write4(gxr, r);
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
  const unsigned char *g;
  unsigned char r;
  for(r = 0; r < CH; ++r) {
    mb[r] = -rowbg[r];                  /* C_INK is 1: 0xff, C_PAPER 0 */
  }
  if(c == ' ') {
    return;
  }
  g = glyph(c);
  if(fg == C_INK) {
    for(r = 0; r < CH; ++r) {
      mb[r] |= g[r];
    }
  } else {
    for(r = 0; r < CH; ++r) {
      mb[r] &= ~g[r];
    }
  }
#elif defined(GFX_EGA) || defined(GFX_VGA)
  /* each plane: the background bit where the letter has none, the
     colour bit where it has */
  /* plane k: the background b, with the letter's bits turned to the
     colour: b ^ (d & m), d = b ^ colour. b and d of the whole cell are
     kept for the last row colours and colour (they change seldom along a
     line of text); a space is b, a letter goes through a loop in
     assembly, two rows at a time, four planes, with word operations */
  unsigned char eq;
  {
    const unsigned char *a = rowbg, *b = tc_bg;
    unsigned short n = CH;
    __asm__ volatile ("push %%es\n\tpush %%ds\n\tpop %%es\n\tcld\n\t"
                      "repe cmpsb\n\tmov $0,%%al\n\tjne 1f\n\tinc %%al\n"
                      "1:\tpop %%es"
                      : "=a" (eq), "+S" (a), "+D" (b), "+c" (n)
                      : : "memory", "cc");
  }
  if(!eq || fg != tc_fg) {
    unsigned char r, k;
    memcpy(tc_bg, rowbg, CH);
    tc_fg = fg;
    for(k = 0; k < 4; ++k) {
      unsigned char f = (fg >> k) & 1 ? 0xff : 0;
      for(r = 0; r < CH; ++r) {
        unsigned char b = (rowbg[r] >> k) & 1 ? 0xff : 0;
        tc_bd[k * CH + r] = b;
        tc_bd[(4 + k) * CH + r] = b ^ f;
      }
    }
  }
  if(c == ' ') {
    memcpy(cb4, tc_bd, 4 * CH);
  } else {
    const unsigned char *g = glyph(c);
    const unsigned char *bd = tc_bd;
    unsigned char *op = cb4;
    unsigned short n = CH / 2;
    __asm__ volatile ("cld\n"
                      "1:\tlodsw\n\t"
                      "mov %%ax,%%dx\n\tand %c[d0](%%bx),%%dx\n\txor (%%bx),%%dx\n\tmov %%dx,(%%di)\n\t"
                      "mov %%ax,%%dx\n\tand %c[d1](%%bx),%%dx\n\txor %c[c1](%%bx),%%dx\n\tmov %%dx,%c[c1](%%di)\n\t"
                      "mov %%ax,%%dx\n\tand %c[d2](%%bx),%%dx\n\txor %c[c2](%%bx),%%dx\n\tmov %%dx,%c[c2](%%di)\n\t"
                      "and %c[d3](%%bx),%%ax\n\txor %c[c3](%%bx),%%ax\n\tmov %%ax,%c[c3](%%di)\n\t"
                      "add $2,%%bx\n\tadd $2,%%di\n\tloop 1b"
                      : "+S" (g), "+D" (op), "+c" (n), "+b" (bd)
                      : [c1] "i" (CH), [c2] "i" (2 * CH), [c3] "i" (3 * CH),
                        [d0] "i" (4 * CH), [d1] "i" (5 * CH), [d2] "i" (6 * CH), [d3] "i" (7 * CH)
                      : "ax", "dx", "memory", "cc");
  }
#elif defined(PL16)
  unsigned char r;
  const unsigned char *g = c != ' ' ? glyph(c) : NULL;
  for(r = 0; r < CH; ++r) {
    unsigned char bg = rowbg[r];
    unsigned char q0 = (bg & 1) ? 0xff : 0, q1 = (bg & 2) ? 0xff : 0,
      q2 = (bg & 4) ? 0xff : 0, q3 = (bg & 8) ? 0xff : 0;
    if(g) {
      FILL16(g[r], fg);
    }
    P0[r] = q0; P1[r] = q1; P2[r] = q2; P3[r] = q3;
  }
#elif defined(GFX_CGA4)
  unsigned char r;
  const unsigned char *g = c != ' ' ? glyph(c) : NULL;
  for(r = 0; r < 8; ++r) {
    unsigned char bg = rowbg[r], p1 = (bg & 2) ? 0xff : 0, p0 = (bg & 1) ? 0xff : 0;
    if(g) {
      unsigned char m = g[r];
      p1 = (fg & 2) ? (p1 | m) : (p1 & ~m);
      p0 = (fg & 1) ? (p0 | m) : (p0 & ~m);
    }
    P1[r] = p1;
    P0[r] = p0;
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
#elif defined(GFX_CGA4) || defined(PL16)
#define BLITC(cx, cy) blit_planes(cx, cy)
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
/* a label's text: a character with the top bit set is shown as a cursor
   (the editor's), drawn as the cursor of a text entry */
static void
label_text(const char *s, unsigned char len)
{
  while(len && *s) {
    unsigned char k = 0;
    while(k < len && s[k] && !((unsigned char)s[k] & 0x80)) {
      ++k;
    }
    if(k) {
      text(s, k, C_INK);
      s += k;
      len -= k;
    }
    if(len && *s) {
      rows_solid(C_INK);
#if !defined(MONO)
      rows_bar(1);
#endif
#if defined(MONO)
      text_cell((unsigned char)*s & 0x7f, C_PAPER);
#else
      text_cell((unsigned char)*s & 0x7f, C_TITLE_A);
#endif
      rows_solid(C_PAPER);
      ++s;
      --len;
    }
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
/* the BIOS video mode at start, given back at the end */
static unsigned char
get_mode(void)
{
  unsigned short ax = 0x0f00, bx = 0;
  __asm__ volatile ("push %%bp\n\tint $0x10\n\tpop %%bp"
                    : "+a" (ax), "+b" (bx)
                    : : "cx", "dx", "si", "di", "memory", "cc");
  return ax & 0x7f;
}
static unsigned char start_mode = 3;

#if defined(GFX_HERC)
/* Hercules graphics: no BIOS mode, the 6845 is programmed here (values of
   the Hercules technical reference, as in the nano-X driver of ELKS) */
static void
crtc(const unsigned char *v, unsigned char n)
{
  unsigned char i;
  for(i = 0; i < n; ++i) {
    outb(0x3b4, i);
    outb(0x3b5, v[i]);
  }
}
static const unsigned char herc_gfx[10] = {
  0x35, 0x2d, 0x2e, 0x07, 0x5b, 0x02, 0x57, 0x57, 0x02, 0x03
};
static const unsigned char herc_txt[12] = {
  0x61, 0x50, 0x52, 0x0f, 0x19, 0x06, 0x19, 0x19, 0x02, 0x0d, 0x0b, 0x0c
};
static void
herc_graphics(void)
{
  unsigned short i;
  outb(0x3bf, 3);                       /* allow graphics, both pages */
  outb(0x3b8, 0x02);                    /* graphics, video off */
  crtc(herc_gfx, 10);
  for(i = 0; i < 0x8000; ++i) {
    VRAM[i] = 0;                        /* page 0, black */
  }
  outb(0x3b8, 0x0a);                    /* graphics, video on, page 0 */
}
#endif

void
ctk_cga_exit(void)
{
#if defined(GFX_HERC)
  outb(0x3b8, 0);
  outb(0x3bf, 0);
  crtc(herc_txt, 12);
  set_mode(7);
#else
  set_mode(start_mode);
#endif
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
#if defined(GFX_TANDY4)
  /* palette registers (BIOS 10h/00h): black, light cyan, yellow, white, as
     on the theme trial; yellow takes the place of CGA's magenta */
  bios10(0x1000, 0x0000);
  bios10(0x1000, 0x0b01);
  bios10(0x1000, 0x0e02);
  bios10(0x1000, 0x0f03);
#elif defined(GFX_CGA4)
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

/*---------------------------------------------------------------------------*/
/* Mouse pointer: drawn over the screen with the bytes under it kept, black
   outline, white inside (the other way round where desktop and windows are
   black), hot spot at the top left. Every drawing entry
   takes it off first; the program puts it back when the drawing is done. */
#if defined(GFX_CGA) || defined(GFX_TANDY4)
/* 640x200: pixels more than twice as tall as wide */
static const char *const ptr_shape[] = {
  "X.........",
  "XXX.......",
  "XoooXX....",
  "XoooooXX..",
  "XoooooooXX",
  "XoooXXXX..",
  "XoXXooX...",
  "XX..XooX..",
  ".....XX...",
};
#else
static const char *const ptr_shape[] = {
  "X.......",
  "XX......",
  "XoX.....",
  "XooX....",
  "XoooX...",
  "XooooX..",
  "XoooooX.",
  "XooooooX",
  "XooXXXXX",
  "XoXXoX..",
  "XX.XoX..",
  "X...XoX.",
  "....XoX.",
  ".....XX.",
};
#endif
#define PTR_H (sizeof(ptr_shape) / sizeof(ptr_shape[0]))
#define PTR_PW ((unsigned short)SW * 8)
#define PTR_PH ((unsigned short)SH * CH)
/* bytes kept for eight pixels of a row, and their step in video memory */
#if defined(MONO)
#define GB 1
#define GSTEP 1
#elif defined(GFX_CGA4)                 /* and Tandy 640x200 */
#define GB 2
#define GSTEP 2
#elif defined(GFX_TANDY)
#define GB 4
#define GSTEP 4
#elif defined(GFX_MCGA)
#define GB 8
#define GSTEP 8
#else                                   /* EGA, VGA: a byte in each plane */
#define GB 4
#define GSTEP 1
#endif
#if defined(MONO)
#define PTR_WHITE 1
#elif defined(GFX_CGA4)
#define PTR_WHITE 3
#elif defined(GFX_TANDY)
#define PTR_WHITE 15
#else
#define PTR_WHITE 1                     /* EGA, VGA, MCGA: colour 1 */
#endif

static unsigned char ptr_on;            /* drawn */
static unsigned short ptr_x, ptr_y;
static unsigned char ptr_save[PTR_H][3][GB];
static unsigned char ptr_used[PTR_H];   /* the groups of eight pixels kept, a bit each */

/* video memory offset of pixel row Y */
static unsigned short
ptr_row(unsigned short Y)
{
#if defined(GFX_TANDY) || defined(GFX_TANDY4)
  return (Y & 3) * 0x2000 + (Y >> 2) * 160;
#elif defined(GFX_CGA) || defined(GFX_CGA4)
  return (Y & 1) * 0x2000 + (Y >> 1) * 80;
#elif defined(GFX_HERC)
  return (Y & 3) * 0x2000 + (Y >> 2) * 90;
#elif defined(GFX_ATT)
  return (Y & 3) * 0x2000 + (Y >> 2) * 80;
#elif defined(GFX_MCGA)
  return Y * 320;
#else
  return Y * 80;
#endif
}
static void
ptr_get(unsigned short off, unsigned char *b)
{
  unsigned char k;
#if defined(GFX_EGA) || defined(GFX_VGA)
  for(k = 0; k < 4; ++k) {
    outw(0x3ce, ((unsigned short)k << 8) | 4);       /* read map select */
    b[k] = VRAM[off];
  }
  outw(0x3ce, 4);
#else
  for(k = 0; k < GB; ++k) {
    b[k] = VRAM[off + k];
  }
#endif
}
static void
ptr_put(unsigned short off, const unsigned char *b)
{
  unsigned char k;
#if defined(GFX_EGA) || defined(GFX_VGA)
  for(k = 0; k < 4; ++k) {
    outb(0x3c5, 1 << k);                /* map mask */
    VRAM[off] = b[k];
  }
  outb(0x3c5, 0x0f);
#else
  for(k = 0; k < GB; ++k) {
    VRAM[off + k] = b[k];
  }
#endif
}
/* eight pixels s with the pixels of a black, those of w white, into d */
static void
ptr_mask(const unsigned char *s, unsigned char a, unsigned char w,
         unsigned char *d)
{
#if defined(MONO)
  d[0] = (s[0] & ~a) | w;
#elif defined(GFX_TANDY4) || defined(GFX_EGA) || defined(GFX_VGA)
  /* bit planes: white has the bits of PTR_WHITE */
  unsigned char k;
  for(k = 0; k < GB; ++k) {
    d[k] = (s[k] & ~a) | (((PTR_WHITE >> k) & 1) ? w : 0);
  }
#else
  /* packed pixels: 2, 4 or 8 bits, the left pixel in the high bits */
  unsigned char i, m = 0x80;
  memcpy(d, s, GB);
  for(i = 0; i < 8; ++i, m >>= 1) {
    if(a & m) {
      unsigned char v = (w & m) ? PTR_WHITE : 0;
#if defined(GFX_CGA4)
      unsigned char sh = 6 - 2 * (i & 3);
      d[i >> 2] = (d[i >> 2] & ~(3 << sh)) | (v << sh);
#elif defined(GFX_TANDY)
      unsigned char sh = (i & 1) ? 0 : 4;
      d[i >> 1] = (d[i >> 1] & ~(15 << sh)) | (v << sh);
#else
      d[i] = v;
#endif
    }
  }
#endif
}

void
ctk_gfx_pointer_off(void)
{
  unsigned char r, g, gx = ptr_x >> 3;
  if(!ptr_on) {
    return;
  }
  for(r = 0; r < PTR_H; ++r) {
    unsigned short off = ptr_row(ptr_y + r) + gx * GSTEP;
    for(g = 0; g < 3; ++g) {
      if(ptr_used[r] & (1 << g)) {
        ptr_put(off + g * GSTEP, ptr_save[r][g]);
      }
    }
  }
  ptr_on = 0;
}
#define PTR_OFF() do { if(ptr_on) ctk_gfx_pointer_off(); } while(0)

/* the pointer at pixel x, y (taken off where it was) */
void
ctk_gfx_pointer(unsigned short x, unsigned short y)
{
  unsigned char r, g, c, gx = x >> 3;
  unsigned char out[GB];
  if(ptr_on && x == ptr_x && y == ptr_y) {
    return;
  }
  ctk_gfx_pointer_off();
  if(x >= PTR_PW || y >= PTR_PH) {
    return;
  }
  ptr_x = x;
  ptr_y = y;
  for(r = 0; r < PTR_H; ++r) {
    const char *s = ptr_shape[r];
    unsigned short a = 0, w = 0, off;
    unsigned char sh = x & 7, am[3], wm[3];
    ptr_used[r] = 0;
    if(y + r >= PTR_PH) {
      continue;
    }
    /* the row as 16 bits, its first pixel in the top bit, then spread over
       the three bytes of eight pixels it can touch (16-bit arithmetic: the
       long shifts went wrong with ia16-gcc) */
    for(c = 0; s[c]; ++c) {
      if(s[c] != '.') {
        a |= 0x8000U >> c;
        if(s[c] == 'o') {
          w |= 0x8000U >> c;
        }
      }
    }
    am[0] = (a >> 8) >> sh;
    am[1] = a >> sh;
    am[2] = (a << (8 - sh)) & 0xff;
    wm[0] = (w >> 8) >> sh;
    wm[1] = w >> sh;
    wm[2] = (w << (8 - sh)) & 0xff;
#if defined(GFX_INVERT) || defined(GFX_CGA4)
    /* black desktop and windows: a white outline round a black inside */
    wm[0] = am[0] & ~wm[0];
    wm[1] = am[1] & ~wm[1];
    wm[2] = am[2] & ~wm[2];
#endif
    off = ptr_row(y + r) + gx * GSTEP;
    for(g = 0; g < 3 && gx + g < SW; ++g) {
      unsigned char a8 = am[g], w8 = wm[g];
      if(a8) {
        ptr_get(off + g * GSTEP, ptr_save[r][g]);
        ptr_mask(ptr_save[r][g], a8, w8, out);
        ptr_put(off + g * GSTEP, out);
        ptr_used[r] |= 1 << g;
      }
    }
  }
  ptr_on = 1;
}

/*---------------------------------------------------------------------------*/
/* Cell rows y1 + n .. y2 - 1, columns x1 .. x2 - 1, moved up n rows in video
   memory (screen cells); what was in the last n rows stays, for the caller
   to draw over. For a window in front with nothing over it: the shell
   window scrolls this way instead of drawing all its lines again. */
static void
vmove(unsigned short dst, unsigned short src, unsigned short n)
{
  __asm__ volatile ("push %%ds\n\tpush %%es\n\tmov %%ax,%%ds\n\tmov %%ax,%%es\n\t"
                    "cld\n\trep movsb\n\tpop %%es\n\tpop %%ds"
                    : "+S" (src), "+D" (dst), "+c" (n)
                    : "a" ((unsigned short)((unsigned long)VRAM >> 16))
                    : "memory", "cc");
}
void
ctk_gfx_scroll(unsigned char x1, unsigned char x2, unsigned char y1,
               unsigned char y2, unsigned char n)
{
  unsigned short Y, end = (unsigned short)(y2 - n) * CH, d = (unsigned short)n * CH;
  unsigned short len = (unsigned short)(x2 - x1) * GSTEP, off = (unsigned short)x1 * GSTEP;
  PTR_OFF();
#if defined(GFX_EGA) || defined(GFX_VGA)
  outw(0x3ce, 0x0105);                  /* write mode 1: the four planes through the latches */
#endif
  for(Y = (unsigned short)y1 * CH; Y < end; ++Y) {
    vmove(ptr_row(Y) + off, ptr_row(Y + d) + off, len);
  }
#if defined(GFX_EGA) || defined(GFX_VGA)
  outw(0x3ce, 0x0005);                  /* write mode 0 */
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
  start_mode = get_mode();
#if defined(GFX_HERC)
  herc_graphics();
#elif defined(GFX_ATT)
  set_mode(0x40);                       /* AT&T 6300, Olivetti M24 */
  if(get_mode() != 0x40) {
    /* a BIOS without mode 40h (an Olivetti card in another PC): the CGA
       mode, with the 400-line bit of the mode register at 3DEh */
    set_mode(6);
    outb(0x3de, 1);
  }
#elif defined(GFX_EGA2)
  /* 640x350: mode 10h, or 0Fh on a monochrome monitor (EGA BIOS 12h/10h
     gives BH=1); writes go to all planes, so 0 and 1 bits give black and
     white */
  {
    unsigned short ax = 0x1200, bx = 0x0010;
    __asm__ volatile ("push %%bp\n\tint $0x10\n\tpop %%bp"
                      : "+a" (ax), "+b" (bx)
                      : : "cx", "dx", "si", "di", "memory", "cc");
    set_mode((bx >> 8) == 1 ? 0x0f : 0x10);
  }
#elif defined(GFX_VGA2)
  set_mode(0x11);
#else
  set_mode(6);
#endif
#elif defined(GFX_TANDY4)
  set_mode(0x0a);
#elif defined(GFX_CGA4)
  set_mode(4);
#elif defined(GFX_EGA)
  set_mode(0x10);
#elif defined(GFX_VGA)
  set_mode(0x12);
#elif defined(GFX_TANDY)
  set_mode(9);                          /* standard palette, nothing to set */
#else
  set_mode(0x13);
#endif
#if defined(GFX_EGA) || defined(GFX_VGA)
  load_bios_font();
  outb(0x3ce, 5); outb(0x3cf, 0);       /* write mode 0 */
  outb(0x3ce, 3); outb(0x3cf, 0);       /* replace, no rotation */
  outb(0x3ce, 1); outb(0x3cf, 0);       /* no set/reset */
  outb(0x3ce, 8); outb(0x3cf, 0xff);    /* all bits */
  outb(0x3c4, 2); outb(0x3c5, 0x0f);    /* map mask: all planes; the index
                                           stays there for write4() */
  desk_patterns();
#endif
#if defined(GFX_MCGA)
  mdesk_init();
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
    unsigned char m = left ? (unsigned char)(0xff << (8 - k)) : (0xff >> (8 - k));
#if defined(GFX_INVERT)
    mb[top ? r : CH - 1 - r] &= ~m;     /* black corners: paper */
#else
    mb[top ? r : CH - 1 - r] |= m;
#endif
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
#elif defined(GFX_EGA) || defined(GFX_VGA)
  {
    /* every cell of a desktop row is alike but the one with the line: the
       planes of cell 0, each pixel row filled across the screen at once */
    unsigned char sepc = SEP_X / 8, r, k, q[4], n[4];
    unsigned short base = (unsigned short)line * (CH * 80);
    (void)i;
    for(r = 0; r < CH; r += k) {
      /* this row and the next ones alike: one fill */
      desk16(0, line * CH + r, q);
      for(k = 1; r + k < CH; ++k) {
        desk16(0, line * CH + r + k, n);
        if(n[0] != q[0] || n[1] != q[1] || n[2] != q[2] || n[3] != q[3]) {
          break;
        }
      }
      vrow(base + r * 80, q, 80 * k);
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

#if defined(GFX_EGA) || defined(GFX_VGA)
/* n bytes copied in video memory from src to dst through the latches
   (write mode 1), all four planes at once; dst after src repeats the
   bytes between them */
static void
vcopy(unsigned short dst, unsigned short src, unsigned short n)
{
  outw(0x3ce, 0x0105);                  /* write mode 1 */
  __asm__ volatile ("push %%ds\n\tpush %%es\n\tmov %%ax,%%ds\n\tmov %%ax,%%es\n\t"
                    "cld\n\trep movsb\n\tpop %%es\n\tpop %%ds"
                    : "+S" (src), "+D" (dst), "+c" (n)
                    : "a" (0xa000)
                    : "memory", "cc");
  outw(0x3ce, 0x0005);                  /* write mode 0 */
}
/* the desktop from pixel row y to y2, the icon column line aside: runs of
   alike rows in one fill, runs of two alternating rows (the checker) as
   two rows repeated through the latches */
static void
desk_fill(unsigned short y, unsigned short y2)
{
  /* white down to band 2, the checker of bands 2 and 3, grey below: each
     as one fill, the checker as its first two rows repeated */
  static const unsigned short ends[3] = {DESK_B(2), DESK_B(4), 0xffff};
  unsigned char i, q[4];
  for(i = 0; i < 3 && y < y2; ++i) {
    unsigned short e = ends[i] < y2 ? ends[i] : y2;
    if(y >= e) {
      continue;
    }
    desk16(0, y, q);
    if(i != 1 || e - y < 2) {
      vrow(y * 80, q, (e - y) * 80);      /* (or a single row of the checker) */
    } else {
      vrow(y * 80, q, 80);
      desk16(0, y + 1, q);
      vrow((y + 1) * 80, q, 80);
      if(e - y > 2) {
        vcopy((y + 2) * 80, y * 80, (e - y - 2) * 80);
      }
    }
    y = e;
  }
}
#endif
void
ctk_draw_clear(unsigned char y1, unsigned char y2)
{
  unsigned char i;
  PTR_OFF();
#if defined(MONO)
  /* each pixel row of the desktop is one byte across the screen (the
     wallpaper, or the line over the status bar): a fill a row, where the
     card keeps it, then the cells of the icon column line, then the
     status bar */
  unsigned char ye = y2 < SH - 1 ? y2 : SH - 1;
  if(y1 < ye) {
    unsigned short y, y9 = (unsigned short)ye * CH;
    for(y = (unsigned short)y1 * CH; y < y9; ++y) {
      unsigned char v = y == (SH - 1) * CH - 1 ? 0xff : wall[y & 7];
      unsigned short n = SW;
#if defined(GFX_CGA)
      unsigned short off = (y & 1) * 0x2000 + (y >> 1) * 80, seg = 0xb800;
      v = ~v;
#elif defined(GFX_HERC)
      unsigned short off = (y & 3) * 0x2000 + (y >> 2) * 90, seg = 0xb000;
#elif defined(GFX_ATT)
      unsigned short off = (y & 3) * 0x2000 + (y >> 2) * 80, seg = 0xb800;
      v = ~v;
#else
      unsigned short off = y * 80, seg = 0xa000;
      v = ~v;
#endif
      __asm__ volatile ("push %%es\n\tmov %%dx,%%es\n\tcld\n\trep stosb\n\tpop %%es"
                        : "+D" (off), "+c" (n)
                        : "a" ((unsigned short)v), "d" (seg)
                        : "memory", "cc");
    }
    for(i = y1; i < ye; ++i) {
      desk_cell(SEP_X / 8, i);
    }
  }
  if(y2 == SH) {
    status_line();
  }
  return;
#endif
#if defined(GFX_CGA4) || defined(GFX_TANDY)
  /* the desktop is one colour, a word repeated across a pixel row (black
     on CGA, grey on Tandy, the line over the status bar in its colour): a
     fill a row in the card's banks, then the cells of the icon column
     line, then the status bar */
  unsigned char ye = y2 < SH - 1 ? y2 : SH - 1;
  if(y1 < ye) {
    unsigned short y, y9 = (unsigned short)ye * CH;
    for(y = (unsigned short)y1 * CH; y < y9; ++y) {
      unsigned char line = y == (SH - 1) * CH - 1;
#if defined(GFX_TANDY4)
      /* 640x200, four colours: the low then the high plane byte */
      unsigned short off = (y & 3) * 0x2000 + (y >> 2) * 160, n = 80;
      unsigned short v = line ? 0x00ff : 0;
#elif defined(GFX_TANDY)
      /* 320x200, 16 colours: two pixels a byte */
      unsigned short off = (y & 3) * 0x2000 + (y >> 2) * 160, n = 80;
      unsigned short v = line ? C_ETCH0 * 0x1111 : 0x7777;
#else
      /* CGA 320x200: four pixels a byte, colour 1 = 01 */
      unsigned short off = (y & 1) * 0x2000 + (y >> 1) * 80, n = 40;
      unsigned short v = line ? 0x5555 : 0;
#endif
      __asm__ volatile ("push %%es\n\tmov %%dx,%%es\n\tcld\n\trep stosw\n\tpop %%es"
                        : "+D" (off), "+c" (n)
                        : "a" (v), "d" (0xb800)
                        : "memory", "cc");
    }
    for(i = y1; i < ye; ++i) {
      desk_cell(SEP_X / 8, i);
    }
  }
  if(y2 == SH) {
    status_line();
  }
  return;
#endif
#if defined(GFX_MCGA)
  /* each pixel row of the desktop is one colour: a fill across the screen,
     then the cells of the icon column line, then the status bar */
  unsigned char ye = y2 < SH - 1 ? y2 : SH - 1;
  if(y1 < ye) {
    unsigned short y, y9 = (unsigned short)ye * CH, k;
    for(y = (unsigned short)y1 * CH; y < y9; y += k) {
      unsigned short off = y * 320, n;
      unsigned char v = desk(0, y);
      for(k = 1; y + k < y9 && desk(0, y + k) == v; ++k) ;
      n = k * 320;
      __asm__ volatile ("push %%es\n\tmov %%dx,%%es\n\tcld\n\trep stosb\n\tpop %%es"
                        : "+D" (off), "+c" (n)
                        : "a" ((unsigned short)v), "d" (0xa000)
                        : "memory", "cc");
    }
    for(i = y1; i < ye; ++i) {
      desk_cell(SEP_X / 8, i);
    }
  }
  if(y2 == SH) {
    status_line();
  }
  return;
#endif
#if defined(GFX_EGA) || defined(GFX_VGA)
  /* the desktop rows at once, then the cells of the icon column line, then
     the status bar */
  unsigned char ye = y2 < SH - 1 ? y2 : SH - 1;
  if(y1 < ye) {
    desk_fill((unsigned short)y1 * CH, (unsigned short)ye * CH);
    for(i = y1; i < ye; ++i) {
      desk_cell(SEP_X / 8, i);
    }
  }
  if(y2 == SH) {
    status_line();
  }
  return;
#endif
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
#if defined(MONO) || defined(GFX_CGA4)
/* buttons as on the approved trial: a box with cut corners round the small
   font (rows 1-6), and a second line on each side when it has the focus */
static const unsigned char cap_box[2][8] = {
  {0x07,0x08,0x08,0x08,0x08,0x08,0x08,0x07}, {0xe0,0x10,0x10,0x10,0x10,0x10,0x10,0xe0}};
#endif
#if defined(MONO)
static const unsigned char cap_ef[2][8] = {
  {0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xff}, {0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0xff}};
static const unsigned char cap_boxf[2][8] = {
  {0x07,0x28,0x28,0x28,0x28,0x28,0x28,0x07}, {0xe0,0x14,0x14,0x14,0x14,0x14,0x14,0xe0}};
#else
static void
cap8(unsigned char cx, unsigned char cy, unsigned char right,
     const unsigned char (*pat)[8], unsigned char col, unsigned char col2)
{
  unsigned char r;
#if defined(GFX_CGA4)
  for(r = 0; r < 8; ++r) {
    unsigned char m = pat[right][r], k = r < 4 ? col : col2;
    P1[r] = (k & 2) ? m : 0;            /* paper is colour 0 */
    P0[r] = (k & 1) ? m : 0;
  }
  blit_planes(cx, cy);
#elif defined(GFX_TANDY)
  for(r = 0; r < 8; ++r) {
    unsigned char q0 = 0, q1 = 0, q2 = 0, q3 = 0;
    unsigned char m = pat[right][r];
    unsigned char k = r < 4 ? col : col2;
    FILL16(0xff, C_PAPER);
    FILL16(m, k);
    P0[r] = q0; P1[r] = q1; P2[r] = q2; P3[r] = q3;
  }
  blit_planes(cx, cy);
#else
  unsigned char c;
  for(r = 0; r < 8; ++r) {
    for(c = 0; c < 8; ++c) {
      pix[r][c] = (pat[right][r] & (0x80 >> c)) ? (r < 4 ? col : col2) : C_PAPER;
    }
  }
  blit(cx, cy);
#endif
}
#endif
#endif

static void
cap(unsigned char cx, unsigned char cy, unsigned char right,
    unsigned char fill_t, unsigned char fill_b, unsigned char edge,
    unsigned char ring)
{
  unsigned char r, c, f = right ? 3 : 4;
#if defined(GFX_CGA4) && CGA4_BTN
  if(ring != 2) {
    /* the box in the frame colour, filled when focused */
    for(r = 0; r < 8; ++r) {
      unsigned char line = cap_box[right][r];
      unsigned char in = (r == 0 || r == 7) ? 0 : (right ? 0xe0 : 0x07);
      unsigned char fl = fill_t != C_PAPER ? in : 0;
      P1[r] = (edge & 2 ? line : 0) | (fill_t & 2 ? fl : 0);
      P0[r] = (edge & 1 ? line : 0) | (fill_t & 1 ? fl : 0);
    }
    (void)c; (void)f; (void)fill_b;
    blit_planes(cx, cy);
    return;
  }
#endif
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
#if defined(MONO) && CH == 8
  /* brackets, a pill when focused; entries: side bars and a line under */
  {
    const unsigned char (*pat)[8] = ring == 5 ? cap_boxf : ring == 4 ? cap_box :
      ring == 3 ? cap_ef : ring == 2 ? cap_e : cap_b;
    (void)r; (void)c; (void)f; (void)fill_b; (void)edge;
    blit_bytes(cx, cy, pat[right]);
    return;
  }
#elif defined(MONO)
  /* the same shapes on tall cells: the box with cut corners (a second line
     on each side when focused), the entry's side bars and line under */
  {
    unsigned char top, mid, bot;
    if(ring >= 4) {
      top = bot = right ? 0xe0 : 0x07;
      mid = ring == 5 ? (right ? 0x14 : 0x28) : (right ? 0x10 : 0x08);
    } else if(ring >= 2) {
      mid = top = ring == 3 ? (right ? 0x0f : 0xf0) : (right ? 0x03 : 0xc0);
      bot = 0xff;
    } else {
      top = bot = right ? 0x70 : 0x0e;
      mid = right ? 0x0c : 0x30;
    }
    for(r = 0; r < CH; ++r) {
      mb[r] = r == 0 ? top : r == CH - 1 ? bot : mid;
    }
    (void)c; (void)f; (void)fill_b; (void)edge;
    blit_bytes(cx, cy, mb);
    return;
  }
#endif
#if defined(GFX_EGA) || defined(GFX_VGA)
  /* in planes, a row at a time: paper, the fill inside, the frame column,
     the lines at top and bottom with the corners cut, the focus ring */
  {
    unsigned char in = right ? 0xe0 : 0x07, fb = 0x80 >> f;
    unsigned char hl = right ? 0xf0 : 0x0f, rb = 0x80 >> (right ? f + 2 : f - 2);
    (void)c;
    for(r = 0; r < CH; ++r) {
      unsigned char q0 = 0, q1 = 0, q2 = 0, q3 = 0;
      FILL16(0xff, C_PAPER);
      FILL16(in, r < CH / 2 ? fill_t : fill_b);
      FILL16(fb, edge);
      if(r == 0 || r == CH - 1) {
        FILL16(hl, edge);
        FILL16(fb, C_PAPER);
      } else if(ring && r >= 2 && r <= CH - 3) {
        FILL16(rb, C_RING);
      }
      P0[r] = q0; P1[r] = q1; P2[r] = q2; P3[r] = q3;
    }
    write4(cx, cy);
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
#if CH > 8 && !defined(MONO)
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
#elif defined(GFX_CGA4) && CGA4_BTN
  rows_solid(kf ? C_BAR_AT : C_PAPER);
  rowbg[0] = rowbg[CH - 1] = C_EDGE_A;  /* the box above and below */
#elif CH == 8
  if(!kf) {
    rows_solid(C_PAPER);
  }
#else
  rowbg[0] = rowbg[CH - 1] = e;
#endif
#if defined(GFX_CGA4) && CGA4_BTN == 2
  barfont = 2;                          /* the small font, as on two colours */
#else
  barfont = 1;
#endif
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
#if defined(GFX_CGA4) || defined(PL16)
#define GFX_PALETTE_ONLY                /* only GFX_ICON_PACKED4, GFX_ICON_PLANES16 */
#include "gfx-icons.h"
#undef GFX_PALETTE_ONLY
#endif
#if defined(GFX_ICON_SMALL)
/* titles of the Contiki programs that do not fit in five letters */
static const char * const short_titles[][2] = {
  {"Calculator", "Calc"}, {"Directory", "Dir"}, {"Editor", "Edit"},
  {"E-mail", "Mail"}, {"Network setup", "Net"}, {"Command shell", "Shell"},
  {"Processes", "Procs"}, {"Telnet", "Term"}, {"Telnet server", "TelSv"},
  {"Web server", "WebSv"}, {"Web browser", "WWW"}
};
#endif
static void
icon(struct ctk_widget *w, unsigned char xpos, unsigned char ypos,
     unsigned char clipy1, unsigned char clipy2, unsigned char sel)
{
  const struct gfx_icon *ic = (const struct gfx_icon *)w->widget.icon.bitmap;
  const char *title;
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
        blit_bytes(xpos + i, cy, ic->px + ((unsigned short)j * GFX_ICON_CW + i) * CH);
        continue;
#elif (defined(GFX_CGA4) || defined(GFX_TANDY)) && defined(GFX_ICON_PACKED4)
        /* cells packed as in video memory, over the desktop colour */
        (void)d0; (void)Y0;
        memcpy(cb4, ic->px + ((unsigned short)j * GFX_ICON_CW + i) * sizeof(cb4), sizeof(cb4));
        write4(xpos + i, cy);
        continue;
#elif defined(GFX_ICON_PLANES16)
        /* cells as planes with the mask of the drawing, over the desktop */
        {
          /* each row: the desktop's plane bytes (never the line, out of
             the way of the icons) where the mask has no bits, the icon's
             where it has */
          const unsigned char *s = ic->px + ((unsigned short)j * GFX_ICON_CW + i) * (5 * CH);
          const unsigned char *q = deskrow[Y0];
          unsigned char *o = cb4;
          unsigned short n = CH;
          (void)d0;
          __asm__ volatile ("cld\n"
                            "1:\tlodsb\n\tnot %%al\n\tmov %%al,%%ah\n\t"
                            "mov (%%bx),%%al\n\tand %%ah,%%al\n\tor (%%si),%%al\n\tmov %%al,(%%di)\n\t"
                            "mov 1(%%bx),%%al\n\tand %%ah,%%al\n\tor 1(%%si),%%al\n\tmov %%al,%c[c1](%%di)\n\t"
                            "mov 2(%%bx),%%al\n\tand %%ah,%%al\n\tor 2(%%si),%%al\n\tmov %%al,%c[c2](%%di)\n\t"
                            "mov 3(%%bx),%%al\n\tand %%ah,%%al\n\tor 3(%%si),%%al\n\tmov %%al,%c[c3](%%di)\n\t"
                            "add $4,%%si\n\tadd $4,%%bx\n\tinc %%di\n\tloop 1b"
                            : "+S" (s), "+D" (o), "+c" (n), "+b" (q)
                            : [c1] "i" (CH), [c2] "i" (2 * CH), [c3] "i" (3 * CH)
                            : "ax", "memory", "cc");
          write4(xpos + i, cy);
        }
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
#if defined(GFX_MCGA)
          {
            /* the desktop behind: one colour a row (the icons stay clear
               of the icon column line) */
            unsigned char dv = desk(X0, Y), *o = pix[r];
            for(c = 0; c < 8; ++c) {
              short d = d0 + c;
              unsigned char v = 255;
              if(row && d >= 0 && d < (short)ic->w) {
                v = row[d];
              }
              o[c] = v == 255 ? dv : v;
            }
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
  title = w->widget.icon.title;
  len = strlen(title);
#if defined(GFX_ICON_SMALL)
  /* five letters at most under the small icons: known long titles have a
     short form */
  for(i = 0; i < sizeof(short_titles) / sizeof(short_titles[0]); ++i) {
    if(strcmp(title, short_titles[i][0]) == 0) {
      title = short_titles[i][1];
      len = strlen(title);
      break;
    }
  }
#endif
  if(len > GFX_ICON_CW) {
    for(i = 0; i < len && title[i] != ' '; ++i) ;
    len = i;
#if defined(GFX_ICON_SMALL)
    if(len > GFX_ICON_CW) {
      len = GFX_ICON_CW;
    }
#endif
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
      comp_text(title[i], sel ? C_PAPER : C_INK);
      barfont = 0;
      if(sel && (i == 0 || i == len - 1)) {
        unsigned char bit = i == 0 ? 0x80 : 0x01;
        mb[0] &= ~bit;
        mb[CH - 1] &= ~bit;
      }
      blit_bytes(x + i, ty, mb);
    }
    (void)r; (void)c;
#elif defined(GFX_ICON_SMALL)
    {
      /* centred to the pixel under the icon: a title of an even number of
         letters starts half a cell in, so each cell takes the halves of
         two letters */
      unsigned short lx = xpos * 8 + (GFX_ICON_CW - len) * 4;
      unsigned short lx1 = lx + len * 8, Y0 = ty * CH;
      unsigned char cx;
      (void)x;
      for(cx = lx / 8; cx <= (lx1 - 1) / 8; ++cx) {
        unsigned short X0 = cx * 8;
#if defined(GFX_MCGA)
        /* one desktop colour a row away from the icon column line */
        unsigned char dv[CH];
        for(r = 0; r < CH; ++r) {
          dv[r] = desk(X0, Y0 + r);
        }
#endif
        for(c = 0; c < 8; ++c) {
          unsigned short X = X0 + c;
          unsigned char in = X >= lx && X < lx1;
          const unsigned char *g = in ? glyph(title[(X - lx) / 8]) : NULL;
          unsigned char bit = 0x80 >> ((X - lx) & 7);
          for(r = 0; r < CH; ++r) {
#if defined(GFX_MCGA)
            unsigned char v = cx == SEP_X / 8 ? desk(X, Y0 + r) : dv[r];
#else
            unsigned char v = desk(X, Y0 + r);
#endif
            if(in) {
              if(sel && !((r == 0 || r == CH - 1) && (X == lx || X == lx1 - 1))) {
#if defined(GFX_MCGA)
                v = C_BAR_A + r * 8 / CH;
#else
                v = r < CH / 2 ? C_BAR_AT : C_BAR_AB;
#endif
              }
              if(g[r] & bit) {
                v = sel ? C_TITLE_A : C_INK;
              }
            }
            pix[r][c] = v;
          }
        }
        blit(cx, ty);
      }
    }
#elif defined(GFX_EGA) || defined(GFX_VGA)
    /* in planes: the desktop rows (the line left out), the bar colours of
       the pill when selected with its ends rounded, the letter */
    for(i = 0; i < len; ++i) {
      unsigned short Y0 = ty * CH;
      const unsigned char *g = glyph(title[i]);
      unsigned char fg = sel ? C_TITLE_A : C_INK;
      for(r = 0; r < CH; ++r) {
        const unsigned char *d = deskrow[Y0 + r];
        unsigned char q0 = d[0], q1 = d[1], q2 = d[2], q3 = d[3];
        if(sel) {
          unsigned char m = 0xff;
          if((r == 0 || r == CH - 1) && (i == 0 || i == len - 1)) {
            m = i == 0 ? 0x7f : 0xfe;
          }
          FILL16(m, r < CH / 2 ? C_BAR_AT : C_BAR_AB);
        }
        FILL16(g[r], fg);
        P0[r] = q0; P1[r] = q1; P2[r] = q2; P3[r] = q3;
      }
      write4(x + i, ty);
    }
    (void)c;
#else
    for(i = 0; i < len; ++i) {
      unsigned short X0 = (x + i) * 8, Y0 = ty * CH;
#if defined(GFX_MCGA)
      /* one desktop colour a row (the line is left out here) */
      for(r = 0; r < CH; ++r) {
        memset(pix[r], sel ? C_INK : desk(X0, Y0 + r), 8);
      }
#else
      for(r = 0; r < CH; ++r) {
        for(c = 0; c < 8; ++c) {
          pix[r][c] = sel ? C_INK : desk(X0 + c, Y0 + r);
        }
      }
#endif
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
      pix_glyph(title[i], sel ? C_TITLE_A : C_INK);
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
        label_text(s, w->w);
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
      /* both live in the title row: draw it again with the focus mark;
         right after the borders, only if there is a mark */
      struct ctk_window *win = w->window;
      if(gfresh && !kf) {
        break;
      }
      set_geom(win->x, win->y + 1, win->w, win->h,
               (focus & (CTK_FOCUS_WINDOW | CTK_FOCUS_DIALOG)) != 0,
               win->title, win->titlelen);
      gkf_title = kf && w == (struct ctk_widget *)&win->titlebutton;
      gkf_close = kf && w == (struct ctk_widget *)&win->closebutton;
      {
        /* only the cells the focus mark can reach: the close box or the
           title with its plate */
        unsigned short a, b;
        if(w == (struct ctk_widget *)&win->closebutton) {
          a = gbox0 - 1;
          b = gbox1 + 1;
        } else {
          a = gtx0 > 5 ? gtx0 - 5 : 0;
          b = gtx0 + gn * ADV + 3;
        }
        a /= 8;
        b /= 8;
        title_row(a > gx + 1 ? a : gx + 1, b < gxr - 1 ? b : gxr - 1);
      }
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
#if CH == 8 || defined(MONO)
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
  PTR_OFF();
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
  PTR_OFF();
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
  PTR_OFF();

  ++clipy2;
  if(clipy2 <= y) {
    return;
  }
  if(draw_borders) {
    draw_windowborders(x, y, window->w, window->h, clipy1, clipy2, focus,
                       window->title, window->titlelen);
  }
  gfresh = draw_borders;
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
  gfresh = 0;
}
/*---------------------------------------------------------------------------*/
void
ctk_draw_dialog(struct ctk_window *dialog)
{
  struct ctk_widget *w;
  unsigned char x = dialog->x, y = dialog->y + 1, i, wf;
  unsigned char x1 = x + 1, y1 = y + 1, y2 = y1 + dialog->h;
  PTR_OFF();

  draw_windowborders(x, y, dialog->w, dialog->h, 0, SH,
                     CTK_FOCUS_DIALOG, dialog->title, dialog->titlelen);
  rows_solid(C_PAPER);
  for(i = y1; i < y2; ++i) {
    gotoxy(x1, i);
    blank(dialog->w);
  }
  gfresh = 1;
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
  gfresh = 0;
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

  PTR_OFF();
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
    if(cursx < SW - dl) {
      blank(SW - dl - cursx);
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
