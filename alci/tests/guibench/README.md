# guibench: the D2 GUI benchmark

The same measurements for each desktop candidate (task D2 in
`alci/TASKS.md`): Contiki CTK (`alci/contiki`), Nano-X (Microwindows) and
Gem-elks. Results in `alci/RESULTS.md`, screenshots and notes in
`alci/docs/gui-study.md`.

## What is measured

1. Disk space of everything a desktop needs, plain and packed with
   `elks-pack.py` (LZSA1).
2. Memory a program could still get with the desktop up and idle
   (`memlater`, which takes main memory in blocks until none is left).
3. Start time: from `stamp start` to the moment the screen stops changing
   (`vwatch`, which reads 256 bytes of video memory every 50 ms and
   ignores changes of fewer than 3 of them, a pointer drawn again).
4. Operations, averaged over 4 rounds, by a client of each desktop:
   the whole background drawn again, a window opened (3/4 of the screen
   wide, 2/3 tall, with a title), one line of 60 characters, the window
   full of text, one character, the window moved, a menu opened (CTK
   only: the others have no menu a program can open), the window closed.
   - CTK: `ctk -G` (in `alci/contiki/main.c`); everything is in one
     process, so the time is the time to the screen.
   - Nano-X: `nxbench.c`; after each operation a round trip to the server
     (`GrGetScreenInfo`) waits until it has drawn: the server and its
     window manager draw frames and background.
   - GEM: `gembench.c`; AES calls return when the server has done them,
     and a `WM_REDRAW` for the window is handled before the time is taken.
     The desktop must have no window open (`desktop.inf` without the two
     `#W` folder lines), as gemdesk would redraw its own later.

## Files

| File | What |
|---|---|
| `nxbench.c`, `gembench.c` | the clients for Nano-X and GEM |
| `stamp.c`, `vwatch.c`, `memlater.c`, `textmode.c` | helpers on the test disk |
| `ctkq`, `nxq`, `gemq` | start time |
| `ctkb`, `nxb`, `gemb` | operations |
| `ctkm`, `nxm`, `gemm` | memory |
| `report`, `run` | results on the screen; `run ctk` or `run nx` does all |
| `nxbuild.sh` | builds Nano-X outside its tree (the tree is only read) |
| `gembuild.py` | builds Gem-elks outside its tree, and `gembench` |
| `gem-elks-cga-font.patch`, `gem_mouse_amstrad_stub.c` | the GEM build that starts on a CGA machine (below) |
| `image.sh` | a test disk from a base image and `src:/dest` pairs |

## Running

Build Nano-X (`nxbuild.sh microwindows/src OUT cga`, or `vga`, which
also drives EGA with `EGAMODE=1`) and Gem-elks (`gembuild.py gem-elks
OUT gembench.c`), then make a 720K disk with `image.sh` holding the
programs, the scripts and the helpers (GEM also needs `/lib/gemsys` and
`/etc/gem.conf` from its `assets`). On MartyPC the commands are typed by
`martypc_headless` (`ELKS_CMDS`); `VSEG=a000` for EGA and VGA, and
`GUIBENCH_N=1` makes gembench do one round.

GEM needs care on a 640 KB 8088:

- Once GEM is up, there is no memory left to start a program (21K free,
  largest block 4K): `gembench -w` and `memlater` are started before GEM
  and wait, and they stop GEM themselves (by the pid in `/tmp/gem.pid`).
- The first connection to the server takes the desktop's channel, so a
  program must not connect before gemdesk: `gembench -w` waits for the
  screen to settle first.
- The server does not stop on SIGTERM (it spins), and after a SIGKILL GEM
  did not start again in the same boot: boot once for `gemq`, once for
  `gemb`, once for `gemm`.
- On a CGA machine Gem-elks asks the BIOS for its 8x8 font with INT 10h
  AX=1130h, which only EGA and VGA BIOSes have, and stops ("cannot open
  display or font"). `gem-elks-cga-font.patch` takes the font at
  F000:FA6E, where every PC BIOS has it; with it the server's code passes
  the 128 KB of the medium model by 32 bytes, so the benchmark build also
  leaves out the Amstrad mouse driver (`gem_mouse_amstrad_stub.c`). The
  EGA measurements use Gem-elks as published.
