# Alci: tasks in order

Status: `TODO`, `DOING`, `DONE`, `DROPPED` (tried, not worth it),
`HUMAN` (needs the maintainer), `BLOCKED` (waiting on something).
The agent takes the first `TODO` from the top, except for the order
agreed with the maintainer on 2026-11-19: F4c (done), then E7 (network
time, time zone, summer time), then F4d, then D5 (boot logo).

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
- [x] S2 `DONE 2026-11-19` `clock -w` on AT-class machines after 1999:
  confirmed on 86Box IBM AT and ECS 386 (register 9 = 0xC6 for 2026, not
  BCD). Fixed in `958f9203`: two-digit BCD year, and the IBM AT century
  byte (CMOS 0x32) written only where it already holds 0x19 or 0x20.
  2026 -> 0x26, 1999 -> 0x99 with century 0x19; `clock -r` reads both
  back. Patch for ELKS in `alci/upstream/`. Seen and left alone: the day
  of week is written as 3-9 (Sunday = 3), which 86Box's RTC also uses
  but is outside the 1-7 range of a real MC146818.
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

- [ ] A3 `DOING` Boot from floppy. Measured 2026-11-19 (Alci `010d6729`,
  720K, MartyPC `e1c5187` with `ELKS_FDROT=9`, `ELKS_BOOT_TEXT=login:`,
  `ELKS_FDTRACE=1`): XT 22.8 s to `login:`, of which BIOS POST 4.0 s,
  boot loader 6.9 s (kernel 104 KB), kernel start 1.7 s, the rest 10 s
  reading init, scripts and programs. File order: upstream's
  `image/Image.all` (small boot files next to the inode table, kernel
  after) is already good; putting `bootopts` and `linux` first was 0.4 s
  slower (22.8 -> 23.2 s). Where the time goes instead:
  1. The boot loader goes back to cylinder 0 three times while loading
     `/linux` to re-read the inode block (about 0.5 s each).
  2. It reads one head per call and waits about 50 ms for the other head.
  3. In the kernel, track reads start at sector 1 and wait 150-190 ms for
     it on most new tracks.
  Point 1 done in `6d338ea9` (2026-11-19): the boot loader keeps the
  inode block; boot to `login:` 22.81 -> 22.21 s on the 8088, 18.79 ->
  18.19 s on the 8086; boots on every 86Box machine and QEMU. Needs a
  test on the real T1200 (boot block). Next: point 2, then 3.

- [x] A5 `DONE 2026-11-19` Release network card drivers that find no card,
  as done for EDD. Driver operations are far (`struct eth_farops`); the
  NE2000, WD8003 and 3C509 drivers keep their run-time code in their own
  aligned sections, given back at boot when the card is not found. Freed:
  NE2000 1904, WD8003 1952, 3C509 1744 bytes; image +656 bytes, so 4.9 KB
  more free memory with no card and 3 KB with one card. Tested with an
  NE2000 (QEMU, 86Box PicoMEM) and a WD8003E (86Box): `ktcp`, `urlget`,
  same speed. Not tested with a 3C509: 86Box's 3C509B is not found by
  either build. SMC Ultra and PCnet are compiled only (far operations,
  resident). See `RESULTS.md`.
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
  from then on follows the emulator watch (`RESULTS.md` "Emulator
  updates"). Later changes of ours to 86Box (for example serial
  measurements for S1) go on that branch.

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
- [ ] D5 `TODO` Boot logo (decided 2026-11-20). Image: the maintainer's
  320x200 drawing (two moose, a penguin wearing antlers, sun, ALCI, a PC
  with `#`), CGA mode 5, ready as `alci/img/alci-logo-cga5.bin` (16 KB,
  2.4 KB with LZSA1). After the BIOS mode set, port 3D9h = 10h for the
  bright palette (and bit 2 of 3D8h, as mode 5 already sets it). On a
  composite monitor mode 5 shows in shades of grey. Off at first boot; `logo on` / `logo off` turn it on and off
  (a line in `/bootopts`; on a write-protected disk 1 the command says
  so). When on, `rc.sys` runs its work through a small `bootlogo`
  program: it saves the text screen, shows the logo until the end of
  the boot and keeps the boot output; a key pressed at any time brings
  back the text screen with everything written so far (before the logo
  and during it) and the boot goes on in text. The date question
  (`clock -c`) and `login:` always come in text. Skipped without CGA,
  EGA or VGA. Image 16 KB, 2.6 KB compressed with LZSA1; expected cost
  about 0.7 s on the XT floppy boot, 0 bytes of memory after the boot.
  A kernel message printed while the logo is up ends the logo as a key
  would (decided 2026-11-20): `bootlogo` watches the kernel message
  queue (the one `dmesg` reads) and on a new message brings back the
  text screen, the boot output and the message. No kernel code; the
  message may draw a few wrong pixels for a moment before that. Measure on MartyPC (XT) and check on
  86Box XT, AT, 386 and QEMU.

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
- [ ] E7 `DOING` Clock from the network, time zone from the
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
  Point 4 done 2026-11-19, the question part: `clock -c` in `rc.sys`,
  after the network start, asks for the date when it is older than the
  day `clock` was built; Enter keeps it, no answer lets the boot go on
  after 60 s; a typed date is also written to the RTC. Next: the SNTP
  client (points 1-3); the `net.cfg` example is unchanged.
  Decided 2026-11-19: time from NTP servers run by time institutes (INRIM,
  then PTB, then NIST), asked by `ktcp`, which already speaks UDP for
  DHCP; the question at boot only when none answers. Summer time is
  computed in one place only: `clock`, at boot and whenever the time is
  set, turns the POSIX `TZ` rule (Italy: `CET-1CEST,M3.5.0,M10.5.0/3`)
  into the current offset, the next change and the offset after it, kept
  by the kernel next to the `struct timezone` it already holds; `libc`
  takes the offset from `gettimeofday` instead of parsing `TZ` (today it
  only knows whole hours, no summer time). Measure the kernel and `libc`
  cost first. Floppy systems: the zone is asked once and saved in
  `/bootopts` on disk 1; on a write-protected disk 1 it stays UTC and a
  line at boot says so, without asking each time.
  Point 2, the client, done 2026-11-20: `ktcp -t ip,ip,...` sends an
  SNTP request once the address is known (static, or DHCP bound), to the
  server from DHCP option 42 first, then the `-t` list; two tries 2 s
  apart per server. Answers are taken only from the server asked, in
  server mode, synchronised, stratum 1-15, echoing our request; a time
  before the build of `ktcp` is refused. The clock is set in UTC with
  half the round trip added; a correction above 1000 s of a clock that
  was valid is printed. `ktcp` +1136 bytes code, +112 data, +32 bss; no
  resident cost (only while `ktcp` runs). `/etc/net.cfg` gets
  `ntp="-t 193.204.114.232,192.53.103.108,129.6.15.28"` (INRIM, PTB,
  NIST), passed by `/bin/net`: IP addresses, since names cannot be looked
  up before `ktcp` runs; check them on a real network.
  Still to do, in this order:
  Decided 2026-11-20:
  - Server order: the `-t` list (INRIM, PTB, NIST) first; only if none
    answers, the local network's server (DHCP option 42, else the
    gateway). Today DHCP's server goes first: change it in E7b.
  - The first boot on a real network prints which servers answered, so
    the addresses are checked there.
  - A large jump from a clock that was valid is applied and printed
    (point 3 closed).
  - The two floppy benchmarks moved by the layout shift are accepted;
    the layout is left to A3 and F0.
  - E7b: `clock -c` runs right after `net start`, before the answer can
    arrive (up to 12 s with three silent servers). Make the boot wait for
    the network time (or for `ktcp` to give up) before asking, and write
    the RTC after a network time is set (`clock -w`). While it waits, a
    message says which key combination skips the wait, for when it takes
    too long; the key must work on the console and on a serial terminal.
    Include the server order and the per-server report above.
  - E7c: time zone and summer time (point 1 and the `TZ` decisions
    above): measure the kernel and `libc` cost first.

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

- [ ] F0 `TODO` Floppy budget. The 720K image (Alci 3ecae5b) had 669 of 710
  data blocks in use, 40 KB free; with compressed programs (F4b) disk 1
  has 121 KB free (Alci `70b577df`). 720K is the drive of a real T1200.
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
  3. Document disks, called "Personal Storage" with a sequential number,
     in FAT format so that any PC can read them (decided 2026-11-19):
     any blank floppy can become one, and gets the
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
- [x] F4b `DONE 2026-11-19` Compressed executables that load fast. After
  trying LZ4 and LZSA1 (three rounds, `RESULTS.md`): LZSA1 (the LZSA
  project's 8088 decoder, zlib) plus a text cache that keeps exited
  programs in free memory. On by default in `alci/elks.config` since
  `57265faf`, floppy images only: 720K disk 41 -> 123 KB free, faster
  starts from floppy and repeated starts, 34/34 everywhere. Exception
  approved by the maintainer 2026-11-19: `grep` +3.2% in `bench2.sh`
  (first start from a floppy without rotation delay; faster with it) and
  +1.9 KB of resident kernel.
  Follow-ups: file order on the floppy (A3) now that files moved;
  hard disk images later; (`meminfo` shows kept texts as KEPT and
  counts them as free, done 2026-11-19); with the
  space gained, fewer disks in the 720K set (decided 2026-11-20: yes;
  F0 fixes the number).
- [x] F4c `DONE 2026-11-20` Robust LZSA1 decoding (from a review of F4b).
  The decoder trusted the block; on QEMU 7 of 12 damaged copies of `grep`
  hung the system, and the host model (`tests/lzsa1-model.py`) showed
  writes over the 64 KB above the segment. Now long copies are checked
  before they start and the output after each short match; a damaged
  block returns 0 and exec rejects it. With `9d1b6868`: none of the 12
  hangs (10 rejected, 1 stopped by `SIGSEGV`, 1 runs), the model finds no
  write outside the segment in 2500 corruptions. Decoder 74 bytes
  smaller; starts +0.1% to +1.1%. A damaged block that still decodes to
  the right length runs with wrong code, as an uncompressed program
  would; no checksum for floppies, downloaded packages will carry a hash
  (E0c, F5).
- [ ] F4d `TODO` Text cache, small fix: a cached program file that is
  unlinked without being opened for writing (`mv` over it) keeps its
  disk blocks until the entry leaves the cache (checked on QEMU: `sed`
  replaced by `mv`, its 14 KB came back only after five other programs
  ran). `rm` is not affected (it checks write access, which drops the
  entry). Drop the entry when the link count reaches zero.
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
