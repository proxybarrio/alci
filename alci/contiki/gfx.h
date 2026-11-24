/* Alci desktop drawing layer: screen types and icons (task D7). */
#ifndef GFX_H
#define GFX_H

/* One build per screen type, chosen with -DGFX_<type> (Makefile). */
#if defined(GFX_CGA)            /* CGA 640x200, 2 colours, BIOS mode 6 */
#define GFX_SW 80
#define GFX_SH 25
#define GFX_CH 8
#define GFX_ICON_CW 10          /* icon column: cells wide, rows high */
#define GFX_ICON_RH 4
#define GFX_LABEL "CGA 640x200"
#elif defined(GFX_CGA4)         /* CGA 320x200, 4 colours, BIOS mode 4 */
#define GFX_SW 40
#define GFX_SH 25
#define GFX_CH 8
#define GFX_ICON_CW 5
#define GFX_ICON_RH 4
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
#define GFX_ICON_RH 4
#define GFX_LABEL "MCGA 320x200"
#else
#error "define one of GFX_CGA, GFX_CGA4, GFX_EGA, GFX_VGA, GFX_MCGA"
#endif

/* Desktop icon column, used by ctk.c (patched by "make fetch") */
#define CTK_ICONX_FROM_RIGHT (GFX_ICON_CW + 2)
#define CTK_ICONY_STEP       (GFX_ICON_RH + 2)
#define CTK_ICONY_FROM_BOTTOM (GFX_ICON_RH + 5)

struct gfx_icon {
  unsigned short w, h;          /* pixels */
  const unsigned char *px;      /* rows, one palette index per pixel, 255 = desktop shows through */
};

#endif
