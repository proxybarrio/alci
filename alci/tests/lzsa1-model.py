#!/usr/bin/env python3
# Host model of elks/fs/lzsa1dec.S decoding a program's text in place, as
# exec does (source at the end of the same segment, 16-bit offsets that
# wrap). Corrupts the compressed text of an LZSA1 executable N times in
# five ways (a tail of zeros is what an interrupted write can leave) and counts what the kernel would do: accept a block of the
# right length, reject one of the wrong length, or write outside the
# segment exec allocated (memory of the kernel or of other programs).
# usage: lzsa1-model.py <LZSA1 executable taken from an image> [N]
import random, struct, sys, collections

def ceil16(x): return (x + 15) & ~15

def decode(stream, dlen, maxout=0x10000):
    clen = len(stream); margin = (clen >> 8) + 32
    sp = ceil16(dlen + margin - clen)       # source offset in the segment
    alloc = sp + ceil16(clen)               # bytes exec allocated
    mem = bytearray(0x10000 + sp + 0x10000)
    mem[sp:sp + clen] = stream
    st = {'si': 0, 'di': 0, 'out': False, 'n': 0}
    def rb():
        b = mem[sp + st['si']]; st['si'] = (st['si'] + 1) & 0xffff; return b
    def rw():
        lo = rb(); return lo | rb() << 8
    def wr(v):
        mem[st['di']] = v
        if st['di'] >= alloc: st['out'] = True
        st['di'] = (st['di'] + 1) & 0xffff
        st['n'] += 1
    while st['n'] <= maxout:
        t = rb()
        n = (t & 0x70) >> 4
        if n == 7:
            a = rb() + 7
            n = a if a < 256 else rw() if a == 256 else 256 + rb()
        for _ in range(n):
            wr(rb())
        off = rw() if t & 0x80 else 0xff00 | rb()
        n = t & 0x0f
        if n != 15:
            n += 3
        else:
            a = rb() + 0x12
            if a < 256: n = a
            elif a == 256:
                n = rw()
                if n == 0: return st['di'], st['out'], False
            else: n = 256 + rb()
        src = (st['di'] + off) & 0xffff
        for _ in range(n):
            wr(mem[src]); src = (src + 1) & 0xffff
    return st['di'], True, True             # 64 KB written, no end marker yet

d = open(sys.argv[1], 'rb').read()
N = int(sys.argv[2]) if len(sys.argv) > 2 else 300
hlen = d[4]; tseg = struct.unpack('<I', d[8:12])[0]
ctseg, cdseg, cftseg, fmt = struct.unpack('<4H', d[56:64])
assert fmt == 0x3153 and ctseg, 'not an LZSA1 executable'
base = d[hlen:hlen + ctseg]
assert decode(base, tseg) == (tseg, False, False), 'model does not decode the original'

def mutate(rr, kind):
    b = bytearray(base); L = len(b)
    if kind == 'bit flip':
        i = rr.randrange(L); b[i] ^= 1 << rr.randrange(8)
    elif kind == 'random byte':
        b[rr.randrange(L)] = rr.randrange(256)
    elif kind == '16 random bytes':
        i = rr.randrange(L - 16)
        for k in range(16): b[i + k] = rr.randrange(256)
    elif kind == 'tail garbage':
        i = rr.randrange(L); b[i:] = bytes(rr.randrange(256) for _ in range(L - i))
    elif kind == 'tail zeros':
        i = rr.randrange(L); b[i:] = bytes(L - i)
    return bytes(b)

print('%s: text %d bytes, %d compressed' % (sys.argv[1], tseg, ctseg))
for kind in ['bit flip', 'random byte', '16 random bytes', 'tail garbage', 'tail zeros']:
    rr = random.Random(kind); c = collections.Counter()
    for _ in range(N):
        di, out, runaway = decode(mutate(rr, kind), tseg)
        c['accepted' if di == tseg and not runaway else 'rejected'] += 1
        c['wrote outside the segment'] += out
        c['64 KB written, no end'] += runaway
    print('%-16s %d: %s' % (kind, N, ', '.join('%s %d' % kv for kv in sorted(c.items()))))
