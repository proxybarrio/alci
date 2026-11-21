#!/usr/bin/env python3
# Damaged-program test for exec of plain (uncompressed) a.out programs.
# Builds damaged copies of a small-model program and of a medium-model
# program with relocations, puts them in a copy of an image as /bin/d1 ...,
# and starts each in a fresh boot on QEMU or 86Box. For each it reports how
# the program ended, whether the system still answers, and whether the
# intact program still gives its normal output afterwards.
# usage: exec-corrupt.py <image> <small prog> <medium prog> <workdir> [qemu|86box:<machine>] [case,...]
# Needs an image built by mkimg.sh (serial console, init=/bin/sh).
import os, re, select, socket, struct, subprocess, sys, time

img, small, medium, work = sys.argv[1:5]
emu = sys.argv[5] if len(sys.argv) > 5 else 'qemu'
only = sys.argv[6].split(',') if len(sys.argv) > 6 else None
here = os.path.dirname(os.path.abspath(__file__))
mfs = os.path.join(here, '../../elks/tools/bin/mfs')
os.makedirs(work, exist_ok=True)
S = bytearray(open(small, 'rb').read())
M = bytearray(open(medium, 'rb').read())
assert S[4] == 32 and M[4] == 64

def u32(b, o, v): b[o:o + 4] = struct.pack('<I', v & 0xffffffff)
def g32(b, o): return struct.unpack('<I', b[o:o + 4])[0]
def g16(b, o): return struct.unpack('<H', b[o:o + 2])[0]

# medium-model layout: header 64, text, far text, data, text relocs, far text relocs, data relocs
mt, md = g32(M, 8), g32(M, 12)
mtr, mdr, mft, mftr = g32(M, 32), g32(M, 36), g32(M, 48), g32(M, 52)
R_T = 64 + mt + mft + md          # first text relocation
R_F = R_T + mtr                   # first far text relocation
R_D = R_F + mftr                  # first data relocation
st, sd = g32(S, 8), g32(S, 12)

def case_small(f):
    b = bytearray(S); f(b); return b
def case_med(f):
    b = bytearray(M); f(b); return b
def relocs_at(start, n, step):
    def f(b):
        for i in range(n): u32(b, start + 8 * i, step * (i + 1))
    return f

CASES = [
    ('s-trunc-text', 'small, cut in the middle of the text', case_small(lambda b: b.__delitem__(slice(32 + st // 2, None)))),
    ('s-trunc-data', 'small, last 40 bytes of data missing', case_small(lambda b: b.__delitem__(slice(len(b) - 40, None)))),
    ('s-tseg-high', 'small, text size +64 KB (high word)', case_small(lambda b: u32(b, 8, st + 0x10000))),
    ('s-dseg-big', 'small, data size larger than the file', case_small(lambda b: u32(b, 12, sd + 0x4000))),
    ('s-bss-huge', 'small, bss 0xFFF0', case_small(lambda b: u32(b, 16, 0xfff0))),
    ('s-entry-out', 'small, entry point past the text', case_small(lambda b: u32(b, 20, st + 0x200))),
    ('s-entry-ffff', 'small, entry point 0xFFFE', case_small(lambda b: u32(b, 20, 0xfffe))),
    ('s-hlen-48', 'small, header length 48 (no such header)', case_small(lambda b: b.__setitem__(4, 48))),
    ('s-text-noise', 'small, valid header, 200 text bytes random', case_small(lambda b: b.__setitem__(slice(40, 240), os.urandom(200)))),
    ('m-intact', 'medium, intact (control)', case_med(lambda b: None)),
    ('m-trel-out', 'medium, text relocations point past the text', case_med(relocs_at(R_T, mtr // 8, 0x1000))),
    ('m-frel-out', 'medium, far text relocations point past the far text', case_med(relocs_at(R_F, mftr // 8, 0x1000))),
    ('m-drel-out', 'medium, data relocations point past the data', case_med(relocs_at(R_D, mdr // 8, 0x7f00))),
    ('m-rel-type', 'medium, bad relocation type', case_med(lambda b: b.__setitem__(slice(R_T + 6, R_T + 8), b'\x01\x00'))),
    ('m-rel-size', 'medium, relocation size not a multiple of 8', case_med(lambda b: u32(b, 32, mtr + 3))),
    ('m-rel-trunc', 'medium, relocations cut short', case_med(lambda b: b.__delitem__(slice(len(b) - 20, None)))),
    ('m-ftseg-high', 'medium, far text size +64 KB (high word)', case_med(lambda b: u32(b, 48, mft + 0x10000))),
]
if only: CASES = [c for c in CASES if c[0] in only]

test = os.path.join(work, 'test.img')
subprocess.run(['cp', img, test], check=True)
for f, src in (('s0', S), ('m0', M)):
    p = os.path.join(work, f); open(p, 'wb').write(src); os.chmod(p, 0o755)
    subprocess.run([mfs, test, 'cp', p, '/bin/' + f], capture_output=True, check=True)
for i, (name, _, data) in enumerate(CASES, 1):
    p = os.path.join(work, 'd%d' % i); open(p, 'wb').write(data); os.chmod(p, 0o755)
    subprocess.run([mfs, test, 'cp', p, '/bin/d%d' % i], capture_output=True, check=True)

pw = subprocess.run([mfs, img, 'cat', '/etc/passwd'], capture_output=True).stdout.decode(errors='replace')
NROOT = sum(1 for l in pw.split('\n') if 'root' in l)
def cmd_for(name, prog): return ('%s -c root /etc/passwd' % prog) if name.startswith('s') else prog
CHECK = 's0 -c root /etc/passwd; m0'

def run_qemu(i, name):
    sock = os.path.join(work, 'qm.sock')
    hd = test.endswith('hd32.img') or 'hd' in os.path.basename(img)
    drive = 'file=%s,format=raw,if=%s' % (test, 'ide' if hd else 'floppy')
    p = subprocess.Popen(['qemu-system-i386', '-cpu', '486', '-m', '1', '-display', 'none',
        '-serial', 'stdio', '-monitor', 'unix:%s,server,nowait' % sock, '-boot', 'c' if hd else 'a',
        '-snapshot', '-no-reboot', '-drive', drive],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    buf = [b'']
    def until(tok, tmo):
        end = time.time() + tmo
        while time.time() < end:
            j = buf[0].find(tok)
            if j >= 0:
                o = buf[0][:j + len(tok)]; buf[0] = buf[0][j + len(tok):]; return o
            if select.select([p.stdout], [], [], 0.2)[0]:
                d = os.read(p.stdout.fileno(), 4096)
                if not d: break
                buf[0] += d
        return None
    def send(s): p.stdin.write(s.encode()); p.stdin.flush()
    def clean(b): return ' | '.join(l.strip() for l in b.decode(errors='replace').split('\n') if l.strip() and 'rc=' not in l[:3])[-150:]
    if until(b'# ', 120) is None:
        p.kill(); return 'NO BOOT'
    send('%s; echo rc=$?\n' % cmd_for(name, 'd%d' % i))
    o = until(b'\nrc=', 25)
    if o is not None:
        res = 'ended: ' + clean(o) + ' rc=' + (until(b'\n', 5) or b'').decode(errors='replace').strip()
    else:
        send('\x03'); o = until(b'\nrc=', 10)
        res = 'program hung'
    send('echo al""ive; %s; echo do""ne\n' % CHECK)
    o = until(b'done', 30)
    if o is None:
        m = socket.socket(socket.AF_UNIX); m.connect(sock); time.sleep(0.3); m.recv(4096)
        m.send(b'info registers\n'); time.sleep(0.5); r = m.recv(65536).decode(errors='replace'); m.close()
        cs = re.search(r'CS =(\w+)', r); ip = re.search(r'EIP=(\w+)', r)
        p.kill()
        return res + ' | after: SYSTEM DEAD, Ctrl-C does not help' + (' (CS:IP %s:%s)' % (cs.group(1)[-4:], ip.group(1)[-4:]) if cs and ip else '')
    p.kill()
    t = o.decode(errors='replace')
    ok = 'alive' in t and 'rel ok' in t and re.search(r'\n%d\r?\n' % NROOT, t) is not None
    return res + ' | after: ' + ('system OK, intact programs OK' if ok else 'system answers, intact programs WRONG: ' + clean(o))

def run_86box(i, name, mach):
    suite = os.path.join(work, 'suite%d.txt' % i)
    open(suite, 'w').write('%s; echo rc=$?\necho al""ive\n%s\n' % (cmd_for(name, 'd%d' % i), CHECK))
    env = dict(os.environ, CMD_MS=os.environ.get('CMD_MS', '60000'), INTR='1')
    if 'hd' in os.path.basename(img): env.setdefault('HDGEOM', '63,16,63,ide,' + ('xtide_at' if mach in ('at', '386') else 'xtide'))
    r = subprocess.run([sys.executable, os.path.join(here, 'st86.py'), mach, test, suite], capture_output=True, text=True, env=env)
    out = r.stdout
    parts = out.split('=== ')
    first = parts[1] if len(parts) > 1 else out
    if 'no answer to Ctrl-C' in first:
        return 'program hung | after: SYSTEM DEAD, Ctrl-C does not help'
    if 'Ctrl-C helped' in first:
        res = 'program hung'
    else:
        res = 'ended: ' + ' | '.join(l.strip() for l in first.split('\n')[1:] if l.strip())[-150:]
    rest = '=== '.join(parts[2:])
    if 'TIMEOUT' in rest or len(parts) < 4:
        return res + ' | after: SYSTEM DEAD'
    ok = 'alive' in rest and 'rel ok' in rest and re.search(r'\n%d\n' % NROOT, rest) is not None
    return res + ' | after: ' + ('system OK, intact programs OK' if ok else 'intact programs WRONG')

for i, (name, desc, _) in enumerate(CASES, 1):
    r = run_qemu(i, name) if emu == 'qemu' else run_86box(i, name, emu.split(':')[1])
    print('%-13s %-52s %s' % (name, desc, r), flush=True)
