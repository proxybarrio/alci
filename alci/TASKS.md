# Alci: tasks in order

Status: `TODO`, `DOING`, `DONE`, `DROPPED` (tried, not worth it),
`HUMAN` (needs the maintainer), `BLOCKED` (waiting on something).
The agent takes the first `TODO` from the top.

## Phase 1: baseline speed

- [x] A0 `DONE 2026-11-16` EDD code released at boot when unused (the maintainer chose
  this over a build option: one image for every machine). `bios_edd_rw()`
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
- [ ] A1b `TODO` The remaining fork cost (~17% of the `pwd` loop) is the
  shell's real data and bss, 8.3 KB per fork. Option: `vfork` in ash for
  simple commands. Risky: the child must not touch shared data before
  `exec`. Study how dash does it before trying. Needs the maintainer's go-ahead.
- [ ] A2 `TODO` Timer: the kernel spends ~7.6% of CPU even when idle
  (100 Hz tick). Reduce per-tick work or skip idle ticks. Also measure the
  effect on command times.
- [ ] A2b `TODO` Line reading in user programs. `grep` (minix1) spends 83%
  of its time in user space, about half of it in `get_line()`, a
  `getc()` loop one character at a time. Read lines by scanning the
  stdio buffer with `memchr` and copying with `memcpy`, ideally in libc
  (`fgets`, plus a `getline`-style helper) so `sort`, `sed`, `awk` and
  others gain too. Measure `grep`, `sort` and `sed` before and after.
- [ ] A3 `TODO` File order on the floppy image: place the kernel, shell,
  `bootopts` and the files read at boot close together and in read order.
  Measure boot with `ELKS_FDROT=9`.

- [ ] A5 `TODO` Release network card drivers that find no card, as done for
  EDD (the maintainer's request: the same rule for every optional device). Resident
  cost today with no card present (2026-11-16, Alci acdd752): NE2000 2135
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
  Apache 2.0 (not compatible with the GPLv2 kernel; separate user programs
  only); os8088 is MIT, so its code may be adapted with its notice kept,
  except `reference/` and fonts of unclear origin. Output:
  `alci/docs/gui-study.md`, with what can be reused and under which terms.
- [ ] D2 `TODO` Measure Nano-X on an 8088 with CGA in MartyPC as a drawing
  engine (memory, time per window and per text line).
- [ ] D3 `TODO` Design the kernel loader for discardable code segments:
  document first, then a prototype.

## Phase 5: Claude terminal

- [ ] E0 `TODO` Reuse before writing: ELKS already ships `miniterm`. Check
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
- [ ] E0c `TODO` Encrypted link from Alci to our own relay (the maintainer's point:
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

- [ ] E5 `TODO` Native HTTPS as an option (the maintainer's decision, 2026-11-17): a
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

## Phase 6: universal image

- [ ] F0 `TODO` Floppy budget. The 720K image (Alci acdd752) has 669 of 710
  data blocks in use, 40 KB free, and 720K is the drive of a real T1200.
  List every file with its size and the task it serves, and propose a base
  set (kernel, shell, core tools, networking, the package fetcher) that
  leaves room, with the rest moved to packages.
- [ ] F2 `TODO` Package repository ("alci-get"): software fetched once ELKS is
  running and online, installed on a hard disk or a second floppy. An index
  with a SHA-256 hash per package, fetched over HTTPS (E5) or through the
  relay (E0c); packages may then come over plain HTTP and are checked
  against the index before use. Hosted on the proxybarrio fork (releases or
  a branch).
- [ ] F3 `TODO` Multi-disk set (the maintainer's idea, 2026-11-17, after GEOS on the
  C64): disk 1 boots and holds the base system, disks 2, 3... hold the rest.
  A catalogue on disk 1 maps each program to its disk (the same catalogue
  can point to the package repository, F2). In this order:
  1. Installer that copies the whole set to a hard disk, asking for each
     disk in turn.
  2. Two floppy drives: further disks are mounted from B: on request.
  3. One drive, no hard disk, last: the root filesystem lives on disk 1,
     so a swap needs kernel work (flush and invalidate the cache for the
     drive, keep the root read-only while it is out, load the program into
     memory before asking for disk 1 back) and XT drives give no
     disk-change signal. Test on 86Box and on a real T1200 before use.
  Rule: every program fits on one disk (RAM is the stricter limit anyway).
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
