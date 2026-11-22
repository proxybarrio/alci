# Screens from the 8088 to VGA: resolutions and pixel shapes

Study for the desktop (task D7) and its themes, 2026-11-22. The same video
mode can look very different on different machines: 640x200 is the CGA
mode of a desktop monitor and also of many laptop LCDs, but the panels
have different proportions, so the pixels have different shapes. A theme
drawn for one shape looks squashed or stretched on another.

Pixel shape below = pixel width / pixel height, from the visible area of
the screen and the resolution. 1.0 is a square pixel; 0.42 means the
pixel is 2.4 times taller than wide.

## Desktop monitors (4:3 picture)

| Adapter and mode | Resolution | Colours | Pixel shape |
| --- | --- | --- | --- |
| CGA, IBM 5153 and compatibles | 640x200 | 2 | 0.42 |
| CGA | 320x200 | 4 | 0.83 |
| PCjr, Tandy 1000 | 320x200 | 16 | 0.83 |
| PCjr, Tandy 1000 | 640x200 | 4 | 0.42 |
| Amstrad PC1512 | 640x200 | 16 | 0.42 |
| Hercules (HGC) | 720x348 | 2 | 0.64 |
| Olivetti M24, AT&T 6300 (640x400 "DCGA") | 640x400 | 2 | 0.83 |
| Toshiba T3100 (plasma, same 640x400 mode) | 640x400 | 2 | about 0.83 |
| EGA | 640x350 | 16 | 0.73 |
| EGA | 640x200 | 16 | 0.42 |
| MCGA, VGA | 640x480 | 2 / 16 | 1.00 |
| MCGA, VGA (mode 13h) | 320x200 | 256 | 0.83 |

MDA has no graphics mode (text only, 720x350 character cells).

## Laptop and palmtop LCDs

| Machine | CPU | Panel (visible area) | Resolution | Pixel shape |
| --- | --- | --- | --- | --- |
| Toshiba T1000 | 80C88 | 9.6" x 3" | 640x200 | 1.00 (Toshiba states 1:1) |
| Toshiba T1200 F/H (reflective) | 80C86 | 9.24" x 4.2" | 640x200 | 0.69 |
| Toshiba T1200 FB/HB (EL backlit) | 80C86 | 9" x 4.2" | 640x200 | 0.67 |
| Toshiba T1200XE | 80286 | 7.75" x 4.88" | 640x400 | 0.99 |
| IBM PC Convertible (5140) | 80C88 | very wide panel | 640x200 | close to square or wider: text looks half height |
| Olivetti Quaderno PT-XT-20 | NEC V30 | 14 x 10.5 cm | 640x400 (DCGA) | 0.83 |
| Olivetti Quaderno 33 | 386SX | backlit VGA panel | 640x480 | about 1.00 |
| HP 100LX, 200LX | 80186 | 4.8" diagonal | 640x200 | to measure |
| HP 95LX | V20 | small panel | 240x128 (MDA-like) | to measure; too small for the desktop |

## What it means for Alci

1. Three families of pixel shape, not one:
   - tall (0.42): 640x200 on CGA, PCjr, Tandy and EGA monitors;
   - medium (0.64 to 0.73): Hercules, EGA 640x350, the T1200 LCD;
   - square (0.83 to 1.0): the 640x400 machines (Olivetti M24, AT&T 6300,
     T3100, Quaderno, T1200XE), VGA, the T1000 LCD, all the 320x200 modes.
   A theme is drawn for one family; strokes, corners, icons and the
   desktop pattern change with it. The 8-byte cell patterns stay, so a
   theme family costs no memory beyond its own data.
2. The video mode does not tell the panel: a T1200 and a desktop CGA
   monitor both run 640x200 and look different. The theme is chosen
   automatically where the machine can be recognised (BIOS signatures
   such as the PC Convertible's INT 10h AH=15h, Toshiba's BIOS) and can
   always be set by the user (one line in `/bootopts` or in the desktop
   settings).
3. A 640x400 drawing layer is worth adding early: it serves the Olivetti
   M24 and Quaderno, AT&T 6300, Toshiba T3100 and T1200XE, with square-ish
   pixels and twice the vertical detail (32 KB of video memory, four
   interleaved banks).
4. Order of drawing layers: CGA 640x200 (now), 640x400, Hercules, EGA
   640x350, VGA 640x480, CGA 320x200 four colours. Each one is tested on
   86Box machines with the matching BIOS where they exist, and on real
   hardware when available (Toshiba T1200).

## Sources

- Toshiba T1200, T1000, T1200XE specification sheets (Toshiba America, via
  manualmachine.com and the T1200.SPC file); Wikipedia, "Toshiba T series".
- Olivetti Quaderno: Centre for Computing History; Politecnico di Milano,
  historical collections; Wikipedia.
- IBM PC Convertible: Wikipedia; seasip.info "The IBM 5140"; ardent-tool.com.
- HP 100LX/200LX/95LX: Wikipedia; palmtops.net; retroisle.com.
- CGA, EGA, VGA, Hercules, M24 modes: standard adapter documentation; the
  pixel shapes are computed from a 4:3 picture.
