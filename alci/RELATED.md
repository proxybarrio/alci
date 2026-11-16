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

## What can be reused, and how

| Project | Licence | Reuse |
| --- | --- | --- |
| AiBridgeMCP Nano, Zimodem | to be checked | Used as they are, on the ESP32: they run outside ELKS. Together with ELKS's `miniterm` they may give a Claude terminal with no new code (task E0). |
| PicoMEM | to be checked | Hardware used as it is; ELKS already has the NE2000 driver it emulates (task E0b). |
| doschgpt, http-to-https-proxy | to be checked | The proxy runs on a modern machine and can be used as it is. The client's request and reply code may be adapted only if its licence allows. |
| Nano-X | already in ELKS | Measure first (D2); the cheapest base for the interface. |
| os8088 | not stated on the pages read | Ideas only until the licence is known: CGA drawing, clipping, multitasking on an 8088 in 256K. |
| PC/GEOS (FreeGEOS) | Apache 2.0 | Not compatible with the GPLv2 kernel. Ideas, or separate user programs under their own licence. |
| SymbOS | freeware | Ideas only (redraw windows on demand instead of storing hidden areas). |
| UnoDOS | MPL 2.0 | Phase 7 only (frozen): how one image boots on legacy BIOS and UEFI. |

Sources: the project pages linked above; Ars Technica, March 2023, on
doschgpt; ghaerr/elks issue #2630; a video demonstration of ELKS with
Nano-X. This note was first written outside this repository and checked on
2026-11-16, with further projects found by web search the same day; details of other projects may change.
