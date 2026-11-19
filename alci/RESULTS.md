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
PC1512 and AT. Two differences on the IBM XT, both from characters lost on
the serial input (task S1), not from the patch:

- hard disk image: the previous build prints `1 9 59` for
  `ls -l /tmp/c.Z | wc -l`, because `-l` was lost on the way in.
- `fd720` image: the new image seemed to hang once the floppy was full.
  It does not: the first `@` of the test harness's end marker was lost, so
  the harness waited for a marker that never came; the shell had already
  printed "No space left on device" and answers further commands. Adding
  one `df` before that line moves the loss elsewhere and the suite ends.

Upstream for comparison, same configuration, same suite, IBM XT:
ELKS master `df2a61c` loses a character too (`and-ok` arrives as
`an-ok`); release v0.9.2 (`e7a56a4`) lost none in this run, which only
says the loss fell elsewhere, since it depends on timing. Both upstream
images, filled to the same 25 free blocks, give "No space left on device"
as expected.

## Compressed executables on the 720K floppy, task F4 point 0 (MartyPC, 2026-11-19)

Same tree (Alci `29beabbc`, upstream ELKS `df2a61c5` merged), same
configuration and toolchain; two 720K images made with
`CONFIG_APPS_COMPRESS` off and on (`elks-compress`, exomizer). MartyPC
`elks-bench` `5dea3d5`, GLaBIOS, floppy rotation model (`ELKS_FDROT=9`).
First run after boot, then a second run of the same command. `sh` and `ls`
are marked `:nocomp` upstream and stay uncompressed on both images, so
their rows show only the effect of the different disk layout.

| Command | IBM XT 8088 4.77, plain | compressed | 8086 7.16 MHz (`elks_86`, turbo), plain | compressed |
| --- | --- | --- | --- | --- |
| boot to prompt | 18.36 | 17.64 | 15.53 | 14.57 |
| `vi -r` (59280 -> 37414 bytes), first | 5.11 | 40.36 | 3.64 | 20.03 |
| `vi -r`, again | 4.48 | 39.94 | 3.23 | 19.59 |
| `cat /etc/passwd > /dev/null` (928 -> 829), first | 1.79 | 2.57 | 1.76 | 1.93 |
| `cat /etc/passwd > /dev/null`, again | 0.99 | 1.53 | 0.98 | 1.36 |
| `ls /etc > /dev/null` (not compressed), first | 4.52 | 4.18 | 4.18 | 3.55 |
| `sh -c true` (not compressed), first | 1.68 | 1.58 | 1.48 | 0.97 |

Emulated seconds. Space: 669 -> 539 blocks of 1 KB in use (130 KB freed).
Memory while loading: the kernel decompresses in place, 16 extra bytes per
segment (the 16 bytes of the text segment stay allocated while it is in
use). The kernel decompressor is very slow on the 8088: about 0.7 s per KB
of `vi`. Decision: compression not adopted for the 720K image, since
starting a compressed program is slower on both machines (`vi` 8 times
slower on the XT). A faster decompressor (assembly) could reopen it.

## Functional suite on the 720K floppy, task F4 (2026-11-19)

`alci/tests/functional-fd.txt` (same 34 commands on small files, second
disk on `/mnt` for `tar compress md5sum test_libc test_float`), strict
drivers and `check.py`. 86Box built from upstream `d4f9a04` with the
PicoMEM patch; QEMU 8.2.2 (`486`, 640K).

720K image: 41 -> 82 free blocks (`wc head cmp sleep xargs yes` added,
`ftp ftpd telnet traceroute` moved to 1200K and larger images).

| Image | xt | t1200 | pc1512 | at | 386 | QEMU |
| --- | --- | --- | --- | --- | --- | --- |
| Alci `29beabbc`, `functional.txt` (before) | 10/34 | | | 15/34 | | |
| Alci `59478e75` (F4 layout, later reverted), `functional-fd.txt` | 33/34 (S1: one character lost) | 34/34 | 34/34 | 34/34 | 34/34 | 34/34 |
| Alci `800cfd1e` (F4 + S3), `functional-fd.txt` | 34/34 | 34/34 | 34/34 | 34/34 | 34/34 | 34/34 |

Hard disk (`functional.txt`, with the stricter `tar-ok`): QEMU 34/34 on
`59478e75` and `800cfd1e`, IBM AT 34/34 on `800cfd1e`.

## Out of memory on the AT with 512K, task S3 (86Box, 2026-11-19)

Cause: the data segment of a program is sized with the exact length of
its argv and envp. The interactive `/bin/sh` has a 22624-byte data
segment; `sh -c 'echo p'` started from it gets 22672 bytes. Eight of them
in background: each forked copy (22624) is freed after its exec, but the
hole is 48 bytes too small for the next exec, so the 224 KB free (AT, 512K,
64K of buffers) fragment and six of eight execs fail. The order of earlier
commands decides whether it happens, which is why ELKS and Alci, and the
floppy and hard disk images, traded results (boot line on the AT: ELKS
`df2a61c5` 350K free, Alci 348K). Fix (`800cfd1e`): round the data
segment to 256 bytes in `exec`; both get 22784.

| Alci build, AT 512K, fd720, first 11 commands with absent programs | `sh: not enough memory` (two runs) |
| --- | --- |
| `59478e75` | 6, 6 |
| `800cfd1e` | 0, 0 |

Cost: `exec.o` +17 bytes of code, resident kernel +16 bytes (one
paragraph), about 128 bytes per process on average (+160 for `sh`).
`bench.sh` and `bench2.sh` on MartyPC: every timing within 0.01%.
Vetoes: V1 suite 34/34 everywhere (table above), V2 no change, V3 cost as
stated, V4 and V5 untouched code.

## Serial input losses on the IBM XT, task S1 (86Box, 2026-11-19)

86Box upstream `d4f9a04` with the PicoMEM patch and a measurement probe
in the 8250 (`alci/tests/86box-overrun.patch`, enabled with
`ALCI_OVERRUN=1`: logs every overrun with the CPU state sampled at each bit
time while the byte waited, and the longest wait of a byte in RBR). IBM
XT, 8088 4.77 MHz, IBM BIOS, 720K floppy, console on COM1 at 19200 baud
(one byte every 521 us), 300 commands typed by `st86.py` a whole line at
a time (`s1b`: echo, pipes, `$(...)`, `grep`).

Where the time went in each lost byte: a system call entry with IF=0
(about 100 us), then the timer interrupt, IRQ 0 in service through the
`_irqit` entry, `timer_tick()` and the BIOS IRQ 0 handler (every 5th
tick), the BIOS EOI only at the end, then the serial handler entry.

| Build | longest wait of a byte | bytes lost in 300 commands |
| --- | --- | --- |
| Alci `800cfd1e` | 643 us | 3 (two runs, same 3) |
| Alci with the BIOS handler in a bottom half only | 519 us | 0 (no margin left) |
| Alci `49b6c795` (also EOI before `timer_tick`) | 384 us | 0 (two stress runs, plus the full suites) |
| ELKS `df2a61c5`, commands present on its 720K disk | 638 us | 1, the run stalled at command 42 |
| ELKS `df2a61c5` + `alci/upstream/timer-eoi-com1.patch` | 377 us | 0 |

Functional suite on `49b6c795`: 34/34 on fd720 (`functional-fd.txt`) and
hd32 on xt, t1200, pc1512, at, 386; T1200 own disk (hd20t) 34/34; QEMU
fd and hd 34/34. V4: `sleep 60` takes 60.78 s emulated before and after.
The BIOS floppy motor still turns off (BIOS data 0040:003F = 0 after
idle). MartyPC (`elks-bench` `d6f5ca8`, meter off), `bench.sh` and
`bench2.sh`, Alci `800cfd1e` -> `49b6c795`:

| Benchmark | before | after |
| --- | --- | --- |
| `cat` of 200 lines to screen | 1.505 | 1.514 |
| `ls -l /bin`, from floppy | 7.600 | 7.592 |
| `ls -l /bin`, cached | 4.101 | 4.109 |
| boot to prompt | 14.155 | 14.176 |
| 10 runs of `pwd` | 1.572 | 1.572 |
| three-stage pipe | 3.990 | 3.986 |
| `sort` | 5.479 | 5.492 |
| `grep` | 2.603 | 2.612 |

Largest change +0.6%, under the 1% of V2. Cost: +29 bytes of code
(`timer.o` +26, `irq.o` +6, `irqtab.o` -3), data and bss +2, resident
kernel +32 bytes (two paragraphs).

On MartyPC (GLaBIOS, keyboard console) the new `ELKS_IRQOFF` meter shows
the timer tick holding IRQ 4 off for 1280-1535 cycles, 1792-2059 with
the BIOS call, before the fix; and the keyboard handler holding IRQ 1 in
service for up to 4756 cycles (1 ms) while keys are typed.

## 720K layout change reverted, speed of the whole session (2026-11-19)

MartyPC (`elks-bench` `d6f5ca8`), `bench.sh` and `bench2.sh`, Alci
`29beabbc` (before this session's changes) against `be099d68` (F4 layout,
S3, S1), emulated seconds:

| Benchmark | `29beabbc` | `be099d68` |
| --- | --- | --- |
| `cat` to `/dev/null`, from floppy | 4.258 | 4.032 |
| `cat` of 200 lines to screen | 1.509 | 1.514 |
| `ls -l /bin`, from floppy | 7.865 | 7.592 |
| `ls -l /bin`, cached | 3.984 | 4.110 |
| boot to prompt | 14.184 | 14.176 |
| `cat /bin/vi > /dev/null` | 4.699 | 4.816 |
| three-stage pipe | 4.338 | 3.986 |
| `sort` | 5.454 | 5.492 |
| `grep` | 2.482 | 2.612 |

S3 and S1 alone move timings by 0.6% at most (sections above), so the
larger changes come from the layout change `59478e75`: files at other
places on the floppy and two more entries in `/bin`. It was not measured
before it was committed, and it also left `ftpd` in `/etc/net.cfg` but not
on the disk. Reverted in `0249f4d7`; layout changes now wait for F0 and
F3 and are measured before and after.

Functional suite on the reverted image (`0249f4d7`, `functional-fd.txt`
with the test disk now carrying also `wc head cmp sleep xargs yes`, and no
`/b.txt` on fd720 so that `/tmp` fits): 34/34 on xt, t1200, pc1512, at,
386 and QEMU.

## LZ4 compressed executables, task F4b (MartyPC, 2026-11-19)

Alci `ce5e04a7` built twice with the same kernel (`CONFIG_EXEC_LZ4=y`):
720K image plain, and with `CONFIG_APPS_COMPRESS_LZ4` (programs packed by
`elks-lz4.py`, only when they save a 1K block; `sh`, `ls` and `ps` stay
plain as upstream marks them). MartyPC `elks-bench` `d6f5ca8`, floppy
rotation model on, emulated seconds.

Space: 686 -> 613 blocks in use on the 720K image (73 KB freed; exomizer
freed 130). Correctness: `functional-fd.txt` 34/34 on QEMU, IBM XT and
IBM AT with the LZ4 image.

| Command | XT 8088 4.77 plain | XT LZ4 | 8086 7.16 plain | 8086 LZ4 |
| --- | --- | --- | --- | --- |
| boot to prompt | 18.36 | 17.66 | 15.52 | 14.97 |
| `grep`, first run from floppy | 4.19 | 3.90 | 3.88 | 3.45 |
| `grep` 10 times, from the cache | 2.23 | 3.35 | 1.03 | 1.58 |
| `sort`, first run | 2.02 | 1.96 | 2.02 | 1.89 |
| `sort` 10 times, from the cache | 2.40 | 3.14 | 1.13 | 1.49 |
| `pwd` 10 times (not compressed) | 1.82 | 1.80 | 1.09 | 0.89 |
| `vi -r`, first run | 4.94 | 4.46 | | |

A program read from the floppy starts faster (less to read), one already
in the buffer cache starts slower: the decoder costs about 54 cycles per
byte produced on the 8088, against about 10 for copying a plain program
out of the cache. Kernel cost when the option is on: about 1 KB resident
(`exec.o` +803 bytes, `lz4dec.o` 174, `seg_shrink`); nothing when off,
which is the default. Not adopted yet: the repeated-start slowdown breaks
V2.

## Faster decoders and LZSA1, task F4b round 2 (MartyPC, 2026-11-19)

Alci `30b055bd`, same kernel (`CONFIG_EXEC_LZ4=y`, both decoders) for the
three 720K images: plain, LZ4 (decoder without per-sequence checks, word
copies) and LZSA1 (LZSA project's 8088 speed decoder, zlib licence).
MartyPC `elks-bench` `d6f5ca8`, floppy rotation model on, emulated seconds.

Free space on the 720K image (no test files): plain 41 KB, LZ4 113 KB,
LZSA1 124 KB. Floppy suite 34/34 on QEMU, IBM XT and IBM AT with LZSA1
(and with LZ4, round 1).

| Command | XT plain | XT LZ4 | XT LZSA1 | 8086 plain | 8086 LZ4 | 8086 LZSA1 |
| --- | --- | --- | --- | --- | --- | --- |
| `grep`, first run | 4.39 | 4.10 | 3.90 | 3.88 | 3.44 | 3.44 |
| `grep` x10, from the cache | 2.22 | 3.07 | 3.06 | 1.03 | 1.46 | 1.50 |
| `sort`, first run | 2.02 | 2.02 | 1.89 | 2.02 | 1.81 | 1.43 |
| `sort` x10, from the cache | 2.40 | 2.96 | 2.93 | 1.13 | 1.41 | 1.42 |
| `vi -r`, first run | 4.29 | 3.76 | 3.74 | 3.02 | 2.45 | 2.41 |
| `vi -r`, second run | 4.75 | 1.06 | 1.02 | 3.37 | 0.52 | 0.53 |
| `sed`, first run | 4.76 | 4.18 | 2.86 | 4.26 | 3.78 | 2.43 |
| `sed` x10, from the cache | 3.12 | 4.18 | 4.20 | 1.44 | 1.98 | 2.05 |

Profile of the `grep` loop: decoding costs about 37 cycles per byte
produced with either format (most sequences in program code are short
matches, so the cost per sequence dominates), against about 8-10 for
copying a plain program out of the buffer cache, while reading from the
floppy costs far more than either. Hence: from the floppy, both formats
are faster and LZSA1 the most (it also packs best); from the cache,
both are slower by the same amount. `vi`'s second run is fast compressed
because 44 KB of it fit in the buffer cache and 59 KB plain do not.

Kernel cost with the option on: about 1.5 KB resident (`exec.o` +928
bytes, `lz4dec.o` 146, `lzsa1dec.o` 333, `seg_shrink`). Off by default:
nothing.

## Text cache, task F4b round 3 (MartyPC and 86Box, 2026-11-19)

Alci `b512731a`, `CONFIG_EXEC_TEXT_CACHE` (exited programs kept in free
memory, freed as soon as an allocation needs the room). Four 720K images,
same tree: plain, plain + cache, LZSA1, LZSA1 + cache. MartyPC
`elks-bench` `d6f5ca8`, floppy rotation model on, emulated seconds.

| Command | XT plain | XT cache | XT LZSA1 | XT LZSA1 + cache | 8086 plain | 8086 LZSA1 + cache |
| --- | --- | --- | --- | --- | --- | --- |
| `grep`, first run | 4.39 | 4.20 | 3.90 | 3.71 | 3.88 | 3.26 |
| `grep` x10 | 2.22 | 1.82 | 3.06 | 1.87 | 1.03 | 0.88 |
| `sort`, first run | 2.02 | 2.04 | 1.89 | 2.16 | 2.02 | 1.96 |
| `sort` x10 | 2.40 | 2.14 | 2.93 | 2.18 | 1.13 | 1.04 |
| `vi -r`, first run | 4.29 | 4.35 | 3.74 | 3.81 | 3.02 | 2.48 |
| `vi -r`, second run | 4.75 | 0.44 | 1.02 | 0.25 | 3.37 | 0.12 |
| `sed`, first run | 4.76 | 4.46 | 2.86 | 3.19 | 4.26 | 2.72 |
| `sed` x10 | 3.12 | 2.55 | 4.20 | 2.64 | 1.44 | 1.23 |

LZSA1 + cache against today's plain image: faster everywhere but the
first `sort` on the XT (+7%; -3% on the 8086), where the position of the
files on the floppy differs between the two images (task A3). Free space
on the 720K image: 41 KB plain, 124 KB LZSA1.

Checks with LZSA1 + cache: floppy suite 34/34 on QEMU, XT, T1200, PC1512,
AT and 386; hard disk 34/34 on QEMU and AT; a program file overwritten
or replaced runs its new contents; `umount` works after running a
program from the mounted disk; eight background `sh -c` on the 512K AT
all start (kept texts are freed for them; `meminfo` shows less free
memory while texts are kept).

Resident kernel cost, against `61835b74` with everything off: text cache
704 bytes; text cache, LZ4 and LZSA1 together 2208 bytes.

## Emulator updates

Merges of upstream MartyPC (`dbalsom/martypc` into `elks-bench`) and 86Box
(`86Box/86box` into `proxybarrio/86box` branch `alci`), with the timings
of the same Alci build before and after. As of 2026-11-18: MartyPC
`elks-bench` is based on upstream `05c0d08` (2026-09-07), the latest on
`main`; 86Box is built from upstream `960d6b7` (2026-11-17) plus the
PicoMEM patch.

| Date | Emulator | from -> to | what changed for us | timings |
| --- | --- | --- | --- | --- |
| 2026-11-19 | 86Box | `960d6b7` -> `d4f9a04` | `proxybarrio/86box` has no `alci` branch yet (A6), so `setup.sh` built upstream master plus the PicoMEM patch; needs `libarchive-dev` | functional tests only, no timings compared |

## Merges to review

Commits whose merge into `alci` changed one of our measurements (see
AGENT.md). Usually this means Alci needs adapting; the maintainer decides how.

| sha | title | check | numbers | decision |
| --- | --- | --- | --- | --- |
| | none so far | | | |
