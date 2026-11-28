#!/usr/bin/env python3
# ftfiles.py <trace> <image> [t0]: floppy reads of an FDCTRACE log, as files of a MINIX image
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__))); from minixmap import load
owner,_=load(sys.argv[2]); t0=float(sys.argv[3]) if len(sys.argv)>3 else 0
L=[l.split() for l in open(sys.argv[1])]
lba=lambda c,h,s:(c*2+h)*9+s-1
cur=None; prev_end=None; tot=0
for i,l in enumerate(L):
    if l[1]=='CMD' and l[2] in('e6','66','c6','46','e5','c5'):
        p=[int(x,16) for x in l[3:]]; cur=(float(l[0]),lba(p[1],p[2],p[3]),p[1])
        # time since previous INT (seek/settle included)
        seek=None
        for m in reversed(L[:i]):
            if m[1]=='CMD' and m[2]=='0f': seek=float(m[0]); break
    elif l[1]=='INT' and cur:
        t=float(l[0]); c,h,s=int(l[5]),int(l[6]),int(l[7]); e=lba(c,h,s)
        if cur[0]>=t0:
            blocks=range(cur[1]//2,(e-1)//2+1)
            files=[]
            for b in blocks:
                o=owner.get(b,'-')
                if not files or files[-1][0]!=o: files.append([o,b,b])
                else: files[-1][2]=b
            st=seek if seek and seek<cur[0] else cur[0]
            print(f'{st:8.0f} cyl{cur[2]:3d} {cur[1]:5d}-{e-1:5d} {t-st:5.0f}ms  '+', '.join(f'{f[0]}[{f[1]}-{f[2]}]' for f in files))
            tot+=t-st
        cur=None
print('total ms in reads (incl. seek/settle):',round(tot))
