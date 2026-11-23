# Contiki desktop on Alci (prototype, task D7)

The graphical desktop of Contiki 1.x (CTK: windows, menus, dialogs, icons),
written by Adam Dunkels for the Commodore 64 and other 8-bit machines,
running as an ordinary ELKS program on a CGA screen (640x200, two colours).
No kernel change: the same program runs on ELKS and on Alci.

State (2026-11-22): desktop, menus, dialogs, icons, keyboard focus, the
About dialog and the calculator work on QEMU and on MartyPC (8088). Program
size 13,328 bytes without the benchmark code, 15.8 KB with it (stdio for
the report); the Alci icons add 5 KB (21,776 bytes in all).

First timings, MartyPC 8088 4.77 MHz CGA, 720K floppy, 2026-11-22
(`ctk -q` draws the desktop and quits; `ctk -b` prints timings in ms after
going back to text mode, with 10 ms resolution):

| Step | Time |
| --- | --- |
| `ctk -q`, first run (program read from floppy) | 4.83 s |
| `ctk -q`, program already in memory (text cache) | 1.24 s |
| desktop drawn and settled after start | 0.53 s |
| repaint of the whole background (2,000 cells) | 0.49 s |
| open the calculator, then five keys (`2*21=`) | 0.66 s (about 0.13 s a key) |
| open the About dialog | 0.02 s |

The cell writer (`cell()` in `ctk-cga.c`) writes one byte at a time through
a far pointer; a background repaint at half a second is the first thing
to speed up.

## Build and run

    . ../../env.sh          # ELKS cross toolchain
    make fetch              # Contiki 1.x at the pinned commit
    make                    # produces ./ctk

Copy `ctk` to an Alci or ELKS system and run it from the text console.

Keys: F1 opens the menu (arrows and Enter to choose), Tab and F5 move
between widgets, F3 switches window, Enter presses a button, Ctrl-Q goes
back to the text console.

## Files

- `ctk-cga.c`: the CGA drawing layer. Same structure as the C64 "hires"
  driver of Contiki 1.x (`contiki-c64/ctk/ctk-hires.c` and its 6502
  assembly), rewritten in C for the CGA interleaved frame buffer: 8x8
  cells, window borders and buttons from the theme patterns, C64 colours
  mapped to normal or reversed cells.
- `theme-default.c`, `elks/ctk-hires-theme.h`: the C64 default theme of
  Contiki 1.x, unchanged except for the C64 colour numbers defined in place
  of `conio.h`. The desktop background is a light dot pattern instead of the
  C64 colour gradient.
- `main.c`: keyboard (ELKS console in raw mode: arrows `ESC [ A-D`, F1-F5
  `ESC a-e`), clock, and the event loop, which sleeps in `select()` when
  idle.
- `elks/`: configuration headers (`ctk-conf.h`, `ctk-arch.h` written for
  ELKS; `ek-conf.h`, `cc-conf.h`, `clock-conf.h`, `log-conf.h`,
  `program-handler-conf.h` from the Contiki 1.x GTK port) and
  `font8x8.h`, a public-domain 8x8 font (Daniel Hepper's font8x8, after
  the IBM public-domain VGA fonts) with its bits reversed for CGA.
- `theme/p1.py`, `theme-p1.h`: the Alci theme for CGA 640x200 (profile P1):
  double window border with a dotted shadow, striped black title bar, two-cell
  close box, outlined buttons and text entries, light slashed desktop. The
  pieces are drawn by hand in `theme/p1.py`; `theme/mkheader.py` writes them
  to `theme-p1.h`. Desktop background: `ctk -w dots` (default), `pairs`,
  `slash` or `none`, or the same names in `CTK_WALLPAPER`; window shadows
  are combined with the chosen pattern at start-up.
- `icons/`: the Alci icon sets, one per screen family, ready to build in
  (`icons/out/<set>.h`) or to install as a file (`icons/out/<set>.ais`);
  see `icons/README.md`. The desktop draws the CGA 640x200 set
  (`icons/out/cga640-mono.h`, 10x4 cells per icon): `main.c` gives each
  program's icon the drawing of the same name, `ctk-cga.c` draws it with
  the title centred below. All 16 icons are built in (5 KB); `make fetch`
  also moves the icon column of `ctk.c` for the wider icons.

## Licences

Contiki 1.x: BSD 3-clause, Copyright (c) 2002-2004 Adam Dunkels; the
notices are kept in every file taken or adapted. font8x8: public domain.
The Contiki sources themselves are not copied here: `make fetch` downloads
them, and its only change is to drop a `static` that current compilers
reject (see the Makefile).

## Next steps

1. Measure on MartyPC (8088, 4.77 MHz) and 86Box: start time, redraw of the
   desktop, opening a window, response to a key.
2. Editor, then the file manager (Contiki's "Directory" is minimal).
3. Mouse (ELKS mouse driver), 320x200 four-colour mode, EGA.
4. The D2 benchmark against Nano-X and Gem-elks.
