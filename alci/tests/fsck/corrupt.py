import struct, sys
# corrupt.py <img> <inode>: clear the zone-map bit of the inode's first zone
# and set its link count to 5 (MINIX v1)
f, ino = sys.argv[1], int(sys.argv[2])
b = bytearray(open(f, 'rb').read())
ni, nz, im, zm, fz = struct.unpack('<HHHHH', b[1024:1034])
it = (2 + im + zm) * 1024 + (ino - 1) * 32
zone = struct.unpack('<H', b[it + 14:it + 16])[0]
bitno = zone - fz + 1
off = (2 + im) * 1024 + bitno // 8
b[off] &= ~(1 << (bitno % 8)) & 0xff
b[it + 13] = 5
open(f, 'wb').write(b)
print('inode', ino, 'zone', zone, 'bit cleared, nlinks=5')
