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
