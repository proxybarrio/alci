#!/usr/bin/env python3
"""elks-lz4: compress the text, far text and data of ELKS a.out executables
with LZ4 (ALCI), for in-place decoding by the kernel (fs/lz4dec.S).

usage: elks-lz4.py [-v] [-n] file...

Each section is packed with LZ4 HC (needs the Python 'lz4' module) and
kept only if it is smaller and can be decoded in place, and a file is
rewritten only if it then takes at least one 1K block less: the kernel reads the
compressed bytes starting dlen + margin - clen bytes above the output, with
margin = (clen >> 8) + 32, and the decoder must never write over bytes it has
not read yet. The packer simulates the decode and refuses a section that
would. The header is rewritten in the extended (64-byte) form with
esh_reserved = 0x344C ("L4"). Files already compressed, or with a header
this tool does not know, are left alone. -n: report only, write nothing.
"""
import struct, sys
import lz4.block

MINIX_SPLITID = 0x04200301
MINIX_SPLITID_AHISTORICAL = 0x04300301
ESH_COMPR_LZ4 = 0x344C
HDR = '<IBBHIIIIHHI'         # struct minix_exec_hdr, 32 bytes
SUPL = '<IIIIIIHHHH'         # struct elks_supl_hdr, 32 bytes


def margin(clen):
    return (clen >> 8) + 32


def inplace_ok(comp, dlen):
    """True if a forward decode with the source at dlen + margin - clen
    (rounded up to a paragraph) above the output never overwrites unread
    input."""
    need = dlen + margin(len(comp)) - len(comp)
    s = max(0, need)
    s = (s + 15) & ~15
    r = w = 0
    n = len(comp)
    while r < n:
        tok = comp[r]; r += 1
        ll = tok >> 4
        if ll == 15:
            while True:
                b = comp[r]; r += 1; ll += b
                if b != 255:
                    break
        r += ll; w += ll
        if w > s + r:               # literal bytes are read before written
            return False
        if r >= n:
            break
        off = comp[r] | (comp[r + 1] << 8); r += 2
        ml = tok & 15
        if ml == 15:
            while True:
                b = comp[r]; r += 1; ml += b
                if b != 255:
                    break
        ml += 4
        if off == 0 or off > w:
            return False
        w += ml
        if w > s + r:               # match bytes must not reach unread input
            return False
    return w == dlen


def pack(data):
    if not data:
        return None
    c = lz4.block.compress(bytes(data), mode='high_compression',
                           compression=12, store_size=False)
    if len(c) >= len(data) or len(c) >= 65520:
        return None
    if lz4.block.decompress(c, uncompressed_size=len(data)) != bytes(data):
        return None
    if not inplace_ok(c, len(data)):
        return None
    return c


def do_file(path, verbose, dry):
    b = open(path, 'rb').read()
    if len(b) < 32:
        return 'too short'
    h = list(struct.unpack(HDR, b[:32]))
    typ, hlen = h[0], h[1]
    if typ not in (MINIX_SPLITID, MINIX_SPLITID_AHISTORICAL) or h[4] == 0:
        return 'not a split I/D a.out'
    if hlen == 32:
        supl = [0] * 10
    elif hlen == 48:
        supl = list(struct.unpack('<IIII', b[32:48])) + [0] * 6
    elif hlen == 64:
        supl = list(struct.unpack(SUPL, b[32:64]))
    else:
        return 'header size %d not supported' % hlen
    if supl[6] or supl[7] or supl[8]:
        return 'already compressed'
    tseg, dseg, ftseg = h[4], h[5], supl[4]
    p = hlen
    text = b[p:p + tseg]; p += tseg
    ftext = b[p:p + ftseg]; p += ftseg
    data = b[p:p + dseg]; p += dseg
    rest = b[p:]
    ct, cf, cd = pack(text), pack(ftext), pack(data)
    if not (ct or cf or cd):
        return 'no gain'
    supl[6] = len(ct) if ct else 0
    supl[7] = len(cd) if cd else 0
    supl[8] = len(cf) if cf else 0
    supl[9] = ESH_COMPR_LZ4
    h[1] = 64
    out = struct.pack(HDR, *h) + struct.pack(SUPL, *supl) + \
        (ct or text) + (cf or ftext) + (cd or data) + rest
    blk = lambda n: (n + 1023) // 1024
    if blk(len(out)) >= blk(len(b)):
        return 'saves no 1K block'      # decoding would cost time for nothing
    if verbose:
        print('%s: text %d->%d ftext %d->%d data %d->%d, file %d->%d' % (
            path, tseg, supl[6] or tseg, ftseg, supl[8] or ftseg, dseg,
            supl[7] or dseg, len(b), len(out)))
    if not dry:
        open(path, 'wb').write(out)
    return None


def main():
    args = sys.argv[1:]
    verbose = '-v' in args
    dry = '-n' in args
    files = [a for a in args if not a.startswith('-')]
    for f in files:
        why = do_file(f, verbose, dry)
        if why and verbose:
            print('%s: left as is (%s)' % (f, why))


if __name__ == '__main__':
    main()
