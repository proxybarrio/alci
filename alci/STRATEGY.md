# Alci strategy

Decisions taken so far, and the reasons for them. `TASKS.md` turns them into
work; this file says what the work must respect. Last updated 2026-11-17.

## Goals, in order

1. A faster ELKS that uses as little memory as possible, on every PC from
   the 8088 to the 80386. Memory comes before speed: the graphical
   interface will need every free kilobyte.
2. A graphical interface in the spirit of PC/GEOS.
3. A terminal for talking to Claude or other language models.

What these goals are for. Alci has several uses, none of them secondary:

- Writing and thinking without distraction: an editor and a Claude
  terminal, with no graphical browser, notifications or background apps to
  pull attention away. The text browser (task E6, Lynx or Bobcat style) is
  there for reading and research, and can be left out of a writing
  session. This can help people with ADHD or other attention
  difficulties. Claude should stay quiet while the user writes and answer
  only when called (to reread, summarise or check), so that it does not
  become the distraction itself.
- Reuse: old PCs get a useful second life instead of becoming e-waste.
- Education: a whole system small enough to be read and understood, from
  the boot sector to the shell, on hardware students can open and probe.
- Resilience: simple, repairable, low-power machines that do useful local
  work (writing, reading, archives, text web browsing, simple networking)
  without depending
  on recent hardware or on online services, and keep working where those
  are scarce, expensive or unavailable.

The reference machine is a generic 8088 at 4.77 MHz with CGA. A
real Toshiba T1200 (8086 at 9.54 MHz, 720K floppy, 20 MB hard disk) is the first
real machine for tests, not the design target.

## One image, code only where it is needed

- One system image for every supported machine. Hardware is detected at
  boot and the code for what is absent is released: EDD support already
  works this way (task A0); network card drivers are next (A5) and every
  optional driver follows the same rule.
- Speed-ups must not cost memory. Every change states its cost in bytes,
  measured against upstream (`RESULTS.md`).

## Where the system lives

The hard disk is the preferred home. Wherever a hard disk is present, Alci
is installed on it in full, and the floppies are only the installation
media. Floppy-only operation exists for machines without a hard disk.

| Machine | How Alci runs |
| --- | --- |
| Hard disk present | Full install on the hard disk from the disk set (F3, step 1); the installer is on disk 1 of every distribution. No disk swapping. |
| Two floppy drives | Disk 1 in A:, the other disks mounted from B: on request (F3, step 2). |
| One floppy drive, no hard disk | Disk 1 stays the system disk; other disks are swapped in on request, with kernel support for a safe swap (F3, step 3, built last). |

The base disk is a 720K floppy, the drive of the T1200 and the common
denominator of the period. It holds what is needed to boot, work, communicate (telnet,
ftp, miniterm) and find the rest. The rest lives on further disks of the set and in a package
repository reachable once the system is online (F2). One catalogue says,
for each program, which disk or package holds it. A single program must
fit on one disk; in practice RAM is the stricter limit.

## Reaching the network and Claude

Alci uses the best path the machine has, in this order:

1. Wi-Fi modem on the serial port (ESP32): the modem does TLS, so only the
   desk cable carries clear text. Works on every machine, laptops included,
   and costs Alci almost no memory (E0-E3).
2. Our own relay, reached through any network path, with each message
   sealed by ChaCha20-Poly1305 under a key shared once with the relay. The
   Claude API key stays on the relay (E0c).
3. Native HTTPS from Alci itself (BearSSL), for machines with a network
   path and nothing else. The handshake takes minutes on an 8088; every
   program that uses it says so first, with the wait scaled to the
   detected CPU (E5). This also opens the way to a text web browser (E6).

Network paths: PicoMEM or another NE2000 card (works in emulation with no
new kernel code, E0b), SLIP over the serial port, or the modem.

## How work is accepted

- Every change passes the vetoes and the gain threshold in `AGENT.md`:
  identical behaviour, no slower benchmark, no extra resident memory, no
  clock drift, no lost characters, no new BIOS dependency without a
  fallback.
- Behaviour is checked on QEMU and on 86Box machines with their real BIOS
  (IBM XT, NEC V20, Toshiba T1200 with its own disk, Amstrad PC1512, IBM AT,
  386); speed on MartyPC. Real hardware for changes that touch the BIOS,
  the floppy controller or the boot sector, and before a release.
- Every timing names both versions compared: the upstream ELKS commit and
  the Alci commit.

## Working with others

- Code copied into the kernel, or into a program together with GPL code,
  must have a licence compatible with GPLv2. Separate programs that only use
  the kernel through system calls may keep their own licence: the ELKS
  `LICENSE` file says software beside the kernel may be under different free
  licences, and GPLv2 does not extend to works merely aggregated on the same
  medium (the same model as Android's Apache 2.0 userland on Linux). A port
  of PC/GEOS (Apache 2.0) can therefore ship with Alci as a separate program
  under Apache 2.0, provided it does not link GPL code (ELKS's libc mixes
  LGPL, GPL and unmarked files: audit before linking, avoid GPL parts such as
  termcap, and allow relinking for LGPL parts), its LICENSE and NOTICE files
  travel with it, and no GEOS code goes into the kernel. Programs that run
  outside ELKS (modem firmware, relays) are used as they are. See
  `RELATED.md`.
- The Alci desktop is Nano-X, improved in look and behaviour in the spirit
  of GEOS (Commodore 64 first, then PC/GEOS), the way Haiku follows BeOS.
  Ideas and style are free to follow; GEOS code, icons and fonts stay out of
  GPL code, os8088 code (MIT) may be adapted with its notice kept, and the
  GEOS name is not used for our desktop. Details in task D4.
- Changes useful to ELKS in general are offered to the ELKS project
  regularly, with measurements, for its maintainers to consider. The first,
  the fsck fix for inodes above 1024, was merged as ghaerr/elks #2853 on
  2026-11-16.

## Criteria for 1.0

Alci reaches 1.0 when all of these hold:

1. The three goals are done: faster than ELKS with less memory, a
   graphical interface, a terminal for Claude.
2. Tested on real machines, not only in emulation: at least an XT, a
   Toshiba T1200 and an AT.
3. Installation on a hard disk with an installer, plus the floppy set for
   machines without one.
4. A way to update the system and add programs (`alci-get`).
5. User documentation, besides the developer notes.
6. The automated test suite passes on every emulated machine, and no known
   bug can lose or corrupt data.

## Not now

Modern PCs (USB stick, UEFI, USB networking) wait until everything above is
finished on 8088 to 80386 machines (`TASKS.md`, phase 7, frozen).
