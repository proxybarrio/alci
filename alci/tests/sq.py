#!/usr/bin/env python3
# Strict variant of alci/tests/qtest.py (the original is left untouched).
# Same QEMU command line, same typing, same end marker; in addition it
#  - checks that the echo of each command line comes back intact (!NOECHO),
#  - checks that the shell prompt returns after each command (!NOPROMPT),
#  - reports every command not run as SKIPPED instead of stopping silently,
#  - saves the raw serial transcript (<out>.raw) and exits 1 on any problem.
# usage: sq.py <image> <fd|hd> <cpu> <mem_kb> <cmdfile> <rawfile>
import subprocess, sys, time, select, os
img, kind, cpu, mem, cmdf, rawf = sys.argv[1:7]
drive = ['-drive', f'file={img},format=raw,if=floppy' if kind == 'fd' else f'file={img},format=raw,if=ide']
mem_mb = max(1, int(mem) // 1024)
p = subprocess.Popen(['qemu-system-i386', '-cpu', cpu, '-m', str(mem_mb), '-display', 'none', '-serial', 'stdio',
     '-monitor', 'none', '-boot', 'a' if kind == 'fd' else 'c', '-snapshot', '-no-reboot'] + drive,
     stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
raw = open(rawf, 'wb'); buf = b''; dead = False
def readuntil(tok, tmo):
    global buf, dead
    end = time.time() + tmo
    while True:
        i = buf.find(tok)
        if i >= 0:
            out = buf[:i]; buf = buf[i + len(tok):]; return out
        if dead or time.time() >= end: return None
        r, _, _ = select.select([p.stdout], [], [], 0.2)
        if r:
            d = os.read(p.stdout.fileno(), 4096)
            if not d: dead = True; continue
            raw.write(d); raw.flush(); buf += d
bad = 0
if readuntil(b'# ', 120) is None:
    print('BOOT TIMEOUT'); p.kill(); sys.exit(1)
cmds = [l for l in open(cmdf).read().split('\n') if l.strip() and not l.startswith('##')]
for n, c in enumerate(cmds):
    mark = f'@@E{n}@@'; line = c + '; echo ' + mark
    if len(c) + len(mark) + 7 > 79:
        print(f'=== {c}\nLINE TOO LONG (ELKS tty input is 80 chars)'); bad += 1; continue
    p.stdin.write((line + '\n').encode()); p.stdin.flush()
    flags = ''
    # the echo of the typed line must come back whole before we look for the
    # marker, or the marker would be found inside the echo itself
    if readuntil(line.encode(), 10) is None:
        flags += ' !NOECHO'
        print(f'=== {c}{flags}'); bad += 1; break
    out = readuntil(mark.encode(), 120)
    if out is None:
        print(f'=== {c}\nTIMEOUT'); bad += 1; break
    if readuntil(b'# ', 10) is None:
        flags += ' !NOPROMPT'; bad += 1
    txt = out.decode('latin1').replace('\r', '').strip('\n')
    print(f'=== {c}{flags}\n{txt}')
    if flags: break
else:
    n = len(cmds)
for c in cmds[n + 1:]:
    print(f'## SKIPPED: {c}')
p.kill(); raw.close()
sys.exit(1 if bad else 0)
