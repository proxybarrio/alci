# Patches prepared for ELKS

Changes that also apply to upstream ELKS (`ghaerr/elks`), kept here as
plain patches against ELKS master with the measurements that support them.
They are not sent by the agent: the maintainer decides and sends them.

| File | Against | What | Measured |
| --- | --- | --- | --- |
| `exec-round-dseg.patch` | `df2a61c5` | `exec`: data segment rounded up to 256 bytes, so a program's freed segment fits the next exec of it | 86Box IBM AT 512K: 6 of 8 background `sh -c` failed with "not enough memory", now none; +17 bytes of code, about 128 bytes per process (task S3, `alci/RESULTS.md`) |
| `timer-eoi-com1.patch` | `df2a61c5` | timer interrupt: EOI before `timer_tick()`, BIOS IRQ 0 handler (floppy motor timeout) as a bottom half instead of inside the interrupt | 86Box IBM XT, COM1 at 19200: longest wait of a received byte 638 us -> 377 us (one byte is 521 us), a run of 300 typed commands no longer loses input; +29 bytes of code (task S1, `alci/RESULTS.md`) |
| `clock-cmos-year.patch` | `df2a61c5` | `clock -w` writes a two-digit BCD year (it wrote 0xC6 for 2026) and the AT century byte where present | 86Box IBM AT and 386: CMOS year 0xC6 -> 0x26; 1999 -> 0x99, century 0x19 (task S2) |
| `exec-check-header-relocs.patch` | `df2a61c5` | `exec`: segment sizes and entry point checked (entry inside the text, no high words), and every relocation must patch a word inside its segment | 17 damaged programs on QEMU, 86Box (xt, t1200, pc1512, at) and MartyPC: relocations past their segment left the system dead on most runs, a bad entry point hung the program; now all are rejected with `ENOEXEC`/`EINVAL`; +90 bytes of far kernel code, near code unchanged, no speed change (task S4, `alci/RESULTS.md`) |
| `kilo-safe-save.patch` | `df2a61c5` | `kilo`: save through a temporary file, replace the original only after a complete write; fix a one-byte overrun at every save, a 256-byte stack buffer overrun when copying long lines, null pointer reads (plain text files, search without match), unbounded recent-file names, leaks on opening another file | Full disk on QEMU: before, the edited file was replaced by a truncated new one; after, it stays intact and the error says so. ASan/UBSan clean on a host build. +416 bytes on disk, no kernel change (task S5, `alci/RESULTS.md`) |
| `graphics-disk-wheel.patch` | `3332410` | the BIOS disk driver's wheel (top right character of the text screen) is not drawn while a program holds the console for graphics (`DCGET_GRAPH` .. `DCREL_GRAPH`) | QEMU CGA, ELKS `3332410` with the Contiki desktop listing `/bin`: stray pixels at the right end of the top pixel rows before, none after; +8 lines. Branch `graphics-disk-wheel` on `proxybarrio/elks` |
