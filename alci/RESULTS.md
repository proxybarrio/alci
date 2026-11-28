# Alci: measured results

## Reference measurement (2026-11-16)

Every comparison must name both versions. This one compares:

- **ELKS**: 0.9.2, ghaerr/elks commit `22a4c5d` (master of 2026-11-15).
- **Alci**: 0.1, proxybarrio/alci branch `alci`, commit `3ecae5b`.

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

## LZSA1 and text cache turned on, task F4b (2026-11-19)

Alci `57265faf` (`CONFIG_EXEC_LZSA`, `CONFIG_EXEC_TEXT_CACHE`,
`CONFIG_APPS_COMPRESS_LZSA` on; LZ4 decoder removed) against the same tree
with the three options off. MartyPC `elks-bench` `d6f5ca8`, `bench.sh`
and `bench2.sh` (no floppy rotation model), emulated seconds:

| Benchmark | options off | on |
| --- | --- | --- |
| `cat` of 200 lines to `/dev/null` | 4.258 | 3.944 |
| `cat` of 200 lines to screen | 1.509 | 1.509 |
| `ls -l /bin`, from floppy | 7.860 | 7.496 |
| `ls -l /bin`, cached | 3.992 | 3.942 |
| boot to prompt | 14.197 | 14.016 |
| `cat /bin/sh > /dev/null` | 1.668 | 1.538 |
| `cat /bin/vi > /dev/null` | 4.707 | 4.116 |
| 10 runs of `pwd` | 1.585 | 1.526 |
| three-stage pipe | 4.338 | 4.150 |
| `sort` on 20 KB | 5.463 | 5.454 |
| `grep` on 20 KB | 2.490 | 2.570 |

`grep` is 3.2% slower: its first start, which here reads from a floppy
with no rotation delay, so decoding is not paid back; with the rotation
model (round 3) the first `grep` is 15% faster. Resident kernel: 476 ->
474 KB free (+1936 bytes: text cache, LZSA1 decoder, exec). 720K image:
41 -> 123 KB free with the same programs; the 1.44M image also gets the
programs upstream adds when compressing (137 KB still free). Hard disk
images are not compressed.

Checks on `57265faf`: floppy suite 34/34 on QEMU, XT, T1200, PC1512, AT,
386; hard disk suite 34/34 on QEMU, XT, T1200, PC1512, AT, 386; serial
stress on the XT: no byte lost, longest wait 386 us (as before); `sleep
60` takes 60.36 s on the XT (60.37 before); eight background `sh -c` on
the 512K AT all start.

## Floppy boot, task A3 point 1 (MartyPC, 2026-11-19)

Alci `6d338ea9` against `958f9203`, 720K image with the normal `bootopts`
(init, `rc.sys`, getty), MartyPC `elks-bench` `e1c5187`, `ELKS_FDROT=9`,
time to the `login:` prompt (and to the shell prompt with `init=/bin/sh`):

| Machine | before | after |
| --- | --- | --- |
| IBM XT 8088 4.77, to `login:` | 22.81 | 22.21 |
| IBM XT 8088 4.77, to `#` | 17.86 | 17.26 |
| 8086 7.16 MHz, to `login:` | 18.79 | 18.19 |
| 8086 7.16 MHz, to `#` | 14.97 | 14.37 |

The boot loader no longer re-reads the inode block for `/bootopts` and
`/linux`. Floppy suite 34/34 on QEMU and 86Box XT, XT with V20, T1200,
PC1512, AT, 386; 1.44M, hd32 and the T1200's own disk boot. Also measured:
putting `/bootopts` and `/linux` first in `image/Image.all` was 0.4 s
slower than upstream's order, which stays.

## Network drivers released when no card is found, task A5 (2026-11-19)

Alci `6feabbf1` against `6e094135`, same toolchain and configuration
(`alci/elks.config`: NE2000, WD8003 and 3C509 drivers on). Kernel image
106864 -> 107520 bytes (near code 59760 -> 55728, far code 29808 ->
34464, data 9520 -> 9552, bss 7776). Areas given back at boot when the
card is not found: NE2000 1904, WD8003 1952, 3C509 1744 bytes.

Main memory (`meminfo`, after boot):

| Machine, card | before | after |
| --- | --- | --- |
| QEMU 486, hd32, no card | 484K | 489K |
| QEMU 486, hd32, NE2000 | - | 488K |
| 86Box XT, 720K, no card | 485K | 490K |
| 86Box XT, 720K, PicoMEM NE2000 | 486K | 489K |
| 86Box XT, 720K, WD8003E | 486K | 489K |

Network on the 86Box XT (8088 4.77 MHz, `d4f9a04` with PicoMEM patch),
`urlget` of a 30000-byte file over slirp: `md5sum` right with both cards
and both builds; to `/dev/null`, two runs each:

| Card | before | after |
| --- | --- | --- |
| NE2000 (PicoMEM) | 2.85, 2.77 s | 2.86, 2.79 s |
| WD8003E | 2.81, 2.63 s | 2.66, 2.65 s |

QEMU with an NE2000: `ktcp` up, two 30000-byte downloads with the right
`md5sum`. 3C509: 86Box's 3C509B is not found by the driver, before or
after (not tested with a card). SMC Ultra and PCnet: compiled, not run.

MartyPC `elks-bench` `e1c5187`, `bench.sh` and `bench2.sh` commands on
720K images of the two kernels (same programs), emulated seconds:

| Benchmark | before | after |
| --- | --- | --- |
| `cat` of 200 lines to `/dev/null` | 3.944 | 3.881 |
| `cat` of 200 lines to screen | 1.509, 1.514 | 1.509, 1.514 |
| `ls -l /bin`, from floppy | 7.487 | 7.349 |
| `ls -l /bin`, cached | 3.941 | 3.946 |
| boot to prompt | 13.631 | 13.677 |
| `cat /bin/sh > /dev/null` | 1.542 | 1.501 |
| `cat /bin/vi > /dev/null` | 4.120 | 3.852 |
| 10 runs of `pwd` | 1.534 | 1.497 |
| three-stage pipe | 4.145 | 3.886 |
| `sort` on 20 KB | 5.446 | 5.421 |
| `grep` on 20 KB | 2.574 | 2.545 |

Suites 34/34: hard disk on QEMU and 86Box AT, floppy on QEMU and 86Box XT.

## Date asked at boot, task E7 point 4 (2026-11-19)

Alci `70b577df`. `clock -c -u` runs in `rc.sys` after the network start.
QEMU with a full boot (init, `rc.sys`, getty on the serial line):

| RTC at power on | answer | result |
| --- | --- | --- |
| 2020-01-01 | `202611191230` | date and RTC set to 19 Nov 2026 12:30; a second `clock -c` asks nothing |
| 2020-01-01 | Enter | date kept, boot goes on |
| 2020-01-01 | `bad` | asked again |
| 2020-01-01 | nothing | boot goes on after 60.3 s |
| 2026-12-01 | - | no question |

`clock` 3936 -> 4784 bytes; 720K disk 1: 122 -> 121 KB free. Suites 34/34
(QEMU hard disk, 86Box XT floppy); test images boot with `init=/bin/sh`,
so `rc.sys` does not run there.

## Corrupted LZSA1 programs, review of F4b (2026-11-19)

Alci `70b577df`, 720K image, QEMU 486. Twelve copies of `/bin/grep`
(LZSA1, 7383 compressed bytes of text) each with a different corruption
(one bit flipped, 8 to 1000 bytes overwritten in the text or the data,
256 zero bytes, the end marker removed), each started in a fresh boot:

| Outcome | copies |
| --- | --- |
| ran normally (byte in the data section) | 1 |
| rejected by exec (wrong length), shell then tried it as a script | 3 |
| ran and was stopped (stack overflow, `SIGSEGV`), system fine | 1 |
| whole system hung (CPU found running BIOS code) | 7 |

Host model of the decoder (`alci/tests/lzsa1-model.py`, same steps as
`lzsa1dec.S`, decoding in place as exec does), 500 corruptions of each
kind on the same `grep`:

| Corruption | accepted | rejected | wrote outside the segment |
| --- | --- | --- | --- |
| one bit flipped | 428 | 72 | 13 |
| one byte replaced | 438 | 62 | 14 |
| 16 bytes replaced | 107 | 393 | 168 |
| tail replaced by random bytes | 0 | 500 | 500 |
| tail replaced by zeros | 0 | 500 | 500 |

"Accepted" means the block decodes to the right length: the program then
runs with wrong code, as an uncompressed program with the same damage
would. "Wrote outside" means the decoder wrote past the segment exec
allocated, up to 64 KB of other memory, kernel included; nearly all of
them wrote 64 KB without reaching an end marker. A file cut short is
rejected before decoding (exec checks the bytes read); a file of the right
size with a damaged tail is not. Plan: task F4c.

## Bounds checks in the LZSA1 decoder, task F4c (2026-11-20)

Alci `9d1b6868` against `40dc2a7d`, same toolchain and configuration.
Decoder 333 -> 259 bytes (the 128-byte table is gone), kernel code
55728 -> 55664 bytes, data unchanged.

The same twelve damaged copies of `grep` as on 2026-11-19, QEMU 486,
720K image, one fresh boot each:

| Outcome | before | after |
| --- | --- | --- |
| whole system hung | 7 | 0 |
| rejected by exec (shell then tried it as a script) | 3 | 10 |
| stopped by `SIGSEGV`, system fine | 1 | 1 |
| ran normally (byte in the data section) | 1 | 1 |

Host model with the checks (`lzsa1-model.py grep 500 checked`): 2500
corruptions, none writes outside the segment, none runs without end;
accepted and rejected counts as before.

MartyPC `elks-bench` `e1c5187`, 720K images of the two kernels, emulated
seconds. `bench5` commands with the floppy rotation model:

| Command | XT before | XT after | 8086 before | 8086 after |
| --- | --- | --- | --- | --- |
| `grep`, first run | 3.521 | 3.525 | 3.266 | 3.266 |
| `grep` x10 | 1.870 | 1.878 | 0.878 | 0.880 |
| `sort`, first run | 2.158 | 2.163 | 1.956 | 1.953 |
| `sort` x10 | 2.201 | 2.205 | 1.042 | 1.045 |
| `vi -r`, first run | 3.391 | 3.429 | 2.473 | 2.490 |
| `vi -r`, second run | 0.252 | 0.260 | 0.123 | 0.126 |
| `sed`, first run | 3.101 | 3.051 | 2.582 | 2.760 |
| `sed` x10 | 2.633 | 2.641 | 1.224 | 1.227 |

The 8086 `sed` first run (+0.18 s, one floppy revolution) is the rotation
model: without it the same two runs give `sed` 2.858 -> 2.861 and `vi -r`
3.275 -> 3.291. Decoding itself is about 9% slower (3 cycles more per
byte produced, out of about 37); `vi`'s second start decodes only its
data. `bench.sh` and `bench2.sh`: every line within 0.5% (`cat /bin/sh`
1.501 -> 1.509, the rest equal).

Suites 34/34: floppy on QEMU, 86Box XT, AT, 386; hard disk on QEMU.

## Network time, task E7 point 2: SNTP client in ktcp (2026-11-20)

Alci `57c23dd6` (upstream ELKS `df2a61c5` merged) against the same tree
with the SNTP client, same configuration (`alci/elks.config`) and
toolchain.

Size (`ia16-elf-size`, a.out header of `ktcp`):

| | text | data | bss | file |
| --- | --- | --- | --- | --- |
| `ktcp` before | 22960 | 2752 | 20336 | 25744 |
| `ktcp` with SNTP | 24096 | 2864 | 20368 | 26976 |
| difference | +1136 | +112 | +32 | +1232 |

`sntp.o` 1087 bytes of text, `dhcp.o` +106 (option 42), `ktcp.o` +100
(`-t`). Kernel and `libc` unchanged: no resident cost, the code is in
memory only while `ktcp` runs.

Behaviour, QEMU 8.2.2 (`486`, 640K, NE2000 at 300h, slirp) with a test
SNTP server on the host (10.0.2.2) that answers the first request with a
wrong origin timestamp, then correctly with host time plus one day:

| Case | result |
| --- | --- |
| static address, `-t 10.0.2.9,10.0.2.2` (first silent) | after 4 s moves on; wrong-origin answer ignored; retry sets the clock, `corrected by 86400 s` |
| DHCP (`-d`), `-t 10.0.2.2` | DHCP bound, then clock set |
| server sends 1999 | `time too old` twice, `no answer`, clock unchanged |
| no server (ICMP port unreachable) | `no answer` after two tries |
| `net start ne0` with the default `net.cfg` | `ktcp -t 193.204.114.232,...` started; no UDP to the internet here, so `no answer` after 12 s; daemons up |

86Box IBM XT (8088 4.77 MHz, real BIOS, WD8003E, 720K): clock set from the
test server.

Functional suite: 34/34 on QEMU (hd32) and 86Box XT (hd32), output
identical to `57c23dd6`.

MartyPC `elks-bench` `e1c5187`, `bench.sh` (no rotation model), emulated
seconds:

| Benchmark | `57c23dd6` | with SNTP | `57c23dd6`, net.cfg and ktcp padded to the same sizes |
| --- | --- | --- | --- |
| `cat` to `/dev/null`, from floppy | 4.174 | 4.380 | 4.090 |
| `cat` of 200 lines to screen | 1.509 | 1.509 | 1.505 |
| `cat` of 200 lines, again | 1.514 | 1.514 | 1.509 |
| `ls -l /bin`, from floppy | 7.697 | 7.974 | 8.082 |
| `ls -l /bin`, cached | 3.946 | 3.942 | 3.942 |

No benchmark runs `ktcp`. The two first-from-floppy lines move because
`net.cfg` (1007 -> 1323 bytes) and `ktcp` each take one more block, so
`cat`, `ls` and `b.txt` sit two blocks further on the disk (`cat` 550 ->
552). The control column, upstream code with only the two files padded,
moves the same lines by -2% and +5%: a layout effect, left to the floppy
layout work (A3, F0). Vetoes: V1 identical; V2 no code in the measured
path, layout effect above; V3 no resident change, `ktcp` +1.2 KB; V4 the
clock is set only from a valid answer, timer code unchanged; V5 not
touched; V6 no BIOS dependency. A feature, not a speed change: no gain
threshold.

## Kilo safe save and buffer bounds, task S5 (2026-11-22)

Alci `b76f3381` and the same with the fix; `kilo.c` is identical to ELKS
master `df2a61c5`, so ELKS has the same bugs.

Full disk on Alci (QEMU 486, hd32 image, `alci/tests/kilo-save.py`): a
930-byte file with 4 lines containing "root", the disk filled, 1200
characters typed at the top, Ctrl-S.

| | status shown | file afterwards |
| --- | --- | --- |
| before | `Can't save! I/O error: Error 0` | 1024 bytes of new text, 0 of the original lines |
| after | `Can't save! I/O error: No space left on device (file unchanged)` | 930 bytes, original intact, no `kilo.tmp` left |

A normal save after the fix writes the new text and leaves no temporary
file. The same bugs checked on a host build of `kilo.c` (Linux, ASan and
UBSan), before / after:

| Case | before | after |
| --- | --- | --- |
| save fails (file size limit) | original lost | original kept |
| two saves | heap overflow | clean |
| open a `.txt` file | null pointer read (SEGV on Linux; on ELKS, real mode, garbage instead of a crash) | clean |
| cut a 304-character line | stack overflow | clean |
| search with no match | null pointer read | clean |

Cost: `kilo` 26848 -> 27264 bytes on disk (+416), 256 bytes more of its
own data while running; no kernel change, no resident memory. The save
needs room for a second copy of the file while it writes; when there is
not enough, the save fails and the file stays as it was. Patch for ELKS
master: `alci/upstream/kilo-safe-save.patch`.

## Damaged plain programs, exec header and relocation checks, task S4 (2026-11-21)

ELKS `df2a61c5` (upstream master), Alci `0249a736` and Alci with the
patch, same configuration and toolchain. `alci/tests/exec-corrupt.py`
writes 17 damaged copies of two uncompressed programs into the hd32
image: `grep` (small model) and a medium-model test program with text,
far text and data relocations. Each case is a fresh boot; after it the
shell must answer and both intact programs must give their normal
output. 86Box upstream `fbf5b94` plus the PicoMEM patch, with the real BIOS of each machine
(`st86.py` with `INTR=1`: Ctrl-C after a timeout), QEMU 486, MartyPC
`elks-bench` `e1c5187` (8088 4.77 MHz, fd720).

Before the patch (ELKS and Alci behave alike; "dead" = the shell never
answers again, Ctrl-C included):

| Damage | QEMU | 86Box xt, t1200, pc1512, at | MartyPC 8088 |
| --- | --- | --- | --- |
| entry point past the text | program hangs | program hangs (all four) | system blocked |
| entry point 0xFFFE | hangs or SIGSEGV | runs by chance | - |
| text or far text size with a high word | accepted, runs | accepted, runs | - |
| text relocations past the text | wrong output, or system dead | system dead on 7 of 8 runs | system blocked |
| far text relocations past the far text | system dead (Alci), hang (ELKS) | system dead on 4 of 8 runs, hang on the others | system blocked |
| data relocations past the data | system dead (Alci), hang (ELKS) | system dead on 2 of 8 runs, hang on the others | system blocked |

Already rejected cleanly before the patch: files cut short, data size
larger than the file, bss overflow, unknown header length, bad
relocation type or size, relocations cut short.

The patch (`elks/fs/exec.c`): text, data, bss and entry must have a zero
high word and the entry point must lie inside the text; the far text
size must have a zero high word; every relocation must patch a word
inside the segment it relocates (text, far text or initialized data).
A failed check returns `ENOEXEC` or `EINVAL` before the program runs.

After the patch every header and relocation case above is rejected on
QEMU, on the four 86Box machines and on MartyPC, and the shell and the
intact programs work afterwards. The intact medium-model program still
runs. Random bytes inside the text with a valid header still run and may
hang or crash, on both systems: only a checksum of the whole file would
catch that, at a cost on every start, so it is left to the package
installer (per-file hashes at install time).

Cost: `exec.o` far text +84 bytes in Alci (+90 against ELKS master),
near kernel text unchanged; resident kernel `init 16dd -> 16e2` (+80
bytes), free memory unchanged at 473 KB (QEMU). Speed, MartyPC, Alci
`0249a736` / patched, emulated seconds:

| Operation | before | after |
| --- | --- | --- |
| boot to prompt | 13.849 | 13.849 |
| `cat /b.txt > /dev/null` | 4.380 | 4.380 |
| `ls -l /bin`, from floppy | 7.974 | 7.974 |
| `ls -l /bin`, cached | 3.942 | 3.942 |
| `cat /bin/vi > /dev/null` | 5.172 | 5.172 |
| 10 runs of `pwd` | 1.467 | 1.463 |
| three-stage pipe | 2.100 | 2.096 |
| `sort` on 20 KB | 1.107 | 1.111 |
| `grep` on 20 KB | 1.136 | 1.136 |

Vetoes: V1 functional suites identical before and after, 34/34 with the
strict tools on QEMU (hd32, fd720) and on 86Box xt, t1200, pc1512, at
(hd32), t1200 with its own disk (hd20t) and xt with the fd720 image; V2 all timings within 0.4%, none slower beyond noise; V3 +80
bytes resident, justified by the system-dead cases above; V4 and V5 timer
and tty code not touched; V6 no BIOS dependency. A robustness fix, not a
speed change: no gain threshold. Patch for ELKS master:
`alci/upstream/exec-check-header-relocs.patch`.

Decision (2026-11-21): reverted in Alci for now (`67c222e4`). No program
Alci ships today has relocations, so the 80 resident bytes buy nothing
yet; the patch comes back with the first medium-model program (task S4).

## Upstream merge up to `a3d4940` (2026-11-24)

Upstream ELKS master was at `a450d52` (15 commits after `alci`). Merged
up to `a3d4940` (`mem=` override in `setup`, Cmdr and scrcap external
builds, version 0.9.3-dev, the kilo safe save already in Alci). Not
merged: from `e47e635` on (see "Merges to review"); `342cf3a` and
`139b823` (ept on 1440k images) and `a450d52` (comment in `udp.c`) wait
behind it.

Same configuration (`alci/elks.config`), toolchain and MartyPC `05c0d08`
plus `elks-bench`; Alci before `aec9884`, after the merge `3b32898`.

| bench (MartyPC 8088, 720K) | before | after |
| --- | --- | --- |
| `cat /b.txt > /dev/null` | 3.856 s | 3.852 s |
| `cat /b.txt` (2nd) | 1.526 s | 1.530 s |
| `ls -l /bin` (1st) | 7.668 s | 7.664 s |
| `ls -l /bin` (2nd) | 3.950 s | 3.950 s |
| boot to prompt | 13.677 s | 13.689 s |
| `cat /bin/sh` | 1.568 s | 1.572 s |
| `cat /bin/vi` | 3.965 s | 3.961 s |
| 10 x `pwd` | 1.710 s | 1.706 s |
| `cat \| cat \| cat` | 3.676 s | 3.672 s |
| `sort` | 5.291 s | 5.287 s |
| `grep` | 2.436 s | 2.436 s |

All within 0.3% (V2). V1: strict suites identical before and after,
34/34 with `check.py` on QEMU hd32 and fd720 and on the 86Box IBM XT
(hd32). V3: `setup` +138 bytes (run once at boot, not resident; the
kernel image keeps its size), boot line `Kernel text 330 ftext 10cc init
16e2 data 1936 end 2936 top 9f80 473+9+0K free` unchanged. V4 and V5:
no timer, tty or serial code in the merged commits.

## Upstream merge up to `a450d52` (2026-11-24)

The rest of upstream master after `a3d4940`: `e47e635` (old `/bootopts`
copy cleared at boot) kept for FAT boot sectors only, `342cf3a` and
`139b823` (ept, the ELKS package tool, on 1440k images, so also on disk
2 of the 720K set), `a450d52` (comment). Alci `e68509c` -> `5209a99`,
same configuration, toolchain and MartyPC.

V2: all eleven `bench.sh` and `bench2.sh` timings within 0.1% of the
`3b32898` figures above (boot to prompt 13.694 s). V1: strict suites
34/34 on QEMU hd32 and fd720 and on the 86Box IBM XT; outputs identical
except `ls /bin | wc -l` on hd32, 149 -> 150 (ept). The FAT 1.44M image
boots on QEMU and reads its `/bootopts`. V3: kernel and resident memory
unchanged (same `Kernel text` line); MINIX boot sector unchanged; FAT
boot sector +6 bytes, 0 left. Disk 2 of the 720K set: 82 files, ept
included (6110 bytes), 13 KB free (was 19). V4, V5: not touched.

## Alci programs on an unmodified ELKS kernel, task F4e1 (2026-11-24)

Question: what does an ELKS kernel without the LZSA1 decoder do with a
compressed Alci program? It must refuse it, never run it.

Setup: ELKS master `a450d52` built with `ibmpc-1440.config` (its default
for PCs: `CONFIG_EXEC_COMPRESS=y`, exomizer decoder; `CONFIG_EXEC_MMODEL=y`,
so the 64-byte header is accepted), hd32 image by `alci/tests/mkimg.sh`.
The 36 LZSA1 programs of the Alci 720K disk (`3b32898`) copied into `/z`
and each run once on QEMU (`alci/tests/foreign-exec.sh`).

| result | programs |
| --- | --- |
| machine code run | 0 of 36 |
| `execve` returns `ENOEXEC` | 36 of 36 |
| system intact afterwards (`ls`, `md5sum`, `meminfo` identical to a clean boot) | yes |

How it refuses: the kernel does not read `esh_reserved`, so it takes the
LZSA1 sections for exomizer data and runs its decoder on them; the decoder
ends with the wrong length and exec fails. This is not a check by design:
the exomizer decoder only stops when its output pointer meets its input
pointer, and on foreign data the input pointer could in principle wrap
below 0 first and let writes leave the segment. None of the 36 did.
After `ENOEXEC` the shell reads the file as a script (as for any file it
cannot exec): lines of binary garbage reported "not found", sometimes a
syntax error, exit status 2. With `CONFIG_EXEC_COMPRESS` off the kernel
refuses compressed headers before decoding (`error_exec3`).

Proposal (for the maintainer; no change made): make every ELKS kernel
refuse an Alci program before any decoding. Options:

1. An Alci header form unknown to ELKS: for example `hlen` = 80 (the
   64-byte header plus 16 bytes) or a different value in the `a_flags`
   byte of `mh.type`. Every ELKS kernel rejects it at the header switch,
   before allocating memory. Cost in Alci: one more case in the header
   check of `exec.c` (estimate under 20 bytes of kernel code, to measure),
   and `elks-pack.py` writes the new form. The shell still prints the
   garbage of the script fallback.
2. Offer ELKS a two-line check that refuses `esh_reserved` values it does
   not know. Protects only kernels built after it.
3. Both: 1 for the kernels that exist, 2 so ELKS knows the field.

## GUI benchmark, task D2 (MartyPC, 2026-11-27)

Contiki CTK (Alci `ab4a4484f` plus the `ctk -G` and `ctk -q N` modes;
its rows measured again with the window move by copy, see "CTK window
move by copy" below), Nano-X (ghaerr/microwindows `3b20527`) and Gem-elks
(parabyte/gem-elks `c5fd7d1`), measured with `alci/tests/guibench` on
MartyPC `elks_xt` (IBM XT, 8088 at 4.77 MHz, 640 KB, GLaBIOS) booting a
720K floppy with the benchmark programs, with CGA and with EGA. Kernel
and base disk: the Alci 720K image of `ab4a4484f`, with only the tools
the scripts need left in `/bin`. Summary and screens in
`alci/docs/gui-study.md`.

Builds: CTK with the Alci Makefile; Nano-X with `nxbuild.sh` (its
`Makefile.elks` flags, VGA/EGA planar or CGA driver, fonts compiled in);
Gem-elks with `gembuild.py` (its `Makefile.elks` flags, medium model).
Gem-elks as published does not start on CGA (no INT 10h AX=1130h for its
font); the CGA numbers are from a build with
`gem-elks-cga-font.patch` and without the Amstrad mouse driver (the
patch alone passes the 128 KB code limit by 32 bytes). EGA numbers are
from Gem-elks as published.

Disk space, everything the desktop needs (plain, and with `elks-pack.py`
LZSA1; GEM's resource files are not packed):

| | Plain | Packed |
|---|---|---|
| CTK `ctk` (CGA; icons and font inside) | 51,440 | 39,871 |
| CTK `ctk-ega` | 57,424 | 41,284 |
| Nano-X server `nano-X` (CGA) | 75,184 | 43,369 |
| Nano-X server (EGA/VGA) | 74,016 | 42,558 |
| Nano-X client (nxcalc) | 21,424 | 15,850 |
| GEM `gem` + `gemdesk` | 157,160 + 92,904 | 113,377 + 68,790 |
| GEM `/lib/gemsys` (rsc, icn, inf) + `gem.conf` | 48,526 + 542 | same |
| GEM total | 299,132 | 231,235 |

Memory (`memlater` with the desktop up and idle, run from a script whose
shell holds 22 KB; with nothing running it finds 347 KB): free and
largest free block, and the desktop's own segments from `meminfo`:

| | CGA free | EGA free | Desktop |
|---|---|---|---|
| CTK | 246K, block 64K | 237K | code 44,960 + data 35,328 (CGA) |
| Nano-X (server, no client) | 235K, block 64K | 237K | code 56,624 + data 35,328; a client about 23K more |
| GEM (server + gemdesk) | 21K, largest block 4K | 21K, block 4K | about 304K by difference |

With GEM up no program can be started (`kill` from the script failed);
the benchmark client and `memlater` are started before GEM.

Start time from the 720K floppy, `stamp` to the last change of the
screen (`vwatch`):

| | CGA | EGA |
|---|---|---|
| CTK (`ctk -q 5`) | 4.39 s | 4.50 s |
| Nano-X (server, root window) | 6.55 s | 6.23 s |
| GEM (server and gemdesk) | 17.58 s | 14.28 s |

Operations in ms, 4 rounds averaged (GEM on EGA: 1 round). Window 3/4 of
the screen wide and 2/3 tall; a line is 60 characters; "full" fills the
window (rows given); "move" moves the window by 1/8 of the screen width.

| CGA 640x200 | screen | open | close | move | line | full | char | menu |
|---|---|---|---|---|---|---|---|---|
| CTK | 230 | 140 | 152 | 165 | 32 | 200 (14) | 25 | 380 |
| Nano-X | 817 | 2230 | 47 | 842 | 450 | 4222 (10) | 60 | - |
| GEM | 3400 | 4717 | 2485 | 4430 | 3067 | 19330 (7) | 882 | - |

| EGA 640x350 | screen | open | close | move | line | full | char | menu |
|---|---|---|---|---|---|---|---|---|
| CTK | 562 | 232 | 395 | 260 | 40 | 335 (14) | 22 | 970 |
| Nano-X | 860 | 2097 | 50 | 1000 | 422 | 6677 (17) | 55 | - |
| GEM | 5290 | 1980 | 1850 | 3140 | 4780 | 57640 (13) | 2150 | - |

"screen": CTK draws its whole desktop, Nano-X its root window, GEM the
desktop below the menu bar (`form_dial` FMD_FINISH). CTK and Nano-X
move a window by copying it in video memory. GEM on CGA in one
round, twice, gave screen 7790 and 2840, open 3360 and 7920: single
rounds of GEM vary by up to a factor of two.

### CTK window move by copy

`patches/ctk-window-move.patch` (applied by `make fetch`) and
`ctk_gfx_move` in `ctk-gfx.c`: a window in front with no other window,
menu or dialog near it is moved as Nano-X moves one. Its inside and
title bar are copied in video memory, the desktop it leaves (and the
icons there) is drawn, then the sides and bottom of its frame. When the
desktop round the window is the same at both places (no icon or icon
column line under it, and on EGA/VGA a move that keeps it in the same
shading band) the frame and shadow are copied too and nothing else is
drawn. CTK's `ctk_window_move` only sets x and y; `ctk_window_moveto`
moves a window and copies it, and the title-bar move (mouse or keys) goes
the same way. The desktop is filled a pixel row at a time across the
cells (EGA/VGA: a block for each shading band) instead of cell by cell.

The same patch makes Escape close a menu, as F1 does. The benchmark
pressed Escape after opening the menu, so in the first measurement of
this table (`f2c94844a`) the menu stayed open in two rounds of four: a
move then drew the whole screen, the menu was drawn with the screen,
and in those rounds the text of the window was not drawn at all (CTK
draws no window while a menu is open). The CTK rows above are from the corrected
benchmark. Its move, before and after the copy (ms, 4 rounds; the move
is 10 columns right and one row down, onto the icon column, so the frame
is drawn):

| | CGA | EGA |
|---|---|---|
| CTK, whole screen drawn (old) | 1077 | 2515 |
| CTK, window copied | 177 | 465 |
| Nano-X | 842 | 1000 |

Parts, measured on their own on CGA / EGA (one call each): copying the
58x15-cell window 47 / 78 ms; drawing the frame 93 / 500 ms, about a
third of it the title bar (now copied); 48 desktop cells 24 ms on CGA,
on EGA 245 ms a pixel row at a time and 9 ms as blocks. Moves of 3
columns (half of them also a row up or down) away from the icons: 117 ms
on average on CGA, where the frame is always copied; 290 ms on EGA, where
the moves up and down cross a shading band and draw the frame. Checked on screen in QEMU (CGA, CGA4, EGA, VGA, MCGA,
AT&T) and 86Box (Hercules, Tandy).

### CTK window switching (Alt-Tab)

Desktop with Calc, Files and the Editor open (the Editor in front, 78
columns), keys pushed by a test build, MartyPC XT, ms:

| | CGA | EGA |
|---|---|---|
| Alt-Tab, the box of windows drawn | 200 | 490 |
| Tab again, the next one marked | 190 | 490 |
| Alt let go, the chosen window to front | 910 | 2670 |
| the same before (the window that was in front drawn again whole) | 2210 | 5140 |
| a quick Alt-Tab (box never drawn) | 1480 | 3420 |

The window that leaves the front now only gets its frame and its focused
widget drawn again; the one that comes to front is drawn whole, which is
most of the time left (a quick Alt-Tab to the 78-column Editor). Checked
with the keys held for real on 86Box (XT, EGA) and in QEMU (CGA).

### CTK: less drawn again (2026-11-27)

Three changes, none of which leaves out anything that changed on the
screen:

- A window brought to front that no window in front of it covered, and
  that missed no change while it was behind, is on the screen already:
  only its frame and its focused widget are drawn (the frame and focused
  widget of the window that leaves the front too). CTK now notes the
  windows whose widgets changed while they could not be drawn (behind,
  under a menu or dialog, during a move); those are drawn whole when they
  come to front or the move ends.
- What lies under the Alt-Tab box and the window menu is kept in memory
  (up to 8 KB, taken while the box is up) and put back when it closes; if
  anything else was drawn meanwhile the rows are drawn again as before.
- The blank end of a text row (labels, editor and shell lines) is filled a
  pixel row at a time across the cells instead of cell by cell (two-colour
  screens, EGA, VGA).

MartyPC XT, ms (D2 table above already with these):

| | CGA before | CGA after | EGA before | EGA after |
|---|---|---|---|---|
| D2: window opened (empty labels) | 530 | 205 | 1380 | 930 |
| Alt-Tab to a window behind the Editor | 910 | 240 | 2670 | 1280 |
| quick Alt-Tab to the Editor (78 columns) | 1480 | 1140 | 3420 | 2870 |
| Calc to front, nothing over it | 170 | 120 | 830 | 550 |

Checked on screen in QEMU (all modes, the move test; Alt-Space; two small
windows, one changed while behind) and 86Box (EGA, Alt-Tab with the keys
held).

### CTK: text written straight to video memory (2026-11-27)

A run of letters on a plain background (every row of the cell the same
colour, which is the case for labels, the editor and the shell) no longer
goes cell by cell through a composed buffer. On CGA one piece of assembly
reads each letter, finds its glyph in the font and writes the eight rows
at their places; on the other two-colour screens a loop writes each row
through a table of row addresses; on EGA and VGA the planes where letter
and background differ take the glyph (or its inverse) in one write for
all of them, a pass for each group, and the other planes are filled. The
last letter of a run still goes the old way, so the cell kept for the
next drawing is the same. Bars, boxes and buttons are unchanged.

The pixels are the same: a fixed scene (lines of different lengths, an
empty line, the editor cursor, frame and icons) was taken before and
after in QEMU (CGA, CGA4, EGA, VGA, MCGA, AT&T, EGA2, VGA2, T1200) and
86Box (Hercules) and compared pixel by pixel: no difference.

MartyPC XT, ms:

| | CGA before | CGA after | EGA before | EGA after |
|---|---|---|---|---|
| the 58x14 window of text drawn again | 639 | 209 | 1227 | 425 |
| D2 "full" | 642 | 210 | 1230 | 430 |
| D2 "line" | 65 | 30 | 97 | 40 |
| quick Alt-Tab to the Editor | 1140 | 570 | 2870 | 1830 |

### CTK: frames and windows drawn again for less (2026-11-27)

Measured by parts first (MartyPC XT, the 78x21 Editor on EGA): its frame
370 ms, clearing its inside 327 ms, its lines 558 ms. One frame cell on
EGA took 6.6 ms to compose (2 ms on CGA). Changes, each checked pixel by
pixel against the code before (QEMU in every mode it has, 86Box for
Hercules; the scene now has a line ending in spaces and a line of spaces):

- Frame cells composed lately are kept (80 cells, with the window's place,
  size and look, the title's length and the icon column line they were
  composed from) and copied when the same frame is drawn again, as at
  every Alt-Tab. Memory: 1.3 KB on CGA, 5.1 KB on EGA/VGA.
- On EGA, VGA and MCGA the sides of a frame are composed once and copied
  down while the desktop under the shadow repeats, as on CGA (a cell row's
  desktop pattern worked out once); the plain cells of a title bar are
  composed once a frame.
- Runs of repeated frame cells (the title bar, the bottom edge) are filled
  a pixel row at a time (on EGA/VGA the planes of one colour in a single
  write) instead of cell by cell.
- Rows a label fills from side to side (editor and shell lines) are not
  cleared before the label is drawn over them.
- A blank on a plain background is one block on EGA/VGA, and the trailing
  spaces of a text line are drawn as a blank.

| ms | CGA before | CGA after | EGA before | EGA after |
|---|---|---|---|---|
| Editor frame | 110 | 72 | 370 | 185 |
| clearing the Editor | 72 | 15 | 327 | 20 |
| Alt-Tab Calc -> Editor (78x21) | 572 | 335 | 1832 | 600 |
| Alt-Tab Editor -> Calc (behind the Editor) | 230 | 190 | 1227 | 477 |
| D2: window opened | 207 | 140 | 930 | 232 |
| D2: window moved | 180 | 165 | 467 | 260 |

Free memory with the desktop up: 240K on CGA (246K before the text and
frame changes), 226K on EGA (237K).

### CTK: hidden copy of the windows (2026-11-27)

A window that leaves the front because another comes over it is copied
away as it is on the screen, and copied back when it comes to front
again, if nothing in it was drawn again in the meantime and it has the
same place, size and title (CTK forgets the copy as soon as one of its
widgets or the window is drawn again, moved or closed). The copy stays
while it is right, so going back and forth between two windows costs no
new copy. Two windows at most, the least used one gives way. EGA and VGA
keep them in the video memory past the screen (EGA needs 256 KB; copied
through the latches, four planes at once); CGA, the CGA four-colour
modes, Tandy, AT&T and the T1200 in far memory (`fmemalloc`, at most 32
KB a window: 14 KB for the Editor on CGA), given back when the window is
forgotten. Hercules keeps them in its second page (B800, 32 KB, mapped
but never shown; checked at the first copy, so a card with 32 KB keeps
none). MCGA keeps none (no room past the screen, and a copy in memory
would not be faster than drawing on MCGA).

Checked pixel by pixel against the same build without the copies (QEMU,
CGA, EGA and VGA): Alt-Tab back and forth with the Editor and the
calculator, typing in each window between the switches, a move with
Alt-Space and Alt-F4; every screen the same. Hercules the same way on
86Box, with the keys held as on a real keyboard.

| ms (MartyPC XT) | CGA before | CGA after | EGA before | EGA after |
|---|---|---|---|---|
| Alt-Tab Calc -> Editor (78x21) | 335 | 155 | 600 | 315 |
| Alt-Tab Editor -> Calc (behind the Editor) | 190 | 125 | 477 | 227 |
| D2: window opened | 140 | 140 | 232 | 237 |
| D2: window moved | 165 | 167 | 260 | 262 |

Hercules (MartyPC XT, 720x348): Alt-Tab to the Editor 820 -> 280 ms, to
the calculator behind it 650 -> 280 ms.

From the start of this work (before "less drawn again"), a quick Alt-Tab
to the Editor went from 1480 to 155 ms on CGA and from 3420 to 315 ms on
EGA.

Not measured yet: start from a 1.44M floppy and the hard disk, start of
the packed programs, the 86Box PC1512 and AT, and the response to a key
for Nano-X (its ELKS client library has no `GrInjectKeyboardEvent`) and
GEM.

## Upstream merge up to `8563663` (2026-11-27)

Six upstream commits: full VGA hardware initialisation for graphics
programs on the protected mode kernel (`vga-hwinit`, selector for the ROM
font), a rewritten `runningPM`, and the disk wheel fix offered from Alci
(`7ecbce5`), which conflicted with Alci's copy in comments only (upstream
comments taken). The real mode kernel and every file of the Alci 720K
target are identical to the build of `426a7a7`, apart from the version
string in the kernel, the build time inside `ktcp` and one commented line
added to `/bootopts`. Behaviour, timings and resident memory therefore
unchanged; no check run beyond the comparison of the built files.

## ELKSWeb compressed and plain, task F4e2a (2026-11-27)

ELKSWeb 0.6 (l00nix/ELKSWeb `bdd71c1`, GPL-2.0-or-later) built with
`buildext.sh elksweb` on Alci `9889e1a` (upstream ELKS `8563663`). It is a
medium-model program: 58608 bytes of text, 20112 of far text, 14688 of
data, 38896 of bss, 1576 bytes of relocations. Compressed with
`elks-pack.py` (LZSA1, in-place decoding by the kernel).

| | plain | LZSA1 |
| --- | --- | --- |
| text / far text / data on disk | 58608 / 20112 / 14688 | 43599 / 16112 / 8132 |
| file | 100248 bytes, 98 KB blocks | 74683 bytes, 73 KB blocks (-25%) |
| file gzip -9 (transfer size of a gzipped package) | 62893 | 66624 |
| memory taken while it runs | same in both forms (the decoded program) | same |

Start time on MartyPC `elks-bench` `e1c5187` (IBM XT, 8088 at 4.77 MHz),
booting the `bench.sh` 720K image with `/bin/elksweb` added (plain: 3
blocks left on the disk; LZSA1: 28), command `elksweb --version`, which
loads the whole program and exits. Without and with the floppy rotation
model (`ELKS_FDROT=9`):

| run | plain | LZSA1 | plain, rotation | LZSA1, rotation |
| --- | --- | --- | --- | --- |
| first start (from the disk) | 8.261 s | 7.976 s (-3.4%) | 10.269 s | 9.456 s (-7.9%) |
| second start, at once | 0.549 s | 0.637 s | 0.754 s | 0.692 s |
| third start, after `ls /` | 0.302 s | 0.419 s | 0.302 s | 0.423 s |

The first start is faster compressed (25 fewer blocks read from the
floppy). Starts from the text cache take about 0.12 s more compressed (the
cause is not measured yet). For transfer over a
slow link, a gzipped plain file is the smallest (63 KB), but the ELKS
side would need a gzip decoder; files sent as they are favour LZSA1
(75 KB instead of 98 KB). Not measured yet (F4e2b): start from a 1.44M
floppy and from the hard disk on the 86Box XT and AT, and peak memory
while a page is loaded. No change to the kernel, libc or the programs
of the disks; vetoes not applicable (measurement only).

## LZSA1: one-segment decoding and data copies, task F4f (2026-11-28, dropped)

Tried and taken out again (commit `c13e9bdc9`, reverted by the next):
about 0.1 s saved per restart of a program with 10 to 15 KB of data on
the XT, nothing on first starts from the floppy and on ordinary programs,
for about 1 KB more kernel. Not worth it. The measurements:


Two changes to the start of LZSA1-compressed programs (`CONFIG_EXEC_LZSA`
with `CONFIG_EXEC_TEXT_CACHE`):

1. **One-segment decoder.** `lzsa1dec.S` holds the decoding loop twice:
   as before, with source and output in two segments, and with both in one
   segment, where a match copies from the output without switching DS
   (four segment moves fewer at each match). `exec` takes the second when
   the whole in-place layout (output, margin, compressed bytes) fits in
   64 KB, else the first; every text, far text and data section chooses on
   its own.
2. **Data copies.** A program with LZSA1 data decoded it again at every
   start, even with its text in memory. After decoding, `exec` now keeps a
   copy of the decoded data (before relocation) in memory nobody wants
   (`seg_alloc_spare`: free memory only, nothing pushed out; shown as free
   by `meminfo`), and the next start of the same file copies it. Up to four
   copies, freed first when memory is short, and with the text entries of
   their file (written, size or time changed, unmounted).

MartyPC `elks-bench`, IBM XT, floppy rotation model on, emulated seconds.
Test program `bigp` (33 KB of code and 15 KB of initialized data, LZSA1
22.7 KB on disk), four copies `biga` to `bigd` on the 720K bench image; the
three kernels on the same image (only `/linux` replaced, so the files are
at the same places):

| Command | before | decoder only | both |
| --- | --- | --- | --- |
| `biga`, first start | 5.222 | 5.214 | 5.260 |
| `biga` x10 | 2.587 | 2.494 | 1.400 (-46%) |
| `bigb; bigc; bigd`, first starts | 7.314 | 7.205 | 7.297 |
| `bigb; bigc; bigd; biga` x2, then again | 3.643, 2.109 | | 1.865, 1.115 (-47%) |

The decoder alone saves about 3 cycles per byte produced (8%; estimated
15 to 25% before measuring: the time per match is less of the total than
assumed). Making a data copy costs about 0.04 s at the first start of a
program with 15 KB of data. Ordinary programs of the 720K image (`grep`,
`sed`, `sort`, `ls`, small data): within 1 to 3% either way, the same on
first and repeated starts.

Checks: strict suite 34/34 on QEMU (720K floppy at 640 and 256 KB, hard
disk), 86Box IBM XT and Toshiba T1200 (720K floppy) and IBM AT 512 KB
(hard disk); `lzsa1-corrupt.py` (twelve damaged copies of `grep`): every
one rejected or exits, the system always answers. Kernel: about 1 KB more
(`/linux` 109624 -> 110704 bytes).

## Track reads from the sector under the head, task A3 point 3 (2026-11-28, not kept)

Tried: a new track read in two requests, from the sector expected under
the head (worked out from the PIT time since the last read ended, the
sector it ended on and 30 ms for a one-cylinder step) to the end of the
track, then from its start. Not committed (patch kept outside the tree).

86Box IBM XT (real BIOS, 720K floppy), serial-console image, emulated
seconds: boot to prompt 45.87 -> 46.67 s (worse); first starts `sed` 2.54
-> 2.34, `sort` 1.57 -> 1.47, `ls -l /bin` 6.09 -> 6.00, `grep` 2.10 ->
2.10. MartyPC: no gain either. MartyPC's rotation model places sectors
without gaps and transfers them faster than they pass under the head, so
it does not show where a real drive is after a read: the 150-190 ms waits
for sector 1 measured there (A3) are partly the model's. The real waits
must be measured on 86Box (or hardware) before deciding on this again.

## Floppy boot on 86Box, ELKS against Alci (2026-11-28)

86Box IBM XT (8088 4.77 MHz, 640 KB, IBM BIOS, CGA, 720K drive, screen
console), fd720-minix images built from ELKS `3332410` and Alci
`15cf295cc` with the same configuration, emulated seconds from power-on,
times taken when the text appears on the screen (86Box automation
`wait text`); repeated runs gave the same times to 10 ms:

| | ELKS | Alci |
| --- | --- | --- |
| boot loader running (`/linux` shown) | 16.31 | 16.06 |
| kernel started (`START`) | 44.63 | 27.49 |
| root mounted | 46.68 | 29.48 |
| `/etc/rc.sys` running | 55.07 | 36.44 |
| date asked (Alci only, no clock; Enter typed at once) | - | 38.14 |
| `login:` | 58.82 | 40.39 |
| `sed -n 1p /etc/passwd`, first run (from the floppy) | 2.19 | 1.69 |
| same, second and third run | 0.52 | 0.31 |

The kernel loads in 28.3 s with the ELKS loader (one 1 KB block per BIOS
call: the next sectors have passed under the head and the drive waits
most of a turn) and 11.4 s with Alci's (contiguous blocks in one call).
The MartyPC floppy tables in this file (`ELKS_FDROT=9`) give shorter
times: MartyPC returns sectors without the gaps between them and does
not hold the program while the disk turns, so they compare versions on
the same model but are not times of a real drive. Boot to prompt 21.02 /
18.36 s there against 58.82 / 40.39 s (to `login:`) here; the 16 s of
the IBM BIOS (memory test of 640 KB) is also absent on MartyPC's GLaBIOS.

## Emulator updates

Merges of upstream MartyPC (`dbalsom/martypc` into `elks-bench`) and 86Box
(`86Box/86box` into `proxybarrio/86box` branch `alci`), with the timings
of the same Alci build before and after. As of 2026-11-18: MartyPC
`elks-bench` is based on upstream `05c0d08` (2026-09-07), the latest on
`main`; 86Box is built from upstream `960d6b7` (2026-11-17) plus the
PicoMEM patch.

| Date | Emulator | from -> to | what changed for us | timings |
| --- | --- | --- | --- | --- |
| 2026-11-27 | both | none | MartyPC `main` has nothing new after `05c0d08`. 86Box upstream `7de8cec`: about 500 commits since 2026-11-18, not merged (no `alci` branch yet, A6); relevant: FDC keeps its 16-byte FIFO for non-DMA transfers, pacing fixed after a CPU block size change; the rest mostly video cards (QVision, Rage 128), sound and translations | - |
| 2026-11-24 | both | none | MartyPC `main` has nothing new after `05c0d08`. 86Box upstream `141ae59`: 332 commits after `proxybarrio/86box` master `48b41fd`, not merged (no `alci` branch yet, A6); relevant: FDC drive select gated by motor on (`2ae9966`, IBM compatibles), automation and debugger can lock the user's controls (`15487532`), an 808x core swap reverted the same week; the rest mostly 386+ protected mode and dynarec | - |
| 2026-11-20 | both | none | MartyPC `main` has nothing new after `05c0d08`; 86Box upstream has one commit after `proxybarrio/86box` master `48b41fd` (`f24f956`, UMC 866x parallel port ECP), not merged | - |
| 2026-11-19 | 86Box | `960d6b7` -> `d4f9a04` | `proxybarrio/86box` has no `alci` branch yet (A6), so `setup.sh` built upstream master plus the PicoMEM patch; needs `libarchive-dev` | functional tests only, no timings compared |

## Merges to review

Commits whose merge into `alci` changed one of our measurements (see
`ACCEPTANCE.md`, checks V1 to V5). Usually this means Alci needs
adapting; the maintainer decides how.

| sha | title | check | numbers | decision |
| --- | --- | --- | --- | --- |
| `e47e635` | [boot] Fix reusing deleted FAT /bootopts after a warm reboot | build | adds 6 bytes at the start of `boot_sect.S`; Alci's MINIX boot sector has 0 bytes free (`end_of_code` = `elks_parms_start` = 0x1F3 since the one-request zone reads of A3), so the build stops with "attempt to move .org backwards" | 2026-11-24: taken for FAT boot sectors only (`77dcbc7`); the MINIX loader already clears the old copy in `load_prog()`. FAT boot sector now full (0 bytes free) |
