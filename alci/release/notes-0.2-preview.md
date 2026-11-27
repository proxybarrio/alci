**Alci 0.2 preview**: Alci, an experimental branch of [ELKS](https://github.com/ghaerr/elks) (the Linux-like system for 8088 to 386 PCs by Alan Cox, Greg Haerr and the ELKS team) tuned for speed and memory on the slowest machines, now with a graphical desktop.

**This is a preview, tested on emulators only** (QEMU, 86Box with real BIOSes: IBM XT, Toshiba T1200, Amstrad PC1512, IBM AT; MartyPC, cycle-accurate 8088). It has not yet run on real hardware. If you try it, please tell us how it went, even if it just worked, with the **Alci hardware or emulator report** form under [Issues](https://github.com/proxybarrio/alci/issues/new/choose).

`alci-demo-xt-cga.mp4` shows a session on an IBM XT (8088 at 4.77 MHz, colour CGA, 86Box) at real speed from the 720K floppy: boot, shell, then the desktop with the mouse and the keyboard.

## New in 0.2

- **Graphical desktop.** The [Contiki 1.x](https://github.com/adamdunkels/contiki-1.x) CTK desktop, ported to Alci and redrawn: windows with title bars and close boxes, menus, dialogs and icons; a file manager, a text editor, up to three shell windows, a calculator and an About box. Type `desk` at the shell prompt: it asks the video BIOS which screen there is and starts the matching program (`ctk` for CGA, `ctk-cga4`, `ctk-ega`, `ctk-vga`, `ctk-mcga`, `ctk-herc`, `ctk-tandy`, `ctk-tandy4`, `ctk-att`, `ctk-t1200`, `ctk-ega2` and `ctk-vga2` for monochrome monitors). `DESK=cga4 desk` chooses one by hand. Alt+F4 on the empty desktop, then Quit, goes back to the shell.
- **Mouse and keyboard as on Windows and Xfce.** Microsoft serial mouse on COM1 (`MOUSE_PORT=/dev/ttyS1` for COM2), or the pointer from the keyboard. Alt+Tab (hold Alt to see the open windows), Alt+Esc, Alt+F4, Alt+Space for the window menu (move with the arrow keys), the Windows key or F1 for the main menu. A click on a title bar picks the window up, the next click puts it down.
- **Fast on the 8088.** On the XT a quick Alt+Tab to the full-screen editor takes 0.16 s on CGA and 0.32 s on EGA (1.48 s and 3.42 s with the original CTK drawing); the desktop starts in 4.4 s from the floppy. Measurements against Nano-X and GEM in [`alci/RESULTS.md`](https://github.com/proxybarrio/alci/blob/alci/alci/RESULTS.md#gui-benchmark-task-d2-martypc-2026-11-27).
- **Kernel.** The disk activity wheel no longer draws into a graphics screen (stray pixels on CGA). ELKS master of 2026-11-25 merged.

## Images

| File | For | Desktop programs on it |
| --- | --- | --- |
| `alci-0.2-preview-fd360.img` | 5.25" 360K drives (PC, XT) | none (no room) |
| `alci-0.2-preview-fd720-disk1.img` | 3.5" 720K drives (XT, Toshiba T1200, laptops) | `desk`, CGA two and four colours |
| `alci-0.2-preview-fd720-disk2.img` | Second 720K disk | none (no room); the other programs of the 1.44M image, not bootable, see below |
| `alci-0.2-preview-fd1200.img` | 5.25" 1.2M drives (AT) | `desk`, CGA two and four colours, MCGA |
| `alci-0.2-preview-fd1440.img` | 3.5" 1.44M drives | `desk`, CGA two and four colours, EGA |
| `alci-0.2-preview-hd32.img` | Hard disk, CF card, XT-IDE, emulators | `desk` and every screen type; also the Bobcat web browser |

When the program for the screen is not on the disk, `desk` starts one the card can also show (EGA on VGA, CGA on the other colour screens); on the floppies the Toshiba T1200 runs the CGA build. All images boot the same system on 8088, 8086, V20, 286 and 386 machines. Log in as `root` (no password).

**720K disk 2.** With two drives: boot disk 1 in A, put disk 2 in B, then `mount -o ro /dev/fd1 /mnt` and `. /mnt/setup`. On a system installed on a hard disk: `sh /mnt/install-hd` copies its programs there. With a single drive, do not swap disks while the system runs: safe disk swapping comes in a later release.

**Bobcat** (text web browser derived from Lynx, ported to ELKS by its maintainer), hard disk image only. It needs about 300 KB of free memory: on a 640 KB machine, uncomment `buf=16` in `/bootopts` and start the network without the telnet and FTP servers (`netstart=""` in `/etc/net.cfg`, or just `ktcp -b -p ne0`). HTTP only.

## Writing the images

- Linux: `dd if=alci-0.2-preview-fd720-disk1.img of=/dev/sdX bs=512 conv=fsync` (USB floppy drive; for 720K disks use DD media and a drive that supports 720K, `ufiformat -f 720` to format).
- DOS: `rawrite` or similar; Greaseweazle and other flux tools work too.
- Check the files against `SHA256SUMS` first.

## What is different from ELKS

Measured on the same configuration (details in [`alci/RESULTS.md`](https://github.com/proxybarrio/alci/blob/alci/alci/RESULTS.md)): writing to the screen up to about 10 times faster, `ls -l` about 3 times faster, pipes about 3 times faster, program start-up faster on every CPU; programs on floppies are compressed and decoded by the kernel, which leaves about 120 KB free on the 720K disk instead of about 40; unused disk and network driver code is given back to programs at boot; fixes for lost serial characters on the XT, the RTC year after 1999, memory fragmentation on 512 KB ATs, impossible dates in `clock -c`, and the Kilo editor losing the file on a failed save; the graphical desktop above. Everything else, programs included, comes from ELKS.

## Known limits

- Tested on emulators only, at 640 KB. Machines with less memory, unusual video cards or floppy controllers are exactly what we want reports about.
- The desktop itself is about 80 KB of code and data; on a 640 KB XT with CGA about 240 KB stay free with it running. On CGA the windows behind keep a copy of up to 32 KB each in that free memory.
- No safe floppy swapping yet (see disk 2 above).

## Source and licence

GPL version 2, as ELKS. Source: tag `v0.2-preview` of this repository (`alci/scripts/mkrelease.sh` builds these images). The desktop: Contiki 1.x, BSD 3-clause, Copyright (c) 2002-2004 Adam Dunkels, downloaded at a fixed revision by `make -C alci/contiki fetch` and changed as described in [`alci/contiki/README.md`](https://github.com/proxybarrio/alci/blob/alci/alci/contiki/README.md), which also lists the licences of its fonts and mouse code. Bobcat: GPL (Lynx base), source attached as `bobcat-source-*.tar.gz` and at [rafael2k/bobcat](https://github.com/rafael2k/bobcat). The plan behind each change: [`alci/STRATEGY.md`](https://github.com/proxybarrio/alci/blob/alci/alci/STRATEGY.md), [`alci/TASKS.md`](https://github.com/proxybarrio/alci/blob/alci/alci/TASKS.md).
