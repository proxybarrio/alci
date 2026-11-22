**Alci 0.1 preview**: the first public images of Alci, an experimental branch of [ELKS](https://github.com/ghaerr/elks) (the Linux-like system for 8088 to 386 PCs by Alan Cox, Greg Haerr and the ELKS team), tuned for speed and memory on the slowest machines.

**This is a preview, tested on emulators only** (QEMU, 86Box with real BIOSes: IBM XT, Toshiba T1200, Amstrad PC1512, IBM AT; MartyPC, cycle-accurate 8088). It has not yet run on real hardware. That is where you come in: if you try it, please tell us how it went, even if it just worked, with the **Alci hardware or emulator report** form under [Issues](https://github.com/proxybarrio/alci/issues/new/choose).

## Images

| File | For | Notes |
| --- | --- | --- |
| `alci-0.1-preview-fd360.img` | 5.25" 360K drives (PC, XT) | Reduced program set |
| `alci-0.1-preview-fd720-disk1.img` | 3.5" 720K drives (XT, Toshiba T1200, laptops) | Boot disk, base system and networking |
| `alci-0.1-preview-fd720-disk2.img` | Second 720K disk | The other programs of the 1.44M image; not bootable, see below |
| `alci-0.1-preview-fd1200.img` | 5.25" 1.2M drives (AT) | |
| `alci-0.1-preview-fd1440.img` | 3.5" 1.44M drives | Full program set, programs compressed |
| `alci-0.1-preview-hd32.img` | Hard disk, CF card, XT-IDE, emulators | 32 MB with partition table; full set plus the Bobcat web browser |

All images boot the same system on 8088, 8086, V20, 286 and 386 machines. Log in as `root` (no password).

**720K disk 2.** With two drives: boot disk 1 in A, put disk 2 in B, then `mount -o ro /dev/fd1 /mnt` and `. /mnt/setup`. On a system installed on a hard disk: `sh /mnt/install-hd` copies its programs there. With a single drive, do not swap disks while the system runs: safe disk swapping comes in a later release.

**Bobcat** (text web browser derived from Lynx, ported to ELKS by its maintainer), hard disk image only. It needs about 300 KB of free memory: on a 640 KB machine, uncomment `buf=16` in `/bootopts` and start the network without the telnet and FTP servers (`netstart=""` in `/etc/net.cfg`, or just `ktcp -b -p ne0`). HTTP only. Making it run with the default settings is on the roadmap.

## Writing the images

- Linux: `dd if=alci-0.1-preview-fd720-disk1.img of=/dev/sdX bs=512 conv=fsync` (USB floppy drive; for 720K disks use DD media and a drive that supports 720K, `ufiformat -f 720` to format).
- DOS: `rawrite` or similar; Greaseweazle and other flux tools work too.
- Check the files against `SHA256SUMS` first.

## What is different from ELKS

Measured on the same configuration (details in [`alci/RESULTS.md`](https://github.com/proxybarrio/alci/blob/alci/alci/RESULTS.md)): writing to the screen up to about 10 times faster, `ls -l` about 3 times faster, pipes about 3 times faster, program start-up faster on every CPU; programs on floppies are compressed and decoded by the kernel, which leaves about 120 KB free on the 720K disk instead of about 40; unused disk and network driver code is given back to programs at boot; fixes for lost serial characters on the XT, the RTC year after 1999, memory fragmentation on 512 KB ATs, impossible dates in `clock -c`, and the Kilo editor losing the file on a failed save. Everything else, programs included, comes from ELKS 0.9.2.

## Known limits

- Tested on emulators only, at 640 KB. Machines with less memory, unusual video cards or floppy controllers are exactly what we want reports about.
- No safe floppy swapping yet (see disk 2 above).
- After a graphics program (for example `paint`) returns to the shell, the prompt may stay hidden until the screen scrolls: press Enter a few times. Fixed for the next release.
- The hard disk image's kernel includes the OS/2 program loader for Bobcat (1.7 KB more than the floppy kernels).

## Source and licence

GPL version 2, as ELKS. Source: tag `v0.1-preview` of this repository (`alci/scripts/mkrelease.sh` builds these images). Bobcat: GPL (Lynx base), source attached as `bobcat-source-*.tar.gz` and at [rafael2k/bobcat](https://github.com/rafael2k/bobcat). The plan and the reasons behind each change: [`alci/STRATEGY.md`](https://github.com/proxybarrio/alci/blob/alci/alci/STRATEGY.md), [`alci/TASKS.md`](https://github.com/proxybarrio/alci/blob/alci/alci/TASKS.md).
