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
| `ctk-tandy` | Tandy 1000 and PCjr 320x200, 16 colours (mode 9) | 40x25 of 8x8 | colour style, standard colours |
| `ctk-tandy4` | Tandy 1000 and PCjr 640x200, 4 colours (mode 0Ah) | 80x25 of 8x8 | CGA four-colour style, yellow for magenta |
| `ctk-herc` | Hercules 720x348 (the 6845 programmed directly) | 90x24 of 8x14 | two-colour style, light on black |
| `ctk-att` | AT&T 6300, Olivetti M24 640x400 (mode 40h) | 80x25 of 8x16 | two-colour style |
| `ctk-ega2` | EGA 640x350 in two colours (mode 10h, 0Fh on a mono monitor) | 80x25 of 8x14 | two-colour style |
| `ctk-vga2` | VGA or MCGA 640x480 in two colours (mode 11h) | 80x30 of 8x16 | two-colour style |
| `ctk-t1200` | Toshiba T1200 LCD: CGA 640x200 on a panel with wider pixels | 80x25 of 8x8 | two-colour style, T1200 icons |

State (2026-11-26): desktop, menus, dialogs, icons, keyboard focus, the
About dialog, the calculator, the editor, the shell and the file manager work on the twelve builds: CGA, EGA, VGA
and MCGA on QEMU, Hercules and 640x400 on 86Box (a Turbo XT with a
Hercules card, and with the Olivetti OGC card); timings of `ctk` and
`ctk-cga4` on MartyPC (8088). Program sizes, mouse, editor and shell
included: `ctk` 46.5 KB, `ctk-cga4` 47.1 KB, `ctk-ega` 51.8 KB, `ctk-mcga`
47.5 KB, `ctk-vga` 64.8 KB (its four built-in icons are 104x80 pixels),
`ctk-herc` 46.8 KB, `ctk-att` 48.0 KB, `ctk-ega2` 46.5 KB, `ctk-vga2`
48.0 KB, `ctk-tandy` 47.7 KB, `ctk-tandy4` 47.6 KB, `desk` 1.6 KB; the
mouse takes about 3.6 KB of each, the editor and its file dialog about
8 KB, the shell window about 3.5 KB, the file manager about 4 KB (each
plus its icon).
`ctk-tandy` and `ctk-tandy4` were checked on 86Box (Tandy 1000 HX),
`ctk-t1200` (47.6 KB) on 86Box's Toshiba T1200 with its LCD.

The T1200 shows the CGA 640x200 mode on its LCD, but its pixels are 0.68
as wide as tall (0.42 on a CRT, `alci/docs/screens.md`) and a pixel set
is dark on the panel, light on a CRT. `ctk-t1200` is the CGA build with
the bytes written as they are (the CGA build writes them inverted), the
T1200 icon set (56x32, 7x4 cells, "Calc" under the calculator), the
pointer drawn for less tall pixels and a narrower shadow (2 pixels for
3). On the CGA build the T1200 panel would show a dark desktop with light
lines. `desk` starts it when the BIOS has "T1200" at F000:E000;
`DESK=cga desk` for a T1200 on an external monitor. On 86Box's T1200
(8086 at 9.54 MHz, timings less exact than MartyPC's): a letter typed in
the editor 23 ms, a page 0.30 s, a line scrolled 146 ms.

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

CGA four colours (`ctk-cga4`): the same lined bar in magenta, a double cyan
frame, the title in the 8x8 font on a plate. Buttons are cyan boxes with
cut corners round the small Clean 6x6 font, as on two colours, filled with
magenta when they have the focus (`-DCGA4_BTN=0` gives the older brackets,
`1` the box with the 8x8 font).

Tandy 1000 and PCjr (`ctk-tandy`): the colour style of MCGA on the 16
standard colours, as on the theme trial for those machines: light grey
desktop, title bar light blue over blue with a light cyan line, white
windows with a black frame, a shadow in dark grey and black, the focus in
yellow; the 8x8 font. `ctk-tandy4` (640x200, 4 colours, chosen by hand with
`DESK=tandy4`) is the CGA four-colour style on twice the width, with the
palette set to black, light cyan, yellow and white.

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

The other 8088 builds, on the same XT with the card changed (Hercules,
IBM EGA) or on MartyPC's Tandy 1000 (8088 4.77 MHz), 2026-11-25:

| Step | `ctk-herc` | `ctk-ega2` | `ctk-tandy` |
| --- | --- | --- | --- |
| desktop drawn and settled | 0.83 s | 0.81 s | 0.80 s |
| repaint of the whole background | 0.76 s | 0.74 s | 0.55 s |
| five keys into the calculator (one full redraw included) | 1.10 s | 1.06 s | 1.48 s |

Cells of 14 rows cost more than cells of 8: the background takes 2160
cells of 14 writes on Hercules. `ctk-att` and `ctk-vga2` use the same code
as `ctk-ega2` (MartyPC has no Olivetti here).

The colour builds of EGA, VGA and MCGA on the same XT with MartyPC's VGA
card (Bochs VGA BIOS; it also runs EGA mode 10h), 2026-11-25. "Before" is
commit a9a826752, where these screens were composed pixel by pixel; both
draw the same pixels (screenshots compared on QEMU in twelve situations).

| Step | `ctk-ega` before | `ctk-ega` now | `ctk-vga` before | `ctk-vga` now | `ctk-mcga` before | `ctk-mcga` now |
| --- | --- | --- | --- | --- | --- | --- |
| desktop drawn and settled | 36.6 s | 0.44 s | 52.9 s | 0.68 s | 4.50 s | 0.67 s |
| repaint of the whole background | 33.9 s | 0.25 s | 46.3 s | 0.32 s | 3.87 s | 0.31 s |
| five keys into the calculator (one full redraw included) | 42.0 s | 1.03 s | 59.1 s | 1.35 s | 6.36 s | 1.46 s |

The "keys" step also holds the drawing of the calculator window, which CTK
defers to its next round. Contiki's CTK redrew the whole screen to open or
close a window; a patch applied by `make fetch`
(`patches/ctk-partial-redraw.patch`) redraws only the window opened and
the one it covers, or the rows of the window closed, with the same
pixels on screen. With it the step takes 0.17 s on CGA (was 0.72 s),
0.28 s on Hercules (was 1.10 s), 0.60 s on EGA, 0.67 s on VGA and 0.77 s
on MCGA; five keys alone take about 0.33 s on EGA.

All the builds after the desktop rows are filled a pixel row at a time
(one byte or word across the screen, in each card's banks; 2026-11-25,
same XT, MartyPC with CGA, Hercules, Tandy 1000 or VGA). "Calculator" is
the calculator window opened and five keys:

| Build | Desktop settled | Background | Calculator |
| --- | --- | --- | --- |
| `ctk` CGA 640x200 | 0.19 s (was 0.55) | 0.13 s (was 0.50) | 0.17 s (was 0.72) |
| `ctk-cga4` CGA 320x200 | 0.27 s (was 0.57) | 0.12 s (was 0.35) | 0.32 s (was 0.88) |
| `ctk-herc` Hercules | 0.31 s (was 0.83) | 0.21 s (was 0.76) | 0.27 s (was 1.10) |
| `ctk-ega2` EGA 640x350 | 0.26 s (was 0.81) | 0.18 s (was 0.74) | 0.27 s (was 1.06) |
| `ctk-vga2` VGA 640x480 | 0.33 s | 0.24 s | 0.25 s |
| `ctk-tandy` Tandy 320x200 | 0.47 s (was 0.80) | 0.27 s (was 0.55) | 0.67 s (was 1.48) |
| `ctk-tandy4` Tandy 640x200 | 0.48 s | 0.21 s | 0.34 s |
| `ctk-ega` EGA 16 colours | 0.44 s | 0.25 s | 0.60 s |
| `ctk-vga` VGA 16 colours | 0.69 s | 0.32 s | 0.67 s |
| `ctk-mcga` MCGA 256 colours | 0.68 s | 0.31 s | 0.77 s |

The screens are the same as before: QEMU for the builds it runs, 86Box
for Hercules, the Olivetti card, Tandy and the IBM EGA and VGA cards.

On EGA and VGA cells are composed as the four bit planes video memory
keeps, with masks (window borders and shadows, desktop, letters, button
sides, icon titles), and written a plane at a time in assembly; the
desktop is filled a band at a time (set/reset for plain rows, the checker
as two rows repeated through the latches); the plane bytes of the four
desktop rows are worked out at start, the icons are stored as planes with
a mask, and the background planes of a line of text are kept from one
letter to the next. On MCGA the window cells go by runs of a row, the
desktop is filled a pixel row at a time and cells are copied with word
moves. The build keeps `-fno-inline`: with inlining, ia16-gcc 6.3 left DS
on the BIOS font segment and wrote a wrong value to a VGA register.

On two colours the frequent cells (desktop, text, buttons, icons, the
middle of title bars and borders) are written as 8 bytes, and window
borders are composed a pixel row at a time with bit masks. Four colours
do the same on two bit planes a row (bit 1 and bit 0 of each pixel), spread
into 2-bit pixels by a table; icons are stored as video memory has them.
The Tandy 16-colour build works the same way on four planes, packed into
4-bit pixels by two tables. The EGA, VGA and MCGA builds compose cells
pixel by pixel and are meant for 286 and later machines.

With the mouse built in, the timed steps are the same (MartyPC, `ctk -b`:
CGA desktop settled 0.20 s, EGA 0.45 s); `-b` does not open the port.
With the Editor and Shell icons the desktop has four icons to draw: settled
in 0.23 s on CGA and 0.56 s on EGA.

## Editor

The Editor icon (or `ctk -e [file]`) opens a window as large as the
screen: Open, Save and New, the file's name (`*` when changed) with the
line and column, then the text. Arrows, Home, End, PgUp, PgDn, Backspace,
Delete and Enter edit; Ctrl-O opens, Ctrl-S saves (asking for a name the
first time), Ctrl-N starts a new text; a click in the text puts the cursor
there. Lines end with a newline, tabs stop every eight columns, long lines
scroll sideways. The text is kept in a gap buffer taken from far memory
when the editor starts (60 KB, less on a machine short of memory) and
given back when it closes, so files up to 60 KB open; the desktop's own
data stay under 64 KB on every build. Only the lines that change are drawn
again; the cursor is a label character with its top bit set, which the
drawing layer shows reversed. Closing the window with a text not saved
asks first (Save, Discard, Cancel).

The file dialog (`filedlg.c`) lists the current directory, directories
first with a `/` (choosing one goes into it), with a line for the name and
the button; arrows, PgUp, PgDn, Home and End choose, letters type a name,
Enter takes it, Esc cancels; a click chooses a line, a second click on it
takes it. It gives back the full path (79 characters at most), so the
editor keeps the file's place whatever directory is current afterwards. It replaces Contiki's `ctk-filedialog`, which reads directories
through Contiki's file service.

Checked on QEMU (EGA, VGA, MCGA, CGA two and four colours: open, edit,
save as, open again, scroll both ways, click) and on 86Box with an 8088
and Hercules.

A key draws only what it changes: the cells of the line from the first
difference to the last (two labels laid over the line for that), and the
status when it changes. A line's label is as long as its text, the window
being cleared under its labels when it is drawn whole. `ctk -B [file]`
times the editor and the shell on the machine (`-Be`, `-Bt` one of them);
on MartyPC's 8088 with `/bin/long.txt` (80 lines, some longer than the
window, some with tabs):

| | CGA | Hercules | EGA | VGA |
|---|---|---|---|---|
| a letter typed | 57 ms | 68 ms | 81 ms | 86 ms |
| a line down | 77 ms | 87 ms | 98 ms | 101 ms |
| a page down | 0.76 s | 0.91 s | 1.20 s | 1.37 s |
| a line up past the top (view scrolls) | 320 ms | 412 ms | 425 ms | 530 ms |
| `cat` of the file in the shell | 3.2 s | 3.5 s | 3.6 s | 3.9 s |

When the view moves by less than half a window (a line down past the
bottom, a line up past the top) and the window is in front, the rows
that stay are moved in video memory (`ctk_gfx_scroll`, either way) and
only the new ones drawn; a page down draws the window again, which on
these screens is quicker than moving most of it. CTK's queue of widgets
to draw holds 16 (4 in Contiki), so the rows drawn after a scroll fit in
it; past it CTK draws the whole window again. CTK also used to draw, with
the widgets queued, the old entries a redraw of the whole window had left
in the queue (patches/ctk-redraw-queue.patch): two lines of the editor
for every key, which is why a letter is quicker here than in earlier
measurements.

A window in front is moved as Nano-X moves one (patches/ctk-window-move.patch,
`ctk_gfx_move`): its inside and title bar are copied in video memory, the
desktop it leaves and the icons there are drawn, then the rest of its
frame; when the desktop round it is the same at both places (no icon or
icon column line under it, the same shading band on EGA/VGA) the frame
and shadow are copied too. With another window, a menu or a dialog near
it, CTK draws the rows again as before. On the XT a move takes 0.18 s
on CGA and 0.47 s on EGA (the D2 benchmark: 10 columns onto the icon
column), against 1.1 s and 2.5 s when the whole screen was drawn again
and 0.84 s and 1.0 s for Nano-X; a few columns away from the icons, 0.12 s
on CGA. Programs move a window with `ctk_window_moveto(w, x, y)`
(Contiki's `ctk_window_move` only sets x and y).

The same `cat` on ELKS's text console takes 3.5 s (much of it is loading
`cat` from the floppy). Before these changes a letter took 380 to 630 ms
and the `cat` 7 to 8 s. The shell's prompt comes 4 to 9 s after its window
opens: `/bin/sh` (65 KB) is read from the floppy.

## Shell

The Shell icon (or `ctk -t`) opens a window as large as the screen with
`/bin/sh` on a pseudo-terminal (`/dev/ptyp0`-`3` and `/dev/ttyp0`-`3`, set
up as telnetd does). The shell and what it runs are ELKS processes of
their own, scheduled by the kernel like any other: a long job in the
window goes on while the desktop, the calculator or the editor are used.
The desktop reads what they write only when the event loop sees data on
the pty (`watch.h`: the loop's `select()` waits on it too), up to 1 KB at
a time so that the other windows keep answering, and draws again the
lines that changed; a program writing a lot is drawn at most five times a
second, the lines it scrolls moved up in video memory (`ctk_gfx_scroll`,
when the window is in front with nothing over it, which CTK tells through
`ctk_window_isfront`, patches/ctk-front-window.patch) and only the new ones
drawn. Keys go to the program as the console would send them
(arrows `ESC [ A-D`, Home, End, PgUp, PgDn, Backspace 0x08, Delete 0x7f,
Ctrl-C and the other control keys; Ctrl-Q still leaves the desktop). The
screen follows CR, LF, Backspace, Tab and the ANSI sequences for the
cursor (`A B C D H f`) and for clearing (`J K`); colours (`m`) are
ignored. `TERM=ansi`, `COLUMNS` and `LINES` give the window's size, which
is 78x21 on EGA and 38x21 on the 320x200 screens: programs that write 80
columns whatever they are told wrap there; `ls` and `ps` follow `COLUMNS`
(Alci's versions: `ls` lays its columns out to the width, `ps` under 60
columns shows only PID, TTY, state, CPU, size and command). `exit` shows
`[ended]`; closing the window hangs the program up and waits for it.

Up to three Shell windows at once ("Shell 1" to "Shell 3", from the icon
or the menu, F3 to go from one to the other), each with its own pty and
shell. A window's text is taken from far memory and its lines as drawn
from the heap (2 KB at most) when it opens, and given back when it
closes; nothing is opened, and the loop waits on nothing more, until a
window opens. Each `/bin/sh` takes about 65 KB of the machine's memory.

Checked on QEMU (EGA, CGA 320x200: `ls`, `ps`, `yes` stopped with
Ctrl-C, `exit`, three windows at once) and on 86Box with an 8088 and
Hercules.

## File manager

The Files icon (or `ctk -f [directory]`) opens a window as large as the
screen: Open, Copy, Ren, Del and Mkdir, the directory's path (or the
result of the last operation), then its entries, directories first with a
`/` and files with their size, sorted by name. Arrows, PgUp, PgDn, Home
and End choose, a letter goes to the first name starting with it, Enter
opens (a directory is entered, a file goes to the editor), Backspace goes
to the directory above, Delete deletes (asking first, Cancel chosen, so
Enter alone deletes nothing; a directory must be empty), Ctrl-C copies
(asking for the new path, relative to the directory shown), Ctrl-R
renames (the old name offered for editing), Ctrl-N makes a directory. A
click chooses an entry, a second click opens it; the buttons do the same
as the keys.

Up to 200 entries are read into far memory when the directory is shown
and given back when the window closes; the file manager keeps its own
path and leaves the desktop's current directory alone. Copies go through
a 256-byte buffer and keep the mode.

Checked on QEMU (EGA and CGA 320x200: listing, letters, copy into `/tmp`,
rename, delete of a file, refusal on a directory not empty, new
directory, a file opened in the editor by Enter and by a double click).

On screens where four icons in a column would not fit with a free row
between them (CGA and Tandy 640x200, 640x400, VGA 640x480), they are put
one under the other without it.

## Build and run

    . ../../env.sh          # ELKS cross toolchain
    make fetch              # Contiki 1.x at the pinned commit
    make                    # desk, ctk, ctk-cga4, ctk-ega, ctk-vga, ctk-mcga,
                            # ctk-tandy, ctk-tandy4, ctk-herc, ctk-att, ctk-ega2, ctk-vga2,
                            # ctk-t1200

Copy the programs to `/bin` on an Alci or ELKS system and run `desk` (or
one of the programs) from the text console. `DESK=cga|cga4|ega|vga|mcga|tandy|tandy4|herc|att|ega2|vga2|t1200 desk`
chooses by hand. `ctk -d` starts with the calculator open, `ctk -e [file]`
with the editor, `ctk -t` with a shell, `ctk -f [directory]` with the file
manager. For the D2 benchmark (`alci/tests/guibench`): `ctk -G` times the
desktop's operations (window opened, moved, closed, text, menu) and
prints them in text mode; `ctk -q N` draws the desktop, waits N seconds
and quits. Results against Nano-X and Gem-elks: `alci/docs/gui-study.md`.

The desktop shows four icons (Files, Editor, Shell, Calculator); About is
in the F1 menu with them.

Keys (desktop), as on Windows and Xfce where they have one: F1 or the
Windows key opens the menu (arrows and Enter to choose, Escape or F1
closes it). Alt-Tab shows the open windows in a box while Alt is held,
each Tab chooses the next (Shift-Alt-Tab the one before) and the chosen
window comes to front when Alt is let go; a quick Alt-Tab goes back to
the window used before. Alt-Esc sends the window in front to the back.
Alt-Space opens the menu of the window in front (Move, Send to back,
Close; arrows and Enter, or the first letter; any other key closes it).
Alt-F4 closes the window in front (as its close box does) and, with no
window open, asks whether to leave the desktop. Tab and F5 move between
widgets, F3 brings the last window to front, Enter presses a button;
Enter on a window's title lets the arrows move the window and any other
key puts it down. With the mouse: a click anywhere on the title bar, then
the window follows the pointer until the next click; a click on the close
box (left on the two-colour screens, right on the colour ones) closes it.

The Alt keys need the Alci kernel. Its keyboard driver sends them as
`ESC ESC` and a letter: Alt-F1 .. F10 `a`..`j` when no console is
switched, Alt-Tab `t`, Shift-Alt-Tab `T`, Alt let go after Alt-Tab `r`,
Alt-Esc `x`, Alt-Space `s`, the Windows key `w`. While on the screen the desktop holds
the console (`DCGET_GRAPH`, released on leaving), so Alt-F1 .. F8 do not
switch to a text console under the picture and no console text is drawn
over it; Ctrl-Q goes back to the text console. On the ELKS kernel the
desktop works without these keys.

Ctrl-Q goes back to the text console; with a text not saved in the editor or a
program in a shell it asks first (Quit, Cancel; Ctrl-Q again quits).

Mouse: a Microsoft serial mouse on `/dev/ttyS0` by default;
`MOUSE_TYPE=ms|pc|ps2|none` and `MOUSE_PORT=/dev/ttyS1` choose another
(`ps2` reads `/dev/psaux`), as for the ELKS `mouse` test program. The port
is opened once the desktop is drawn (on a floppy, finding the device can
take a second) and left alone when it is the terminal the program runs on;
the pointer appears with the first movement, so a port with no mouse
changes nothing on the screen. The left button opens icons, presses
buttons, opens the menus from the bar, closes a window from its box and
brings a window to the front; a click on a title starts moving the window,
which follows the mouse until the next click (CTK's own mouse handling).
A window that another covers is copied away as it is on the screen and
copied back when it comes to front again with nothing in it changed (two
windows at most; in the video memory past the screen on EGA and VGA with
256 KB, in far memory on the CGA-like screens, none on Hercules and
MCGA), so Alt-Tab between windows does not draw them again.

The pointer is drawn by the drawing layer with the bytes under it kept
(black outline and white inside, the other way round on the black
desktops of CGA four colours, Tandy 640x200 and Hercules); every drawing
call takes it off first and the event loop puts it back when the desktop
is idle. Checked on QEMU (Microsoft mouse on COM2) for the seven builds it
can show, and on 86Box for Hercules, 640x400, Tandy 320x200 and 640x200:
the screen is the same, pixel for pixel, after the pointer has gone
round it and to the corners, and the same as before without a mouse.

Without a mouse, F6 or Alt-M brings the pointer up on the keyboard: the
arrows move it a cell at a time (CTK finds widgets by cell), Space or Enter
click, Esc (or F6, Alt-M again) puts it away; other keys work as usual.

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
  word and the Hercules status port, "OLIVETTI" in the BIOS for 640x400,
  the model byte for Tandy 1000 and PCjr).
- `main.c`: keyboard (ELKS console in raw mode: arrows `ESC [ A-D`, F1-F5
  `ESC a-e`), mouse position and buttons for CTK, clock, and the event
  loop, which sleeps in `select()` on the keyboard and the mouse when idle.
- `edit.c`, `edit-dsc.c`: the editor; `filedlg.c`: its file dialog.
- `files.c`, `files-dsc.c`: the file manager.
- `term.c`, `term-dsc.c`: the shell window; `watch.h`: descriptors for the
  event loop to wait on (main.c).
- `mouse.c`, `mouse.h`: the ELKS user-mode mouse driver
  (`elkscmd/gui/mouse.c`, from Microwindows) without its messages and test
  program; a Microsoft packet restarts at any byte with the sixth bit set,
  so the "M" a mouse sends when the port opens is skipped.
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
  screen (`obj-<type>/gfx-icons.h`); the MCGA icons come from the set drawn
  for it, with 40 colours of their own. On the 320x200 screens (CGA four
  colours, Tandy 16 colours, MCGA) the icons are 32x24 in a column of five
  cells, with titles of five letters at most ("Calc", "Files", "Edit"),
  centred to the pixel; windows can be moved over the column. Wallpapers (`ctk -w dots|pairs|slash|none`, default dots,
  or `CTK_WALLPAPER`) exist on two colours.

## Licences

Contiki 1.x: BSD 3-clause, Copyright (c) 2002-2004 Adam Dunkels; the
notices are kept in every file taken or adapted. font8x8: public domain.
clR6x6, clB8x14 and clB8x16 (Schumacher Clean, used for `elks/font-bar.h`
and `elks/font-tall*.h`): Copyright 1989 Dale Schumacher, free to use,
copy, modify and distribute with the notice.
`mouse.c`: from the ELKS mouse driver, Microwindows, Copyright (c) 1999
Greg Haerr, portions Copyright (c) 1991 David I. Bell, free to use,
distribute or modify with the notice, which the file keeps.
The Contiki sources themselves are not copied here: `make fetch` downloads
them, drops a `static` that current compilers reject and makes the icon
column follow the icon size (see the Makefile).

## Next steps

1. Timings of the tall two-colour builds and of the Tandy build on an
   8088; the T1200 build on the real machine.
2. The rest of the D2 benchmark (1.44M floppy and hard disk, packed
   programs, 86Box PC1512 and AT); first results against Nano-X and
   Gem-elks in `alci/docs/gui-study.md`.
