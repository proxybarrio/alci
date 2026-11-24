# Contiki desktop on Alci (prototype, task D7)

The graphical desktop of Contiki 1.x (CTK: windows, menus, dialogs, icons),
written by Adam Dunkels for the Commodore 64 and other 8-bit machines,
running as an ordinary ELKS program. No kernel change: the same programs
run on ELKS and on Alci.

One program per screen type, all from the same sources, and `desk`, which
asks the video BIOS what is fitted and starts the right one:

| Program | Screen | Cells | Look |
| --- | --- | --- | --- |
| `ctk` | CGA 640x200, 2 colours (also EGA, VGA, LCDs in that mode) | 80x25 of 8x8 | two-colour style |
| `ctk-cga4` | CGA 320x200, 4 colours (palette 1, high intensity) | 40x25 of 8x8 | lined bar, double frame |
| `ctk-ega` | EGA 640x350, 16 of the 64 EGA colours | 80x25 of 8x14 | colour style |
| `ctk-vga` | VGA 640x480, 16 colours | 80x30 of 8x16 | colour style |
| `ctk-mcga` | MCGA (and VGA) 320x200, 256 colours | 40x25 of 8x8 | colour style, graded |
| `ctk-herc` | Hercules 720x348 (the 6845 programmed directly) | 90x24 of 8x14 | two-colour style, light on black |
| `ctk-att` | AT&T 6300, Olivetti M24 640x400 (mode 40h) | 80x25 of 8x16 | two-colour style |
| `ctk-ega2` | EGA 640x350 in two colours (mode 10h, 0Fh on a mono monitor) | 80x25 of 8x14 | two-colour style |
| `ctk-vga2` | VGA or MCGA 640x480 in two colours (mode 11h) | 80x30 of 8x16 | two-colour style |

State (2026-11-24): desktop, menus, dialogs, icons, keyboard focus, the
About dialog and the calculator work on the nine screens: CGA, EGA, VGA
and MCGA on QEMU, Hercules and 640x400 on 86Box (a Turbo XT with a
Hercules card, and with the Olivetti OGC card); timings of `ctk` and
`ctk-cga4` on MartyPC (8088). Program sizes: `ctk` 22.3 KB, `ctk-cga4`
21.8 KB, `ctk-ega` 24.7 KB, `ctk-mcga` 22.3 KB, `ctk-vga` 35.1 KB (its two
built-in icons are 104x80 pixels), `ctk-herc` 22.2 KB, `ctk-att` 22.8 KB,
`ctk-ega2` 22.0 KB, `ctk-vga2` 22.8 KB, `desk` 1.5 KB.

## Looks

CGA two colours (`ctk`): the title bar stays one text row high (the window
layout of Contiki is unchanged) and carries thin lines one pixel row apart,
as on the bars of the trials: the frame in the last pixel row of the row
above the title, lines on rows 1, 3 and 5, the line under the bar on row 7
(`-DCGA_BAR=1` keeps everything inside the row: frame, two lines, line
under the bar on row 6); the title in the small font on a white plate, the
close box on the left on a plate of its own; inactive bars are plain.
Bars, buttons and icon titles use the small Clean 6x6 font (rows 1-6 of the
cell, so the lines stay clear of it); text inside windows uses the 8x8
font. Buttons are boxes with cut corners, with a second line on each side
when they have the focus; text entries have side bars and a line under the
text. Corners cut by one pixel, solid shadow, rounded screen corners, the
selected icon title in an inverted pill. `ctk -w none` gives the plain white
desktop of the trial.

Hercules, 640x400, EGA and VGA in two colours (`ctk-herc`, `ctk-att`,
`ctk-ega2`, `ctk-vga2`): the same style on cells of 14 and 16 rows, as on
the trials for those screens: the whole bar in the title row, frame on
row 0, lines on every other row from 2, the line under the bar on the last
row, the close box from row 2; Schumacher Clean Bold 8x14 and 8x16
everywhere (one row lower in the bars); a shadow of 2x2 pixels. Hercules is
drawn light on black, as on its trial, with a plain black desktop by
default (`-w dots` brings the dots back).

## Timings

MartyPC, IBM XT, 8088 4.77 MHz, CGA, 720K floppy, 2026-11-24. `ctk -q`
draws the desktop and quits; `ctk -b` prints timings in ms after going
back to text mode (10 ms resolution). "Before" is the CGA-only layer of
2026-11-22 (commit eb1d144).

| Step | `ctk` before | `ctk` now | `ctk-cga4` |
| --- | --- | --- | --- |
| `-q`, first run (program read from floppy) | 5.32 s | 5.27 s | 5.13 s |
| desktop drawn and settled | 0.55 s | 0.55 s | 0.57 s |
| repaint of the whole background | 0.50 s | 0.50 s | 0.35 s |
| open the calculator | 0.03 s | 0.04 s | 0.04 s |
| five keys into the calculator (`2*21=`, one full redraw included) | 0.67 s | 0.72 s | 0.88 s |
| open the About dialog | 0.01 s | 0.02 s | 0.01 s |

On two colours the frequent cells (desktop, text, buttons, icons, the
middle of title bars and borders) are written as 8 bytes, and window
borders are composed a pixel row at a time with bit masks. Four colours
do the same on two bit planes a row (bit 1 and bit 0 of each pixel), spread
into 2-bit pixels by a table; icons are stored as video memory has them.
The EGA, VGA and MCGA builds compose cells pixel by pixel and are meant
for 286 and later machines.

## Build and run

    . ../../env.sh          # ELKS cross toolchain
    make fetch              # Contiki 1.x at the pinned commit
    make                    # desk, ctk, ctk-cga4, ctk-ega, ctk-vga, ctk-mcga

Copy the programs to `/bin` on an Alci or ELKS system and run `desk` (or
one of the programs) from the text console. `DESK=cga|cga4|ega|vga|mcga desk`
chooses by hand. `ctk -d` starts with the calculator open.

Keys: F1 opens the menu (arrows and Enter to choose), Tab and F5 move
between widgets, F3 switches window, Enter presses a button, Ctrl-Q goes
back to the text console.

## Files

- `ctk-gfx.c`: the drawing layer for every screen type. Same structure as
  the C64 "hires" driver of Contiki 1.x (`contiki-c64/ctk/ctk-hires.c` and
  its 6502 assembly), in C: the screen is a grid of cells, each cell is
  composed (into 8 bytes on two colours, into a buffer of colour indices
  otherwise) and written by the routine of the screen type (CGA interleaved,
  CGA two bits a pixel, EGA and VGA planes in write mode 2, MCGA bytes).
  The colours and the two styles are at the top of the file.
- `gfx.h`: the screen types (cells, rows, icon size) chosen with
  `-DGFX_<type>`; it also places the icon column for `ctk.c`.
- `desk.c`: the launcher (video BIOS functions 1Ah and 12h, the equipment
  word and the Hercules status port, "OLIVETTI" in the BIOS for 640x400).
- `main.c`: keyboard (ELKS console in raw mode: arrows `ESC [ A-D`, F1-F5
  `ESC a-e`), clock, and the event loop, which sleeps in `select()` when
  idle.
- `elks/`: configuration headers (`ctk-conf.h`, `ctk-arch.h` written for
  ELKS; `ek-conf.h`, `cc-conf.h`, `clock-conf.h`, `log-conf.h`,
  `program-handler-conf.h` from the Contiki 1.x GTK port),
  `font8x8.h`, a public-domain 8x8 font (Daniel Hepper's font8x8, after
  the IBM public-domain VGA fonts) with its bits reversed for CGA;
  `font-bar.h`, `font-tall14.h`, `font-tall16.h`, written by
  `theme/mkbarfont.py` and `theme/mktallfont.py` from the X11 Schumacher
  Clean fonts.
- `icons/`: the Alci icon sets, one per screen family (see
  `icons/README.md`). `icons/tools/mkgfx.py` writes, for each build, a
  header with only the icons the desktop uses, in the palette of that
  screen (`obj-<type>/gfx-icons.h`); the MCGA icons are for now reduced from
  the modern set. Wallpapers (`ctk -w dots|pairs|slash|none`, default dots,
  or `CTK_WALLPAPER`) exist on two colours.

## Licences

Contiki 1.x: BSD 3-clause, Copyright (c) 2002-2004 Adam Dunkels; the
notices are kept in every file taken or adapted. font8x8: public domain.
clR6x6, clB8x14 and clB8x16 (Schumacher Clean, used for `elks/font-bar.h`
and `elks/font-tall*.h`): Copyright 1989 Dale Schumacher, free to use,
copy, modify and distribute with the notice.
The Contiki sources themselves are not copied here: `make fetch` downloads
them, drops a `static` that current compilers reject and makes the icon
column follow the icon size (see the Makefile).

## Next steps

1. Faster window borders on EGA, VGA and MCGA (pixel by pixel today).
2. Editor, then the file manager (Contiki's "Directory" is minimal).
3. Mouse (ELKS mouse driver).
4. Drawing layers for the T1200 LCD (its icon set exists), Tandy 1000 and
   PCjr; timings of the tall two-colour builds on an 8088.
5. The D2 benchmark against Nano-X and Gem-elks.
