"""Write the 32x24 icon sets of the 320x200 screens from their 40x32 sets:
python3 shrink32x24.py [SET...]

Each icon loses 8 of its 40 columns and 8 of its 32 rows, one in every 5
columns and one in every 4 rows, so the spacing stays even and round shapes
stay round. In each group the line removed is the one most like a neighbour,
and the pattern of offsets with the least loss wins. No colour is mixed:
every pixel left is a pixel of the drawing. The calculator, whose symbols do
not survive the cut, is drawn again on the reduced frame."""
import itertools, os, sys
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
PNG = os.path.join(HERE, '..', 'png')
SETS = ['cga320-4col', 'tandy320-16col', 'mcga320-256col']
NAMES = ['about', 'calculator', 'directory', 'editor', 'email', 'ftp', 'irc', 'netconf',
         'processes', 'shell', 'telnet', 'telnetd', 'vnc', 'webserver', 'www', 'dhcp']
SYM = {'+': ["...#...", "...#...", "#######", "...#...", "...#..."],
       '-': [".......", ".......", "#######", ".......", "......."],
       'x': ["##...##", ".##.##.", "..###..", ".##.##.", "##...##"],
       '/': ["...#...", ".......", "#######", ".......", "...#..."]}


def loss(a, drop, axis):
    n = 0
    for c in drop:
        line = np.take(a, c, axis=axis)
        nb = [np.take(a, c + d, axis=axis) for d in (-1, 1) if 0 <= c + d < a.shape[axis]]
        n += min(int((line != b).sum()) for b in nb)
    return n


def cut(a, axis, step):
    """drop one line in every `step` along axis"""
    size = a.shape[axis]
    best = None
    for base in range(step):
        drop = []
        for b in range(size // step):
            cands = [c for c in (b * step + (base + j) % step for j in (-1, 0, 1)) if 0 <= c < size]
            drop.append(min(cands, key=lambda c: loss(a, [c], axis)))
        drop = sorted(set(drop))
        if len(drop) != size // step:
            continue
        gaps = np.diff([-1] + drop + [size])
        cost = loss(a, drop, axis) * 4 + int(gaps.max() - gaps.min()) * 6
        if best is None or cost < best[0]:
            best = (cost, drop)
    return np.delete(a, best[1], axis=axis)


def symbol(a, x0, y0, s, col):
    for r, row in enumerate(SYM[s]):
        for k, ch in enumerate(row):
            if ch == '#':
                a[y0 + r, x0 + k] = col


def calculator(set_, a):
    if set_ == 'cga320-4col':
        # white frame and cross, cyan + and -, magenta x and divide
        a = np.zeros((24, 32), np.uint8)
        a[1, :] = a[22, :] = 3
        a[1:23, 0:2] = a[1:23, 30:32] = a[1:23, 15:17] = 3
        a[11:13, :] = 3
        for x0, y0, s, c in ((2, 2, '+', 1), (17, 2, '-', 1), (2, 13, 'x', 2), (17, 13, '/', 2)):
            symbol(a, x0 + 3, y0 + 2, s, c)
        return a
    # Tandy and MCGA: four bevelled keys (light top and left, dark bottom
    # and right) with black symbols, inside the reduced frame
    hi, face, shade = (15, 9, 1) if set_ == 'tandy320-16col' else (11, 9, 6)
    a = a.copy()
    a[3:21, 4:29] = 0
    for bx, by, s in ((5, 4, '+'), (17, 4, '-'), (5, 13, 'x'), (17, 13, '/')):
        b = np.full((8, 11), face, np.uint8)
        b[0, :] = hi
        b[:, 0] = hi
        b[7, :] = shade
        b[:, 10] = shade
        b[0, 10] = b[7, 0] = face
        a[by:by + 8, bx:bx + 11] = b
        symbol(a, bx + 2, by + 1, s, 0)
    return a


def main():
    for set_ in sys.argv[1:] or SETS:
        out = os.path.join(PNG, set_ + '-32x24')
        os.makedirs(out, exist_ok=True)
        for n in NAMES:
            im = Image.open(os.path.join(PNG, set_, n + '.png'))
            a = cut(cut(np.asarray(im), 1, 5), 0, 4)
            if n == 'calculator':
                a = calculator(set_, a)
            o = Image.fromarray(a.astype(np.uint8), 'P')
            o.putpalette(im.getpalette())
            t = im.info.get('transparency')
            o.save(os.path.join(out, n + '.png'), **({'transparency': t} if t is not None else {}))


main()
