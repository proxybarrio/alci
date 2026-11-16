# Alci: instructions for the scheduled agent

Alci is a branch of ELKS (GitHub account `proxybarrio`): a faster,
memory-frugal ELKS, heading towards a GEOS-like GUI and a Claude terminal,
on any PC from the 8088 to the 286. Each scheduled run does ONE task from
`alci/TASKS.md`, measures it, saves it and writes a report.

## Fixed rules

1. Write ONLY to `proxybarrio/elks` (branch `alci`) and `proxybarrio/martypc`
   (branch `elks-bench`). Never push to `ghaerr/elks` or `dbalsom/martypc`,
   never open issues, pull requests or comments upstream: the maintainer does that.
2. Never `push --force`, never rewrite published commits.
3. One task per run. If it is too big, split it in `TASKS.md` and do only
   the first piece.
4. Memory comes before speed: every patch states its cost in kernel code
   bytes, bytes per program, and resident RAM. A patch that slows down a
   measured case, or takes RAM permanently without good reason, is rejected.
5. Reference machine: 8088 at 4.77 MHz with CGA (MartyPC). Patches that
   depend on the BIOS must have an automatic fallback and stay
   "experimental" until tested on real hardware.
6. No irreversible decisions alone: prepare them and flag them in the report.
7. Never work on tasks marked `FROZEN` (phase 7), and never pick them as
   the next task.
8. Project files (code, commit messages, documents in `alci/`) are written
   in English. The final report to the maintainer is written in Italian.

## Acceptance rule (when a patch is worth it)

A patch is kept only if it passes every veto AND clears the gain threshold.

Vetoes (any one rejects the patch, whatever the speed gain):

- V1 Behaviour: functional tests give output identical to the previous
  `alci` build (shell, pipes, background jobs, subshells, scripts).
- V2 No bench gets slower: MartyPC is deterministic, so any slowdown
  above 1% on any benchmark in `bench.sh` / `bench2.sh` is real.
- V3 Memory: measure the cost by comparing `ia16-elf-size` of the changed
  objects with the previous build, and report it in bytes. Resident RAM (kernel data + bss + heap at boot, `Kernel
  text ... free` line) must not grow. Code growth must be justified in
  the commit message.
- V4 Time precision: the system clock must not drift. Check `date`
  against emulated time over at least 60 s: error must stay below 0.1%,
  and timer resolution (jiffies, `sleep`, `select` timeouts) unchanged.
- V5 Latency: keyboard and serial must not lose characters or respond
  later. Typed commands echo as before; serial input at 9600 baud must
  not drop bytes.
- V6 Hardware: no new BIOS or chipset dependency without a fallback,
  and such patches stay `EXPERIMENTAL` (see rule 6).

Gain threshold:

    value = gain% on the target workload / (1 + weighted lines / 50)

Weighted lines: lines added in the core (interrupt and timer code,
scheduler, `fork`/`exec`, memory management, block drivers) count three
times; all other lines count once.

Keep the patch if value >= 2. Example: a 6% gain with 50 added lines
gives 3, kept; a 3% gain with 100 added lines gives 1, rejected.
"Target workload" is the benchmark the task names; when in doubt, the
mean of the `bench2.sh` timings.

How to check V1 and the other hardware: see `alci/tests/README.md`.
Build images with `alci/tests/mkimg.sh` for the current tree and for the
previous commit (git worktree), run `alci/tests/functional.txt` with
`alci/tests/t86.py` on all four 86Box machines (xt, t1200, pc1512, at;
hd32 image), on the t1200 with its own disk (hd20t image, see
`alci/tests/README.md`; a real machine has this disk) and once on
the xt with the fd720 image, plus `qtest.py` on
QEMU, and diff the outputs: they must be identical. 86Box uses the real
BIOS of each machine; MartyPC stays the reference for speed on the 8088. Add a test line to `functional.txt` for every
behaviour your patch touches (keep lines under 60 characters: the ELKS
tty input line is 80). Real hardware (a real T1200) is needed only for patches that touch the
BIOS, the floppy controller or the boot sector, and before a release.

Every timing you record (`RESULTS.md`, reports, commit messages) names
both versions: the upstream ELKS commit and the Alci commit, measured with
the same configuration and toolchain. Add new results as a new dated
section; never overwrite older figures.

Report the vetoes checked and the value in the final message.

## Reusing other projects

Before writing something new, check `alci/RELATED.md` for a project that
already does it. Code copied into the kernel or into ELKS programs must have
a licence compatible with GPLv2 (MIT, BSD, GPLv2 or later; MPL 2.0 only with
care; Apache 2.0 is not compatible with GPLv2). Code with an incompatible
or unknown licence may be studied for ideas but not copied. Programs that
run outside ELKS (modem firmware, a proxy on another machine) can be used
as they are. Record every reuse and its licence in `alci/RELATED.md`.

## Procedure

1. Prepare the environment:
   - `git clone --depth 50 -b alci https://github.com/proxybarrio/elks /home/claude/elks`
   - `bash /home/claude/elks/alci/scripts/setup.sh` (prebuilt toolchain,
     MartyPC with the ELKS runner, first build). It must print
     `ALCI SETUP OK`.
2. Read `alci/TASKS.md`, `alci/RESULTS.md` and the latest entries of
   `alci/LOG.md`. Pick the first task with status `TODO` (skip `HUMAN` and
   `BLOCKED`).
3. Measure before: `alci/scripts/bench.sh base` and, if relevant,
   `alci/scripts/bench2.sh base`. Profiling: the `ELKS_PROF`,
   `ELKS_PROF_FROM`, `ELKS_SAMPLE`, `ELKS_PROF_BOOT` and `ELKS_FDROT=9`
   (floppy rotation) variables of the `martypc_headless` runner (see
   `crates/bin/martypc_headless/src/run_elks.rs` in the MartyPC fork);
   `alci/scripts/prof.py profile user_symbols` reads a profile.
4. Do the work, then measure again with a new label. Check that the
   on-screen output of the commands is identical to before (compare the
   screens). After libc changes run `make -C elkscmd clean`; after header
   changes, `make -C elks clean`.
5. If the task brings no measurable gain or makes something worse, revert
   it and record the negative result: it is a result.
6. Update `alci/TASKS.md` (status, notes), `alci/RESULTS.md` (table) and add
   a dated entry to `alci/LOG.md`.
7. Commit on branch `alci` (one commit per logical change, English message
   in ELKS style `[area] description`), then
   `git -c push.negotiate=true push origin alci`.
8. Final message (sent to the maintainer by email): in Italian, using "tu", no em
   dashes, short. What you did, numbers before and after, cost in bytes,
   what he needs to decide, the next task on the list.
