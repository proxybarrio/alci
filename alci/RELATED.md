# Related projects

No other known project combines a small 16-bit Unix-like kernel, a
PC/GEOS-style graphical interface and a client for language models over the
serial port. Several projects cover parts of the same ground; Alci should
learn from them and avoid duplicating them.

| Project | What it covers | Relation to Alci |
| --- | --- | --- |
| [ELKS](https://github.com/ghaerr/elks) with Microwindows / Nano-X | Upstream kernel; a multi-process windowing system and basic desktop programs for 16-bit x86 | Alci is a branch of ELKS. Nano-X is the obvious drawing layer to measure first (task D2) before designing a GEOS-like interface on top of it or beside it. |
| [doschgpt](https://yeokhengmeng.com/2023/03/building-a-dos-chatgpt-client-in-2023/) by Yeo Kheng Meng | ChatGPT client for MS-DOS that runs on a 4.77 MHz 8088 (IBM 5155); networking through a packet driver and mTCP, with a proxy on a modern machine handling HTTPS | Closest precedent for the Claude terminal. Alci differs in the transport: a Wi-Fi modem (ESP32) on the serial port instead of an Ethernet card, and a Unix-like system underneath. Its proxy design is worth studying for task E1. |
| [llama2 on DOS](https://yeokhengmeng.com/2025/04/llama2-llm-on-dos/), same author | Small Llama 2 models run locally on DOS PCs, down to a 486 | The opposite of Alci's "asymmetric" choice (model in the cloud, minimal machine on the desk); useful as a comparison, not a direction. |
| Wi-Fi modem firmware (Zimodem, WiFi232) and retro BBS bridges | 8- and 16-bit machines reach modern services through a Hayes-style modem on the serial port; some hobbyist setups proxy requests to OpenAI or Anthropic APIs | The hardware route Alci plans to use (tasks E1-E3). Reuse existing firmware where possible. |
| [PC/GEOS](https://en.wikipedia.org/wiki/GEOS_(16-bit_operating_system)), Breadbox Ensemble, FreeGEOS | The original GEOS environment for 8088-80386 PCs, running on DOS; partly open-sourced as FreeGEOS | The model for Alci's interface and memory management (discardable code segments, task D1-D3). It has no Unix-like kernel underneath. |
| [os8088](https://os8088.com/) (2026) | Graphical OS for the IBM PC/XT in the style of Macintosh System 1: overlapping windows, menus, pre-emptive multitasking, fits in 256K, real-mode assembly written with AI help, no DOS underneath | The closest rival to Alci's interface (phase 4). A yardstick for what an 8088 can do; no Unix-like kernel, its own program format. |
| [UnoDOS](https://github.com/hmofet/unodos) (2026) | One source tree that boots on 22 machines, from an 8088 PC/XT to modern UEFI PCs; own window manager, TCP/IP stack and applications | The same "one system for every PC" idea as phases 6-7, already built; shows that a UEFI path is feasible. Worth studying before G1-G3. |
| [AiBridgeMCP Nano](https://hackaday.io/project/205094-bridge-across-decades) (2026) | ESP32 bridge that connects 1980s computers (CP/M, Apple II, MSX, C64) to Claude over Wi-Fi through the serial port | Very close to Alci's modem plan (E1-E3). Check its firmware and protocol before writing our own. |
| [PicoMEM](https://github.com/FreddyVRetro/ISA-PicoMEM) | 8-bit ISA card with a Raspberry Pi Pico W: disk and floppy emulation, extra memory, NE2000 network card emulated over Wi-Fi; works on 8088 PCs | An alternative to the serial modem: ELKS already has an NE2000 driver, so an XT with this card could reach the network without new kernel code. To be tested. |
| GEOS kernel rebuilt for the XT (Vogons forum, 2024) | Recompiled kernel that lets Geoworks, NewDeal and Breadbox Ensemble run on 8088 machines | Confirms that a GEOS-class interface fits an XT; a reference for D1. |
| [SymbOS](https://en.wikipedia.org/wiki/SymbOS) 4.0 (2025) | Pre-emptive multitasking OS with a Windows-like GUI for Z80 computers, in assembly | Not x86, but its memory-saving GUI design (windows redrawn on demand instead of keeping hidden areas in memory) is directly useful for D3. |
| [FUZIX](https://github.com/EtchedPixels/FUZIX) | Small Unix-like system for 8-bit and other CPUs; the 8086 port is only sketches | A peer, not a rival on PCs today. |
| RetroCampus BBS | ChatGPT for the Commodore 64 through a Wi-Fi modem and a BBS that relays requests | Another example of the "modem plus relay" pattern. |
| [ept](https://github.com/sammothxc/ept) (2026) | Package tool for ELKS: downloads programs over plain HTTP from a repository, checks size and MD5 of each file, upgrades; packages have no dependencies; a download-only build is about 8.5 KB | Close to F5 and the 720K set: with a network, programs of disk 2 can be fetched instead of swapping floppies. Planned for inclusion in ELKS, so it may arrive through the upstream merge: check before building anything similar. |
| [Gem-elks](https://github.com/parabyte/Gem-elks) (2026) | The GEM desktop (sources released under the GPL by Caldera) ported to stock ELKS: a user-space AES/VDI process talks to the desktop client over pipes, programs start one at a time, BIOS 8x8 fonts, ELKS timer instead of GEM's own; tested on an Amstrad PC1640 | The nearest existing desktop to Alci's interface (phase 4). To compare with PC/GEOS and Nano-X in D1/D2 before choosing the base for D4. |
| [elks-w16](https://github.com/astrobleem/elks-w16) (2026) | Loader for Windows 3.0 (Win16, NE format) programs on ELKS, with USER and GDI translated to Nano-X calls; one task at a time; needs patches to ELKS and Microwindows; tried on emulated Tandy and PCjr | A way to give the desktop existing programs later; heavy in memory. Not before D2 and D4. |
| [ELKSWeb](https://github.com/l00nix/ELKSWeb) (2026) | Graphical web browser for ELKS, drawing directly on CGA, EGA and VGA; HTTP only; version 0.5 of 2026-11-18, early beta; installable with ept | Changes E6: measure it before porting a text browser; HTTPS would come from E5. |
| [picomem-elks](https://github.com/l00nix/picomem-elks) (2026) | Configuration tools for the PicoMEM card on ELKS (network IRQ, fast boot), with backup and rollback; kept outside the ELKS tree | Useful with E0b on real hardware (PicoMEM on an XT). |
| [unzx0_x86](https://github.com/emmanuel-marty/unzx0_x86) | ZX0 decompressors for x86, including an 8088-tuned one, by Emmanuel Marty (also the author of LZSA) | ZX0 compresses better than LZSA1; whether it decodes fast enough on the 8088 is to be measured in F4e on the same programs. |
| [LZPACK](https://github.com/johnsonjh/lzpack) (2026) | Executable compressor for CP/M-80 (self-extracting `.COM`, PopCom!-compatible format), portable C89; on ELKS (8086/8088, a 16 KB binary) it compresses and restores files, with a hand-written 8086 decoder; LZ with an 8 KB window and a bit-oriented stream | Not an ELKS executable packer, so no replacement for the kernel's LZSA1 path. Useful as a compressor that runs on Alci itself (LZSA's compressor does not) and as one more codec to measure in F4e. Its author is also adding ELKS executables to UPX: self-extracting programs that would need no kernel support, to compare in F4e when available. |
| [8086tiny on ELKS](https://github.com/tyama501/8086tinyFile) (2026) | The 8086tiny emulator running on ELKS, to run DOS programs such as FreeDOS; experimental and very slow | Not for Alci now; DOS programs on ELKS would belong to phase 7 (frozen). |
| [Contiki 1.x](https://github.com/adamdunkels/contiki-1.x) | Adam Dunkels' small operating system of 2002-2004, with the CTK graphical desktop (windows, menus, icons, mouse) on the Commodore 64 and other 8-bit machines; also ports to Win32 and GTK; applications: editor, web browser, e-mail, FTP, IRC, calculator | Desktop candidate D7: the C core of CTK and the applications on top of ELKS files and sockets, with a new CGA drawing layer. Contiki 2.x (`contiki-os/contiki`, `core/ctk`) is the fallback base. |
| [Bobcat](https://github.com/rafael2k/bobcat) | Text web browser derived from Lynx, from 16-bit DOS; ELKS port built by `buildext.sh` since January 2026 | E6: build and measure before anything else. |
| [astrobleem/elks](https://github.com/astrobleem/elks) (2026) | ELKS fork with experimental branches: a `/dev/ems` storage device for EMS boards (Lo-tech, PicoMEM), a loader for character-device modules, Tandy and PCjr support | EMS studied in B3; the module loader is research only (loading code into a kernel with no memory protection is riskier than freeing unused code, as A0 and A5 do). |

## What can be reused, and how

| Project | Licence | Reuse |
| --- | --- | --- |
| AiBridgeMCP Nano, Zimodem | to be checked | Used as they are, on the ESP32: they run outside ELKS. Together with ELKS's `miniterm` they may give a Claude terminal with no new code (task E0). |
| PicoMEM | to be checked (firmware); os8088's 86Box patch and libpmhost build are MIT | Hardware used as it is; ELKS already has the NE2000 driver it emulates. Tested in 86Box on 2026-11-16: Alci reached the network on an emulated 8088 XT with no new code (task E0b). |
| doschgpt, http-to-https-proxy | to be checked | The proxy runs on a modern machine and can be used as it is. The client's request and reply code may be adapted only if its licence allows. |
| Nano-X | already in ELKS | Measure first (D2); the cheapest base for the interface. |
| [os8088](https://github.com/jggonz/os8088) | MIT (checked 2026-11-16) | Compatible: code may be adapted if the copyright notice and licence text are kept. Not covered: `reference/` (third-party material such as Microsoft's Gorillas and Nintendo's Dr. Mario data) and possibly the fonts (derived from an outline font whose licence must be checked). About 180,000 lines of NASM for its own kernel and calling conventions, so pieces are lifted and rewritten, not dropped in: CGA drawing, clipping and redraw (`REDRAW-SPEC.md`), the `.F88` font format, mouse driver, memory layout. Its MartyPC debug server (`tools/martypc/`: breakpoints, registers, cycle counts with no code in the guest) could replace part of our profiler. |
| PC/GEOS (FreeGEOS) | Apache 2.0 | Cannot go into the GPLv2 kernel or be linked with GPL code. Can ship as a separate program under Apache 2.0 (see `STRATEGY.md`, working with others): a port is legally possible, with a libc audit first. |
| SymbOS | freeware | Ideas only (redraw windows on demand instead of storing hidden areas). |
| UnoDOS | MPL 2.0 | Phase 7 only (frozen): how one image boots on legacy BIOS and UEFI. |
| ept | MIT (checked 2026-11-21) | Compatible. Prefer the version included in ELKS, if it is merged upstream, to a copy of our own. |
| Gem-elks | GPLv2 (checked 2026-11-21) | Compatible: code may be adapted with its notices kept. |
| elks-w16 | MIT (checked 2026-11-21) | Compatible: code may be adapted with its notice kept. |
| ELKSWeb | GPL-2.0-or-later (checked 2026-11-21); font data public domain; one test fixture GPL-3.0 | Compatible, except the GPL-3.0 test fixture, which must not be copied into GPLv2 code. |
| picomem-elks | GPL-2.0-or-later (checked 2026-11-21) | Compatible; used as it is, outside the ELKS tree. |
| unzx0_x86 | zlib (checked 2026-11-22) | Compatible, notice kept. |
| LZPACK | MIT-0 (checked 2026-11-21) | Compatible, no notice required. |
| 8086tiny on ELKS | MIT (8086tiny, checked 2026-11-21) | Compatible; not needed now (phase 7). |
| Contiki 1.x and 2.x | BSD 3-clause in the CTK sources (checked 2026-11-22); each application and data file to be checked on its own | Compatible with GPLv2 and usable in a separate program: notices kept in every file taken. |
| Bobcat | GPL for the Lynx base; other parts under their own licences (README, checked 2026-11-22) | Compatible as a separate program; list the licences of its parts before shipping binaries. |
| astrobleem/elks | GPLv2 (ELKS fork) | Compatible, notices kept. |

Sources: the project pages linked above; Ars Technica, March 2023, on
doschgpt; ghaerr/elks issue #2630; a video demonstration of ELKS with
Nano-X. This note was first written outside this repository and checked on
2026-11-16, with further projects found by web search the same day; details of other projects may change.

Technical points from the ELKS project that bear on Alci (2026-11-21):
the ELKS kernel is near its size limits (near code above 63 KB, far code
above 36 KB), so features that can live in an application, without kernel
changes or new system calls, are preferred; UDP exists only for DHCP; new
tools under active development are kept outside the ELKS tree and built
with `buildext.sh`; video mode switching for graphical programs goes
through a console driver `ioctl`, not new kernel code. On 286 boards with
a NEC D80C42C keyboard controller, and on the EC1842, the A20 code that
reads and writes back the 8042 output port hangs the keyboard;
`USE_HIMEM_AT` avoids the read. ELKS supports a PS/2 mouse on the IBM PC.
MFM, SMC Ultra and PEGA drivers derived from Linux 2.x, tested on an
Amstrad PC1640, are now in the ELKS tree and in Alci (`mfmhd.c`,
`ultra.c`, `direct-amstrad.c`; upstream since August 2026).

Study of os8088's distribution and what Alci takes from it: `docs/os8088-study.md` (2026-11-19).

LZ4 (2026-11-19): tried and dropped in favour of LZSA1 (`RESULTS.md`); our own decoder, no code copied, removed in `80cc11ac`.

LZSA (2026-11-19, github.com/emmanuel-marty/lzsa, zlib licence, compatible): `elks/fs/lzsa1dec.S` is an altered copy of `asm/8088/LZSA1FTA.ASM` (Jim Leonard, Emmanuel Marty), notice kept; the `lzsa` compressor runs on the build host (`elks/tools/elks-compress/elks-pack.py`).
