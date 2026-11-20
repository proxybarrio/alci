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
