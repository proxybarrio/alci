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
  'xtv20': ('machine = ibmxt86\ncpu_family = necv20\ncpu_speed = 4772728\nmem_size = 640\n', 'cga', '35_2dd'),
  '386':   ('machine = ecs386\ncpu_family = i386dx\ncpu_speed = 16000000\nmem_size = 1024\n', 'ega', '35_2dd'),
  'at':    ('machine = ibmat\ncpu_family = 286\ncpu_speed = 8000000\nmem_size = 512\n', 'ega', '35_2dd'),
}
mach, img, cmdf = sys.argv[1:4]; times = '--times' in sys.argv
B86 = os.environ.get('B86', '/home/claude/86box/86box/build/src/86Box')
R86 = os.environ.get('R86', '/home/claude/86box/roms')
vm = tempfile.mkdtemp(prefix='alci86-'); m, vid, fdt = MACHINES[mach]
shutil.copy(img, vm + '/disk.img')
hd = img.endswith('hd32.img') or os.environ.get('HDGEOM') is not None
# HDGEOM='spt,heads,cyls,bus,controller' overrides the hard disk: e.g. the
# T1200's own RLL adapter is '17,8,306,mfm,internal'. EXTRA_HD adds it as a
# second, empty-or-given disk while booting from the floppy image.
hdgeom = os.environ.get('HDGEOM')
cfg = '[Machine]\n' + m + 'cpu_use_dynarec = 0\n\n[Video]\ngfxcard = %s\n\n[Ports (COM & LPT)]\nserial1_device = automation\n\n' % vid
hdfile = vm + '/disk.img'
if os.environ.get('EXTRA_HD'):
    shutil.copy(os.environ['EXTRA_HD'], vm + '/hd.img'); hdfile = vm + '/hd.img'
if hd:
    spt, hpc, cyl, bus, ctl = (hdgeom or '63,16,63,ide,' + ('xtide_at' if mach in ('at', '386') else 'xtide')).split(',')
    cfg += '[Storage controllers]\nhdc_1 = %s\n\n' % ctl
    cfg += '[Hard disks]\nhdd_01_parameters = %s, %s, %s, 0, %s\nhdd_01_fn = %s\n' % (spt, hpc, cyl, bus, hdfile)
    cfg += ('hdd_01_ide_channel = 0:0\n\n' if bus == 'ide' else 'hdd_01_mfm_channel = 0\n\n')
if hd and hdfile == vm + '/disk.img':
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
if mach in ('at', '386'):
    # IBM AT keeps drive types in CMOS and has no setup in ROM: write them
    cm = {0x0e: 0, 0x0f: 0, 0x10: 0x30, 0x12: 0, 0x14: 0x01, 0x15: 0x00, 0x16: 0x02, 0x17: 0, 0x18: 0} if mach == 'at' else \
         {0x0e: 0, 0x0f: 0, 0x10: 0x30, 0x12: 0, 0x14: 0x01, 0x15: 0x80, 0x16: 0x02, 0x17: 0x80, 0x18: 0x01}
    for r in range(0x10, 0x2e): cm.setdefault(r, 0)
    cs = sum(cm[r] for r in range(0x10, 0x2e))
    cm[0x2e] = cs >> 8; cm[0x2f] = cs & 0xff
    for r, v in sorted(cm.items()):
        q('iow 0x70 1 0x%x' % r); q('iow 0x71 1 0x%x' % v)
    q('reset hard')
    if mach == '386':
        # AMI BIOS: extended CMOS left unset, continue with F1 as a user would
        if q('wait text 30000 to RESUME').get('matched'):
            q('step 300'); key(0x3b)
            if q('wait text 10000 EXIT FOR BOOT').get('matched'):
                q('step 300'); key(0x1c)
if mach == 't1200':
    # empty configuration RAM: accept the default type at the prompt
    if q('wait text 30000 SYSTEM CONFIGURATION').get('matched'):
        q('step 500'); key(0x1c)
if os.environ.get('MEMR'): print(q('memr %s 16' % os.environ['MEMR']))
for t in os.environ.get('SHOTS', '').split(','):
    if t: q('step %s' % t); q('screenshot /tmp/t86-%s.png' % q('status')['ms'])
SCAN = {**{c: k for c, k in zip('1234567890-=', range(2, 14))},
        **{c: k for c, k in zip('qwertyuiop', range(0x10, 0x1a))},
        **{c: k for c, k in zip('asdfghjkl;', range(0x1e, 0x28))},
        **{c: k for c, k in zip('zxcvbnm,./', range(0x2c, 0x36))},
        ' ': 0x39, "'": 0x28, '\\': 0x2b, '\n': 0x1c}
SHIFTED = {'!': '1', '@': '2', '#': '3', '$': '4', '%': '5', '&': '7', '*': '8', '(': '9', ')': '0',
           '_': '-', '+': '=', ':': ';', '"': "'", '<': ',', '>': '.', '?': '/', '|': '\\'}
# the AT keyboard controller loses keys sent 2 ms apart: give it time
keydelay = int(os.environ.get('KEYDELAY', 20 if mach in ('at', '386') else 0))
def typeline(text):
    for ch in text:
        sh = ch in SHIFTED or ch.isupper()
        base = SHIFTED.get(ch, ch.lower())
        if sh: q('key down 0x2a')
        q('key down 0x%x' % SCAN[base]); q('key up 0x%x' % SCAN[base])
        if sh: q('key up 0x2a')
        if keydelay: q('step %d' % keydelay)
if os.environ.get('SCREEN'):
    # console on the machine's own screen: type on the keyboard, read the text screen
    r = q('wait text %d # ' % int(os.environ.get('BOOT_MS', 400000)))
    if not r.get('matched'):
        q('screenshot /tmp/t86-boot.png'); print('BOOT TIMEOUT, screen in /tmp/t86-boot.png'); q('quit'); sys.exit(1)
    if times: print('## boot to prompt %.2f s (emulated, includes BIOS POST)' % (r['ms'] / 1000))
    q('step 1000')
    for n, c in enumerate(l for l in open(cmdf).read().split('\n') if l.strip() and not l.startswith('##')):
        typeline(c + '; echo @@S""%d@@' % n)
        q('step 200')
        t0 = q('status')['ms']
        typeline('\n')
        r = q('wait text %d @@S%d@@' % (int(os.environ.get('CMD_MS', 600000)), n))
        if not r.get('matched'): q('screenshot /tmp/t86-timeout.png')
        print('=== ' + c + (' [%.2f s]' % ((r['ms'] - t0) / 1000) if times else '') + ('' if r.get('matched') else ' TIMEOUT'))
        q('step 500')
    q('quit'); shutil.rmtree(vm, ignore_errors=True); sys.exit(0)
out, ms = wait('# ', int(os.environ.get('BOOT_MS', 400000)))
if out is None:
    q('screenshot /tmp/t86-boot.png'); print(json.dumps(q('info'))[:900]); print(q('log').get('text','')[-1500:]); print('BOOT TIMEOUT, screen in /tmp/t86-boot.png'); q('quit'); sys.exit(1)
if os.environ.get('BOOTLOG'):
    for l in out.replace('\r', '').split('\n'):
        if any(k in l for k in os.environ['BOOTLOG'].split(',')): print('## boot: ' + l)
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
