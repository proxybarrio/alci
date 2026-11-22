"""Render the hand-drawn CGA icons to indexed PNG files.

Usage: python3 render.py <png-directory>
Writes <png-directory>/cga640-mono/*.png and <png-directory>/cga320-4col/*.png.
"""
import os
import sys
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mono  # noqa: E402
import col   # noqa: E402

NAMES = ['about', 'calculator', 'dhcp', 'directory', 'editor', 'email', 'ftp', 'irc',
         'netconf', 'processes', 'shell', 'telnet', 'telnetd', 'vnc', 'www', 'webserver']

SETS = (
    ('cga640-mono', mono, [(255, 255, 255), (0, 0, 0)]),
    ('cga320-4col', col, [(0, 0, 0), (85, 255, 255), (255, 85, 255), (255, 255, 255)]),
)


def main(out):
    for sub, mod, pal in SETS:
        os.makedirs(os.path.join(out, sub), exist_ok=True)
        flat = [v for rgb in pal for v in rgb]
        for n in NAMES:
            cv = getattr(mod, n)()
            im = Image.new('P', (cv.w, cv.h))
            im.putpalette(flat + [0] * (768 - len(flat)))
            im.putdata([int(v) for v in cv.a.ravel()])
            im.save(os.path.join(out, sub, n + '.png'))


if __name__ == '__main__':
    main(sys.argv[1])
