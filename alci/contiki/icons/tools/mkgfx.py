"""Write the icon header for one desktop build: python3 mkgfx.py TARGET OUT.h name...

TARGET is cga, cga4, ega, vga or mcga. Each icon is stored as plain rows, one
byte per pixel, holding the colour index of the target's palette; 255 marks the
paper around the drawing, where the desktop shows through (colour sets only).
Only the icons named are built in, so a set larger than the data segment (the
VGA one) still fits. For cga (two colours) the icons are stored as 8x8 cells instead, one byte per
pixel row, ready to copy to the screen. For mcga the icons are the 40x32 set
drawn for it (mcga320-256col), sharing at most 48 palette entries (200-247);
the palette is written too. With SETS['mcga'] = 'modern-64' the modern set is
reduced to 40x32 instead, as the first MCGA build did."""
import os, sys
import numpy as np
from PIL import Image
from scipy import ndimage

HERE = os.path.dirname(os.path.abspath(__file__))
PNG = os.path.join(HERE, '..', 'png')
SETS = {'cga': 'cga640-mono', 'cga4': 'cga320-4col-32x24', 'ega': 'ega640x350-16col',
        'vga': 'vga640-16col', 'mcga': 'mcga320-256col-32x24', 'tandy': 'tandy320-16col-32x24', 'tandy4': 'tandy640-4col',
        'herc': 'hercules-mono', 'att': 'hires640x400-mono',
        'ega2': 'hercules-mono', 'vga2': 'hires640x400-mono'}
# two-colour screens and the height of their cells
MONO_CH = {'cga': 8, 'herc': 14, 'att': 16, 'ega2': 14, 'vga2': 16}
MCGA_BASE, MCGA_N = 200, 48


def outer_paper(idx, paper):
    """Pixels of the paper colour connected to the border: the transparent part."""
    same = idx == paper
    lab, _ = ndimage.label(same)
    border = set(np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]]))) - {0}
    return np.isin(lab, list(border))


def main():
    target, out, names = sys.argv[1], sys.argv[2], sys.argv[3:]
    d = os.path.join(PNG, SETS[target])
    icons, pal = [], None
    if target == 'mcga' and SETS['mcga'] != 'modern-64':
        # drawn for MCGA: 40x32 palette PNGs sharing one palette, the last
        # used index transparent; their colours go to the entries from
        # MCGA_BASE on
        pal = None
        for n in names:
            im = Image.open(os.path.join(d, n + '.png'))
            if im.mode != 'P' or im.size not in ((40, 32), (32, 24)):
                raise SystemExit('%s: expected a 40x32 or 32x24 palette PNG' % n)
            t = im.info.get('transparency')
            a = np.asarray(im).astype(int)
            if pal is None:
                p = im.getpalette()
                ncol = t if t is not None else MCGA_N
                if ncol > MCGA_N:
                    raise SystemExit('more than %d colours' % MCGA_N)
                pal = [tuple(p[3 * i:3 * i + 3]) for i in range(ncol)]
            idx = a + MCGA_BASE
            if t is not None:
                idx[a == t] = 255
            if (idx[idx != 255] >= MCGA_BASE + len(pal)).any():
                raise SystemExit('%s: colour outside the shared palette' % n)
            icons.append(idx)
    elif target == 'mcga':
        ims = []
        for n in names:
            im = Image.open(os.path.join(d, n + '.png')).convert('RGBA').resize((40, 32), Image.LANCZOS)
            ims.append(np.asarray(im))
        strip = np.concatenate([a[:, :, :3] for a in ims], axis=1)
        q = Image.fromarray(strip.astype(np.uint8)).quantize(MCGA_N, method=Image.Quantize.MEDIANCUT,
                                                              dither=Image.Dither.NONE)
        qa = np.asarray(q)
        p = q.getpalette()[:3 * MCGA_N]
        pal = [tuple(p[3 * i:3 * i + 3]) for i in range(MCGA_N)]
        for k, a in enumerate(ims):
            idx = qa[:, 40 * k:40 * (k + 1)].astype(int) + MCGA_BASE
            idx[a[:, :, 3] < 128] = 255
            icons.append(idx)
    else:
        for n in names:
            im = Image.open(os.path.join(d, n + '.png'))
            if im.mode != 'P':
                raise SystemExit('%s: expected a palette PNG' % n)
            idx = np.asarray(im).astype(int)
            if target in MONO_CH:
                # 1 = ink in the drawing layer, drawn opaque. The figure is
                # dark on white in most sets; the Hercules set has it light
                # on black, so there the light pixels are the ink
                p = im.getpalette()
                lum = np.array([sum(p[3 * i:3 * i + 3]) for i in range(len(p) // 3)])
                light = SETS[target] == 'hercules-mono'
                idx = np.where((lum[idx] >= 384) if light else (lum[idx] < 384), 1, 0)
            else:
                paper = int(np.bincount(np.concatenate([idx[0], idx[-1], idx[:, 0], idx[:, -1]])).argmax())
                idx = idx.copy()
                idx[outer_paper(idx, paper)] = 255
                if target in ('cga4', 'tandy') and idx.shape == (24, 32):
                    # 32x24 in a column of five cells: four transparent
                    # pixels each side, so that whole cells are stored
                    idx = np.pad(idx, ((0, 0), (4, 4)), constant_values=255)
            icons.append(idx)
    with open(out, 'w') as f:
        f.write('/* Desktop icons for the %s build, generated by icons/tools/mkgfx.py from icons/png/%s. */\n'
                % (target, SETS[target]))
        # four colours: icons that fill their cells exactly are stored packed
        packed4 = (target == 'cga4' and all(a.shape in ((32, 40), (24, 40)) for a in icons)) or \
            (target == 'tandy4' and all(a.shape == (32, 80) for a in icons)) or \
            (target == 'tandy' and all(a.shape in ((32, 40), (24, 40)) for a in icons))
        if packed4:
            f.write('#define GFX_ICON_PACKED4 1\n')
        f.write('#ifndef GFX_PALETTE_ONLY\n')
        for n, a in zip(names, icons):
            h, w = a.shape
            if target in MONO_CH:
                # two colours: cells of 8 x cell height, rows of cells top to
                # bottom, one byte per pixel row, leftmost pixel in the top
                # bit, 1 = ink
                ch = MONO_CH[target]
                if h % ch or w % 8:
                    raise SystemExit('%s: %dx%d is not whole cells of 8x%d' % (n, w, h, ch))
                cells = []
                for cy in range(h // ch):
                    for cx in range(w // 8):
                        for r in range(ch):
                            row = a[cy * ch + r, cx * 8:cx * 8 + 8]
                            cells.append(int(''.join(str(int(v)) for v in row), 2))
                flat = np.array(cells)
                f.write('static const unsigned char gfx_icon_%s_px[%d] = {\n' % (n, len(flat)))
            elif target == 'tandy' and packed4:
                # Tandy 320x200 in 16 colours: cells of 8x8, four bytes a
                # pixel row, two 4-bit pixels a byte, the left one high, as
                # the video memory has them; around the drawing the grey of
                # the desktop (7)
                cells = []
                b = np.where(a == 255, 7, a)
                for cy in range(h // 8):
                    for cx in range(w // 8):
                        for r in range(8):
                            row = b[cy * 8 + r, cx * 8:cx * 8 + 8]
                            for k in range(0, 8, 2):
                                cells.append((int(row[k]) << 4) | int(row[k + 1]))
                flat = np.array(cells)
                f.write('static const unsigned char gfx_icon_%s_px[%d] = {\n' % (n, len(flat)))
            elif target == 'tandy4' and packed4:
                # Tandy 640x200 in four colours: cells of 8x8, two bytes a
                # pixel row, the low bit of the eight pixels then the high
                # bit, as the video memory has them
                cells = []
                b = np.where(a == 255, 0, a)
                for cy in range(h // 8):
                    for cx in range(w // 8):
                        for r in range(8):
                            row = b[cy * 8 + r, cx * 8:cx * 8 + 8]
                            for bit in (1, 2):
                                v = 0
                                for x in row:
                                    v = (v << 1) | (1 if int(x) & bit else 0)
                                cells.append(v)
                flat = np.array(cells)
                f.write('static const unsigned char gfx_icon_%s_px[%d] = {\n' % (n, len(flat)))
            elif target == 'cga4' and packed4:
                # four colours: cells of 8x8 as the video memory has them,
                # two bytes a pixel row, four 2-bit pixels a byte, leftmost
                # pixel in the top bits; transparent is the black desktop
                cells = []
                b = np.where(a == 255, 0, a)
                for cy in range(h // 8):
                    for cx in range(w // 8):
                        for r in range(8):
                            row = b[cy * 8 + r, cx * 8:cx * 8 + 8]
                            for k in (0, 4):
                                v = 0
                                for x in row[k:k + 4]:
                                    v = (v << 2) | int(x)
                                cells.append(v)
                flat = np.array(cells)
                f.write('static const unsigned char gfx_icon_%s_px[%d] = {\n' % (n, len(flat)))
            else:
                f.write('static const unsigned char gfx_icon_%s_px[%d] = {\n' % (n, w * h))
                flat = a.flatten()
            for i in range(0, len(flat), 24):
                f.write('  ' + ','.join(str(v) for v in flat[i:i + 24]) + ',\n')
            f.write('};\n')
        f.write('#define GFX_ICON_COUNT %d\n' % len(names))
        f.write('static const struct gfx_icon gfx_icons[GFX_ICON_COUNT] = {\n')
        for n, a in zip(names, icons):
            f.write('  {%d, %d, gfx_icon_%s_px},\n' % (a.shape[1], a.shape[0], n))
        f.write('};\n')
        f.write('static const char * const gfx_icon_names[GFX_ICON_COUNT] = {%s};\n'
                % ','.join('"%s"' % n for n in names))
        f.write('#endif\n')
        if pal:
            f.write('#define GFX_ICON_PAL_BASE %d\n#define GFX_ICON_PAL_N %d\n' % (MCGA_BASE, len(pal)))
            f.write('static const unsigned char gfx_icon_pal[%d][3] = {%s};\n'
                    % (len(pal), ','.join('{%d,%d,%d}' % c for c in pal)))


main()
