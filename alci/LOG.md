# Alci: work log

## 2026-11-16

- Project documents translated to English.
- Boot banner: "ALCI 0.1: Asymmetric Linux Compact Initiative", crediting
  ELKS, Alan Cox, Greg Haerr and the ELKS team.

## 2026-11-15, session

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
