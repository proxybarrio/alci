#!/usr/bin/env python3
# Kilo save test on QEMU: copies a file of known lines, optionally fills the
# disk, types text into kilo and saves with Ctrl-S, then checks that the file
# still holds its original lines (full disk) or the new text (normal save).
# usage: kilo-save.py <hd32 image with the kilo under test> <kilo command> [full|ok]
import os, select, subprocess, sys, time, re
img, kbin = sys.argv[1], sys.argv[2]
mode = sys.argv[3] if len(sys.argv) > 3 else 'full'
p = subprocess.Popen(['qemu-system-i386','-cpu','486','-m','1','-display','none','-serial','stdio','-snapshot','-no-reboot','-boot','c','-drive','file=%s,format=raw,if=ide'%img], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
buf=[b'']; allout=[b'']; seen=[0]
def rd(t):
    end=time.time()+t
    while time.time()<end:
        if select.select([p.stdout],[],[],0.1)[0]:
            d=os.read(p.stdout.fileno(),4096)
            if not d: return
            buf[0]+=d
            allout[0]+=d
            n=allout[0].count(b'\x1b[6n')
            while seen[0]<n: send('\x1b[25;80R'); seen[0]+=1
def until(tok,t):
    end=time.time()+t
    while time.time()<end:
        j=buf[0].find(tok)
        if j>=0: o=buf[0][:j+len(tok)]; buf[0]=buf[0][j+len(tok):]; return o
        rd(0.3)
    return None
def send(s):
    p.stdin.write(s.encode('latin1')); p.stdin.flush()
def typ(s,d=0.01):
    for c in s: send(c); time.sleep(d)
def sh(c,t=60):
    send(c+'\n'); send('echo do""ne\n'); o=until(b'done\r\n',t); return re.sub(r'\x1b\[[0-9;]*[A-Za-z]','',(o or b'TIMEOUT').decode(errors='replace'))
assert until(b'# ',120)
for c in ['mkdir /k','cd /k','cp /etc/passwd a.c','cat /etc/inittab >> a.c','wc -c a.c','grep -c root a.c']: print(sh(c).strip().split('\n')[-2:])
if mode == 'full': print(sh('dd if=/dev/zero of=fill bs=1024 count=40000 2>&1 | tail -1'))
print(sh('df /'))
send(kbin+' a.c\n'); rd(9); buf[0]=b''
typ('X'*1200 if mode == 'full' else 'NEW '); rd(2)
send('\x13'); rd(8)
t=re.sub(r'\x1b\[[0-9;?]*[A-Za-z]','',buf[0].decode(errors='replace'))
i=max(t.rfind('written'),t.rfind("Can't"))
print('STATUS:', t[max(0,i-3):i+70].replace('\r',' ').replace('\n',' ') if i>0 else '(none)')
send('\x11'); rd(2); send('y'); rd(4); buf[0]=b''
print(sh('wc -c a.c; grep -c root a.c; ls kilo.tmp'))
p.kill()
