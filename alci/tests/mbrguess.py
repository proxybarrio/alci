#!/usr/bin/env python3
# usage: mbrguess.py <image> [sectors]: QEMU's geometry guess from the partition
# table bytes of sector 0 (hw/block/hd-geometry.c). On a MINIX image with no
# MBR those bytes are boot loader code: no entry may decode to a geometry.
import sys,struct
# QEMU hd-geometry guess_disk_lchs on sector 0 of an image
b=open(sys.argv[1],'rb').read(512); n=int(sys.argv[2]) if len(sys.argv)>2 else 63504
ok=True
for i in range(4):
    e=b[0x1be+16*i:0x1be+16*i+16]
    nr=struct.unpack('<I',e[12:16])[0]; eh=e[5]; es=e[6]&63
    if nr and eh and es:
        c=n//((eh+1)*es)
        if 1<=c<=16383 and eh+1<=16: print('entry',i+1,'GUESSED heads',eh+1,'sectors',es,'cyl',c); ok=False; continue
    print('entry',i+1,'rejected')
print('OK' if ok else 'BAD')
