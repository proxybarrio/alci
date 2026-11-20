# Alci

**ALCI: Asymmetric Linux Compact Initiative**, an experimental branch of
[ELKS](https://github.com/ghaerr/elks) by Alan Cox, Greg Haerr and the ELKS team.

The name is a pun: *alci* is the Italian plural of *alce*, "elk".
"Asymmetric" describes the idea behind it: a minimal machine at the user's
end, large services elsewhere (an 8088 on the desk, an AI model in the cloud).

Goal: a faster, memory-frugal ELKS that runs on any PC from the 8088 to the
80386, with a graphical interface in the spirit of GEOS and a terminal for
talking to Claude or other language models. Where a hard disk is present
Alci is installed on it in full; floppy-only machines boot from a 720K base
disk and get the rest from further disks of the set or from a package
repository.

- `STRATEGY.md`: the decisions that all work must respect (start here).
- `TASKS.md`: the roadmap, task by task, with status.
- `RESULTS.md`: measurements and costs of every change.
- `LOG.md`: work log.
- `RELATED.md`: other projects that cover parts of the same ground.
- `AGENT.md`: instructions for the automated agent that works on this branch.
- `scripts/`: environment setup, benchmarks on MartyPC, profile reader.
- `tests/`: functional and speed tests on QEMU and 86Box (see `tests/README.md`).
- `upstream/`: patches prepared for ELKS, sent by the maintainer.
- `docs/`: studies (`os8088-study.md`).
- `img/`: the Alci logo. `alci-logo-320x200.png` is the original, CGA
  mode 5 (black, light cyan, light red, white); `alci-logo.png` is the same
  shown at 4:3; `alci-logo-cga5.bin` is the 16 KB CGA screen for the boot
  logo (task D5).

Every change on branch `alci` is a separate commit, so that any of them can
be offered upstream on its own. Changes are developed with the help of Claude
(Anthropic) and measured on the cycle-accurate MartyPC emulator, then
checked on 86Box (six machines with their real BIOS), QEMU and real hardware.
