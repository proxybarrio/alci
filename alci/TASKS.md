# Alci: tasks in order

Status: `TODO`, `DOING`, `DONE`, `DROPPED` (tried, not worth it),
`HUMAN` (needs the maintainer), `BLOCKED` (waiting on something).
The agent takes the first `TODO` from the top.

## Phase 1: baseline speed

- [x] A0 `DONE 2026-11-16` EDD code released at boot when unused (chosen over a build
  option: one image for every machine). `bios_edd_rw()`
  lives in `.fartext.edd`, placed by `elks-small.ld` just before
  `.fartext.init`; `kernel_init()` frees from there when
  `bios_edd_drives == 0`. Checked: XT, T1200, PC1512, AT on 86Box (EDD
  absent, code freed, suite identical to upstream), QEMU (EDD present,
  code kept), and `alci/tests/bigdisk.sh` (1 GB disk, partition at
  cylinder 1190, written and read back across two boots). First use of
  the discardable-code mechanism planned in D3.
- [x] A1 `DONE 2026-11-16` Program start-up. Profiling the `pwd` loop showed
  37% of the time in `fmemcpyw`: `fork` copied the whole 22 KB data segment
  of the shell, mostly the empty gap between the break and the stack.
  `seg_dup_data()` now copies only [0, brk) and [sp - 32, end). 10 runs of
  `pwd`: 2.00 s -> 1.58 s cold, 1.68 s -> 1.30 s warm. Three-stage pipe
  4.75 s -> 4.33 s. No extra RAM. Shell behaviour checked identical
  (background jobs, pipes, subshells, command substitution, `sh -c`).
- [ ] A1b `HUMAN` The remaining fork cost (~17% of the `pwd` loop) is the
  shell's real data and bss, 8.3 KB per fork. Option: `vfork` in ash for
  simple commands. Risky: the child must not touch shared data before
  `exec`. Study how dash does it before trying. Code changes wait for the
  maintainer's go-ahead after the study in A1c.
- [x] A2 `DROPPED 2026-11-17` Timer. Measured: the 100 Hz tick costs 1523
  cycles, 3.2% of an 8088 at 4.77 MHz (not 7.6%). Trial patch (skip
  `serial_bh` with no port open, CPU usage far call every 2 s instead of
  every tick, no `schedule()` in the idle loop when nothing is runnable):
  idle tick -21%, benchmarks -0.2% to -1.0%, mean 0.46%, value 0.28.
  Rejected by the gain threshold. Tickless idle would save power only, not
  time. Details in `RESULTS.md`. Could be reconsidered as part of a wider
  power-saving task. Also measured on 86Box (IBM XT, hard disk, screen
  console): mean gain 1.1%, value 0.67, still below the threshold.
- [x] A2b `DONE 2026-11-17` Line reading. libc: `memchr` in assembly
  (`repne scasb`), `fgets` copies whole runs from the stdio buffer with
  `memchr` and `memcpy` instead of one `getc()` per character. `grep`
  (minix1): `get_line()` does the same, keeping its exact behaviour (long
  lines, NUL bytes, truncation when memory runs out). `grep` on 20 KB
  3.685 s -> 2.482 s (-33%), no benchmark slower, kernel unchanged. `sort`
  (minix3) reads with `read()` and gains nothing; `sed` (minix3) reads one
  `getchar()` at a time through a look-ahead variable shared by the whole
  program, left for A2d.
- [x] S1 `DONE 2026-11-19` Serial input lost characters on the IBM XT (8250
  without FIFO, 19200 baud: one byte every 521 us). Measured with a probe
  in 86Box's 8250 (`alci/tests/86box-overrun.patch`, `ALCI_OVERRUN=1`):
  every lost byte had waited behind the timer interrupt, which kept IRQ 0
  in service through `timer_tick()` and, every 5th tick, through the BIOS
  IRQ 0 handler called from the interrupt path; a system call entry (IF=0)
  just before made it longer. Fix in `49b6c795`: EOI before the timer
  handler, BIOS IRQ 0 handler moved to a bottom half (`BIOS_BH`, so it
  also runs during kernel init, where the XT-IDE BIOS waits on the tick
  count). Longest wait of a byte 643 us -> 384 us, lost bytes in 300
  typed commands 3 -> 0 (ELKS `df2a61c5`: 638 us, a lost byte stalled the
  run at command 42; with the patch 377 us, none). Cost +29 bytes of code,
  +32 bytes resident. Patch for ELKS in `alci/upstream/`, not sent. The
  test drivers still type whole lines at once, as asked.
  Remaining margin: 137 us. What is left in the 384 us is the system call
  and interrupt entry with IF=0 in `_irqit` and the entry of the serial
  handler itself; a later task could shorten those (MartyPC meter
  `ELKS_IRQOFF`). At the console, the keyboard handler keeps IRQ 1 in
  service for up to 1 ms, which blocks COM1 too (seen on MartyPC).
- [ ] S2 `TODO` Check `clock -w` on AT-class machines after 1999.
  `cmos_settime()` in `elkscmd/sys_utils/clock.c` writes `tm_year` (126 for
  2026) to CMOS register 9 as BCD, giving 0xC6, which is not valid BCD; the
  century register is not written. The other RTC paths in the same file
  subtract 100 first. Reproduce on 86Box (AT and 386): `date -s`, `clock
  -w`, reboot, and see what the BIOS setup and `clock -r` show. If
  confirmed, the fix is a few lines and a candidate to offer upstream.
- [x] S3 `DONE 2026-11-19` Out of memory on 86Box `at` (286, 512K) with the
  fd720 image (`sh: not enough memory` in functional command 9). Cause: the
  data segment size included the exact length of argv and envp, so
  `sh -c` got 48 bytes more than the shell that forked it; the segments
  freed by the forked copies were too small for the next exec and memory
  fragmented. Whether it showed depended on earlier commands, hence the
  results traded between ELKS and Alci, floppy and hard disk. `exec` now
  rounds the data segment to 256 bytes: 6 failures -> 0, hard disk still
  34/34. Cost +17 bytes of code, +16 bytes resident, about 128 bytes per
  process. Also applies to ELKS (`alci/upstream/`).
- [ ] A2d `TODO` `sed` and `uniq` line reading: read whole lines (`fgets` or
  the A2b scan) instead of one `getchar()`/`getc()` per character. In
  `sed` the current character is a global look-ahead used by the parser
  too: change only the input of `ourgetline()`. Measure with a `sed`
  substitution on 20 KB and `uniq` on sorted input.
- [ ] A1c `TODO` Study for A1b, no code changes: how dash uses `vfork`
  for simple commands and what the child may touch before `exec`; whether
  the ELKS kernel has `vfork` or what it would need; the expected gain on
  the 10 runs of `pwd` and on pipes, estimated from the A1 profile (8.3 KB
  copied per fork). Write the findings in `RESULTS.md` and a plan for ash
  in this file; the maintainer decides whether to go on.
- [ ] A2c `TODO` Idle loop without the wake-up race. Today a task woken
  between `schedule()` and `hlt` waits for the next interrupt (up to 10 ms
  with no other activity). Do `cli`, check the run queue, call
  `schedule()` only when a task is runnable, then `sti; hlt` (atomic on
  x86). Measure latency, not throughput: time from a key press or a byte
  on the serial port to the process running, and power at idle (share of
  time in `hlt`). Also measure on its own the CPU usage change from the A2
  trial (far call every 2 s, -2 bytes) and keep it if it is free.

- [ ] A3 `TODO` File order on the floppy image: place the kernel, shell,
  `bootopts` and the files read at boot close together and in read order.
  Measure boot with `ELKS_FDROT=9`.

- [ ] A5 `TODO` Release network card drivers that find no card, as done for
  EDD (the same rule for every optional device). Resident
  cost today with no card present (2026-11-16, Alci 3ecae5b): NE2000 2135
  bytes (`ne2k.o` + `ne2k-asm.o`), WD8003 2303, 3C509 2116, about 6.5 KB in
  all. Put each driver's run-time code in its own paragraph-aligned
  `.fartext.<drv>` section, mark the driver absent at boot when the probe
  fails, guard every entry point (open returns ENODEV without calling into
  freed code), and give the region back to the memory manager. Check on
  86Box with and without the PicoMEM card, with the functional suite and
  with `ktcp` plus `urlget`.
- [ ] A4 `TODO` Adopt os8088's MartyPC patch set (github.com/jggonz/os8088,
  `tools/martypc/`, MIT): 17 patches plus `debug_server.rs`, pinned to
  MartyPC `e15cb04f` (2026-06-28). Checked 2026-11-16: all 17 apply in
  order on that pin and the headless binary builds. They do not apply to
  our base `05c0d08` (2026-09-07). Gains: a debug server (breakpoints,
  registers, exact cycle counts, no code in the guest), PicoMEM emulation
  (lets E0b be tried without buying the card), fixes for V20 flags, the PIT
  and floppy timing. Work: rebase our four `elks-bench` commits (keyboard
  FIFO, FDC rotation, 8086 overlay and turbo, ELKS runner) onto pin plus
  patches, rerun the reference benchmarks and record any change in timing.
  Keep credit to os8088 in the branch README.

- [ ] A6 `TODO` 86Box fork: `proxybarrio/86box` exists (created
  2026-11-18 from `86Box/86box`) and the scheduled agent has access. The
  agent creates branch `alci` from the fork's `master`, commits os8088's PicoMEM
  patch (`tools/86box/patches/07-picomem.patch`, MIT, credited in the
  commit and in `RELATED.md`), checks that `setup.sh` builds from it, and
  from then on follows the emulator watch in `AGENT.md`. Later changes of
  ours to 86Box (for example serial measurements for S1) go on that
  branch.

## Phase 2: memory

- [ ] B1 `TODO` Measure and document where kernel memory goes (code, data,
  heap, buffers, cache) in `alci/docs/memory.md`.
- [ ] B2 `TODO` Elastic disk cache: reads only, writes go to disk at once,
  flushed on mount/umount and media change, at most 64 KB of conventional
  memory and given back to programs on demand; in EMS or extended memory
  when present. First a design document in `alci/docs/`, then the
  implementation in steps.

## Phase 3: tests on varied hardware

- [ ] C1 `TODO` Test kit: 360 KB, 720 KB and 1.44 MB images with all patches,
  plus a benchmark script that runs on ELKS and prints the times.
- [ ] C2 `HUMAN` Run the kit on real machines (Toshiba T1200, an XT, an AT)
  and report the numbers. Verify two-head floppy reads.

## Phase 4: graphics

- [ ] D1 `TODO` Study PC/GEOS (fork `proxybarrio/pcgeos`), os8088 and
  SymbOS for ideas only: memory management, discardable segments, fonts,
  window clipping and redraw on demand, CGA drawing routines. PC/GEOS is
  Apache 2.0: it can ship as a separate program under its own licence, not
  inside the kernel or linked with GPL code (see `STRATEGY.md`); os8088 is MIT, so its code may be adapted with its notice kept,
  except `reference/` and fonts of unclear origin. Output:
  `alci/docs/gui-study.md`, with what can be reused and under which terms.
- [ ] D2 `TODO` Measure Nano-X on an 8088 with CGA in MartyPC as a drawing
  engine (memory, time per window and per text line).
- [ ] D3 `TODO` Design the kernel loader for discardable code segments:
  document first, then a prototype.
- [ ] D4 `TODO` Alci desktop: improve Nano-X (look and behaviour) in the
  spirit of GEOS, the way Haiku follows BeOS. GEOS existed on the Commodore
  64 (Berkeley Softworks, 1986) and later on the PC as PC/GEOS; today only
  the PC/GEOS source (FreeGEOS, Apache 2.0) is available to study. Rules:
  1. Ideas, behaviour and general style may be taken from GEOS on any
     platform (menus, dialogs, font handling, redraw strategy). They are not
     protected by copyright.
  2. No GEOS code, icons, fonts or bitmaps go into Nano-X or any GPL code:
     Apache 2.0 is not compatible with GPLv2. Write our own from scratch
     after studying the design (clean-room: the notes in
     `alci/docs/gui-study.md` describe behaviour, never copy code).
  3. os8088 (MIT) code may be adapted, keeping its copyright notice in each
     file taken and a line in `RELATED.md`.
  4. "GEOS" is a trademark: never name the Alci desktop GEOS, and never copy
     its screens pixel by pixel. Say "inspired by GEOS".
  Depends on D1 and D2.

## Phase 5: Claude terminal

- [ ] E0 `TODO` Reuse before writing: ELKS already ships `miniterm` (raw
  keyboard to serial link, no terminal emulation) and, under Nano-X, `nxterm`
  (terminal emulator with ANSI sequences, for the desktop phase). Check
  whether an existing ESP32 firmware that already talks to Claude
  (AiBridgeMCP Nano) or a Wi-Fi modem firmware (Zimodem) plus `miniterm`
  gives a working Claude terminal with no new code on the ELKS side. Read
  their documentation and licenses; record the result in
  `alci/docs/claude-terminal.md`. If it works, E1 and E2 shrink to small
  improvements.
- [x] E0b `DONE in emulation 2026-11-16` Network without the serial port.
  86Box IBM XT (8088 4.77 MHz) with an emulated PicoMEM 2 (os8088's 86Box
  patch `07-picomem.patch` and `libpmhost`, both MIT): ELKS found the card's
  NE2000 at 300h/IRQ 3 with its existing driver, `ktcp` came up, `ping` to
  the host answered and `urlget` fetched a file over HTTP. No new kernel
  code. Images: `BOOTOPTS_EXTRA='ne0=3,0x300,,0x80' mkimg.sh`; run with
  `PICOMEM=<libpmhost> t86.py xt <label>-fd720.img` (not the hd32 image:
  XT-IDE also sits at 300h). Next: HTTPS through a proxy on a modern
  machine, then the real card.
- [ ] E0c `TODO` Encrypted link from Alci to our own relay (why:
  a plain proxy leaves the Alci-to-proxy hop in clear). Anthropic's API
  only accepts TLS and nothing can be installed on its side, so the relay
  is ours (home Raspberry Pi, the ESP32, or a small server). Design: a key
  shared once between Alci and the relay, each message sealed with
  ChaCha20-Poly1305 from Monocypher (BSD-2-Clause or CC0, compatible), no
  public-key handshake on the 8088. The Claude API key stays on the relay.
  Steps: build Monocypher's ChaCha20 and Poly1305 with ia16-gcc, measure
  speed and size on MartyPC, write the message format in
  `alci/docs/claude-terminal.md`. Do not invent a cipher.
- [ ] E1 `TODO` Only if E0 is not enough: check whether Zimodem/WiFi232 can
  handle HTTPS to `api.anthropic.com`.
- [ ] E2 `TODO` Only if E0 is not enough: command-line `claude` client for
  ELKS (AT commands, HTTP request, minimal JSON), with a fake modem for
  tests. Look at doschgpt's request and reply handling first (license to be
  checked).
- [ ] E3 `HUMAN` Buy or build the Wi-Fi modem and try it.

- [ ] E5 `TODO` Native HTTPS as an option (decided 2026-11-17): a
  TLS client in ELKS itself, for machines with a network path (PicoMEM or
  another NE2000, or SLIP over the serial port) and no modem doing TLS.
  BearSSL (MIT; has an "i15" big-number code for 16-bit multiplies) is the
  first candidate. Rough estimate, to be measured: handshake 2-5 minutes on
  an 8088 at 4.77 MHz, about half on a T1200, 20-60 s on a 286; then a few
  KB/s; 50-100 KB on disk, 60-100 KB of RAM while running, nothing when not
  running. Every program using it shows a notice before connecting, with
  the expected wait scaled to the detected CPU. First step: build BearSSL's
  X25519 and RSA/ECDSA verify for ia16 and time them on MartyPC.
- [ ] E6 `TODO` Text web browser on top of E5. Current Lynx needs a 386 (DJGPP
  port); Bobcat, a Lynx derivative, ran on the 8088 under DOS. Check
  Bobcat's sources and licence, and the effort to port it, against writing
  a small browser for ELKS.

- [ ] E8 `TODO` SSH client (decided 2026-11-19: wanted). ELKS has none.
  First a study, no code: existing small clients and their licences, the
  memory a client needs, and the time of the key exchange and host key
  check on an 8088, a T1200 and a 286 (measure the curve25519 and
  ed25519 or RSA code on MartyPC, as for E5). Output: size, RAM, waiting
  time, and on which disk it goes (disk 3 on 720K, and on 1.44M if it does
  not fit). Until then, a way that works today: `telnet` from Alci to a
  machine on the desk (a Raspberry Pi), and `ssh` from there.
- [ ] E7 `TODO` (low priority) Clock from the network, time zone from the
  user. The clock is kept in UTC and the time zone is never fixed in the
  sources:
  1. Time zone: asked once (at install or first boot, from a short list of
     zone names) and stored as `TZ` in `/bootopts`; changeable later.
  2. Time: a small SNTP client sets the clock in UTC at network start. It
     asks the nearest source first: the NTP server given by DHCP (option
     42) or the gateway, which many routers serve, then a public pool
     (pool.ntp.org) or our relay (E0c). No single private server.
  3. Plausibility check: if the network time differs from the RTC (where
     present) by more than a set margin, report it instead of setting it
     silently.
  4. Order at boot: if the machine clock is valid, use it (and correct it
     from the network when a network is present). If it is clearly wrong
     (earlier than the build date, as on an XT without a real-time clock,
     which starts at 1970), take the time from the network; ask the user
     for date and time only when no network time can be obtained. Boots
     with a valid clock, or with a working network, never stop to ask.
  The commented `date -s` example in `/etc/net.cfg` stays as it is; this
  task adds an alternative. Measure size of the client (target
  under 4 KB) and keep it out of memory when no network is present.

## Phase 6: universal image

Principle (`STRATEGY.md`): wherever a hard disk is present, Alci is
installed on it in full; the disk set and the repository exist for
installation and for floppy-only machines.

Rules for the floppy set (decided 2026-11-19):
- No program leaves the 720K base disk until disk 2 exists in the same
  change, with the disk swap working on every machine, the XT included
  (XT drives give no disk-change signal: the system asks for the disk and
  checks a label on it, it never relies on the signal).
- Disk 1 is chosen first (F0), then disk 2 and further disks: disk 3 for
  other applications, for example the SSH client (E8). The same holds for
  1.44M images if a program does not fit.
- Disks 2, 3... carry whatever they need to be used on their own while
  disk 1 is out of the drive; whether that includes a copy of the kernel
  is decided in F0 (it costs about 104 KB per disk).
- Every change of layout is measured before and after (`bench.sh`,
  `bench2.sh`): the position of files on the floppy changes timings.
- Disk 1 also keeps the communication programs: `telnet`, `telnetd`,
  `ftp`, `ftpd` and `miniterm` (communicating is part of the base).
- Every distribution (720K, 1.44M and any other floppy set) includes the
  installer that puts the whole set on a hard disk (F3 step 1), on disk 1.
- 720K set (decided 2026-11-19, replaces the earlier split): disk 1 is
  the current 720K image, unchanged. Disks 2 and 3 hold only programs
  that are not on disk 1: disk 2 everything else that exists today
  (archives, `wc head cmp sleep xargs yes`, `diff find`, script helpers,
  system and disk maintenance, printing, games, `basic`, `paint`, sound:
  about 500 KB), disk 3 the SSH client (E8) and later additions. Disks
  2 and 3 carry a small kit (`ls cat cp more mount umount`, about 27 KB)
  and a label. Uses: installing everything on a hard disk from 720K
  floppies (T1200), reaching the extra programs without a hard disk.
- `wc head cmp sleep xargs yes` stay off disk 1 (2026-11-19): with them
  disk 1 would have 23 KB free instead of 41, and that space is the
  working room of a floppy-only system: `vi` keeps its temporary file in
  `/tmp` (about the size of the text being edited), `sort` too, and the
  user's own files go there. Editing a 20-30 KB text would fail.
- Nothing is split before the disk manager (F5) works.
- Free space is part of the design: disk 1 keeps enough room for `/tmp`
  and everyday work (a minimum to fix in F0, checked by the build), and
  the split is accepted only when the floppy suite passes 34/34 with the
  strict checker on the real three-disk set (no timeouts, no "No space
  left"), on every machine, with one drive and with two.

- [ ] F0 `TODO` Floppy budget. The 720K image (Alci 3ecae5b) has 669 of 710
  data blocks in use, 40 KB free, and 720K is the drive of a real T1200.
  List every file with its size and the task it serves, and propose a base
  set (kernel, shell, core tools, networking, the package fetcher) that
  leaves room, with the rest moved to packages. The proposal says, for each
  program, disk 1, 2 or 3, and what each disk needs to work alone. The
  maintainer decides; nothing moves before F3 step 3 works.
- [ ] F2 `TODO` Package repository ("alci-get"): software fetched once ELKS is
  running and online, installed on a hard disk or a second floppy. An index
  with a SHA-256 hash per package, fetched over HTTPS (E5) or through the
  relay (E0c); packages may then come over plain HTTP and are checked
  against the index before use. Hosted on the proxybarrio fork (releases or
  a branch).
- [ ] F3 `TODO` Multi-disk set (2026-11-17, after GEOS on the
  C64): disk 1 boots and holds the base system, disks 2, 3... hold the rest.
  A catalogue on disk 1 maps each program to its disk (the same catalogue
  can point to the package repository, F2). In this order:
  1. Installer that copies the whole set to a hard disk, asking for each
     disk in turn. This is the main path: on any machine with a hard disk,
     the floppies are installation media only. It goes on disk 1 of every
     distribution. Reuse first: ELKS already has `sys` (a script that
     copies the booted system to `/dev/hda1` and similar), `fdisk`,
     `mkfs` and `makeboot`; the installer adds the questions, the
     partition step and the copy of disks 2, 3...
  2. Two floppy drives: further disks are mounted from B: on request.
  3. One drive, no hard disk, last: the root filesystem lives on disk 1,
     so a swap needs kernel work (flush and invalidate the cache for the
     drive, keep the root read-only while it is out, load the program into
     memory before asking for disk 1 back) and XT drives give no
     disk-change signal. Test on 86Box and on a real T1200 before use.
  Rule: every program fits on one disk (RAM is the stricter limit anyway).
- [x] F4 `DONE 2026-11-19` The functional suite on the 720K floppy, with the
  strict checker. Done:
  0. Compressed programs (`CONFIG_APPS_COMPRESS`) measured on MartyPC and
     not adopted: 130 KB freed, but every compressed program starts slower
     (`vi` 5.1 s -> 40.4 s on the XT, 3.6 s -> 20.0 s on the 8086 at
     7.16 MHz; the kernel decompressor is slow). Figures in `RESULTS.md`.
  1. Layout change tried in `59478e75` and reverted in `0249f4d7`: it took
     programs off the base disk before disk 2 exists, left `ftpd` started
     by `/etc/net.cfg` but absent, and made some reads slower (file
     positions). The 720K disk keeps its program set until F0 and F3.
  2. `alci/tests/functional-fd.txt`: the same commands on small files; the
     programs not on the base disk come from a test disk made by
     `mkimg.sh` (`<label>-fd720b.img`), mounted on `/mnt`. `tar-ok` fails
     when the extraction directory cannot be entered.
- [ ] F5 `TODO` Disk manager (decided 2026-11-19), before the disk split.
  Design document first (`alci/docs/disks.md`), for the maintainer:
  1. Every disk has a label: name, kind (system, programs, documents) and
     a unique number, read to check that the right disk is in the drive
     (XT drives give no disk-change signal, so the system asks and then
     checks the label, never trusting the signal).
  2. A catalogue on disk 1 (and a copy on each program disk) says which
     disk holds each program. A command that is not on the disk in use
     gets an answer like "tar is on disk 2 Tools: insert it and press
     Enter"; the program is loaded, then disk 1 is asked for again.
  3. Document disks, called "Personal Storage" with a sequential number
     (decided 2026-11-19): any blank floppy can become one, and gets the
     next free number ("Personal Storage 1", "Personal Storage 2"...),
     kept in the list of known disks on disk 1; the number is also in its
     label, so two of them are never confused. Its path is short, without
     spaces and within the 14 characters of a MINIX name (`/disks/ps1`).
     When the program has finished with it, the system asks for the
     disk that was there before: disk 1 with one drive, disk 2 in drive B
     with two. All of F5 is for machines without a hard disk. Saving to it must work from every program, not only
     those with a menu: the plan to evaluate is a path per disk name
     (for example `/disks/ps1/letter.txt`), so that any program (`vi`,
     `cp`, later the desktop's save dialogs) can save there, and the
     system asks for that disk when it is needed. Everything pending is
     written before a swap; disk 1 stays read-only while it is out.
  4. Two drives: disk 1 stays in drive A, disk 2 in drive B. When disk 3
     or a Personal Storage disk is needed, the swap happens in drive B,
     then disk 2 is asked for again; disk 1 never leaves drive A, so this
     case needs no kernel work. One drive: every swap happens in drive A.
     Hard disk: the same names point to folders, nothing to swap.
  Study first: AmigaOS volume names and "Please insert volume" requests,
  GEOS disk handling (ideas only, see `RELATED.md` rules). Then the steps
  in order: labels and the `disk` command, the catalogue and the
  missing-command message, document disks on two drives, then single
  drive swaps (kernel work, F3 step 3). Test on 86Box XT, T1200, AT.
- [ ] F4b `TODO` Faster `exec` decompression: the in-kernel exomizer
  decruncher (`elks/fs/exodecr.c`, C with a far pointer per byte) costs
  about 0.7 s per KB on the 8088 (F4 point 0). Profile it on MartyPC and
  write the inner loop in assembly; adopt compression for the 720K image
  only if starting `vi`, `cat` and the others from floppy is then not
  slower than plain.
- [ ] F1 `TODO` CPU detection at boot and choice of routines (8088, 186,
  286); 286 extended memory for the disk cache.

## Phase 7: modern PCs (very long term, FROZEN)

Not to be started, by the agent or anyone, until the maintainer declares phases 1-6
finished and satisfactory on 8088 to 80386 machines. Recorded only so that
earlier choices (single image, code loaded only where needed) stay
compatible with it.

- [ ] G1 `FROZEN` Standalone USB stick for modern PCs: MINIX system
  partition used as a hard disk, FAT data partition shared with the
  modern OS; same contents as the CF/XT-IDE image for old machines.
- [ ] G2 `FROZEN` Boot on UEFI-only PCs (no legacy BIOS): a layer that
  provides BIOS services on top of UEFI.
- [ ] G3 `FROZEN` LLM connection on machines without a serial port:
  minimal USB driver to reach the ESP32 modem as a USB serial device,
  loaded only on machines that need it.

## Done so far (session of 2026-11-15)

See `alci/RESULTS.md`. Patches are on branch `alci`, one commit each.
