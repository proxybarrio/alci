# Patches prepared for ELKS

Changes that also apply to upstream ELKS (`ghaerr/elks`), kept here as
plain patches against ELKS master with the measurements that support them.
They are not sent by the agent: the maintainer decides and sends them.

| File | Against | What | Measured |
| --- | --- | --- | --- |
| `exec-round-dseg.patch` | `df2a61c5` | `exec`: data segment rounded up to 256 bytes, so a program's freed segment fits the next exec of it | 86Box IBM AT 512K: 6 of 8 background `sh -c` failed with "not enough memory", now none; +17 bytes of code, about 128 bytes per process (task S3, `alci/RESULTS.md`) |
| `timer-eoi-com1.patch` | `df2a61c5` | timer interrupt: EOI before `timer_tick()`, BIOS IRQ 0 handler (floppy motor timeout) as a bottom half instead of inside the interrupt | 86Box IBM XT, COM1 at 19200: longest wait of a received byte 638 us -> 377 us (one byte is 521 us), a run of 300 typed commands no longer loses input; +29 bytes of code (task S1, `alci/RESULTS.md`) |
| `clock-cmos-year.patch` | `df2a61c5` | `clock -w` writes a two-digit BCD year (it wrote 0xC6 for 2026) and the AT century byte where present | 86Box IBM AT and 386: CMOS year 0xC6 -> 0x26; 1999 -> 0x99, century 0x19 (task S2) |
