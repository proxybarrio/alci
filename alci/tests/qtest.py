#!/usr/bin/env python3
# usage: qtest.py <image> <fd|hd> <cpu> <mem_kb> <cmdfile>  -- boots ELKS in QEMU on serial console, runs commands, prints output
import subprocess, sys, time, select, os
img, kind, cpu, mem, cmdf = sys.argv[1:6]
drive = ['-drive', f'file={img},format=raw,if=floppy' if kind=='fd' else f'file={img},format=raw,if=ide']
boot = 'a' if kind=='fd' else 'c'
mem_mb = max(1, int(mem)//1024)
p = subprocess.Popen(['qemu-system-i386','-cpu',cpu,'-m',str(mem_mb),'-display','none','-serial','stdio',
     '-monitor','none','-boot',boot,'-snapshot','-no-reboot']+drive, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
buf=b''
def readuntil(tok, tmo):
    global buf
    end=time.time()+tmo
    while time.time()<end:
        r,_,_=select.select([p.stdout],[],[],0.2)
        if r:
            d=os.read(p.stdout.fileno(),4096)
            if not d: break
            buf+=d
            i=buf.find(tok)
            if i>=0:
                out=buf[:i]; buf=buf[i+len(tok):]; return out
    return None
if readuntil(b'# ', 120) is None: print('BOOT TIMEOUT'); print(buf.decode('latin1')[-800:]); p.kill(); sys.exit(1)
for n,c in enumerate(l for l in open(cmdf).read().split('\n') if l.strip() and not l.startswith('##')):
    mark=f'@@E{n}@@'
    if len(c)+len(mark)+7 > 79: print(f'=== {c}\nLINE TOO LONG (ELKS tty input is 80 chars)'); continue
    p.stdin.write((c+'; echo '+mark+'\n').encode()); p.stdin.flush()
    readuntil((mark+'\r').encode() if False else (c+'; echo '+mark).encode(), 10)  # skip echo of the command line
    out=readuntil(mark.encode(), 120)
    if out is None: print(f'=== {c}\nTIMEOUT'); break
    txt=out.decode('latin1').replace('\r','').strip('\n')
    print(f'=== {c}\n{txt}')
    readuntil(b'# ', 10)
p.kill()
