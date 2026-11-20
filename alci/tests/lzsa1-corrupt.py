#!/usr/bin/env python3
# Damaged-program test for the LZSA1 decoder (tasks F4b review, F4c).
# Takes an LZSA1 program from a 720K image (default /bin/grep), writes
# twelve damaged copies /bin/c1 ... /bin/c12 into a copy of the image and
# starts each one in a fresh QEMU boot. For each it reports whether the
# program exited (and how), hung but Ctrl-C helped, or the whole system
# hung (with QEMU's CS:IP), and whether the system still answers after.
# usage: lzsa1-corrupt.py <fd720 image> [program] [workdir]
# Needs the image built by mkimg.sh (serial console, init=/bin/sh).
import os, random, re, select, socket, struct, subprocess, sys, time

img = sys.argv[1]
prog = sys.argv[2] if len(sys.argv) > 2 else '/bin/grep'
work = sys.argv[3] if len(sys.argv) > 3 else 'lzsa1-corrupt'
mfs = os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../elks/tools/bin/mfs')
os.makedirs(work, exist_ok=True)
base = subprocess.run([mfs, img, 'cat', prog], capture_output=True).stdout
hlen = base[4]
ct, cd = struct.unpack('<HH', base[56:60])
assert struct.unpack('<H', base[62:64])[0] == 0x3153, 'not an LZSA1 program'
T0, D0 = hlen, hlen + ct

def flip(o): return lambda b: b.__setitem__(o, b[o] ^ 0xff)
def rnd(seed, o, n):
    def f(b):
        r = random.Random(seed)
        for i in range(n): b[o + i] = r.randrange(256)
    return f
def fill(o, n, v):
    def f(b):
        for i in range(n): b[o + i] = v
    return f
damage = [flip(T0 + 3000 % ct), flip(T0 + 100), rnd(3, T0 + 2000 % ct, 64),
          fill(T0 + 4000 % ct, 256, 0), fill(T0 + 5000 % ct, 32, 0xff), flip(D0 + 100 % cd),
          rnd(7, T0 + 1000 % ct, 512), fill(T0 + ct - 3, 3, 0xff), rnd(9, T0 + 10, 200),
          rnd(10, T0 + 6000 % ct, min(1000, ct - 6000 % ct)), fill(T0 + 200, 8, 0xff),
          rnd(12, D0 + 10, min(300, cd - 10))]
test = os.path.join(work, 'test.img')
subprocess.run(['cp', img, test], check=True)
for i, f in enumerate(damage, 1):
    b = bytearray(base); f(b)
    p = os.path.join(work, 'c%d' % i); open(p, 'wb').write(b); os.chmod(p, 0o755)
    subprocess.run([mfs, test, 'cp', p, '/bin/c%d' % i], capture_output=True, check=True)

def run(name):
    sock = os.path.join(work, 'qm.sock')
    p = subprocess.Popen(['qemu-system-i386', '-cpu', '486', '-m', '1', '-display', 'none',
        '-serial', 'stdio', '-monitor', 'unix:%s,server,nowait' % sock, '-boot', 'a',
        '-snapshot', '-no-reboot', '-drive', 'file=%s,format=raw,if=floppy' % test],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    buf = [b'']
    def until(tok, tmo):
        end = time.time() + tmo
        while time.time() < end:
            i = buf[0].find(tok)
            if i >= 0:
                o = buf[0][:i + len(tok)]; buf[0] = buf[0][i + len(tok):]; return o
            if select.select([p.stdout], [], [], 0.2)[0]:
                d = os.read(p.stdout.fileno(), 4096)
                if not d: break
                buf[0] += d
        return None
    def send(s): p.stdin.write(s.encode()); p.stdin.flush()
    def clean(b): return ' | '.join(l.strip() for l in b.decode(errors='replace').split('\n') if l.strip())[-160:]
    until(b'# ', 120)
    send('%s -c root /etc/passwd; echo rc=$?\n' % name)
    o = until(b'\nrc=', 25)
    if o is not None:
        res = 'exited: ' + clean(o + (until(b'\n', 5) or b''))
    else:
        send('\x03'); o = until(b'\nrc=', 10)
        res = ('hung, Ctrl-C helped: ' + clean(o)) if o is not None else 'whole system hung'
    send('echo al""ive\n')
    alive = until(b'alive', 15) is not None
    if not alive:
        m = socket.socket(socket.AF_UNIX); m.connect(sock); time.sleep(0.3); m.recv(4096)
        m.send(b'info registers\n'); time.sleep(0.5); r = m.recv(65536).decode(errors='replace'); m.close()
        cs = re.search(r'CS =(\w+)', r); ip = re.search(r'EIP=(\w+)', r)
        if cs and ip: res += ' (CS:IP %s:%s)' % (cs.group(1)[-4:], ip.group(1)[-4:])
    p.kill()
    return res + ('; system answers' if alive else '; system dead')

for i in range(1, len(damage) + 1):
    print('c%d: %s' % (i, run('c%d' % i)), flush=True)
