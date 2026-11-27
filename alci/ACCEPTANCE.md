# Alci: acceptance rules

How a change to Alci is judged before it is kept: the vetoes, the gain
threshold, how behaviour and speed are checked, and the rules for reusing
code from other projects. The measurements in `RESULTS.md` follow these
rules; `STRATEGY.md` says what the work must respect.

## When a change is kept

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

## Two kinds of change

Every change belongs to one of two classes, named in its commit message
or results entry.

- **Performance changes** (speed, memory, disk space): all vetoes apply
  and the gain threshold above decides. An exception (for example a
  benchmark slower on one case, or resident memory that grows) needs a
  written reason, figures before and after, and the maintainer's
  approval, recorded in `RESULTS.md`.
- **Integrity fixes** (a demonstrated loss or corruption of user data, or
  a crash or hang of the whole system): the gain threshold does not
  apply, because the gain is the removed risk. The fix must come with a
  test that shows the failure before and its absence after, on the
  machines it concerns. V1, V2, V4, V5 and V6 still apply. Under V3 the
  cost is reported in bytes, resident and per program, and the effect on
  machines with 256, 512 and 640 KB is stated; resident growth is
  accepted only if the risk exists in what Alci ships today (a fix for
  programs Alci does not yet run waits until it does) and the maintainer
  approves it. A risk found by reading the code alone is first
  reproduced; if it cannot be, the change is not an integrity fix.

Neither class can be used to carry the other: a speed change does not
become acceptable because it also fixes something, and an integrity fix
does not justify unrelated changes in the same commit.

How to check V1 and the other hardware: see `alci/tests/README.md`.
Build images with `alci/tests/mkimg.sh` for the current tree and for the
previous commit (git worktree), run `alci/tests/functional.txt` with
`alci/tests/t86.py` on all four 86Box machines (xt, t1200, pc1512, at;
hd32 image), on the t1200 with its own disk (hd20t image, see
`alci/tests/README.md`; a real machine has this disk) and once on
the xt with the fd720 image, plus `qtest.py` on
QEMU, and diff the outputs: they must be identical. Results count only
when run with the strict tools (`sq.py`, `st86.py`, then `check.py`: 34/34),
see `alci/tests/README.md`. 86Box uses the real
BIOS of each machine; MartyPC stays the reference for speed on the 8088. Add a test line to `functional.txt` for every
behaviour your patch touches (keep lines under 60 characters: the ELKS
tty input line is 80). Real hardware (a T1200) is needed only for patches that touch the
BIOS, the floppy controller or the boot sector, and before a release.

Upstream merges: `alci/scripts/upstream-merge.py` merges ELKS master,
resolves a conflicted block to upstream's text only when both sides are
the same code apart from comments and blanks (never a whole file), aborts
on any other conflict, and refuses to commit if a line Alci added is
missing afterwards (`--check-only <merge>` checks an existing merge).
Then `alci/scripts/build-compare.sh <previous alci commit>` builds both
and compares the kernel and every file of `target/`: identical files
(apart from the version string and build times) mean unchanged behaviour
and timings; any other difference needs the checks V1 to V5.

Every timing you record (`RESULTS.md`, reports, commit messages) names
both versions: the upstream ELKS commit and the Alci commit, measured with
the same configuration, toolchain and emulator commit (MartyPC or 86Box). Add new results as a new dated
section; never overwrite older figures.

Every change reports the vetoes checked and its value (commit message or
`RESULTS.md`).

## Reusing other projects

Before writing something new, check `alci/RELATED.md` for a project that
already does it. Code copied into the kernel or into ELKS programs must have
a licence compatible with GPLv2 (MIT, BSD, GPLv2 or later; MPL 2.0 only with
care; Apache 2.0 is not compatible with GPLv2). Code with an incompatible
or unknown licence may be studied for ideas but not copied. Programs that
run outside ELKS (modem firmware, a proxy on another machine) can be used
as they are. Record every reuse and its licence in `alci/RELATED.md`.
