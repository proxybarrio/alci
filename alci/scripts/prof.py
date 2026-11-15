import os
import sys, bisect, collections
# usage: cyc2.py profile usersyms   (profile lines: flat cs count)
sysb=open(os.environ.get('ALCI_ROOT','/home/claude')+'/elks/elks/arch/i86/boot/system','rb').read()
textsz=int.from_bytes(sysb[8:12],'little'); tseg=0x330; fseg=int(__import__("os").environ.get("FSEG","116e"),16)
ks=[]
for l in open(os.environ.get('ALCI_ROOT','/home/claude')+'/elks/elks/arch/i86/boot/system.map'):
    p=l.split()
    if len(p)==3 and p[1] in 'Tt': ks.append((int(p[0],16),p[2]))
ks.sort(); kt=[(a-0x10000,n) for a,n in ks if a<0x20000]; kf=[(a-0x20000,n) for a,n in ks if a>=0x20000]
us=sorted((int(a,16)-0x10000,n) for a,n in (l.split() for l in open(sys.argv[2])))
def look(tab,off):
    i=bisect.bisect_right([x[0] for x in tab],off)-1; return tab[i][1] if i>=0 else '?'
h=collections.Counter(); reg=collections.Counter(); tot=0
for l in open(sys.argv[1]):
    a,cs,c=l.split(); a=int(a,16); cs=int(cs,16); c=int(c); tot+=c; ip=a-cs*16
    if cs==tseg: reg['kernel']+=c; h['K:'+look(kt,ip)]+=c
    elif cs==fseg: reg['kernel']+=c; h['K:'+look(kf,ip)]+=c
    elif a>=0xF0000: reg['BIOS']+=c; h['BIOS']+=c
    else: reg['user']+=c; h['U:'+look(us,ip)]+=c
print('samples',tot)
for k,v in reg.most_common(): print('  %-8s %5.1f%%'%(k,100*v/tot))
for k,v in h.most_common(int(sys.argv[3]) if len(sys.argv)>3 else 20): print('  %-26s %5.1f%%'%(k,100*v/tot))
