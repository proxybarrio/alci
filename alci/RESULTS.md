# Alci: measured results

MartyPC (cycle-accurate), IBM 5160 with GLaBIOS, CGA, 640 KB, 8088 at 4.77 MHz
unless stated otherwise. Scripts: `alci/scripts/bench.sh` and `bench2.sh`.

| Operation | Original | Alci | Main patches |
| --- | --- | --- | --- |
| `cat` of 200 lines to screen | 17.7 s | 1.54 s | fast console path, hardware scrolling, block tty_write |
| `ls -l /bin`, cached | 10.4 s | 4.1 s | namei, _wake_up, iget, time conversion, fputs |
| `ls -l /bin`, from floppy | 14.7 s | 7.9 s | as above |
| Three-stage pipe, 20 KB | 3.86 s | 1.13 s | 512-byte pipe buffer |
| `grep` on 20 KB, cached | 3.59 s | 2.89 s | block copy in fputs |
| 10 runs of `pwd` | 2.26 s | 1.58 s | various, fork copies only the live parts of the data segment |
| Boot to prompt, with floppy rotation model (`ELKS_FDROT=9`) | 21.0 s | 18.4 s | block reads in boot loader, two-head floppy reads |

On an 8086 at 4.77 MHz the relative gains are the same; where the floppy
dominates they are slightly smaller.

## Costs

Measured by comparing object sections with an upstream build of the same
commit. Code in `.fartext.init` is freed after boot, so it is counted apart.
(An earlier estimate said +550 bytes, a later one +1706 counting init code:
both were wrong.)

- Kernel, resident after boot: +1352 bytes before the EDD change below.
  - `console-direct.o` +428 (fast console, hardware scrolling).
  - `bios.o` +268 (EDD, large disks), now released at boot when no disk
    uses EDD: the EDD read/write code (186 bytes) sits in `.fartext.edd`,
    just before `.fartext.init`, and is freed with it. On machines without
    EDD the resident kernel ends 8 paragraphs earlier (`init 163d`
    instead of `1645`).
  - `malloc.o` + `fork.o` +216 (fork copies only the live data).
  - `sleepwake.o` +129, `bioshd.o` +99, `ntty.o` +54, `main.o` +47
    (banner), `fmemory.o` +40, others +71.
- Kernel init code (freed after boot): +354 bytes.
- Programs using `fputs`: about +150 bytes; programs using time conversion: +80 bytes.
- Pipes: 432 more bytes of RAM per open pipe.
- Boot sector: uses the 69 bytes that were still free.

## Experimental

- Two-head floppy reads (`bioshd.c`): large files read 15% faster, small
  files 2-3% slower; falls back automatically if the BIOS refuses
  multi-track reads.

## Confirmation on 86Box (2026-11-15)

86Box 6.0, IBM PC/XT, 8088 at 4.77 MHz, IBM CGA, 3.5" 720 KB floppy, images
`elks-originale-720.img` (original) and `alci-720.img`. Times measured with
`time` inside ELKS.

| Operation | Original | Alci | Gain |
| --- | --- | --- | --- |
| Kernel start to login | 13.53 s | 12.37 s | -9% |
| `ls -l /bin`, first run (floppy) | 17.77 s | 10.00 s | 1.8x |
| `ls -l /bin`, second run (cached) | 12.17 s | 4.32 s | 2.8x |
| `cat` of `ls -l /bin /dev` output (~180 lines), first run | 12.32 s | 3.02 s | 4.1x |
| same `cat`, second run (cached) | 12.29 s | 1.23 s | 10x |

The gains match MartyPC (1.9x and 2.5x). Absolute times are higher in 86Box,
probably because it models the floppy and the display more closely.

## Functional tests on 86Box (2026-11-16)

Suite `alci/tests/functional.txt`, Alci against upstream at the same
commit, hard disk image on XT-IDE, real machine BIOSes:

| Machine | Result |
| --- | --- |
| IBM XT 1986, 8088 4.77 MHz, CGA, 640K | identical |
| Toshiba T1200, 8086 9.54 MHz | identical |
| Amstrad PC1512, 8086 8 MHz | identical |
| IBM AT, 286 8 MHz, EGA, 512K | upstream fails 2 of 8 parallel `sh -c` with "not enough memory"; Alci runs all 8 (processes end sooner, so fewer are alive at once) |

All four machines also boot the 720K floppy image.

## Speed on 86Box (2026-11-16)

`alci/tests/speed.txt` with `t86.py --times`, hard disk image, emulated
seconds, upstream / Alci. Console on serial, so screen output is not
measured.

| Command | XT 8088 4.77 | T1200 8086 9.54 | PC1512 8086 8 | AT 286 8 |
| --- | --- | --- | --- | --- |
| `ls -l /bin` (cached) | 16.68 / 9.44 (-43%) | 6.28 / 3.54 (-44%) | 7.25 / 4.09 (-44%) | 2.64 / 1.53 (-42%) |
| 10 runs of `pwd` | 2.78 / 1.75 (-37%) | 0.99 / 0.63 (-36%) | 1.14 / 0.73 (-36%) | 0.41 / 0.29 (-29%) |
| three-stage pipe, 48 KB | 10.22 / 3.07 (-70%) | 3.82 / 1.12 (-71%) | 4.40 / 1.29 (-71%) | 1.60 / 0.47 (-71%) |
| `grep` on 60 KB | 7.68 / 7.15 (-7%) | 3.08 / 2.88 (-6%) | 3.53 / 3.31 (-6%) | 1.44 / 1.36 (-6%) |
| `sort` small file | 0.90 / 0.68 (-24%) | 0.35 / 0.27 (-23%) | 0.40 / 0.31 (-23%) | 0.16 / 0.14 (-12%) |
