# How os8088 is distributed and why: a study for Alci

Study of 2026-11-19, from the os8088 repository (github.com/jggonz/os8088,
commit `90e7ee1`, 2026-11-18): `README.md`, `CLAUDE.md`, `SPEC.md`
sections 2.8, 19, 20.13, 24, 52.10 and 92. Ideas only: nothing here is
copied code. os8088 is MIT, so code could be adapted with its notice kept
(`RELATED.md`), except `reference/` and the files derived from the Word
1.1a source, which carry no licence grant and stay out.

## What os8088 is

A graphical operating system for the 8086 in the style of the first
Macintosh, written in real-mode assembly, booting from a floppy straight
into the desktop: overlapping windows, menus, a dock, pre-emptive
multitasking. Its own kernel (not Unix), one segment per program, FAT12
and FAT16 disks. The small kernel boots a 128 KB machine; the kernel is
about 54 KB of code and data, 103 KB with buffers and stacks, and the
build fails if a memory budget is exceeded.

## How it is distributed

1. **Two disks, by role.** A *system disk* (drive A:) that boots into the
   desktop, and an *apps disk* (drive B:) with the programs. Both exist in
   four sizes from one build: 360K, 720K, 1.2M, 1.44M.
2. **Core programs ride the system disk as a second copy, never a move.**
   Seven small programs (Note Pad, Paint, Browser, Telnet, Mines, a font
   viewer, a benchmark) are on the system disk too, so a machine with only
   that disk can already write, browse and connect. The full set stays on
   the apps disk. When the 360K system disk ran out, the project removed
   core programs one at a time, each with a written reason and a date;
   Telnet was the one that stayed longest, for the reason below.
3. **Network on the system disk.** The disk that carries the network
   driver also carries the programs that use it (Telnet, Browser) and
   "The Wire", a 12 KB program that lists the project's software online
   and either runs a program straight from memory or writes it to a floppy.
   "A networked XT with one 360KB drive reaches the whole collection
   without a second disk."
4. **Disks by subject when space is short.** At 360K the programs are split
   into office, network, games, emulators, utilities and media disks: one
   subject per disk, programs at the root, the documents they open in a
   `MEDIA/` folder, and each program's settings in `SYSTEM/APPDATA/` rather
   than among the user's documents. An "everything" set puts every program
   on as many 1.44M floppies as it takes, with a `CONTENTS.TXT` map on each.
5. **Standard disks.** Every disk is plain FAT12: any DOS, Windows, macOS or
   Linux machine can read and write it, so files move between old and new
   machines with no special tool. Everything read off a disk is still
   checked before use.
6. **Compression that is fast to undo.** Every program, driver and data
   file, and the kernel itself, is stored LZ4-compressed. LZ4 was chosen
   over a format with a 10% better ratio because it expands about four
   times faster: "a disk saving once against a launch cost for ever". On
   an 8088 a disk read costs far more than the expansion, so smaller files
   load faster.
7. **Kernel code loaded on demand.** Features such as the formatter, disk
   copy or the control panel are files on the system disk, read into memory
   when asked for and freed afterwards. When that file is needed and the
   system disk is not in the drive, the system asks: "Insert the system
   disk in A:", then "Insert the disk to format, then Enter". Two drives, a
   hard disk or a second use in the same session ask nothing.
8. **Swap detection learned the hard way.** A changed disk is recognised
   by a signature of its boot sector. Because the build gave every disk the
   same volume serial, three different floppies looked identical, and a
   swap left the old disk's directory and allocation table in use against
   the new one, so a write could damage it. The fix: a serial per disk.
9. **Installation on a hard disk.** An installer copies both floppies onto
   a hard-disk partition and makes it bootable; the same disk is also
   published as a live USB stick and CD image.

## What Alci can take from it (strategy, not code)

| os8088 | Alci, proposed |
| --- | --- |
| System disk boots to a usable desktop, core apps copied on it | Disk 1 stays a complete working system; later it boots to the Nano-X desktop with a few core programs. Copies, never moves. |
| Network driver and its programs on the system disk, plus The Wire | Matches the decision to keep `telnet telnetd ftp ftpd miniterm` on disk 1; `alci-get` (F2) goes on disk 1 too. |
| Apps disk in B: on two drives | Already decided: disk 1 in A:, disk 2 in B:. |
| Disks by subject, documents in a folder | Disks 2 and 3 by subject (tools, connections); Personal Storage disks for documents. |
| Plain FAT12 disks | Personal Storage disks in FAT (ELKS already has `CONFIG_FS_FAT=y` and `mkfat`), so documents can be read on any modern PC. The system disks stay MINIX. To check: how much memory and code FAT costs, and long names. |
| LZ4 everywhere | Task F4b: replace the slow exomizer decoder in `exec` with LZ4 (an 8086 decoder exists in os8088 under MIT) and measure again. If it is faster than reading the plain file, compression frees space on disk 1 at no cost in speed. |
| On-demand kernel modules, with a swap prompt | Same idea as tasks D3 and A5 (code released when unused); F5 must ask for the system disk the same way. |
| Unique serial per disk | F5: every Alci disk gets a unique number in its label; never trust a pinned serial. |
| Installer and live images | F3 step 1; live images for later. |
| Build fails on a memory budget | Alci measures by hand (`AGENT.md` V3); a build-time check is worth adding. |

What differs, and stays different: os8088 is its own kernel in assembly
with a Macintosh-style desktop built in. Alci is ELKS, a small Unix in C,
where programs, shell, pipes and multiple users already work, and the
interface is meant to be Nano-X in the spirit of GEOS. The lesson to take
is the organisation of the disks and the care for the slow machine, not
the architecture.

## Open questions for the maintainer

- Personal Storage in FAT (readable everywhere) or MINIX (same as the
  system)?
- Which core programs should disk 1 carry once the desktop exists?
- LZ4 in `exec` first (F4b), before any new layout of the disks?
