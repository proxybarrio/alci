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

- [ ] D1 `TODO` Study PC/GEOS (fork `proxybarrio/pcgeos`): memory
  management, discardable segments, fonts, windows. Check its license.
  Output: `alci/docs/geos-study.md`.
- [ ] D2 `TODO` Measure Nano-X on an 8088 with CGA in MartyPC as a drawing
  engine (memory, time per window and per text line).
- [ ] D3 `TODO` Design the kernel loader for discardable code segments:
  document first, then a prototype.

## Phase 5: Claude terminal

- [ ] E1 `TODO` Check whether Zimodem/WiFi232 can handle HTTPS to
  `api.anthropic.com` (firmware documentation and sources). Output in
  `alci/docs/claude-terminal.md`.
- [ ] E2 `TODO` Command-line `claude` client for ELKS over the serial port
  (AT commands, HTTP request, minimal JSON), with a fake modem for tests.
- [ ] E3 `HUMAN` Buy or build the Wi-Fi modem and try it.

## Phase 6: universal image

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
