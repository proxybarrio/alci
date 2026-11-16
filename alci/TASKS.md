# Alci: tasks in order

Status: `TODO`, `DOING`, `DONE`, `DROPPED` (tried, not worth it),
`HUMAN` (needs the maintainer), `BLOCKED` (waiting on something).
The agent takes the first `TODO` from the top.

## Phase 1: baseline speed

- [ ] A1 `TODO` Program start-up. Today 10 runs of `pwd` take ~2.0 s on an
  8088 (bench2). Profile `exec`/`fork`/a.out loading and reduce the cost
  without increasing resident RAM.
- [ ] A2 `TODO` Timer: the kernel spends ~7.6% of CPU even when idle
  (100 Hz tick). Reduce per-tick work or skip idle ticks. Also measure the
  effect on command times.
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

## Done so far (session of 2026-11-15)

See `alci/RESULTS.md`. Patches are on branch `alci`, one commit each.
