#!/usr/bin/env python3
# minixmap.py <image>: block -> file map of a MINIX v1 image (1K blocks)
import struct,sys
def load(img):
    d=open(img,'rb').read()
    B=lambda n: d[n*1024:(n+1)*1024]
    sb=B(1); ninodes,nzones,imap,zmap,firstdata,logz,maxsize,magic=struct.unpack('<HHHHHHIH',sb[:18])
    itab=2+imap+zmap
    def inode(n):
        off=itab*1024+(n-1)*32; b=d[off:off+32]
        mode,uid,size,mtime,gid,nl=struct.unpack('<HHIIBB',b[:14]); z=struct.unpack('<9H',b[14:32])
        return mode,size,z
    owner={}
    for b in range(itab,itab+(ninodes*32+1023)//1024): owner[b]='<inodes>'
    for b in range(2,itab): owner[b]='<maps>'
    owner[0]='<boot>'; owner[1]='<super>'
    def zones(z):
        out=list(z[:7]); 
        if z[7]:
            out.append(('ind',z[7])); out+= list(struct.unpack('<512H',B(z[7])))
        if z[8]:
            out.append(('dind',z[8]))
            for zz in struct.unpack('<512H',B(z[8])):
                if zz: out.append(('ind',zz)); out+=list(struct.unpack('<512H',B(zz)))
        return out
    def walk(ino,path):
        mode,size,z=inode(ino)
        nb=(size+1023)//1024; zl=zones(z); k=0
        for e in zl:
            if isinstance(e,tuple): owner[e[1]]=path+' [ind]'; continue
            if k>=nb: break
            if e: owner[e]=path
            k+=1
        if mode&0o170000==0o040000:
            data=b''.join(B(e) for e in [x for x in zl if not isinstance(x,tuple)][:nb] if e)[:size]
            for i in range(0,len(data),16):
                n,=struct.unpack('<H',data[i:i+2]); name=data[i+2:i+16].split(b'\0')[0].decode('latin1')
                if n and name not in('.','..'): walk(n,(path.rstrip('/')+'/'+name))
    walk(1,'/')
    return owner,nzones
if __name__=='__main__':
    owner,nz=load(sys.argv[1])
    prev=None;start=0
    for b in range(nz+1):
        o=owner.get(b,'-')
        if o!=prev:
            if prev is not None: print(f'{start:5d}-{b-1:5d} {prev}')
            prev=o;start=b
    print(f'{start:5d}-{nz:5d} {prev}')
