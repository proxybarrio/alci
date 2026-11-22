"""Draw every icon set on one page, each in the pixel shape of its screen.

Usage: python3 preview.py <png-directory> <output.png>
"""
import os
import sys
from PIL import Image, ImageDraw, ImageFont

NAMES = ['about', 'calculator', 'dhcp', 'directory', 'editor', 'email', 'ftp', 'irc',
         'netconf', 'processes', 'shell', 'telnet', 'telnetd', 'vnc', 'www', 'webserver']
# set, title, horizontal and vertical scale (pixel shape), background
ROWS = [
    ('cga640-mono', 'CGA 640x200, 2 colours, 80x32', 1, 2.4, (255, 255, 255)),
    ('cga320-4col', 'CGA 320x200, 4 colours, 40x32', 2, 2.4, (0, 0, 0)),
    ('hires640x400-mono', '640x400, 2 colours, 96x64', 1, 1.2, (255, 255, 255)),
    ('vga640-16col', 'VGA 640x480, 16 colours, 104x80', 1, 1, (255, 255, 255)),
    ('modern-64', 'Modern, 64x64', 1, 1, (70, 110, 140)),
    ('modern-128', 'Modern, 128x128', 1, 1, (70, 110, 140)),
]
BOX = 136
PER_ROW = 8


def main(pngdir, out):
    try:
        font = ImageFont.truetype('DejaVuSans.ttf', 15)
    except OSError:
        font = ImageFont.load_default()
    rows_h = sum(30 + 2 * (BOX + 6) for _ in ROWS)
    page = Image.new('RGB', (16 + PER_ROW * (BOX + 6), rows_h + 10), (228, 228, 228))
    d = ImageDraw.Draw(page)
    y = 8
    for name, title, sx, sy, bg in ROWS:
        d.text((10, y), title, fill=(0, 0, 0), font=font)
        y += 26
        for k, n in enumerate(NAMES):
            im = Image.open(os.path.join(pngdir, name, n + '.png')).convert('RGBA')
            w, h = round(im.width * sx), round(im.height * sy)
            k2 = min(1.0, (BOX - 4) / w, (BOX - 4) / h)
            im = im.resize((max(1, round(w * k2)), max(1, round(h * k2))),
                           Image.NEAREST if k2 == 1.0 else Image.LANCZOS)
            box = Image.new('RGBA', (BOX, BOX), bg + (255,))
            box.alpha_composite(im, ((BOX - im.width) // 2, (BOX - im.height) // 2))
            page.paste(box.convert('RGB'), (10 + (k % PER_ROW) * (BOX + 6), y + (k // PER_ROW) * (BOX + 6)))
        y += 2 * (BOX + 6) + 4
    page.save(out)


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
