# Alci: work log

## 2026-11-16

- Project documents translated to English.
- Boot banner: "ALCI 0.1: Asymmetric Linux Compact Initiative", crediting
  ELKS, Alan Cox, Greg Haerr and the ELKS team.

## 2026-11-15

- Patches: EDD support for large disks, fsck inode seek fix, fast console
  and hardware scrolling, directory name lookup, _wake_up, iget, 512-byte
  pipes, fputs and time conversion in libc, block reads in the boot loader,
  two-head floppy reads (experimental).
- MartyPC (branch `elks-bench`): keyboard queue fixed (it used to drop
  keys), 8086 option, floppy rotation model, ELKS runner with profiler.
- Strategy agreed: GUI modeled on PC/GEOS, discardable code
  segments as the basis of memory management, Claude terminal through a
  Wi-Fi modem and later an ESP32 with microSD.
- Issue #2851 (fsck) filed on ghaerr/elks.
- Results confirmed independently on 86Box (see `RESULTS.md`).

## 2026-11-16

- Merged 3 upstream commits (80C188EB support, cleanup); timings unchanged.
- A1 done: `fork` skips the unused gap in the data segment. 10 runs of
  `pwd` 21% faster, pipes 9% faster, no memory cost. MartyPC harness can
  now type `( ) $ " ' <` and print the final screen (`ELKS_SCREEN`).

## 2026-11-17

- Upstream ghaerr/elks master (`9b3c134`) already contained in `alci`:
  nothing to merge.
- A1b set to `HUMAN` (it needs the maintainer's go-ahead).
- A2 dropped: the timer tick costs 3.2% of the CPU (1523 cycles per tick).
  A trial patch cut the idle tick by 21% but gained only 0.46% on the
  benchmarks (value 0.28), so it was not kept. Figures in `RESULTS.md`.
- `setup.sh`: refresh the package lists when `xxd` is missing, install
  `libzstd-dev` for 86Box, and switch an existing MartyPC clone to
  `elks-bench`.
- A2 trial also measured on 86Box (XT, hard disk): mean gain 1.1%, value
  0.67, still rejected. New tasks: A1c (study for `vfork` in ash, no code)
  and A2c (idle loop without the wake-up race, measured on latency).
- A2b done: `memchr` in assembly, `fgets` and `grep` read whole runs from
  the stdio buffer. `grep` 33% faster, no benchmark slower, kernel
  unchanged, `grep` +96 bytes. Five `grep` lines added to the functional
  suite.
- New task S1: on 86Box XT with the 720K image the suite hangs once the
  floppy is full; it depends on the image layout, not on the A2b code.
  New task A2d for `sed` and `uniq`.
- S1 rewritten: the "hang on a full floppy" was a character lost on the
  serial input (the harness's end marker), not a hang. The same loss shows
  up on upstream ELKS master on the IBM XT. S1 is now about that serial
  loss.
- Emulators are now watched like ELKS: upstream MartyPC and 86Box commits
  are counted each run and merged at most weekly, with timings before and
  after (`AGENT.md`, `RESULTS.md` "Emulator updates"). Task A6: fork
  86Box into `proxybarrio/86box`, waiting for the maintainer.

## 2026-11-19

- Merged upstream ELKS `df2a61c5` (3 commits: `net.cfg` trimmed and
  network time option); only `/etc/net.cfg` changes. Functional suites
  pass after the merge.
- Upstream MartyPC: no new commits. 86Box: `proxybarrio/86box` has no
  `alci` branch yet, so `setup.sh` built upstream `d4f9a04` plus the
  PicoMEM patch; it now also installs `libarchive-dev`.
- F4 done: compressed programs measured and not adopted (`vi` starts 8
  times slower on the XT); 720K image gets the core text tools and loses
  four network programs (41 -> 82 free blocks); new floppy suite
  `functional-fd.txt` with a second disk; `tar-ok` made strict. 34/34 on
  all six machines. New task F4b (faster decompressor).
- S3 done: `exec` rounds data segments to 256 bytes, which removes the
  fragmentation behind `sh: not enough memory` on the 512K AT.
- S1 done: serial bytes were lost behind the timer interrupt (IRQ 0 kept in
  service through the BIOS IRQ 0 handler). EOI now comes before the
  timer handler and the BIOS handler runs as a bottom half. Longest wait
  of a byte 643 -> 384 us, no lost bytes; +29 bytes of code. Patch for
  ELKS prepared in `alci/upstream/`, not sent. MartyPC `elks-bench` got an
  interrupt latency meter (`ELKS_IRQOFF`), 86Box a probe kept as a patch
  in `alci/tests/`.
- The 720K layout change (`59478e75`) is reverted: programs leave disk 1
  only together with disk 2 and a disk swap that works on the XT. It had
  also made some reads slower (grep +5%) and left `ftpd` in `net.cfg`.
  New rules for the floppy set in `TASKS.md` (phase 6), new task E8 (SSH
  client, study first). The floppy suite gets its missing programs from
  the test disk: 34/34 on all six machines.
- Decided: `telnet telnetd ftp ftpd miniterm` stay on disk 1; every
  distribution carries on disk 1 an installer for the whole set on a hard
  disk (built on ELKS's `sys`, `fdisk`, `mkfs`, `makeboot`). Recorded in
  `TASKS.md` (phase 6) and `STRATEGY.md`.
- 720K set redefined: disk 1 unchanged, disks 2 and 3 only with what is
  not on it. `wc head cmp sleep xargs yes` stay on disk 2 (disk 1 keeps
  its 41 KB of working room). Long-term aim in `STRATEGY.md`: disk 1
  boots into the graphical interface.
- Study of os8088's distribution (`docs/os8088-study.md`). Personal
  Storage disks will be FAT. F4b started: LZ4 executables work (34/34,
  73 KB freed on 720K) but are kept off by default: faster from floppy,
  slower from the cache. Next: a faster decoder and other formats (LZSA).
- F4b round 2: LZ4 decoder sped up, LZSA1 added (zlib decoder by the LZSA
  authors). Both faster from floppy, both slower from the cache by the
  same amount; LZSA1 packs best (124 KB free on 720K). Still off by
  default. Proposed next: a text cache for repeated starts.
- F4b round 3: text cache (exited programs kept in free memory, freed on
  demand). With LZSA1 it makes repeated starts faster than today's plain
  system; 34/34 on all machines. Off by default until the maintainer
  decides.
- LZSA1 and the text cache are on in `alci/elks.config` (LZ4 removed).
  720K: 123 KB free. Suites 34/34 on every machine, serial and clock
  checks unchanged. Next idea from the maintainer: fewer disks in the
  720K set, now that there is room.
- A3 started: the floppy boot traced command by command on MartyPC
  (new `ELKS_FDTRACE` and `ELKS_BOOT_TEXT` in `elks-bench`). Upstream's
  file order is already good; the time goes to the boot loader re-reading
  the inode block and to waits for sector 1. Plan in `TASKS.md`.
- `meminfo`: texts kept by the text cache are listed as KEPT and counted as
  free memory, since they are given back on demand.
- S2 done: `clock -w` wrote an invalid year to the AT's CMOS after 1999;
  fixed (two-digit BCD year, century byte where present). Patch for ELKS
  prepared, not sent.
- A3 point 1: the floppy boot loader keeps the inode block; boot 0.6 s
  faster on the XT. Needs a test on the real T1200.
- A5 done: the NE2000, WD8003 and 3C509 drivers give their code back at
  boot when no card is found (about 1.9 KB each). With no card 4.9 KB
  more free memory, with one card 3 KB; network speed unchanged.
- E7 point 4: when the date is older than the system, `rc.sys` asks for
  it after the network start (`clock -c`); no answer in 60 s, boot goes
  on. The network time client is still to do.
- Review of F4b: corrupted LZSA1 programs can hang the system (7 of 12 on
  QEMU); a host model shows the decoder writing past its segment. Tasks
  F4c (bounds checks in the decoder) and F4d (text cache and `mv`).
- Decisions on date and time (recorded in `STRATEGY.md` and E7): UTC in
  the clock, the user's zone as a POSIX `TZ` rule, summer time computed in
  one place, time from NTP servers of time institutes, the question at
  boot as the last resort; floppy rules for saving the zone. Order agreed:
  F4c, E7, F4d. Documents brought up to date.

## 2026-11-20

- F4c done: the LZSA1 decoder stops on a damaged block and exec rejects
  it. Of the twelve damaged `grep` copies that hung the system in 7
  cases, none does now. Decoder 74 bytes smaller, program starts up to
  1.1% slower.
- Boot logo task D5 written; the Alci logo is on the README.
- E7 point 2: SNTP client in `ktcp` (`-t` list, DHCP option 42 first),
  answers checked, times before the build refused; default servers of
  INRIM, PTB and NIST in `/etc/net.cfg`. `ktcp` +1.2 KB, kernel and libc
  unchanged; tested on QEMU and the 86Box XT with a test server. Two
  floppy benchmarks move by a two-block layout shift (control run with
  padded files shows the same). Next: E7b, the boot waits for network
  time before asking.
- E7 decisions: institute servers first, the local network's server only
  as a fallback; first real boot reports which servers answer; large
  jumps applied and printed; the layout effect on two floppy benchmarks
  accepted. E7b adds a key to skip the wait for network time at boot.

## 2026-11-21

- S4 done: `exec` rejects programs whose header points the entry outside
  the text, whose segment sizes do not fit 64K, or whose relocations write
  outside their segment. Before, such a program could leave the whole
  system dead on the IBM XT, T1200, PC1512 and AT (86Box) and on MartyPC.
  +84 bytes of far kernel code, timings unchanged. New test
  `alci/tests/exec-corrupt.py`; `st86.py` gains `CMD_MS` and `INTR`.
- S4 parked: the exec checks are reverted until the first medium-model
  program enters Alci (no program with relocations today, so 80 resident
  bytes for nothing). Test tool, results and the ELKS patch stay.

## 2026-11-22

- S5 done: kilo no longer destroys the file when a save fails (full disk
  reproduced on Alci), and its buffer overruns and null pointer reads are
  fixed. +416 bytes on disk, no kernel change. Test `alci/tests/kilo-save.py`.
- S6 done: `clock -c` refuses impossible dates. F5 records what the kernel
  does today when a floppy is swapped and what the design must guarantee.
  F4e also compares ZX0.
- `setup.sh` copied the old `elks.config` of the toolchain branch, which
  lacks program compression, when no `.config` existed; it now always
  starts from `alci/elks.config` and warns when `.config` differs. The
  strict suites were run again on images built with `alci/elks.config`
  after the kilo and clock changes: 34/34 (QEMU hd32 and fd720).
- Kilo on the 720K disk, measured (MartyPC 8088, `bench.sh` image):
  27264 bytes plain, 19968 compressed with LZSA1; first start 6.19 ->
  6.10 s without rotation delay, 7.56 -> 7.36 s with it. Disk 1 has
  103 KB free, 83 KB with compressed kilo on it. Not added yet.

## 2026-11-24

- D7: one drawing layer for every screen type (`alci/contiki/ctk-gfx.c`,
  programs `ctk`, `ctk-cga4`, `ctk-ega`, `ctk-vga`, `ctk-mcga`, launcher
  `desk`), replacing the CGA-only layer. Both CGA builds: lined active
  title bar with the title on a plate, the 8x8 font throughout, no line
  through letters. Checked on QEMU; MartyPC 8088 timings in
  `alci/contiki/README.md`.
- D7: faster two-colour window drawing and four-colour CGA on bit planes
  (8088: full redraw with the calculator 0.72 s and 0.88 s). New programs
  `ctk-herc`, `ctk-att`, `ctk-ega2`, `ctk-vga2` (two colours, cells of 14
  and 16 rows), checked on QEMU and 86Box; `desk` detects them.
- Upstream ELKS merged up to `a3d4940` (`mem=` override in `setup`,
  external builds of Cmdr and scrcap, version 0.9.3-dev); timings, the
  strict suites (34/34, QEMU and 86Box XT) and resident memory unchanged.
  `e47e635` (FAT `/bootopts` fix in the boot sector) does not fit in
  Alci's MINIX boot sector, which has no free byte left: not merged,
  listed in "Merges to review" with the three commits after it.
- F4e split into four pieces. F4e1 done: upstream ELKS with its default
  configuration refuses all 36 LZSA1 programs of the 720K disk and stays
  intact, but only because its exomizer decoder fails on LZSA1 data; the
  shell then prints the file as a garbled script. Header marker options
  in `RESULTS.md`, for decision. New test `alci/tests/foreign-exec.sh`.
  No change to the kernel or the programs.
- Rest of upstream ELKS merged (`a450d52`): the `/bootopts` boot fix
  for FAT boot sectors only (MINIX disks already clear the old copy in
  the loader), and ept, the ELKS package tool, which reaches disk 2 of
  the 720K set (13 KB left there). Timings, suites and resident memory
  unchanged.

## 2026-11-25

- D7: icons of 32x24 in a column of five cells on the 320x200 screens
  (CGA four colours, Tandy, MCGA), with titles of five letters at most;
  the face icon is the Commodore 64 one of Contiki.
- D7: the EGA, VGA and MCGA colour builds composed in bit planes and by
  runs instead of pixel by pixel, the same pixels as before (8088, desktop
  settled: EGA 36.6 s to 0.44 s, VGA 52.9 s to 0.68 s, MCGA 4.5 s to
  0.67 s). Timings in `alci/contiki/README.md`.
- D7: opening a window redraws that window and the one it covers, closing
  one the rows it took (CTK patch applied by `make fetch`); the same
  screens as before, the calculator step 4x faster on CGA and Hercules,
  about 2x on EGA, VGA and MCGA. The calculator shows 0 when empty.
- D7: the desktop of every build filled a pixel row at a time; desktop
  settled on the 8088 between 0.19 s (CGA) and 0.69 s (VGA 16 colours),
  same screens (QEMU and 86Box). Byte operands of inline assembly passed
  as words: ia16-gcc could put them in AH.
- D7: mouse on the desktop of every build: Microsoft, PC or PS/2 mouse
  through the ELKS user-mode driver, pointer drawn by the drawing layer
  with the screen under it kept, opened after the desktop is drawn. Clicks
  open icons, press buttons, open menus, close and move windows. Checked
  on QEMU and 86Box (automation socket): the screen comes back pixel for
  pixel. The pointer masks use 16-bit arithmetic: ia16-gcc got the 32-bit
  shifts wrong there.
- D7: the pointer from the keyboard (F6 or Alt-M; arrows, Space or Enter,
  Esc) for machines with no mouse.
- D7: a text editor on the desktop, written for Alci (`edit.c`): window as
  large as the screen, gap buffer in far memory (files up to 60 KB), open
  and save through a new file dialog (`filedlg.c`), scrolling both ways,
  a click puts the cursor. Home, End, PgUp, PgDn and Delete read from the
  console.
- D7: the editor asks before closing an unsaved text (Save, Discard,
  Cancel).
- D7: a shell window: /bin/sh on a pseudo-terminal, a separate ELKS
  process scheduled by the kernel; the event loop waits on its pty only
  while the window is open. Four icons fit in one column on every screen.
- D7: up to three shell windows at once, text in far memory and drawn
  lines in the heap while open (desktop heap 12 KB). `ls` and `ps` follow
  `COLUMNS`, so their output stays in columns in the 38-column windows of
  the 320x200 screens.
- D7: Ctrl-Q asks first when the editor has a text not saved or a shell a
  program. Editor and shell timed on the 8088 (`ctk -B`): a key drawn in
  66-104 ms (was 380-630), the changed cells only; the shell draws a busy
  window five times a second and scrolls in video memory, `cat` of an
  80-line file as fast as on the text console.
## 2026-11-26

- D7: file manager (`files.c`, Files icon, `ctk -f [dir]`): sorted list in
  far memory, open (directories entered, files to the editor), copy,
  rename, delete with Cancel as default, new directory; keys, buttons and
  mouse. About moves to the menu, so four icons stay in the column. The
  editor moves its rows in video memory when the view scrolls by less
  than half a window (a line past the bottom: 365 ms on CGA, 467 ms on
  EGA on the 8088); `ctk_gfx_scroll` works both ways. The file dialog
  returns full paths.
- D7: two fixes found by the timings. CTK drew, with every queue of
  widgets, the old entries a redraw of the whole window had left behind
  (`patches/ctk-redraw-queue.patch`): two editor lines more for every key.
  The queue holds 16 widgets (was 4): on VGA the short last page of a
  file overflowed it after a scroll, and the window was drawn twice.
  A letter now takes 57-86 ms on the 8088, a page on VGA 1.37 s (was
  1.85 s). `ctk -B` times a real scroll (lines up past the top).
- D7: `ctk-t1200` for the Toshiba T1200 LCD: CGA mode 6 with the bytes
  not inverted (dark pixels on the panel), the 56x32 T1200 icon set,
  pointer and shadow for its pixel shape; `desk` recognises the T1200
  BIOS. Checked on 86Box (desktop, file manager with the mouse, editor,
  shell, calculator): a letter 23 ms, a page 0.30 s on its 8086.
- D2: GUI benchmark (`alci/tests/guibench`): clients for Nano-X
  (`nxbench`) and GEM (`gembench`), `ctk -G` for CTK, helpers for start
  time (`stamp`, `vwatch`) and free memory (`memlater`); Nano-X and
  Gem-elks built outside their trees. Measured on MartyPC with CGA and
  EGA: CTK starts in 4.2 s, opens a window in 0.77 s and draws a line in
  40 ms on CGA; Nano-X 6.6 s, 2.2 s, 0.45 s; Gem-elks 17.6 s, 4.7 s,
  3.1 s, and leaves 21K free. Gem-elks needs a font fix for CGA, at the
  128 KB code limit. Results in `RESULTS.md`, `alci/docs/gui-study.md`.
- D7: a window in front is moved by copying it in video memory, as
  Nano-X does (`patches/ctk-window-move.patch`, `ctk_gfx_move`): only the
  desktop it uncovers and its frame are drawn, and the frame too is copied
  when the desktop round it is the same at both places. The D2 move takes
  177 ms on CGA and 465 ms on EGA on the 8088 (was 1077 and 2515; Nano-X
  842 and 1000). The desktop is filled a row at a time (EGA: a block per
  shading band, 245 ms down to 9 ms for 48 cells). Escape now closes a
  menu: the benchmark pressed it and left the menu open in two rounds of
  four, so the first CTK row of D2 is replaced (`RESULTS.md`).
- D7: the whole title bar moves a window and the close box answers where
  it is drawn (left on the two-colour screens, right on the colour ones;
  CTK put both buttons at fixed places, so most clicks on a mono title
  missed). A dragged window keeps the point where it was taken. Alt-F4
  closes the window in front: the kernel's keyboard driver passes Alt and
  a function key on as `ESC ESC a`..`j` when it does not switch console
  (`Console_set_vc` now says whether it did); before, Alt-F1 to F8 were
  always swallowed.
- D7: desktop keys as on Windows and Xfce. Alt-Tab shows the windows in a
  box while Alt is held and brings the chosen one to front when Alt is let
  go (Shift-Alt-Tab goes back); Alt-Esc sends the front window to the
  back; Alt-F4 on an empty desktop asks to leave; the Windows key opens
  the menu. The kernel's keyboard driver sends them as `ESC ESC` and a
  letter, and `ESC ESC r` when Alt is let go after Alt-Tab. The desktop
  holds the console while on the screen (`DCGET_GRAPH`), so Alt-F1 to F8
  no longer switch to a text console under it. Bringing a window to front
  no longer draws the old front window whole (frame and focused widget
  only): 2.2 s down to 0.9 s on CGA, 5.1 to 2.7 s on EGA with the Editor.
- D7: Alt-Space opens the menu of the window in front, as on Windows and
  Xfce: Move (then the arrows, any other key puts it down), Send to back,
  Close. The kernel sends it as `ESC ESC s`.
- D7: less drawn again. A window brought to front that nothing covered
  and that missed no change gets only its frame and focused widget; CTK
  notes the windows whose widgets changed while they could not be drawn
  and draws those whole. The Alt-Tab box and the window menu keep what is
  under them and put it back. Blank ends of text rows are filled as rows.
  On the XT: a window opened 530 -> 205 ms on CGA (1380 -> 930 on EGA),
  Alt-Tab to a window behind the Editor 910 -> 240 ms (2670 -> 1280).
- D7: text written straight to video memory: a run of letters on a plain
  background is drawn by one loop (assembly on CGA, EGA and VGA) instead
  of cell by cell. Same pixels, checked by comparing screenshots before
  and after in every mode. The window of 58x14 letters is drawn in 209 ms
  on CGA (was 639) and 425 ms on EGA (was 1227); a quick Alt-Tab to the
  Editor 570 ms on CGA (was 1140).
- D7: frames and windows drawn again for less: composed frame cells kept
  and reused (Alt-Tab draws the same frames again), side cells reused on
  EGA/VGA/MCGA too, repeated frame cells and blanks filled as rows or
  blocks, full-width label rows not cleared first. Same pixels in every
  mode. Alt-Tab to the Editor: 572 -> 335 ms on CGA, 1832 -> 600 ms on
  EGA; to the calculator behind it 1227 -> 477 ms on EGA.
- D7: hidden copy of the windows: a window covered by another is copied
  away as it is on the screen and copied back when it comes to front
  unchanged (video memory past the screen on EGA/VGA, far memory on the
  CGA-like screens, none on Hercules and MCGA). Same pixels as drawing it
  again. Alt-Tab to the Editor: 335 -> 155 ms on CGA, 600 -> 315 ms on
  EGA; to the calculator behind it 190 -> 125 and 477 -> 227 ms.
- D7: Hercules keeps the window copies in its second page (B800, mapped
  and never shown), when the card has one: Alt-Tab to the Editor 820 ->
  280 ms, to the calculator 650 -> 280 ms. Same pixels (86Box).
