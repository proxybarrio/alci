# Alci strategy

Decisions taken so far, and the reasons for them. `TASKS.md` turns them into
work; this file says what the work must respect. Last updated 2026-11-19.

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
  boot and the code for what is absent is released: EDD support (task A0)
  and the NE2000, WD8003 and 3C509 network drivers (A5) already work this
  way, and every optional driver follows the same rule.
- Speed-ups must not cost memory. Every change states its cost in bytes,
  measured against upstream (`RESULTS.md`).
- Exceptions are decided by the maintainer, case by case, and written
  down. So far one: compressed programs and the text cache (F4b, +1.9 KB
  of resident kernel, 82 KB more room on the 720K disk), approved
  2026-11-19; the same day A5 gave back more than that (3 to 5 KB).
- Programs on the floppies are LZSA1-compressed and decoded by the kernel
  at exec; exited programs stay in free memory until it is needed (text
  cache). A damaged file must never bring the system down: the decoder
  checks its bounds and a damaged program is rejected (F4c).
- Compression is decided per program, by measurement under the rules of
  `ACCEPTANCE.md` (decided 2026-11-21): every program Alci distributes,
  its own or another project's, on floppy or in the Alci package
  repository, is shipped compressed when that works better (disk space,
  start time, transfer time, memory) and uncompressed when it does not.
  Data files (fonts, icons, resources, executables that a program loads
  by itself) stay uncompressed for now, except the files of graphical
  environments (fonts, icons, resources), studied in D2, I2 and D6: there
  they may be compressed with the most efficient method measured on the
  8088. Who decodes them is chosen by measurement (code bytes, resident
  RAM, speed): the environment itself, a small shared library, or the
  LZSA1 decoder already in the kernel, reused through one small call that
  reads a whole compressed file into a buffer the program provides. The
  licence of the environment does not matter here: the kernel decoding a
  file of an Apache 2.0 program mixes no code, as when it starts one. Alci runs both kinds, so ELKS programs keep
  working unchanged.
- The Alci package repository is made for Alci. An ELKS system that adds
  it must first install support for compressed programs (the LZSA1 exec
  decoder) as a prerequisite, like a dependency; packages that need it
  say so, and the installer checks it before installing them.
- Memory beyond 640 KB (decided 2026-11-22, study in B3): Alci must also
  serve machines with more memory, extended (XMS, 286 and later) or
  expanded (EMS boards, on any CPU, the 8088 included), without costing
  the machines that have none. ELKS's XMS support adds 3856 resident bytes
  when compiled in. Both ways are kept: the universal image carries the
  extra-memory code and frees it at boot when the memory is absent, as A0
  and A5 do; and kernels and releases per machine class (8088/8086, with
  EMS, 286 and later) are built where a class gains from it, with the
  same programs, the installer picking the kernel. A disk moved to a
  smaller machine must still boot (the universal kernel stays on it as a
  fallback). Programs may later come in per-CPU variants through the
  package repository (I3), always with an 8086 baseline. B3 measures what
  each class gains before any class kernel is published.

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

## Long-term aim for the floppy set (2026-11-19, proposal to confirm after `docs/os8088-study.md`)

A machine with only a 720K drive should, in the end, boot disk 1
straight into the graphical interface, the way os8088 starts its desktop
from its boot floppy and GEOS on the C64 started from its system disk,
with further programs and the user's files on other disks reached through
the disk manager (task F5) and Personal Storage disks. Whether the kernel,
Nano-X and a desktop fit on one 720K disk with room left is not known yet:
D2 measures Nano-X, and disk 1 may then be rebuilt around the interface,
with the text tools moving to disk 2. Until then disk 1 stays the text
system it is today.

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

## Date, time and time zone (2026-11-19)

- The system clock and the RTC keep UTC. The time zone is the user's,
  stored as a POSIX `TZ` rule in `/bootopts` (Italy:
  `CET-1CEST,M3.5.0,M10.5.0/3`); never fixed in the sources.
- Summer time is computed in one place only (`clock`, at boot and when
  the time is set); the kernel keeps the resulting offset and the moment
  of the next change, and every program asks it (E7).
- Time sources, in order: a working RTC; the network, from NTP servers of
  time institutes (INRIM, then PTB, then NIST), once the network card is
  up; the user, asked at boot only when the date is older than the
  system (no answer in 60 s: the boot goes on).
- No protocol an 8088 can run authenticates the time (NTS needs TLS), so
  trust comes from the servers chosen and a plausibility check: a time
  earlier than the build is refused, a large jump from a working RTC is
  reported, not applied silently.
- Floppy systems: the zone is asked once at first boot and saved on disk
  1; a write-protected disk 1 runs on UTC and says so at boot instead of
  asking every time. Settings never go on Personal Storage disks.

## How work is accepted

- Every change passes the vetoes and the gain threshold in `ACCEPTANCE.md`:
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
- The base of the Alci desktop (Nano-X or Gem-elks; PC/GEOS only as a
  separate program) is chosen after the GUI benchmark of D2, I2 and D6
  (decided 2026-11-21). Whatever the base, it is improved in look and
  behaviour in the spirit
  of GEOS (Commodore 64 first, then PC/GEOS), the way Haiku follows BeOS.
  Ideas and style are free to follow; GEOS code, icons and fonts stay out of
  GPL code, os8088 code (MIT) may be adapted with its notice kept, and the
  GEOS name is not used for our desktop. Details in task D4.
- Several graphical environments may be installed side by side as
  separate packages, each under its own licence, the way a Linux
  distribution carries software under many licences (decided 2026-11-21):
  Alci's own desktop, Gem-elks, a port of PC/GEOS as a separate program,
  and programs such as ELKSWeb. One runs at a time, chosen by the user;
  none of them is resident when not in use.
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
