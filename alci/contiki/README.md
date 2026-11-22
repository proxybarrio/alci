# Contiki desktop on Alci (prototype, task D7)

The graphical desktop of Contiki 1.x (CTK: windows, menus, dialogs, icons),
written by Adam Dunkels for the Commodore 64 and other 8-bit machines,
running as an ordinary ELKS program on a CGA screen (640x200, two colours).
No kernel change: the same program runs on ELKS and on Alci.

State (2026-11-22): desktop, menus, dialogs, icons, keyboard focus, the
About dialog and the calculator work on QEMU. Program size 13,328 bytes
(10.6 KB code, 2.7 KB data, 0.9 KB bss, plus 8 KB heap and 4 KB stack set
at link time). Not yet measured on MartyPC or 86Box.

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
