# Alci

**ALCI: Asymmetric Linux Compact Initiative**, an experimental branch of
[ELKS](https://github.com/ghaerr/elks) by Alan Cox, Greg Haerr and the ELKS team.

The name is a pun: *alci* is the Italian plural of *alce*, "elk".
"Asymmetric" describes the idea behind it: a minimal machine at the user's
end, large services elsewhere (an 8088 on the desk, an AI model in the cloud).

Goal: a faster, memory-frugal ELKS that runs on any PC from the 8088 to the
80386, with a graphical interface in the spirit of GEOS and a terminal for
talking to Claude or other language models.

- `TASKS.md`: the roadmap, task by task, with status.
- `RESULTS.md`: measurements and costs of every change.
- `LOG.md`: work log.
- `RELATED.md`: other projects that cover parts of the same ground.
- `AGENT.md`: instructions for the automated agent that works on this branch.
- `scripts/`: environment setup, benchmarks on MartyPC, profile reader.

Every change on branch `alci` is a separate commit, so that each can be
proposed upstream on its own. Changes are developed with the help of Claude
(Anthropic) and measured on the cycle-accurate MartyPC emulator, then
checked on 86Box (six machines with their real BIOS), QEMU and real hardware.
- `tests/`: functional and speed tests on QEMU and 86Box (see `tests/README.md`).
