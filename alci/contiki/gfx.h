/* Alci desktop drawing layer: screen types and icons (task D7). */
#ifndef GFX_H
#define GFX_H

/* One build per screen type, chosen with -DGFX_<type> (Makefile). */
#if defined(GFX_T1200)          /* Toshiba T1200 LCD: CGA 640x200, 2 colours, mode 6 */
/* The CGA mode on a panel whose pixels are 0.68 as wide as tall (0.42 on
   a CRT): the CGA build with the T1200 icons (56x32), a pointer and a
   shadow for that shape, and the bits the other way round, since a pixel
   set is dark on the LCD and light on a CRT */
#define GFX_CGA 1
#define GFX_LCD 1
#define GFX_SW 80
#define GFX_SH 25
#define GFX_CH 8
#define GFX_ICON_CW 7
#define GFX_ICON_RH 4
#define GFX_LABEL "T1200 LCD 640x200"
#elif defined(GFX_CGA)          /* CGA 640x200, 2 colours, BIOS mode 6 */
#define GFX_SW 80
#define GFX_SH 25
#define GFX_CH 8
#define GFX_ICON_CW 10          /* icon column: cells wide, rows high */
#define GFX_ICON_RH 4
#define GFX_LABEL "CGA 640x200"
#elif defined(GFX_TANDY4)       /* Tandy 1000 SL/TL/RL, PCjr: 640x200, 4 colours, mode 0Ah */
#define GFX_SW 80
#define GFX_SH 25
#define GFX_CH 8
#define GFX_ICON_CW 10
#define GFX_ICON_RH 4
#define GFX_LABEL "Tandy 640x200"
#define GFX_CGA4 1                      /* drawn as CGA four colours */
#elif defined(GFX_CGA4)         /* CGA 320x200, 4 colours, BIOS mode 4 */
#define GFX_SW 40
#define GFX_SH 25
#define GFX_CH 8
#define GFX_ICON_CW 5
#define GFX_ICON_RH 3
#define GFX_ICON_SMALL 1        /* 32x24 icons, short titles */
#define GFX_LABEL "CGA 320x200"
#elif defined(GFX_EGA)          /* EGA 640x350, 16 of 64 colours, mode 10h */
#define GFX_SW 80
#define GFX_SH 25
#define GFX_CH 14
#define GFX_ICON_CW 9
#define GFX_ICON_RH 3
#define GFX_LABEL "EGA 640x350"
#elif defined(GFX_VGA)          /* VGA 640x480, 16 colours, mode 12h */
#define GFX_SW 80
#define GFX_SH 30
#define GFX_CH 16
#define GFX_ICON_CW 13
#define GFX_ICON_RH 5
#define GFX_LABEL "VGA 640x480"
#elif defined(GFX_MCGA)         /* MCGA/VGA 320x200, 256 colours, mode 13h */
#define GFX_SW 40
#define GFX_SH 25
#define GFX_CH 8
#define GFX_ICON_CW 5
#define GFX_ICON_RH 3
#define GFX_ICON_SMALL 1        /* 32x24 icons, short titles */
#define GFX_LABEL "MCGA 320x200"
#elif defined(GFX_TANDY)        /* Tandy 1000, PCjr: 320x200, 16 colours, mode 9 */
#define GFX_SW 40
#define GFX_SH 25
#define GFX_CH 8
#define GFX_ICON_CW 5
#define GFX_ICON_RH 3
#define GFX_ICON_SMALL 1        /* 32x24 icons, short titles */
#define GFX_LABEL "Tandy 320x200"
/* Two-colour screens with tall cells, drawn like CGA two colours */
#elif defined(GFX_HERC)         /* Hercules 720x348, programmed directly */
#define GFX_SW 90
#define GFX_SH 24
#define GFX_CH 14
#define GFX_ICON_CW 10
#define GFX_ICON_RH 3
#define GFX_INVERT 1            /* light lines on black, as on the trial */
#define GFX_LABEL "Hercules 720x348"
#elif defined(GFX_ATT)          /* AT&T 6300 / Olivetti M24 640x400, mode 40h */
#define GFX_SW 80
#define GFX_SH 25
#define GFX_CH 16
#define GFX_ICON_CW 12
#define GFX_ICON_RH 4
#define GFX_LABEL "640x400"
#elif defined(GFX_EGA2)         /* EGA 640x350, 2 colours, mode 10h (0Fh mono) */
#define GFX_SW 80
#define GFX_SH 25
#define GFX_CH 14
#define GFX_ICON_CW 10
#define GFX_ICON_RH 3
#define GFX_LABEL "EGA 640x350 2"
#elif defined(GFX_VGA2)         /* VGA 640x480, 2 colours, mode 11h */
#define GFX_SW 80
#define GFX_SH 30
#define GFX_CH 16
#define GFX_ICON_CW 12
#define GFX_ICON_RH 4
#define GFX_LABEL "VGA 640x480 2"
#else
#error "define one of GFX_CGA, GFX_T1200, GFX_CGA4, GFX_EGA, GFX_VGA, GFX_MCGA, GFX_TANDY, GFX_TANDY4, GFX_HERC, GFX_ATT, GFX_EGA2, GFX_VGA2"
#endif
#if defined(GFX_CGA) || defined(GFX_HERC) || defined(GFX_ATT) || \
    defined(GFX_EGA2) || defined(GFX_VGA2)
#define GFX_MONO 1              /* two colours, cells kept as bytes */
#endif

/* Desktop icon column, used by ctk.c (patched by "make fetch") */
#if defined(GFX_ICON_SMALL)
/* 320x200: a column of five cells, the icon filling it, its title right
   under it and the next icon right above (the desktop draws its icons one
   cell right and two down from these places) */
#define CTK_ICONX_FROM_RIGHT (GFX_ICON_CW + 1)
#define CTK_ICONY_STEP       (GFX_ICON_RH + 1)
#define CTK_ICONY_FROM_BOTTOM (GFX_ICON_RH + 6)
#define GFX_SEP_GAP 0           /* the line in the cell left of the icons */
#else
#define CTK_ICONX_FROM_RIGHT (GFX_ICON_CW + 2)
/* a free row between an icon's title and the next icon, where the four
   icons (About, Calculator, Editor, Shell) still fit in one column */
#if GFX_SH >= 4 * GFX_ICON_RH + 11
#define CTK_ICONY_STEP       (GFX_ICON_RH + 2)
#else
#define CTK_ICONY_STEP       (GFX_ICON_RH + 1)
#endif
#define CTK_ICONY_FROM_BOTTOM (GFX_ICON_RH + 5)
#define GFX_SEP_GAP 3
#endif
/* five icons: one column where they fit from the bottom place up to the
   top row, else two (the second CTK_ICONX_FROM_RIGHT + 4 cells further
   left, as ctk.c places it), the line left of both */
#if GFX_SH - CTK_ICONY_FROM_BOTTOM >= 4 * CTK_ICONY_STEP
#define GFX_ICON_COLS 1
#else
#define GFX_ICON_COLS 2
#endif

struct gfx_icon {
  unsigned short w, h;          /* pixels */
  const unsigned char *px;      /* rows, one palette index per pixel, 255 = desktop shows through */
};

#endif
