# Alci: measured results

## Reference measurement (2026-11-16)

Every comparison must name both versions. This one compares:

- **ELKS**: 0.9.2, ghaerr/elks commit `22a4c5d` (master of 2026-11-15).
- **Alci**: 0.1, proxybarrio/elks branch `alci`, commit `3ecae5b`.

Same configuration (`alci/elks.config`) and toolchain (branch `toolchain`).
Each image reports its commit with `uname -a`. Results for later commits of
either branch must be measured again and added as a new dated section.

MartyPC (8088 4.77 MHz, CGA, 720K floppy, `alci/scripts/bench.sh` and
`bench2.sh`), emulated seconds:

| Operation | ELKS 22a4c5d | Alci 3ecae5b |
| --- | --- | --- |
| `cat` of 200 lines to screen | 17.71 | 1.51 |
| `ls -l /bin`, from floppy | 14.95 | 7.93 |
| `ls -l /bin`, cached | 11.08 | 4.06 |
| 10 runs of `pwd` | 2.26 | 1.58 |
| three-stage pipe, 20 KB | 7.09 | 4.34 |
| `grep` on 20 KB | 4.61 | 3.65 |
| `sort` on 20 KB | 5.55 | 5.45 |
| boot to prompt | 14.48 | 14.18 |
| boot to prompt, floppy rotation model (`ELKS_FDROT=9`) | 21.02 | 18.36 |

86Box 7.0, real BIOS, hard disk, serial console (`alci/tests/speed.txt`),
emulated seconds, ELKS 22a4c5d / Alci 3ecae5b:

| Command | IBM XT 8088 4.77 | Toshiba T1200 8086 9.54, internal disk | IBM AT 286 8 |
| --- | --- | --- | --- |
| boot to prompt (with BIOS POST) | 38.17 / 37.74 | 11.89 / 11.74 | 20.98 / 20.92 |
| `ls -l /bin`, first run | 17.17 / 9.86 | 6.45 / 3.69 | 2.77 / 1.65 |
| `ls -l /bin`, cached | 16.68 / 9.45 | 6.28 / 3.53 | 2.64 / 1.53 |
| 10 runs of `pwd` | 2.79 / 1.75 | 0.98 / 0.62 | 0.41 / 0.29 |
| three-stage pipe, 48 KB | 10.22 / 3.07 | 3.80 / 1.11 | 1.60 / 0.47 |
| `grep` on 60 KB | 7.69 / 7.16 | 3.04 / 2.85 | 1.44 / 1.36 |
| `sort` small file | 0.90 / 0.68 | 0.32 / 0.25 | 0.16 / 0.14 |

86Box 7.0, console on each machine's own display (`SCREEN=1`, commands
typed on the emulated keyboard, time from Enter until the output is complete
on screen), hard disk, emulated seconds, ELKS 22a4c5d / Alci 3ecae5b:

| Command | IBM XT 8088 4.77, CGA | Toshiba T1200 8086 9.54, LCD, internal disk | IBM AT 286 8, EGA |
| --- | --- | --- | --- |
| `cat` of 200 lines, first run | 21.06 / 2.30 (9.2x) | 7.61 / 0.82 (9.3x) | 4.57 / 0.42 (10.9x) |
| `cat` of 200 lines, cached | 20.57 / 1.89 (10.9x) | 7.46 / 0.68 (11.0x) | 4.46 / 0.34 (13.1x) |
| `ls -l /bin`, first run | 31.19 / 11.10 (2.8x) | 11.55 / 4.16 (2.8x) | 5.89 / 1.86 (3.2x) |
| `ls -l /bin`, cached | 30.71 / 10.69 (2.9x) | 11.38 / 4.00 (2.8x) | 5.78 / 1.77 (3.3x) |

## Earlier measurements

The sections below are kept as a record. Unless they say otherwise, they
compare ELKS at the upstream commit of the day (`3e6b940` on 2026-11-15,
`22a4c5d` on 2026-11-16) with the Alci patches as they were on that day;
use the reference section above for current figures.


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

## Toshiba T1200 with its own hard disk (86Box, 2026-11-16)

T1200 BIOS 019e, internal RLL adapter, 20 MB disk seen by the BIOS as
611 cylinders, 4 heads, 17 sectors (`hd20t` image from `mkimg.sh`, run with
`HDGEOM=17,8,306,mfm,internal`). No EDD in this BIOS: the EDD code is
released at boot. Functional suite identical to upstream.

| Command | Upstream | Alci |
| --- | --- | --- |
| `ls -l /bin` (cached) | 6.28 s | 3.53 s (-44%) |
| 10 runs of `pwd` | 0.98 s | 0.62 s (-37%) |
| three-stage pipe, 48 KB | 3.81 s | 1.11 s (-71%) |
| `grep` on 60 KB | 3.04 s | 2.85 s (-6%) |
| `sort` small file | 0.32 s | 0.25 s (-22%) |

## EDD release across CPUs (86Box and QEMU, 2026-11-16)

The EDD check runs for every hard disk the BIOS reports, with no CPU or
machine-class condition; the code is freed when no disk has EDD. Checked:

| Machine | CPU | Disk | EDD | Suite vs upstream |
| --- | --- | --- | --- | --- |
| IBM XT 1986 | 8088 4.77 | XT-IDE / floppy | no, freed | identical |
| IBM XT 1986 | NEC V20 4.77 (186 instruction set) | XT-IDE | no, freed | upstream drops one serial input character (`/etc/passwd` read as `/tc/passwd`), every run; Alci does not |
| Toshiba T1200 | 8086 9.54 | internal RLL 20 MB | no, freed | identical |
| Amstrad PC1512 | 8086 8 | XT-IDE | no, freed | identical |
| IBM AT | 286 8 | XT-IDE (AT) | no, freed | Alci better (see above) |
| ECS 386 (AMI BIOS) | 386DX 16 | XT-IDE (AT) | no, freed | identical |
| QEMU | 486 | IDE 32 MB + 1 GB | yes, kept | identical; data past cylinder 1024 written and read back |

A true 80186 PC was not available (86Box has no PC-compatible 80186
machine); the V20 runs the same instruction set.

## Timer tick cost, task A2 (MartyPC, 2026-11-17)

ELKS ghaerr/elks `9b3c134` (fully merged in Alci), Alci `cf9e7b7`, same
configuration (`alci/elks.config`) and toolchain. Idle cost measured with
`@idle 5` and the sampling profiler (sample every 7 cycles), cycles per
100 Hz tick outside the `hlt` of the idle loop.

| Where | Cycles per tick |
| --- | --- |
| interrupt entry and exit (`_irqit`, `save_regs`, `doirq`, `do_IRQ`, `timer_tick`, `was_trap`, EOI, `restore_regs`) | about 720 |
| `do_bottom_half` | 350 |
| `schedule()` called by the idle loop | 168 |
| `serial_bh` (no serial port open) | 168 |
| `timer_bh`, `run_timer_list`, idle loop | 126 |
| `calc_cpu_usage` (far call every tick) | 56 |
| BIOS IRQ 0, every 5th tick | 29 |
| total | 1523 (3.2% of the CPU, not 7.6% as estimated before) |

Trial patch (rejected, not committed): skip `serial_bh` when no port is
open, count CPU usage ticks in `timer_bh` and make the far call only every
2 s, and skip `schedule()` in the idle loop when no task is runnable.
Idle tick 1523 -> 1210 cycles (-21%). Cost +22 bytes of kernel code, +1
byte of bss.

| Benchmark | Alci cf9e7b7 | with trial patch |
| --- | --- | --- |
| `cat` of 200 lines to screen | 1.509 | 1.497 (-0.8%) |
| `ls -l /bin`, from floppy | 7.961 | 7.923 (-0.5%) |
| `ls -l /bin`, cached | 4.055 | 4.013 (-1.0%) |
| `cat /bin/sh` | 1.706 | 1.698 (-0.5%) |
| `cat /bin/vi` | 4.724 | 4.715 (-0.2%) |
| 10 runs of `pwd` | 1.618 | 1.610 (-0.5%) |
| three-stage pipe, 20 KB | 4.632 | 4.619 (-0.3%) |
| `sort` on 20 KB | 5.479 | 5.442 (-0.7%) |
| `grep` on 20 KB | 3.685 | 3.660 (-0.7%) |
| boot to prompt | 14.184 | 14.180 |

Mean `bench2.sh` gain 0.46%: value 0.46 / (1 + 33 / 50) = 0.28, below the
threshold of 2.

Same trial on 86Box 7.0, IBM XT (8088 4.77 MHz, CGA), hard disk image on
XT-IDE, console on screen (`SCREEN=1`, so no serial port open),
`alci/tests/speed.txt`, emulated seconds (resolution 0.01 s):

| Command | Alci cf9e7b7 | with trial patch |
| --- | --- | --- |
| boot to prompt (with BIOS POST) | 37.94 | 37.94 |
| `ls -l /bin`, first run | 9.65 | 9.55 (-1.0%) |
| `ls -l /bin`, cached | 9.24 | 9.14 (-1.1%) |
| 10 runs of `pwd` | 1.46 | 1.43 (-2.1%) |
| three-stage pipe | 2.88 | 2.84 (-1.4%) |
| `grep` | 6.93 | 6.86 (-1.0%) |
| `sort` small file | 0.50 | 0.50 |

Mean gain 1.1%, about twice the floppy figure (less time waiting on the
drive, so the tick is a larger share), value 0.67: still below the
threshold. The whole tick costs 3.2% of the CPU, so no change to the
per-tick path alone can reach the threshold on these benchmarks.

## Line reading, task A2b (2026-11-17)

ELKS ghaerr/elks `9b3c134` (fully merged in Alci), Alci `64182af` before and
with the A2b patch, same configuration and toolchain. MartyPC, 8088
4.77 MHz, 720K floppy, emulated seconds:

| Benchmark | Alci 64182af | with A2b |
| --- | --- | --- |
| `grep` on 20 KB | 3.685 | 2.482 (-32.6%) |
| `sort` on 20 KB | 5.479 | 5.454 (-0.5%) |
| `cat` of 200 lines to screen | 1.509 | 1.509 |
| `ls -l /bin`, from floppy | 7.961 | 7.865 (-1.2%) |
| `ls -l /bin`, cached | 4.055 | 3.984 (-1.8%) |
| `cat /bin/sh` | 1.706 | 1.672 (-2.0%) |
| `cat /bin/vi` | 4.724 | 4.699 (-0.5%) |
| 10 runs of `pwd` | 1.618 | 1.581 (-2.3%) |
| three-stage pipe, 20 KB | 4.632 | 4.338 (-6.3%) |
| boot to prompt | 14.184 | 14.184 |

Only `grep` changes in code among the benchmarked programs; `ls` uses the
new `memchr`. The other small gains come from the files sitting in
different places on the floppy image, since `grep` grew.

Cost: kernel 0 bytes, resident RAM 0. libc: `fgets` 96 -> 188 bytes,
`memchr` 33 -> 40 bytes; only programs that use them pay. `grep` text
8992 -> 9088 bytes (+96), data and bss unchanged. On the 720K image the
root filesystem has one free block less (25 instead of 26).

Value: 32.6 / (1 + 127 / 50) = 9.2, above the threshold.

Behaviour: `grep` with `-n`, `-c`, `-v`, `-i`, `-l`, exit status, lines of
0 to 3000 characters (including heap exhaustion and the "line too long"
path), a last line without newline, binary files, and stdin from a pipe:
output identical to the previous build (MartyPC, md5 of each output).
`test_libc`: 38 tests, 5805 assertions, none failed. Functional suite with
five new `grep` lines: identical on QEMU, T1200 (XT-IDE and its own disk),
PC1512 and AT. Two differences, both not caused by the patch:

- IBM XT, hard disk: the previous build prints `1 9 59` for
  `ls -l /tmp/c.Z | wc -l`, the new one `1`. The previous build loses
  `-l` on the serial input at that point, every run: a serial input loss
  that moves with timing (V5), already seen on the V20.
- IBM XT, `fd720` image: the new image hangs once the floppy is full
  (task S1). The same image with the old `grep` hangs too.

## Merges to review

Commits whose merge into `alci` changed one of our measurements (see
AGENT.md). Usually this means Alci needs adapting; the maintainer decides how.

| sha | title | check | numbers | decision |
| --- | --- | --- | --- | --- |
| | none so far | | | |
