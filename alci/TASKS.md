# Alci: tasks in order

Status: `TODO`, `DOING`, `DONE`, `DROPPED` (tried, not worth it),
`HUMAN` (needs the maintainer), `BLOCKED` (waiting on something).
The agent takes the first `TODO` from the top, except for the order
agreed with the maintainer on 2026-11-19: F4c (done), then E7 (network
time, time zone, summer time), then F4e (added 2026-11-21), then F4d,
then B1 (memory survey, block cache first: 64 KB of buffers in main
memory, added 2026-11-22), then D2, I2 and D7 (GUI benchmark on Nano-X,
Gem-elks and Contiki CTK, moved forward 2026-11-21, Contiki added
2026-11-22), then D5 (boot logo), then I1, I3 and I4 (integrations).

## Phase 1: baseline speed

- [x] A0 `DONE 2026-11-16` EDD code released at boot when unused (chosen over a build
  option: one image for every machine). `bios_edd_rw()`
  lives in `.fartext.edd`, placed by `elks-small.ld` just before
  `.fartext.init`; `kernel_init()` frees from there when
  `bios_edd_drives == 0`. Checked: XT, T1200, PC1512, AT on 86Box (EDD
  absent, code freed, suite identical to upstream), QEMU (EDD present,
  code kept), and `alci/tests/bigdisk.sh` (1 GB disk, partition at
  cylinder 1190, written and read back across two boots). First use of
  the discardable-code mechanism planned in D3.
- [x] A1 `DONE 2026-11-16` Program start-up. Profiling the `pwd` loop showed
  37% of the time in `fmemcpyw`: `fork` copied the whole 22 KB data segment
  of the shell, mostly the empty gap between the break and the stack.
  `seg_dup_data()` now copies only [0, brk) and [sp - 32, end). 10 runs of
  `pwd`: 2.00 s -> 1.58 s cold, 1.68 s -> 1.30 s warm. Three-stage pipe
  4.75 s -> 4.33 s. No extra RAM. Shell behaviour checked identical
  (background jobs, pipes, subshells, command substitution, `sh -c`).
- [ ] A1b `HUMAN` The remaining fork cost (~17% of the `pwd` loop) is the
  shell's real data and bss, 8.3 KB per fork. Option: `vfork` in ash for
  simple commands. Risky: the child must not touch shared data before
  `exec`. Study how dash does it before trying. Code changes wait for the
  maintainer's go-ahead after the study in A1c.
- [x] A2 `DROPPED 2026-11-17` Timer. Measured: the 100 Hz tick costs 1523
  cycles, 3.2% of an 8088 at 4.77 MHz (not 7.6%). Trial patch (skip
  `serial_bh` with no port open, CPU usage far call every 2 s instead of
  every tick, no `schedule()` in the idle loop when nothing is runnable):
  idle tick -21%, benchmarks -0.2% to -1.0%, mean 0.46%, value 0.28.
  Rejected by the gain threshold. Tickless idle would save power only, not
  time. Details in `RESULTS.md`. Could be reconsidered as part of a wider
  power-saving task. Also measured on 86Box (IBM XT, hard disk, screen
  console): mean gain 1.1%, value 0.67, still below the threshold.
- [x] A2b `DONE 2026-11-17` Line reading. libc: `memchr` in assembly
  (`repne scasb`), `fgets` copies whole runs from the stdio buffer with
  `memchr` and `memcpy` instead of one `getc()` per character. `grep`
  (minix1): `get_line()` does the same, keeping its exact behaviour (long
  lines, NUL bytes, truncation when memory runs out). `grep` on 20 KB
  3.685 s -> 2.482 s (-33%), no benchmark slower, kernel unchanged. `sort`
  (minix3) reads with `read()` and gains nothing; `sed` (minix3) reads one
  `getchar()` at a time through a look-ahead variable shared by the whole
  program, left for A2d.
- [x] S1 `DONE 2026-11-19` Serial input lost characters on the IBM XT (8250
  without FIFO, 19200 baud: one byte every 521 us). Measured with a probe
  in 86Box's 8250 (`alci/tests/86box-overrun.patch`, `ALCI_OVERRUN=1`):
  every lost byte had waited behind the timer interrupt, which kept IRQ 0
  in service through `timer_tick()` and, every 5th tick, through the BIOS
  IRQ 0 handler called from the interrupt path; a system call entry (IF=0)
  just before made it longer. Fix in `49b6c795`: EOI before the timer
  handler, BIOS IRQ 0 handler moved to a bottom half (`BIOS_BH`, so it
  also runs during kernel init, where the XT-IDE BIOS waits on the tick
  count). Longest wait of a byte 643 us -> 384 us, lost bytes in 300
  typed commands 3 -> 0 (ELKS `df2a61c5`: 638 us, a lost byte stalled the
  run at command 42; with the patch 377 us, none). Cost +29 bytes of code,
  +32 bytes resident. Patch for ELKS in `alci/upstream/`, not sent. The
  test drivers still type whole lines at once, as asked.
  Remaining margin: 137 us. What is left in the 384 us is the system call
  and interrupt entry with IF=0 in `_irqit` and the entry of the serial
  handler itself; a later task could shorten those (MartyPC meter
  `ELKS_IRQOFF`). At the console, the keyboard handler keeps IRQ 1 in
  service for up to 1 ms, which blocks COM1 too (seen on MartyPC).
- [x] S2 `DONE 2026-11-19` `clock -w` on AT-class machines after 1999:
  confirmed on 86Box IBM AT and ECS 386 (register 9 = 0xC6 for 2026, not
  BCD). Fixed in `958f9203`: two-digit BCD year, and the IBM AT century
  byte (CMOS 0x32) written only where it already holds 0x19 or 0x20.
  2026 -> 0x26, 1999 -> 0x99 with century 0x19; `clock -r` reads both
  back. Patch for ELKS in `alci/upstream/`. Seen and left alone: the day
  of week is written as 3-9 (Sunday = 3), which 86Box's RTC also uses
  but is outside the 1-7 range of a real MC146818.
- [x] S3 `DONE 2026-11-19` Out of memory on 86Box `at` (286, 512K) with the
  fd720 image (`sh: not enough memory` in functional command 9). Cause: the
  data segment size included the exact length of argv and envp, so
  `sh -c` got 48 bytes more than the shell that forked it; the segments
  freed by the forked copies were too small for the next exec and memory
  fragmented. Whether it showed depended on earlier commands, hence the
  results traded between ELKS and Alci, floppy and hard disk. `exec` now
  rounds the data segment to 256 bytes: 6 failures -> 0, hard disk still
  34/34. Cost +17 bytes of code, +16 bytes resident, about 128 bytes per
  process. Also applies to ELKS (`alci/upstream/`).
- [x] S6 `DONE 2026-11-22` `clock -c` accepted impossible dates (31
  February, 31 April) and rolled them into the next month, and years past
  2037 that overflow the 32-bit time. Now it checks the real length of
  each month, 29 February in leap years, and 1970 to 2037, and says "No
  such date" before asking again. +112 bytes in `clock`.
- [x] S5 `DONE 2026-11-22` Kilo (on the 1.44M set and the hard disk
  install) lost the file being edited when a save failed: it removed the
  file before writing the new text, so a full disk left a truncated file
  (reproduced on Alci: 930 bytes became 1024 bytes of new text, none of the
  original lines left). It also wrote one byte past each line at every
  save, overflowed a 256-byte stack buffer when copying a line of 255
  characters or more, read through a null pointer for files without
  syntax highlighting and in a search with no match, copied file names
  into a 128-byte field with a limit of 200, and lost memory when opening
  another file. Now the text goes to `kilo.tmp` in the same directory and
  replaces the file only after it is fully written (rename, or a copy
  where rename is not possible); the other bugs are fixed. Program +416
  bytes on disk, no kernel change. Test: `alci/tests/kilo-save.py`; ELKS
  patch in `alci/upstream/`.
- [ ] S4 `PARKED 2026-11-21` Damaged plain (uncompressed) programs: `exec`
  checked the file length and the relocation type, but not the entry point,
  the high words of the segment sizes, nor where a relocation writes. A
  relocation past its segment wrote into other memory and left the system
  dead on most runs (86Box xt, t1200, pc1512, at; MartyPC 8088); a bad
  entry point hung the program. Now rejected before the program runs.
  +84 bytes of far kernel code, no speed change. Test:
  `alci/tests/exec-corrupt.py`; ELKS patch in `alci/upstream/`. Random
  bytes inside a valid program are not caught (only a whole-file checksum
  would, at a cost on every start); package hashes at install time (I3)
  cover downloads. Parked (decided 2026-11-21): today no Alci program has
  relocations, so the patch (commit `79c7bc34`, reverted) costs 80 bytes
  of resident kernel memory for nothing yet. Apply it again, with the
  `exec-corrupt.py` check, in the same change that brings the first
  medium-model program into Alci (for example elks-w16, a medium-model
  Nano-X build, 8086tiny or the PicoMEM tools: I2, I4, D6).
- [x] S9 `DONE 2026-11-22` Console output invisible after a graphics program
  (Alci regression from hardware scrolling, c1ea1006): a program that sets a
  video mode through the BIOS (going back to text too) resets the CRTC start
  to 0 and the BIOS copy at 0040:004E, while the console kept writing at its
  scrolled start, outside the visible page, until the next scroll. Seen on
  MartyPC with the Contiki prototype (D7): the shell prompt never appeared;
  affects every graphics program (`paint` in 0.1, Nano-X, ELKSWeb,
  Gem-elks). Now each console write first compares 0040:004E with its own
  start and, if they differ, goes back to the start of its region. +80 bytes
  of near kernel code (approved 2026-11-22: covers all programs, including
  ones that end badly). MartyPC: `cat` of 200 lines 1.509 -> 1.522 s warm,
  4.887 -> 4.913 s first, `ls -l /bin` 7.756 -> 7.760 s; strict suites
  34/34 on QEMU hd32 and fd720. Not an ELKS bug (ELKS has no hardware
  scrolling).
- [ ] S7 `TODO` `ftpd` after an upload closes the data connection
  normally; another ELKS fork changes it to a reset, to stop control
  blocks piling up in `ktcp` during many uploads. Not proven: in an
  upload the client closes first, so the server should not wait long,
  and a reset changes what clients see. First the test only: 100
  uploads in a row from the host on QEMU, `netstat` and the `ktcp` heap
  before and after. Fix only if blocks are really left behind (as an
  integrity fix if `ktcp` runs out of memory). The TCP accept and
  `SYN_RECV` fixes of the same fork are already in Alci (ELKS, August
  2026).
  Same test, from the fork census (2026-11-22): connections never
  accepted or stuck in `SYN_RECEIVED` have no timeout of their own; and
  the `ftp` client, while still open, may get 426 replies from some
  servers (a 13-line change in another fork, not in ELKS).
- [ ] S8 `TODO` Kernel stack overflow (`KSTACK`) seen in another ELKS
  fork on a real IBM PS/2 with asynchronous I/O and 700-byte kernel
  stacks, the Alci configuration; cause not isolated. Try to reproduce
  on 86Box (heavy disk and serial I/O together) before anything else.
- [ ] A2d `TODO` `sed` and `uniq` line reading: read whole lines (`fgets` or
  the A2b scan) instead of one `getchar()`/`getc()` per character. In
  `sed` the current character is a global look-ahead used by the parser
  too: change only the input of `ourgetline()`. Measure with a `sed`
  substitution on 20 KB and `uniq` on sorted input.
- [ ] A1c `TODO` Study for A1b, no code changes: how dash uses `vfork`
  for simple commands and what the child may touch before `exec`; whether
  the ELKS kernel has `vfork` or what it would need; the expected gain on
  the 10 runs of `pwd` and on pipes, estimated from the A1 profile (8.3 KB
  copied per fork). Write the findings in `RESULTS.md` and a plan for ash
  in this file; the maintainer decides whether to go on.
- [ ] A2c `TODO` Idle loop without the wake-up race. Today a task woken
  between `schedule()` and `hlt` waits for the next interrupt (up to 10 ms
  with no other activity). Do `cli`, check the run queue, call
  `schedule()` only when a task is runnable, then `sti; hlt` (atomic on
  x86). Measure latency, not throughput: time from a key press or a byte
  on the serial port to the process running, and power at idle (share of
  time in `hlt`). Also measure on its own the CPU usage change from the A2
  trial (far call every 2 s, -2 bytes) and keep it if it is free.

- [ ] A3 `DOING` Boot from floppy. Measured 2026-11-19 (Alci `010d6729`,
  720K, MartyPC `e1c5187` with `ELKS_FDROT=9`, `ELKS_BOOT_TEXT=login:`,
  `ELKS_FDTRACE=1`): XT 22.8 s to `login:`, of which BIOS POST 4.0 s,
  boot loader 6.9 s (kernel 104 KB), kernel start 1.7 s, the rest 10 s
  reading init, scripts and programs. File order: upstream's
  `image/Image.all` (small boot files next to the inode table, kernel
  after) is already good; putting `bootopts` and `linux` first was 0.4 s
  slower (22.8 -> 23.2 s). Where the time goes instead:
  1. The boot loader goes back to cylinder 0 three times while loading
     `/linux` to re-read the inode block (about 0.5 s each).
  2. It reads one head per call and waits about 50 ms for the other head.
  3. In the kernel, track reads start at sector 1 and wait 150-190 ms for
     it on most new tracks.
  Point 1 done in `6d338ea9` (2026-11-19): the boot loader keeps the
  inode block; boot to `login:` 22.81 -> 22.21 s on the 8088, 18.79 ->
  18.19 s on the 8086; boots on every 86Box machine and QEMU. Needs a
  test on the real T1200 (boot block). Next: point 2, then 3.

- [x] A5 `DONE 2026-11-19` Release network card drivers that find no card,
  as done for EDD. Driver operations are far (`struct eth_farops`); the
  NE2000, WD8003 and 3C509 drivers keep their run-time code in their own
  aligned sections, given back at boot when the card is not found. Freed:
  NE2000 1904, WD8003 1952, 3C509 1744 bytes; image +656 bytes, so 4.9 KB
  more free memory with no card and 3 KB with one card. Tested with an
  NE2000 (QEMU, 86Box PicoMEM) and a WD8003E (86Box): `ktcp`, `urlget`,
  same speed. Not tested with a 3C509: 86Box's 3C509B is not found by
  either build. SMC Ultra and PCnet are compiled only (far operations,
  resident). See `RESULTS.md`.
- [ ] A4 `TODO` Adopt os8088's MartyPC patch set (github.com/jggonz/os8088,
  `tools/martypc/`, MIT): 17 patches plus `debug_server.rs`, pinned to
  MartyPC `e15cb04f` (2026-06-28). Checked 2026-11-16: all 17 apply in
  order on that pin and the headless binary builds. They do not apply to
  our base `05c0d08` (2026-09-07). Gains: a debug server (breakpoints,
  registers, exact cycle counts, no code in the guest), PicoMEM emulation
  (lets E0b be tried without buying the card), fixes for V20 flags, the PIT
  and floppy timing. Work: rebase our four `elks-bench` commits (keyboard
  FIFO, FDC rotation, 8086 overlay and turbo, ELKS runner) onto pin plus
  patches, rerun the reference benchmarks and record any change in timing.
  Keep credit to os8088 in the branch README.

- [ ] A6 `TODO` 86Box fork: `proxybarrio/86box` exists (created
  2026-11-18 from `86Box/86box`) and the scheduled agent has access. The
  agent creates branch `alci` from the fork's `master`, commits os8088's PicoMEM
  patch (`tools/86box/patches/07-picomem.patch`, MIT, credited in the
  commit and in `RELATED.md`), checks that `setup.sh` builds from it, and
  from then on follows the emulator watch (`RESULTS.md` "Emulator
  updates"). Later changes of ours to 86Box (for example serial
  measurements for S1) go on that branch.

## Phase 2: memory

- [ ] B1 `TODO` Measure and document where kernel memory goes (code, data,
  heap, buffers, cache) in `alci/docs/memory.md`. Start with the block
  cache: today 64 KB of L2 buffers in main memory plus 8 KB of L1
  (`CONFIG_FS_NR_EXT_BUFFERS=64`, `buf=` in `/bootopts` overrides it).
  Measure, on 256, 512 and 640 KB machines, the memory left to programs
  and the hit rate and timings (boot, first and second start of a
  program, `ls -l`, a file copy, an editor save) with 16, 32 and 64
  buffers, on floppy and hard disk. Output: a table and a proposal for
  the default per machine, for the maintainer; no change before that.
  Also `ktcp`: three static buffers of about 4.4 KB sized on the receive
  window instead of the MTU (another fork recovers about 9 KB with a few
  lines); measure with the network up, since Bobcat (E6) and the GUI
  candidates need that memory.
- [ ] B2 `TODO` Elastic disk cache: reads only, writes go to disk at once,
  flushed on mount/umount and media change, at most 64 KB of conventional
  memory and given back to programs on demand; in EMS or extended memory
  when present. First a design document in `alci/docs/`, then the
  implementation in steps.
- [ ] B3 `TODO` Memory beyond 640 KB (added 2026-11-22, after B1, with B2).
  Two kinds: extended memory (XMS, above 1 MB, 286 and later only) and
  expanded memory (EMS, a bank-switched board, which works on an 8088 too:
  Lo-tech EMS, Intel Above Board, AST, and PicoMEM, which emulates one).
  ELKS already has XMS support (`CONFIG_FS_XMS`: block cache and RAM disk
  above 1 MB, through unreal mode, INT 15h or LOADALL, detected at boot);
  Alci has it off. Measured 2026-11-22 on Alci `acf26ba9`: turning on the
  XMS block cache adds 3856 bytes to the kernel (2496 near code, 1008 far
  code, 240 data, 112 bss), resident on every machine, XT included. EMS:
  nothing in ELKS; another fork has a `/dev/ems` storage device
  (`RELATED.md`). Study, no code: what each kind would give (block cache,
  RAM disk, text cache, `/tmp`, program data), on which machines, how the
  code can cost nothing where the memory is absent (freed after detection
  as in A0 and A5, which today works for far code only), and the cost of
  the alternative, one kernel per machine class. Output for the
  maintainer: `alci/docs/extra-memory.md`. Decided 2026-11-22
  (`STRATEGY.md`): both, code freed when the memory is absent and kernels
  per machine class where a class gains; B3 gives the figures for each.

## Phase 3: tests on varied hardware

- [ ] C1 `TODO` Test kit: 360 KB, 720 KB and 1.44 MB images with all patches,
  plus a benchmark script that runs on ELKS and prints the times.
- [ ] C2 `HUMAN` Run the kit on real machines (Toshiba T1200, an XT, an AT)
  and report the numbers. Verify two-head floppy reads.

## Phase 4: graphics

- [ ] D1 `TODO` Study PC/GEOS (fork `proxybarrio/pcgeos`), os8088 and
  SymbOS for ideas only: memory management, discardable segments, fonts,
  window clipping and redraw on demand, CGA drawing routines. PC/GEOS is
  Apache 2.0: it can ship as a separate program under its own licence, not
  inside the kernel or linked with GPL code (see `STRATEGY.md`); os8088 is MIT, so its code may be adapted with its notice kept,
  except `reference/` and fonts of unclear origin. Output:
  `alci/docs/gui-study.md`, with what can be reused and under which terms.
  Also study Gem-elks (GEM desktop on ELKS, GPLv2) and elks-w16 (Win16
  programs on Nano-X, MIT): see `RELATED.md`.
- [ ] D2 `TODO` GUI benchmark (decided 2026-11-21): the base of the desktop
  is chosen on measurements, not impressions. Write the benchmark once
  (`alci/tests/guibench/`) and run it on Nano-X first; I2 (Gem-elks), D7
  (Contiki CTK, a full candidate) and D6 (PC/GEOS, if feasible) run the
  same one. Machines: MartyPC XT 8088
  CGA 640x200 (reference), 86Box PC1512 and AT with EGA. For each
  candidate, with its files compressed and uncompressed:
  1. Disk space of everything it needs (programs, fonts, icons,
     resources).
  2. Memory: resident while the desktop is open and idle, and what is left
     for one program (a terminal or an editor).
  3. Start time of the desktop from a 720K floppy, a 1.44M floppy and the
     hard disk.
  4. Drawing: full-screen redraw, opening and closing a window, moving a
     window, opening a menu, a line of text and a full window of text.
  5. Response: time from a key or mouse click to the screen changing.
  6. Screenshots of the same screens on each machine, for the look.
  Output: a table in `RESULTS.md` and the screenshots in
  `alci/docs/gui-study.md`, for the maintainer's choice of base.
- [ ] D6 `TODO` PC/GEOS on Alci as a separate program: feasibility study,
  no code (decided 2026-11-21; after D1, before I2). Licence rules in
  `STRATEGY.md` (Apache 2.0, separate program, no GPL code linked).
  Questions: which DOS calls and direct hardware accesses the GEOS kernel
  makes; a DOS-to-ELKS layer (the Gem-elks port did the same for GEM;
  look for existing DOS-call bridges first, `RELATED.md`); how GEOS gets
  its own heap of several hundred KB (`fmemalloc`); which toolchain
  builds it and whether ELKS can load the result or needs a loader; how
  much memory stays free. Compression: GEOS programs and its data files
  (fonts, resources, geodes) compressed with the most efficient method
  under `ACCEPTANCE.md` (LZSA1 as today, or another codec if measured
  better on the 8088), decoded by GEOS's own loader, a small shared
  library, or the kernel's LZSA1 decoder through one small whole-file
  call (see `STRATEGY.md`); compare their cost. Output: `alci/docs/pcgeos-port.md` with
  the work needed, for the maintainer's decision; if feasible, D6 runs
  the D2 benchmark.
- [ ] D7 `DOING` Contiki CTK desktop on Alci, a full candidate for the base
  (decided 2026-11-22, with D2 and I2). Contiki 1.x (Adam Dunkels,
  BSD 3-clause, `RELATED.md`) ran a graphical desktop with windows,
  menus, icons and a mouse on the Commodore 64: 64 KB of RAM, a 1 MHz
  6502, 320x200 bitmap. An 8088 at 4.77 MHz has a similar raw speed and
  runs C code far better than a 6502; against it, a CGA screen is 16 KB
  to the C64's 9 KB and the C64 had video help (sprites, colour memory).
  It may beat Nano-X and Gem-elks; only D2 can say. Plan:
  1. One user-space program, the same for ELKS and Alci, no kernel patch.
     Take from 1.x the C parts: `contiki/ctk/ctk.c` (windows, menus,
     focus), the event kernel and the applications; write the drawing
     layer behind `ctk-draw.h` for CGA, since the C64 one (`ctk-hires`) is
     6502 assembly. Keep the look of the 1.x desktop; `contiki-win32` and
     `contiki-gtk` show how a non-C64 port was done. Contiki 2.x `core/ctk`
     is a fallback if 1.x proves hard to build; the two APIs differ.
  2. Screen: CGA 640x200 two colours first (80 columns, all CGA, EGA and
     VGA), then 320x200 four colours; EGA later. Video mode set the way the
     ELKS console allows, screen restored on exit and on errors.
  3. Input: keyboard first, then the ELKS mouse driver. No busy loop when
     idle.
  4. Network and files: the applications go through ELKS files and
     sockets, never a second TCP/IP stack (no uIP). Order: about and
     calculator (first build), editor (first real test), then the web
     browser, compared with ELKSWeb and Bobcat (E6), then FTP, IRC, e-mail.
  5. First milestone: two overlapping windows, a menu, an icon, a working
     button, redraw after a move, return to the console; then the D2
     benchmark, files compressed and uncompressed.
  Licence: BSD keeps the notices in every file taken; each application and
  data file checked on its own. Output: the program under `alci/contiki/`,
  results in `RESULTS.md`, the look in `alci/docs/gui-study.md`.
  Progress (2026-11-22): 1.x chosen over 2.x after building both on
  Linux (2.x draws its windows with characters on 8-bit machines; only
  1.x has the graphical desktop). First prototype in `alci/contiki/`:
  CTK core, event kernel, About and calculator, a CGA 640x200 drawing
  layer in C after the C64 hires driver, keyboard; 13,328-byte program,
  no kernel change; desktop, menus, dialogs and calculator work on QEMU.
  Next: timings on MartyPC and 86Box, editor, file manager, mouse.
  Look and screens (decided 2026-11-22): one program and one CTK core, a
  drawing layer per adapter chosen at start, each with its own theme drawn
  for its pixel shape: CGA 640x200 on a CRT and on the Toshiba T1200 LCD
  (pixels of a different shape: see `alci/docs/screens.md`), CGA 320x200
  four colours, EGA 640x350, VGA 640x480 (16 colours). Themes are 8-byte
  cell patterns as today, so a better look costs no memory or speed. First
  step: a CGA theme of our own (thin borders, striped title bars, rounded
  buttons, shadows), icons for the CGA pixel shape, a finer 8x8 font.
  Later, on modern PCs (phase 7, frozen), the same desktop in a richer
  form that keeps visible traces of the 8088 one (the family of programs
  from the smallest machine to the largest).
  Icons (decided 2026-11-22): on the 8088 the Contiki icons keep their
  shape and idea, only a little cleaner; the stronger the machine, the more
  refined, same idea. A set per screen profile (P1-P13, private brief in
  alci-private/icone/), drawn for each pixel shape and colour count; the
  disks carry only the family defaults, the other sets are packages that
  ept installs on request, like the display drivers of old Windows.
  Contiki 1.x has only 5 distinct icon drawings for its 16 programs (and
  an empty one for the shell): every program gets its own.
  Icon sets done (2026-11-22), in `alci/contiki/icons/`: CGA 640x200
  (80x32, drawn by hand), CGA 320x200 four colours (40x32, by hand),
  640x400 (96x64), VGA 16 colours (104x80), modern 64 and 128 (PNG), each
  as C arrays and as one `.ais` file. The CGA desktop draws the new
  10x4-cell icons (checked on QEMU: drawing, focus, opening a program).
  T1200 LCD (56x32), Hercules (80x42) and EGA 640x350 (72x42, 16 of the
  64 EGA colours) sets added the same day. Next: the desktop theme
  (windows, menus, buttons) per screen family; choose the icon set from the
  screen family once the other drawing layers exist.
  Wallpaper is a user option (`ctk -w dots|pairs|slash|none`, default
  dots); later also a Preferences item in the Desktop menu that saves it.
  Theme styles (decided 2026-11-23), two families. Two-colour screens (CGA
  640x200, T1200 and other LCDs, Hercules, 640x400, VGA mono) get their
  finish from shape: striped title bar on the active window with the
  title centred on a plain plate, close box on the left, inactive bars
  plain; corners cut by one pixel on windows, buttons and selections;
  solid shadow instead of the checker; default button with a ring;
  rounded screen corners; selected icon label in an inverse pill; plain
  menu and status bars with a dividing line. Dithered shading was tried
  and dropped: on 200-line screens it reads as noise. Colour screens get
  the rich style: two-tone or gradient title bar with a gloss line that
  stops clear of the title, soft or stepped shadow, rounded corners (flat
  tones and stepped shadow on 16-colour cards). On 8-pixel cells every
  bar uses a 6x6 bold font from the second row, so no letter touches a
  bar edge.
  Before closing D7, ask which screen modes are still missing and get icons
  and theme for them: Tandy 1000 / PCjr (320x200 16 colours, 640x200 4
  colours), MCGA 320x200 256 colours, Super VGA (800x600 and 1024x768, for
  tests on modern PCs). Desktop background ideas: a mountain strip in the
  bottom 8 text rows (640x64, 5 KB raw or about 3 KB as 302 tiles plus map,
  copied like the desktop pattern, loaded from a theme file and skipped on
  small machines); a background computed by formula only on 386 and VGA.
- [ ] D3 `TODO` Design the kernel loader for discardable code segments:
  document first, then a prototype.
- [ ] D4 `TODO` Alci desktop: improve the base chosen after D2, I2 and D7
  (Nano-X, Gem-elks or Contiki CTK; look and behaviour) in the
  spirit of GEOS, the way Haiku follows BeOS. GEOS existed on the Commodore
  64 (Berkeley Softworks, 1986) and later on the PC as PC/GEOS; today only
  the PC/GEOS source (FreeGEOS, Apache 2.0) is available to study. Rules:
  1. Ideas, behaviour and general style may be taken from GEOS on any
     platform (menus, dialogs, font handling, redraw strategy). They are not
     protected by copyright.
  2. No GEOS code, icons, fonts or bitmaps go into Nano-X or any GPL code:
     Apache 2.0 is not compatible with GPLv2. Write our own from scratch
     after studying the design (clean-room: the notes in
     `alci/docs/gui-study.md` describe behaviour, never copy code).
  3. os8088 (MIT) code may be adapted, keeping its copyright notice in each
     file taken and a line in `RELATED.md`.
  4. "GEOS" is a trademark: never name the Alci desktop GEOS, and never copy
     its screens pixel by pixel. Say "inspired by GEOS".
  Depends on D1, D2 and I2.
- [ ] D5 `TODO` Boot logo (decided 2026-11-20). Image: the maintainer's
  320x200 drawing (two moose, a penguin wearing antlers, sun, ALCI, a PC
  with `#`), CGA mode 5, ready as `alci/img/alci-logo-cga5.bin` (16 KB,
  2.4 KB with LZSA1). After the BIOS mode set, port 3D9h = 10h for the
  bright palette (and bit 2 of 3D8h, as mode 5 already sets it). On a
  composite monitor mode 5 shows in shades of grey. Off at first boot; `logo on` / `logo off` turn it on and off
  (a line in `/bootopts`; on a write-protected disk 1 the command says
  so). When on, `rc.sys` runs its work through a small `bootlogo`
  program: it saves the text screen, shows the logo until the end of
  the boot and keeps the boot output; a key pressed at any time brings
  back the text screen with everything written so far (before the logo
  and during it) and the boot goes on in text. The date question
  (`clock -c`) and `login:` always come in text. Skipped without CGA,
  EGA or VGA. Image 16 KB, 2.6 KB compressed with LZSA1; expected cost
  about 0.7 s on the XT floppy boot, 0 bytes of memory after the boot.
  A kernel message printed while the logo is up ends the logo as a key
  would (decided 2026-11-20): `bootlogo` watches the kernel message
  queue (the one `dmesg` reads) and on a new message brings back the
  text screen, the boot output and the message. No kernel code; the
  message may draw a few wrong pixels for a moment before that. Measure on MartyPC (XT) and check on
  86Box XT, AT, 386 and QEMU.

## Phase 5: Claude terminal

- [ ] E0 `TODO` Reuse before writing: ELKS already ships `miniterm` (raw
  keyboard to serial link, no terminal emulation) and, under Nano-X, `nxterm`
  (terminal emulator with ANSI sequences, for the desktop phase). Check
  whether an existing ESP32 firmware that already talks to Claude
  (AiBridgeMCP Nano) or a Wi-Fi modem firmware (Zimodem) plus `miniterm`
  gives a working Claude terminal with no new code on the ELKS side. Read
  their documentation and licenses; record the result in
  `alci/docs/claude-terminal.md`. If it works, E1 and E2 shrink to small
  improvements.
- [x] E0b `DONE in emulation 2026-11-16` Network without the serial port.
  86Box IBM XT (8088 4.77 MHz) with an emulated PicoMEM 2 (os8088's 86Box
  patch `07-picomem.patch` and `libpmhost`, both MIT): ELKS found the card's
  NE2000 at 300h/IRQ 3 with its existing driver, `ktcp` came up, `ping` to
  the host answered and `urlget` fetched a file over HTTP. No new kernel
  code. Images: `BOOTOPTS_EXTRA='ne0=3,0x300,,0x80' mkimg.sh`; run with
  `PICOMEM=<libpmhost> t86.py xt <label>-fd720.img` (not the hd32 image:
  XT-IDE also sits at 300h). Next: HTTPS through a proxy on a modern
  machine, then the real card.
- [ ] E0c `TODO` Encrypted link from Alci to our own relay (why:
  a plain proxy leaves the Alci-to-proxy hop in clear). Anthropic's API
  only accepts TLS and nothing can be installed on its side, so the relay
  is ours (home Raspberry Pi, the ESP32, or a small server). Design: a key
  shared once between Alci and the relay, each message sealed with
  ChaCha20-Poly1305 from Monocypher (BSD-2-Clause or CC0, compatible), no
  public-key handshake on the 8088. The Claude API key stays on the relay.
  Steps: build Monocypher's ChaCha20 and Poly1305 with ia16-gcc, measure
  speed and size on MartyPC, write the message format in
  `alci/docs/claude-terminal.md`. Do not invent a cipher.
- [ ] E1 `TODO` Only if E0 is not enough: check whether Zimodem/WiFi232 can
  handle HTTPS to `api.anthropic.com`.
- [ ] E2 `TODO` Only if E0 is not enough: command-line `claude` client for
  ELKS (AT commands, HTTP request, minimal JSON), with a fake modem for
  tests. Look at doschgpt's request and reply handling first (license to be
  checked).
- [ ] E3 `HUMAN` Buy or build the Wi-Fi modem and try it.

- [ ] E5 `TODO` Native HTTPS as an option (decided 2026-11-17): a
  TLS client in ELKS itself, for machines with a network path (PicoMEM or
  another NE2000, or SLIP over the serial port) and no modem doing TLS.
  BearSSL (MIT; has an "i15" big-number code for 16-bit multiplies) is the
  first candidate. Rough estimate, to be measured: handshake 2-5 minutes on
  an 8088 at 4.77 MHz, about half on a T1200, 20-60 s on a 286; then a few
  KB/s; 50-100 KB on disk, 60-100 KB of RAM while running, nothing when not
  running. Every program using it shows a notice before connecting, with
  the expected wait scaled to the detected CPU. First step: build BearSSL's
  X25519 and RSA/ECDSA verify for ia16 and time them on MartyPC.
- [ ] E6 `TODO` Text web browser on top of E5. Current Lynx needs a 386 (DJGPP
  port); Bobcat, a Lynx derivative, ran on the 8088 under DOS. Bobcat is
  already ported: an ELKS external application since January 2026
  (`buildext.sh bobcat`, OpenWatcom, large model, Lynx base under the GPL;
  found 2026-11-22). First step: build it, check whether it is
  large-model in the a.out sense (if it has relocations, apply S4 first),
  and measure disk space, memory, start time and page load on an 8088.
  Priority (decided 2026-11-22): before compressing Bobcat or adding
  kernel code for it, look for a text browser that is already built for
  ELKS, faster or lighter, and use that if it does the same job.
  Candidates to compare (2026-11-22), ELKS-native first: ELKSWeb
  (graphical, original code, GPL-2.0-or-later, already an ELKS external
  app: HTML, forms, tables, GIF; measure first); Bobcat (text); the
  Contiki 1.x web browser on the D7 desktop (written for the C64, would
  use ELKS sockets); MicroWeb (DOS, 8088, CGA/EGA/VGA/Hercules, still
  developed, v2.1 March 2026, uses mTCP: a port would replace the network
  layer; check its licence file); DOSLynx and Arachne (older DOS browsers,
  check licence and CPU needs). The fork census (104 of 168 forks) found
  no other browser; the weekly reading of the ELKS discussions continues.
  HTTPS and images (decided 2026-11-22): all these browsers speak HTTP
  only. Two modes, both offered: text through a public service such as
  FrogFind (its address set in a configuration line, not in the program,
  in case the service changes or closes), and full pages through a proxy
  of the user's own (WebOne or similar on a PC, or a ready-made Raspberry
  Pi image prepared by us), which also shrinks pictures to what the
  screen can show: a small preview by default, the full picture on
  request (maps), or the whole page as one picture (Browservice style).
  The relay planned for E0c can be the same box.
  First results (2026-11-22, OpenWatcom snapshot, Bobcat 0.8): builds
  (243 KB, OS/2 NE format, not convertible by `os2toelks`: too many
  segments), so it needs `CONFIG_EXEC_OS2` in the kernel: +1696 resident
  bytes on every machine (48 near code, 1312 far code, 144 data, 192
  bss). On QEMU 640 KB it shows a local page; with the network up (`ktcp`
  alone) exec fails for lack of memory with 64 buffers and works with
  `buf=16`, which leaves 296 KB free; it completed an HTTP exchange. With
  `-dump` it clears the screen on exit. Decided 2026-11-22: on the hard
  disk image only, with the OS/2 loader in that image's kernel
  (`STRATEGY.md`, exceptions), and a commented `buf=16` in its
  `/bootopts`; floppy images without it. Goal still open: Bobcat with
  no extra kernel code, running with 64 buffers and the network on
  (smaller `ktcp`, B1; Bobcat's own memory use, with its author).
  Then measure ELKSWeb (graphical, CGA/EGA/VGA,
  HTTP only, GPL-2.0-or-later, see `RELATED.md`): memory, speed on an
  8088 with CGA, and what HTTPS from E5 would need to work with it.

- [ ] E8 `TODO` SSH client (decided 2026-11-19: wanted). ELKS has none.
  First a study, no code: existing small clients and their licences, the
  memory a client needs, and the time of the key exchange and host key
  check on an 8088, a T1200 and a 286 (measure the curve25519 and
  ed25519 or RSA code on MartyPC, as for E5). Output: size, RAM, waiting
  time, and on which disk it goes (disk 3 on 720K, and on 1.44M if it does
  not fit). Until then, a way that works today: `telnet` from Alci to a
  machine on the desk (a Raspberry Pi), and `ssh` from there.
- [ ] E7 `DOING` Clock from the network, time zone from the
  user. The clock is kept in UTC and the time zone is never fixed in the
  sources:
  1. Time zone: asked once (at install or first boot, from a short list of
     zone names) and stored as `TZ` in `/bootopts`; changeable later.
  2. Time: a small SNTP client sets the clock in UTC at network start. It
     asks the nearest source first: the NTP server given by DHCP (option
     42) or the gateway, which many routers serve, then a public pool
     (pool.ntp.org) or our relay (E0c). No single private server.
  3. Plausibility check: if the network time differs from the RTC (where
     present) by more than a set margin, report it instead of setting it
     silently.
  4. Order at boot: if the machine clock is valid, use it (and correct it
     from the network when a network is present). If it is clearly wrong
     (earlier than the build date, as on an XT without a real-time clock,
     which starts at 1970), take the time from the network; ask the user
     for date and time only when no network time can be obtained. Boots
     with a valid clock, or with a working network, never stop to ask.
  The commented `date -s` example in `/etc/net.cfg` stays as it is; this
  task adds an alternative. Measure size of the client (target
  under 4 KB) and keep it out of memory when no network is present.
  Point 4 done 2026-11-19, the question part: `clock -c` in `rc.sys`,
  after the network start, asks for the date when it is older than the
  day `clock` was built; Enter keeps it, no answer lets the boot go on
  after 60 s; a typed date is also written to the RTC. Next: the SNTP
  client (points 1-3); the `net.cfg` example is unchanged.
  Decided 2026-11-19: time from NTP servers run by time institutes (INRIM,
  then PTB, then NIST), asked by `ktcp`, which already speaks UDP for
  DHCP; the question at boot only when none answers. Summer time is
  computed in one place only: `clock`, at boot and whenever the time is
  set, turns the POSIX `TZ` rule (Italy: `CET-1CEST,M3.5.0,M10.5.0/3`)
  into the current offset, the next change and the offset after it, kept
  by the kernel next to the `struct timezone` it already holds; `libc`
  takes the offset from `gettimeofday` instead of parsing `TZ` (today it
  only knows whole hours, no summer time). Measure the kernel and `libc`
  cost first. Floppy systems: the zone is asked once and saved in
  `/bootopts` on disk 1; on a write-protected disk 1 it stays UTC and a
  line at boot says so, without asking each time.
  Point 2, the client, done 2026-11-20: `ktcp -t ip,ip,...` sends an
  SNTP request once the address is known (static, or DHCP bound), to the
  server from DHCP option 42 first, then the `-t` list; two tries 2 s
  apart per server. Answers are taken only from the server asked, in
  server mode, synchronised, stratum 1-15, echoing our request; a time
  before the build of `ktcp` is refused. The clock is set in UTC with
  half the round trip added; a correction above 1000 s of a clock that
  was valid is printed. `ktcp` +1136 bytes code, +112 data, +32 bss; no
  resident cost (only while `ktcp` runs). `/etc/net.cfg` gets
  `ntp="-t 193.204.114.232,192.53.103.108,129.6.15.28"` (INRIM, PTB,
  NIST), passed by `/bin/net`: IP addresses, since names cannot be looked
  up before `ktcp` runs; check them on a real network.
  Still to do, in this order:
  Decided 2026-11-20:
  - Server order: the `-t` list (INRIM, PTB, NIST) first; only if none
    answers, the local network's server (DHCP option 42, else the
    gateway). Today DHCP's server goes first: change it in E7b.
  - The first boot on a real network prints which servers answered, so
    the addresses are checked there.
  - A large jump from a clock that was valid is applied and printed
    (point 3 closed).
  - The two floppy benchmarks moved by the layout shift are accepted;
    the layout is left to A3 and F0.
  - E7b: `clock -c` runs right after `net start`, before the answer can
    arrive (up to 12 s with three silent servers). Make the boot wait for
    the network time (or for `ktcp` to give up) before asking, and write
    the RTC after a network time is set (`clock -w`). While it waits, a
    message says which key combination skips the wait, for when it takes
    too long; the key must work on the console and on a serial terminal.
    Include the server order and the per-server report above.
  - E7c: time zone and summer time (point 1 and the `TZ` decisions
    above): measure the kernel and `libc` cost first.

## Phase 6: universal image

Principle (`STRATEGY.md`): wherever a hard disk is present, Alci is
installed on it in full; the disk set and the repository exist for
installation and for floppy-only machines.

Rules for the floppy set (decided 2026-11-19):
- No program leaves the 720K base disk until disk 2 exists in the same
  change, with the disk swap working on every machine, the XT included
  (XT drives give no disk-change signal: the system asks for the disk and
  checks a label on it, it never relies on the signal).
- Disk 1 is chosen first (F0), then disk 2 and further disks: disk 3 for
  other applications, for example the SSH client (E8). The same holds for
  1.44M images if a program does not fit.
- Disks 2, 3... carry whatever they need to be used on their own while
  disk 1 is out of the drive; whether that includes a copy of the kernel
  is decided in F0 (it costs about 104 KB per disk).
- Every change of layout is measured before and after (`bench.sh`,
  `bench2.sh`): the position of files on the floppy changes timings.
- Disk 1 also keeps the communication programs: `telnet`, `telnetd`,
  `ftp`, `ftpd` and `miniterm` (communicating is part of the base).
- Every distribution (720K, 1.44M and any other floppy set) includes the
  installer that puts the whole set on a hard disk (F3 step 1), on disk 1.
- 720K set (decided 2026-11-19, replaces the earlier split): disk 1 is
  the current 720K image, unchanged. Disks 2 and 3 hold only programs
  that are not on disk 1: disk 2 everything else that exists today
  (archives, `wc head cmp sleep xargs yes`, `diff find`, script helpers,
  system and disk maintenance, printing, games, `basic`, `paint`, sound:
  about 500 KB), disk 3 the SSH client (E8) and later additions. Disks
  2 and 3 carry a small kit (`ls cat cp more mount umount`, about 27 KB)
  and a label. Uses: installing everything on a hard disk from 720K
  floppies (T1200), reaching the extra programs without a hard disk.
- `wc head cmp sleep xargs yes` stay off disk 1 (2026-11-19): with them
  disk 1 would have 23 KB free instead of 41, and that space is the
  working room of a floppy-only system: `vi` keeps its temporary file in
  `/tmp` (about the size of the text being edited), `sort` too, and the
  user's own files go there. Editing a 20-30 KB text would fail.
- Nothing is split before the disk manager (F5) works.
- Free space is part of the design: disk 1 keeps enough room for `/tmp`
  and everyday work (a minimum to fix in F0, checked by the build), and
  the split is accepted only when the floppy suite passes 34/34 with the
  strict checker on the real three-disk set (no timeouts, no "No space
  left"), on every machine, with one drive and with two.

- [x] F0a `DONE 2026-11-22` Disk 2 of the 720K set, first version, for
  the 0.1 preview: every file of the 1.44M image that is not on disk 1
  (72 programs, about 620 KB, already compressed; 19 KB free), built by
  `alci/scripts/mkdisk2.sh`, with `alci/disk2/` (README, `setup` for a
  second drive: PATH and links for `/lib`; `install-hd` to copy it onto a
  hard-disk system). Disk 1 unchanged. Not bootable: usable with two
  drives or for a hard disk; with one drive only after F5 (safe swap).
  Checked on QEMU, two drives and hard disk: `bc cal tr md5sum tar head
  uptime advent`, data files found.
- [ ] F0 `TODO` Floppy budget. The 720K image (Alci 3ecae5b) had 669 of 710
  data blocks in use, 40 KB free; with compressed programs (F4b) disk 1
  has 121 KB free (Alci `70b577df`). 720K is the drive of a real T1200.
  List every file with its size and the task it serves, and propose a base
  set (kernel, shell, core tools, networking, the package fetcher) that
  leaves room, with the rest moved to packages. The proposal says, for each
  program, disk 1, 2 or 3, and what each disk needs to work alone. The
  maintainer decides; nothing moves before F3 step 3 works.
- [ ] F2 `TODO` Package repository ("alci-get"): software fetched once ELKS is
  running and online, installed on a hard disk or a second floppy. An index
  with a SHA-256 hash per package, fetched over HTTPS (E5) or through the
  relay (E0c); packages may then come over plain HTTP and are checked
  against the index before use. Hosted on the proxybarrio fork (releases or
  a branch).
- [ ] F3 `TODO` Multi-disk set (2026-11-17, after GEOS on the
  C64): disk 1 boots and holds the base system, disks 2, 3... hold the rest.
  A catalogue on disk 1 maps each program to its disk (the same catalogue
  can point to the package repository, F2). In this order:
  1. Installer that copies the whole set to a hard disk, asking for each
     disk in turn. This is the main path: on any machine with a hard disk,
     the floppies are installation media only. It goes on disk 1 of every
     distribution. Reuse first: ELKS already has `sys` (a script that
     copies the booted system to `/dev/hda1` and similar), `fdisk`,
     `mkfs` and `makeboot`; the installer adds the questions, the
     partition step and the copy of disks 2, 3...
  2. Two floppy drives: further disks are mounted from B: on request.
  3. One drive, no hard disk, last: the root filesystem lives on disk 1,
     so a swap needs kernel work (flush and invalidate the cache for the
     drive, keep the root read-only while it is out, load the program into
     memory before asking for disk 1 back) and XT drives give no
     disk-change signal. Test on 86Box and on a real T1200 before use.
  Rule: every program fits on one disk (RAM is the stricter limit anyway).
  Placement decided 2026-11-22: `kilo` (compressed, about 20 KB) goes on
  disk 2 of the 720K set and stays on disk 1 of the 1.44M image; it is not
  added to the 720K disk 1.
- [x] F4 `DONE 2026-11-19` The functional suite on the 720K floppy, with the
  strict checker. Done:
  0. Compressed programs (`CONFIG_APPS_COMPRESS`) measured on MartyPC and
     not adopted: 130 KB freed, but every compressed program starts slower
     (`vi` 5.1 s -> 40.4 s on the XT, 3.6 s -> 20.0 s on the 8086 at
     7.16 MHz; the kernel decompressor is slow). Figures in `RESULTS.md`.
  1. Layout change tried in `59478e75` and reverted in `0249f4d7`: it took
     programs off the base disk before disk 2 exists, left `ftpd` started
     by `/etc/net.cfg` but absent, and made some reads slower (file
     positions). The 720K disk keeps its program set until F0 and F3.
  2. `alci/tests/functional-fd.txt`: the same commands on small files; the
     programs not on the base disk come from a test disk made by
     `mkimg.sh` (`<label>-fd720b.img`), mounted on `/mnt`. `tar-ok` fails
     when the extraction directory cannot be entered.
- [ ] F5 `TODO` Disk manager (decided 2026-11-19), before the disk split.
  Design document first (`alci/docs/disks.md`), for the maintainer:
  1. Every disk has a label: name, kind (system, programs, documents) and
     a unique number, read to check that the right disk is in the drive
     (XT drives give no disk-change signal, so the system asks and then
     checks the label, never trusting the signal).
  2. A catalogue on disk 1 (and a copy on each program disk) says which
     disk holds each program. A command that is not on the disk in use
     gets an answer like "tar is on disk 2 Tools: insert it and press
     Enter"; the program is loaded, then disk 1 is asked for again.
  3. Document disks, called "Personal Storage" with a sequential number,
     in FAT format so that any PC can read them (decided 2026-11-19):
     any blank floppy can become one, and gets the
     next free number ("Personal Storage 1", "Personal Storage 2"...),
     kept in the list of known disks on disk 1; the number is also in its
     label, so two of them are never confused. Its path is short, without
     spaces and within the 14 characters of a MINIX name (`/disks/ps1`).
     When the program has finished with it, the system asks for the
     disk that was there before: disk 1 with one drive, disk 2 in drive B
     with two. All of F5 is for machines without a hard disk. Saving to it must work from every program, not only
     those with a menu: the plan to evaluate is a path per disk name
     (for example `/disks/ps1/letter.txt`), so that any program (`vi`,
     `cp`, later the desktop's save dialogs) can save there, and the
     system asks for that disk when it is needed. Everything pending is
     written before a swap; disk 1 stays read-only while it is out.
  4. Two drives: disk 1 stays in drive A, disk 2 in drive B. When disk 3
     or a Personal Storage disk is needed, the swap happens in drive B,
     then disk 2 is asked for again; disk 1 never leaves drive A, so this
     case needs no kernel work. One drive: every swap happens in drive A.
     Hard disk: the same names point to folders, nothing to swap.
  Study first: AmigaOS volume names and "Please insert volume" requests,
  GEOS disk handling (ideas only, see `RELATED.md` rules), and ept (MIT,
  see `RELATED.md`): with a network, a program missing from disk 1 could
  be downloaded instead of asking for disk 2. Then the steps
  in order: labels and the `disk` command, the catalogue and the
  missing-command message, document disks on two drives, then single
  drive swaps (kernel work, F3 step 3). Test on 86Box XT, T1200, AT.
  What the kernel does today with a swapped floppy (`directfd.c`, checked
  2026-11-22, same as ELKS): the change is detected only on AT-class
  controllers (disk change line, `FD_DIR`); then every queued and later
  request to that drive is discarded until `check_disk_change()` runs,
  which unmounts the disk without checking the result, drops its cached
  inodes and blocks (pending writes are lost, not written to the new
  disk) and panics ("Root media changed") if it is the root. On an XT
  nothing is detected: cached blocks of the old disk are read as if they
  belonged to the new one, and pending writes can land on the wrong disk.
  So the design must: write and invalidate everything for the drive
  before asking for a swap; check the label of the disk that comes in
  before any access, on every machine (the signal is a hint at best);
  never leave dirty blocks for a disk that may be taken out; turn the
  root case into "insert disk 1" instead of a panic only if the kernel
  can wait safely, otherwise keep a clear stop. Acceptance tests, on
  throwaway images (86Box XT and AT, one and two drives): swap two disks
  of the same format with files open for reading and for writing, with
  pending writes, a missing or write-protected disk, the root disk taken
  out; after each, reboot and compare checksums of both disks to prove no
  block went to the wrong one.
  Approved design (2026-11-22), the same principle MS-DOS used on drives
  without a change line, for every machine and the only protection on an
  XT: (a) no pending writes for a floppy once its motor stops: everything
  for that drive is written before the motor-off timeout, so a disk that
  can be taken out (light off) never has data waiting; (b) after every
  motor-off period, before any cached block of that drive is used or
  written, read the identity of the disk (FAT volume serial; for MINIX
  disks a serial number written with the F5 label) and compare it with
  the one recorded at mount; if it differs, drop the cache for the drive
  and ask for the right disk. Cost: one sector read only when the drive
  was idle (the motor already needs about half a second to start), plus
  some hundred bytes in the floppy driver, to be measured. Not covered: a
  disk pulled out while the light is on, which no system can protect.
- [x] F4b `DONE 2026-11-19` Compressed executables that load fast. After
  trying LZ4 and LZSA1 (three rounds, `RESULTS.md`): LZSA1 (the LZSA
  project's 8088 decoder, zlib) plus a text cache that keeps exited
  programs in free memory. On by default in `alci/elks.config` since
  `57265faf`, floppy images only: 720K disk 41 -> 123 KB free, faster
  starts from floppy and repeated starts, 34/34 everywhere. Exception
  approved by the maintainer 2026-11-19: `grep` +3.2% in `bench2.sh`
  (first start from a floppy without rotation delay; faster with it) and
  +1.9 KB of resident kernel.
  Follow-ups: file order on the floppy (A3) now that files moved;
  hard disk images later; (`meminfo` shows kept texts as KEPT and
  counts them as free, done 2026-11-19); with the
  space gained, fewer disks in the 720K set (decided 2026-11-20: yes;
  F0 fixes the number).
- [x] F4c `DONE 2026-11-20` Robust LZSA1 decoding (from a review of F4b).
  The decoder trusted the block; on QEMU 7 of 12 damaged copies of `grep`
  hung the system, and the host model (`tests/lzsa1-model.py`) showed
  writes over the 64 KB above the segment. Now long copies are checked
  before they start and the output after each short match; a damaged
  block returns 0 and exec rejects it. With `9d1b6868`: none of the 12
  hangs (10 rejected, 1 stopped by `SIGSEGV`, 1 runs), the model finds no
  write outside the segment in 2500 corruptions. Decoder 74 bytes
  smaller; starts +0.1% to +1.1%. A damaged block that still decodes to
  the right length runs with wrong code, as an uncompressed program
  would; no checksum for floppies, downloaded packages will carry a hash
  (E0c, F5).
- [ ] F4e `TODO` Compression for programs from other projects (decided
  2026-11-21, see `STRATEGY.md`). Build ELKSWeb and Gem-elks, the largest
  candidates, with `buildext.sh`, and measure each compressed
  (`elks-pack.py`) and uncompressed: disk space, start time from a 720K
  and a 1.44M floppy and from the hard disk (MartyPC 8088, 86Box XT and
  AT), peak memory while running, and package size for transfer over a
  slow link. Check what an unmodified ELKS kernel does with a compressed
  program: it must refuse it cleanly, never run it; if it does not,
  propose a header marker that makes it refuse. Output: the per-program
  rule in `RESULTS.md`, and a design note for the prerequisite an ELKS
  system installs before using the Alci repository (the kernel decoder,
  as a patch or package), with its cost in bytes. Codecs to compare on
  the same programs: LZSA1 (today), ZX0 (Emmanuel Marty's 8088 decoder
  `unzx0_x86`, zlib licence: better ratio, decode speed to be measured),
  LZPACK's 8 KB-window LZ (ratio and
  decode speed on the 8088), and self-extracting ELKS programs (UPX, when
  its ELKS support exists), which would run on an unmodified ELKS at the
  cost of a stub and a decode at every start (see `RELATED.md`).
- [ ] I1 `TODO` ELKSWeb in Alci (after F4e; see `RELATED.md`). Built
  outside the tree with `buildext.sh`, compressed or not as F4e decides,
  on the 1.44M set and the hard disk install (not the 720K disk 1).
  Checks V1-V6, memory and speed on an 8088 with CGA. Replaces the text
  browser plan of E6 if it fits; HTTPS from E5.
- [ ] I2 `TODO` Gem-elks as a desktop candidate (after F4e, with D1 and
  D2; if any candidate is a medium-model program, apply S4 first): run
  the D2 benchmark on it, with its programs and its resource
  files (`.rsc`, `.icn`, fonts) compressed and uncompressed. Output in
  `RESULTS.md` and `alci/docs/gui-study.md`; the choice of base for D4 is
  the maintainer's.
- [ ] I3 `TODO` Alci package repository for ept (after F4e and F5 design):
  layout, which programs it carries (disk 2 and 3 programs first),
  compressed or not per F4e, a hash per file, the prerequisite check for
  ELKS systems (`STRATEGY.md`), and where the sources of GPL programs are
  published. Hosting is decided by the maintainer. Repository order on
  Alci (decided 2026-11-21): the Alci repository comes first (last `REPO=`
  line in `/etc/ept.cfg`), the ELKS repository second. The installer
  records which repository each package came from; every upgrade checks
  the order again, so a package taken from ELKS moves to the Alci build as
  soon as Alci publishes one. When ELKS has a newer version than Alci, the
  Alci version stays until the new one is rebuilt, measured and published
  in the Alci repository. The
  user can pin a package to the ELKS repository by hand. Check whether ept
  already records the origin of a package; if not, add it (MIT).
  Later (not in the first version): variants of one package per CPU class
  (8086 baseline always present, 186 or 286 builds only where measured
  faster), the installer picking one and falling back to the baseline when
  the disk moves to a smaller machine; see F1 and B3.
- [ ] I4 `TODO` picomem-elks tools (after E0b on real hardware): build
  with `buildext.sh`, on the 1.44M set and the hard disk install.
  These tools are medium model: apply S4 first. PicoMEM can also act as
  an EMS board: the EMS storage device of another ELKS fork (`RELATED.md`)
  is studied in B3, measured in kernel bytes first.
- [ ] F4d `TODO` Text cache, small fix: a cached program file that is
  unlinked without being opened for writing (`mv` over it) keeps its
  disk blocks until the entry leaves the cache (checked on QEMU: `sed`
  replaced by `mv`, its 14 KB came back only after five other programs
  ran). `rm` is not affected (it checks write access, which drops the
  entry). Drop the entry when the link count reaches zero.
- [ ] F1 `TODO` CPU detection at boot and choice of routines (8088, 186,
  286); 286 extended memory for the disk cache.

## Phase 7: modern PCs (very long term, FROZEN)

Not to be started, by the agent or anyone, until the maintainer declares phases 1-6
finished and satisfactory on 8088 to 80386 machines. Recorded only so that
earlier choices (single image, code loaded only where needed) stay
compatible with it.

- [ ] G1 `FROZEN` Standalone USB stick for modern PCs: MINIX system
  partition used as a hard disk, FAT data partition shared with the
  modern OS; same contents as the CF/XT-IDE image for old machines.
- [ ] G2 `FROZEN` Boot on UEFI-only PCs (no legacy BIOS): a layer that
  provides BIOS services on top of UEFI.
- [ ] G3 `FROZEN` LLM connection on machines without a serial port:
  minimal USB driver to reach the ESP32 modem as a USB serial device,
  loaded only on machines that need it.

## Done so far (session of 2026-11-15)

See `alci/RESULTS.md`. Patches are on branch `alci`, one commit each.
