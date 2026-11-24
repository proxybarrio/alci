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

State (2026-11-24): desktop, menus, dialogs, icons, keyboard focus, the
About dialog and the calculator work on all five screens on QEMU; timings
of `ctk` and `ctk-cga4` on MartyPC (8088). Program sizes: `ctk` 19.8 KB,
`ctk-cga4` 22.2 KB, `ctk-ega` 25.1 KB, `ctk-mcga` 22.6 KB, `ctk-vga`
35.8 KB (its two built-in icons are 104x80 pixels), `desk` 1.3 KB.

## Looks

CGA two colours (`ctk`): the active title bar carries thin horizontal
lines (rows 1, 3 and 5 of the bar), with the title centred on a white plate
that also opens the top line, and the close box on the left on a plate of
its own; inactive bars are plain. Corners cut by one pixel, solid shadow,
rounded screen corners, the selected icon title in an inverted pill. The
8x8 font throughout and no line through the letters: buttons are rounded
brackets, `(7)`, and a filled pill when focused; text entries have side
bars and a line under the text; the line over the status bar runs in the
row above it. (Tried and dropped: a black bar with oblique lines, dots
on the lower half of the bar, a small bold font.)

Colour screens (`ctk-gfx.c`): a two-tone title bar (graded on MCGA),
`/// title` on the left, a light line on the bar after the title, close box
on the right, a stepped shadow (soft on MCGA, dotted on CGA four colours),
an etched line beside the icon column, buttons two-toned and blue when
they have the focus, with a light ring on cells 14 and 16 pixels high. The
EGA colours are those of the EGA icon set; the desktop is white above, a
fine white and grey checker across the middle, grey below. CGA four
colours takes the lined bar of the two-colour screen (magenta lines on
black, white title on a plate) and a double frame, thick and thin, in
cyan. On 8-pixel cells (CGA four
colours, MCGA) the 8x8 font is used everywhere, the title bar fills the
whole top row so no line runs through the letters, and buttons and
entries are drawn as on two colours. Cells of 14 and 16 pixels use the font of the
video BIOS, read at start-up.

## Timings

MartyPC, IBM XT, 8088 4.77 MHz, CGA, 720K floppy, 2026-11-24. `ctk -q`
draws the desktop and quits; `ctk -b` prints timings in ms after going
back to text mode (10 ms resolution). "Before" is the CGA-only layer of
2026-11-22 (commit eb1d144).

| Step | `ctk` before | `ctk` now | `ctk-cga4` |
| --- | --- | --- | --- |
| `-q`, first run (program read from floppy) | 5.32 s | 5.26 s | 6.56 s |
| desktop drawn and settled | 0.55 s | 0.58 s | 1.37 s |
| repaint of the whole background | 0.50 s | 0.50 s | 0.72 s |
| open the calculator | 0.03 s | 0.07 s | 0.15 s |
| five keys into the calculator (`2*21=`, one full redraw included) | 0.67 s | 1.05 s | 2.97 s |
| open the About dialog | 0.01 s | 0.02 s | 0.01 s |

On two colours the frequent cells (desktop, text, buttons, icons, the
middle of title bars and borders) are written as 8 bytes, and window
borders are composed a pixel row at a time with bit masks. The colour builds compose cells pixel by pixel and are meant first for
286 and later machines; on the 8088 the four-colour CGA build works but is
slower, and its window borders are the next thing to speed up.

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
- `desk.c`: the launcher (video BIOS functions 1Ah and 12h).
- `main.c`: keyboard (ELKS console in raw mode: arrows `ESC [ A-D`, F1-F5
  `ESC a-e`), clock, and the event loop, which sleeps in `select()` when
  idle.
- `elks/`: configuration headers (`ctk-conf.h`, `ctk-arch.h` written for
  ELKS; `ek-conf.h`, `cc-conf.h`, `clock-conf.h`, `log-conf.h`,
  `program-handler-conf.h` from the Contiki 1.x GTK port),
  `font8x8.h`, a public-domain 8x8 font (Daniel Hepper's font8x8, after
  the IBM public-domain VGA fonts) with its bits reversed for CGA.
- `icons/`: the Alci icon sets, one per screen family (see
  `icons/README.md`). `icons/tools/mkgfx.py` writes, for each build, a
  header with only the icons the desktop uses, in the palette of that
  screen (`obj-<type>/gfx-icons.h`); the MCGA icons are for now reduced from
  the modern set. Wallpapers (`ctk -w dots|pairs|slash|none`, default dots,
  or `CTK_WALLPAPER`) exist on two colours.

## Licences

Contiki 1.x: BSD 3-clause, Copyright (c) 2002-2004 Adam Dunkels; the
notices are kept in every file taken or adapted. font8x8: public domain.
The Contiki sources themselves are not copied here: `make fetch` downloads
them, drops a `static` that current compilers reject and makes the icon
column follow the icon size (see the Makefile).

## Next steps

1. Faster window borders on the colour builds, first CGA four colours.
2. Editor, then the file manager (Contiki's "Directory" is minimal).
3. Mouse (ELKS mouse driver).
4. Drawing layers for the T1200 LCD, Hercules and 640x400 (their icon sets
   exist), Tandy 1000 and PCjr.
5. The D2 benchmark against Nano-X and Gem-elks.
