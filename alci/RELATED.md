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

Sources: the project pages linked above; Ars Technica, March 2023, on
doschgpt; ghaerr/elks issue #2630; a video demonstration of ELKS with
Nano-X. This note was first written outside this repository and checked on
2026-11-16; details of other projects may change.
