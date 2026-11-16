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
| 10 runs of `pwd` | 2.26 s | 2.03 s | various |
| Boot to prompt, with floppy rotation model (`ELKS_FDROT=9`) | 21.0 s | 18.4 s | block reads in boot loader, two-head floppy reads |

On an 8086 at 4.77 MHz the relative gains are the same; where the floppy
dominates they are slightly smaller.

## Costs

- Kernel: about +550 bytes of code in total, +34 bytes of data.
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
