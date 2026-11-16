#!/usr/bin/env python3
# usage: t86.py <machine> <image> <cmdfile> [--times]
# Boots an ELKS image (serial console build, see mkimg.sh) on 86Box through the
# automation socket, runs one command per line, prints the output and, with
# --times, the emulated seconds each command took.
# Needs 86Box built with -DAUTOMATION=ON: env B86 (binary), R86 (roms dir).
import socket, json, subprocess, sys, os, time, tempfile, shutil
MACHINES = {
  # name: (cfg body, floppy type, drive type key)
  'xt':    ('machine = ibmxt86\ncpu_family = 8088\ncpu_speed = 4772728\nmem_size = 640\n', 'cga', '35_2dd'),
  't1200': ('machine = t1200\ncpu_family = 8086\ncpu_speed = 9545456\nmem_size = 1024\n', 'internal', '35_2dd'),
  'pc1512':('machine = pc1512\ncpu_family = 8086\ncpu_speed = 8000000\nmem_size = 640\n', 'internal', '35_2dd'),
  'at':    ('machine = ibmat\ncpu_family = 286\ncpu_speed = 8000000\nmem_size = 512\n', 'ega', '35_2dd'),
}
mach, img, cmdf = sys.argv[1:4]; times = '--times' in sys.argv
B86 = os.environ.get('B86', '/home/claude/86box/86box/build/src/86Box')
R86 = os.environ.get('R86', '/home/claude/86box/roms')
vm = tempfile.mkdtemp(prefix='alci86-'); m, vid, fdt = MACHINES[mach]
shutil.copy(img, vm + '/disk.img')
hd = img.endswith('hd32.img')
cfg = '[Machine]\n' + m + 'cpu_use_dynarec = 0\n\n[Video]\ngfxcard = %s\n\n[Ports (COM & LPT)]\nserial1_device = automation\n\n' % vid
if hd:
    cfg += '[Storage controllers]\nhdc_1 = %s\n\n' % ('xtide_at' if mach == 'at' else 'xtide') + '[Hard disks]\nhdd_01_parameters = 63, 16, 63, 0, ide\nhdd_01_fn = %s/disk.img\nhdd_01_ide_channel = 0:0\n\n' % vm
    cfg += '[Floppy and CD-ROM drives]\nfdd_01_type = %s\n' % fdt
else:
    cfg += '[Floppy and CD-ROM drives]\nfdd_01_type = %s\nfdd_01_fn = %s/disk.img\n' % (fdt, vm)
cfg += 'fdd_02_type = none\n'
open(vm + '/86box.cfg', 'w').write(cfg)
sock = vm + '/a.sock'
env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
p = subprocess.Popen([B86, '-R', R86, '-P', vm, '--automation', 'unix:' + sock, '--automation-hold'],
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env)
for _ in range(100):
    if os.path.exists(sock): break
    time.sleep(0.1)
s = socket.socket(socket.AF_UNIX); s.connect(sock); f = s.makefile('rw')
def q(c):
    f.write(c + '\n'); f.flush(); return json.loads(f.readline())
pos = 0
def wait(text, tmo):
    global pos
    r = q('wait serial %d 1 %d %s' % (tmo, pos, text))
    if not r.get('matched'): return None, r['ms']
    d = q('serial read 1 %d' % pos); data = bytes.fromhex(d['data'])
    i = data.find(text.encode()); out = data[:i]; pos += i + len(text)
    return out.decode('latin1'), r['ms']
def key(code):
    q('key down 0x%x' % code); q('key up 0x%x' % code)
if mach == 'at':
    # IBM AT keeps drive types in CMOS and has no setup in ROM: write them
    cm = {0x0e: 0, 0x0f: 0, 0x10: 0x30, 0x12: 0, 0x14: 0x01, 0x15: 0x00, 0x16: 0x02, 0x17: 0, 0x18: 0}
    for r in range(0x10, 0x2e): cm.setdefault(r, 0)
    cs = sum(cm[r] for r in range(0x10, 0x2e))
    cm[0x2e] = cs >> 8; cm[0x2f] = cs & 0xff
    for r, v in sorted(cm.items()):
        q('iow 0x70 1 0x%x' % r); q('iow 0x71 1 0x%x' % v)
    q('reset hard')
if mach == 't1200':
    # empty configuration RAM: accept the default type at the prompt
    if q('wait text 30000 SYSTEM CONFIGURATION').get('matched'):
        q('step 500'); key(0x1c)
if os.environ.get('MEMR'): print(q('memr %s 16' % os.environ['MEMR']))
for t in os.environ.get('SHOTS', '').split(','):
    if t: q('step %s' % t); q('screenshot /tmp/t86-%s.png' % q('status')['ms'])
out, ms = wait('# ', int(os.environ.get('BOOT_MS', 400000)))
if out is None:
    q('screenshot /tmp/t86-boot.png'); print(json.dumps(q('info'))[:900]); print(q('log').get('text','')[-1500:]); print('BOOT TIMEOUT, screen in /tmp/t86-boot.png'); q('quit'); sys.exit(1)
if times: print('## boot to prompt %.2f s (emulated, includes BIOS POST)' % (ms / 1000))
for n, c in enumerate(l for l in open(cmdf).read().split('\n') if l.strip() and not l.startswith('##')):
    mark = '@@E%d@@' % n; typed = '@@E""%d@@' % n
    line = c + '; echo ' + typed
    if len(line) > 78:
        print('=== %s\nLINE TOO LONG (ELKS tty input is 80 chars)' % c); continue
    t0 = q('status')['ms']
    q('serial write 1 ' + (line + '\r').encode().hex())
    out, ms = wait(mark, 600000)
    if out is None:
        print('=== %s\nTIMEOUT' % c); break
    body = out.replace('\r', '').split('\n', 1)
    body = body[1] if len(body) > 1 else ''
    print('=== ' + c + (' [%.2f s]' % ((ms - t0) / 1000) if times else ''))
    if body.strip('\n'): print(body.strip('\n'))
    wait('# ', 20000)
q('quit')
shutil.rmtree(vm, ignore_errors=True)
