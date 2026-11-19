# Alci tests

Functional tests check that Alci behaves exactly like upstream ELKS. Speed is
measured separately on MartyPC (`alci/scripts`).

- `mkimg.sh <tree> <label>`: builds `<label>-fd720.img`, `-fd1440.img` and
  `-hd32.img` with the console on the serial port and `/bin/sh` as init.
- `functional.txt`: the suite, one command per line, under 60 characters
  (the ELKS tty input line is 80 and the drivers add a marker).
- `functional-fd.txt`: the same suite for the 720K floppy image, on small
  files (`/tmp` is on the floppy). The programs it needs that are not on the
  720K base disk (`tar compress md5sum test_libc test_float`) come from a
  second disk, `<label>-fd720b.img` (made by `mkimg.sh`), mounted read-only
  on `/mnt` by the first command. Give it to the drivers as `FD2=<image>`
  (`st86.py` and `sq.py`).
- `qtest.py <img> <fd|hd> <cpu> <mem_kb> <suite>`: runs the suite on QEMU.
- `t86.py <machine> <img> <suite> [--times]`: runs it on 86Box with the real
  BIOS of an IBM XT (8088 4.77 MHz, CGA), Toshiba T1200 (8086 9.54 MHz),
  Amstrad PC1512 (8086 8 MHz), IBM AT (286 8 MHz, EGA, 512K), `xtv20`
  (IBM XT with a NEC V20) or `386` (ECS 386, AMI BIOS, 386DX 16 MHz). Hard disk
  images go on an XT-IDE controller. `--times` prints emulated seconds per
  command. Needs 86Box 7.0 built with `-DAUTOMATION=ON` (`B86`) and the
  86Box ROM set (`R86`), see `alci/scripts/setup.sh`.

- Toshiba T1200 with its internal hard disk: `t86.py t1200 <label>-hd20t.img`
  with `HDGEOM=17,8,306,mfm,internal` (the BIOS presents it as 611/4/17).
- Screen mode: build images with `CONSOLE=screen mkimg.sh ...` and run
  `t86.py` with `SCREEN=1`: commands are typed on the emulated keyboard and
  completion is read from the text screen, so console speed is measured on
  each machine's own display (`KEYDELAY` ms between keys, 20 on the AT).
  Images get `/b.txt`, 200 lines of text.
- Network: `PICOMEM=<path to libpmhost-v2.so>` (and `PICOMEM_SD`) adds an
  emulated PicoMEM card to the 86Box machine, its NE2000 bridged to 86Box's
  network (slirp; the host is 10.0.2.2). Needs os8088's 86Box patch, see
  `alci/scripts/setup.sh`. Build the image with
  `BOOTOPTS_EXTRA='ne0=3,0x300,,0x80'`; use a floppy image, since XT-IDE
  also uses port 300h.
- `bigdisk.sh <hd32 image>`: INT 13h extensions test on QEMU (1 GB disk,
  data past cylinder 1024, written and read back across two boots).
- `t86.py` options: `BOOTLOG=word,word` prints boot lines containing the
  words; `BOOT_MS` boot timeout. `qtest.py`: `EXTRA_HD` second disk,
  `NOSNAP=1` keeps writes.

Procedure: build images for the patched tree and for the previous commit (a
git worktree), run the suite on each machine, diff the outputs. They must be
identical, except where upstream fails and Alci does not (record it in
`RESULTS.md`).

## Strict checking

`qtest.py` and `t86.py` exit 0 even after a TIMEOUT, ignore a missing
prompt, and are normally judged by comparing with ELKS output, so a failure
shared by both systems passes. For acceptance use the strict tools:

- `sq.py <image> <fd|hd> <cpu> <mem_kb> <suite> <rawlog>`: QEMU driver that
  checks the echo of each command and the return of the prompt, lists the
  commands not run, and exits 1 on any problem.
- `st86.py <machine> <image> <suite> [--times]`: the same for 86Box (same
  machines and options as `t86.py`; `RAWLOG=<file>` saves the serial log).
- `check.py <suite> <output> <image>`: checks every command of
  `functional.txt` against expected values computed from the image itself
  (file lists, checksums, /etc/passwd), and reports TIMEOUT, SKIPPED,
  NOECHO, ECHOBAD, NOPROMPT and wrong output as FAIL. Exit 1 on any FAIL.
  It also takes `functional-fd.txt` (the floppy probe message of the
  `mount` in its first line is allowed, and `tar` may report a record size
  under 20 for the small archive).

## Serial latency probe

`86box-overrun.patch` (apply to 86Box, measurement only): with
`ALCI_OVERRUN=1` the 8250 logs each receive overrun with the CPU state
sampled at every bit time while the byte waited, the longest wait of a
byte in RBR (`MAXWAIT`) and waits over 400 us (`LONGWAIT`). Read them with
`DEBUGLOG=maxwait,overrun,longwait st86.py ...`. On MartyPC the runner has
`ELKS_IRQOFF=<step cycles>` (see `run_elks.rs`).

