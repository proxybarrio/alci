"""Alci icons for CGA 320x200 four colours (palette 1, high intensity).
Each function draws one 40x32 icon (5x4 text cells). 0 black, 1 cyan, 2 magenta, 3 white."""
from px import Cv
import math
W,H=40,32; B,C,M,Wh=0,1,2,3
def about():
    c=Cv(W,H); o=1
    c.clip=(2,o+2,37,o+26)
    cx,cy=19.5,o+13.5
    c.ell(cx,cy,21.5,16.2,M,tx=1.7,ty=1.4)
    for y in range(o+2,o+27):
        if (y-o-2)%3==2: continue
        for x in range(3,37):
            if c.inell(cx,cy,17.2,12.6,x,y): c.px(x,y,C)
    c.ell(37,o+30,17.5,14.2,B,fill=True); c.ell(37,o+30,16.3,13.2,M,fill=True)
    c.ell(30.5,o+24,4.6,3.6,B,fill=True)
    c.clip=(0,0,W-1,H-1); c.frame(0,o,39,o+28,M,2,2)
    return c
def calculator():
    c=Cv(W,H); c.frame(0,2,39,29,Wh,2,2); c.fill(19,2,20,29,Wh); c.fill(0,15,39,16,Wh)
    c.fill(6,8,13,9,C); c.fill(9,5,10,12,C)
    c.fill(25,8,32,9,C)
    c.pat(6,19,['##.....##','.##...##.','..##.##..','...###...','..##.##..','.##...##.','##.....##'],{'#':M})
    c.fill(25,21,32,22,M); c.fill(28,18,29,19,M); c.fill(28,24,29,25,M)
    return c
from mono import DH
def dhcp():
    c=Cv(W,H)
    for r,sp in enumerate(DH):
        for t in sp.split():
            a,b=map(int,t.split('-'))
            col=C if a<=1 else (M if a>=63 else Wh)
            c.fill(a//2,r+2,b//2,r+2,col)
    return c
def directory():
    c=Cv(W,H)
    c.fill(1,4,12,4,C); c.line(12,4,15,6,C,1); c.fill(15,6,38,6,C)
    c.fill(1,4,2,27,C); c.fill(37,6,38,27,C); c.fill(1,26,38,27,C)
    c.fill(3,9,36,10,M); c.fill(3,9,4,25,M)
    for y in (13,17,21):
        for x in (10,15,20,25,30): c.fill(x,y,x+1,y+1,C)
    return c
def brk(c,y0,y1,col):
    for (x,y,dx,dy) in ((0,y0,1,1),(39,y0,-1,1),(0,y1,1,-1),(39,y1,-1,-1)):
        c.fill(x,y,x+dx*5,y+dy*1,col); c.fill(x,y,x+dx*1,y+dy*4,col)
def editor():
    c=Cv(W,H); brk(c,3,28,M); cx,cy,rx,ry=19.5,15.5,16,12.7
    c.ell(cx,cy,rx,ry,C,tx=2,ty=1.6)
    ok=lambda a,b: c.inell(cx,cy,rx-2.4,ry-1.8,a,b)
    for j,y in enumerate(range(6,26,3)):
        off=0 if j%2==0 else 3
        for x in range(4+off,37,5):
            if ok(x,y) and ok(x+2,y+1): c.fill(x,y,x+2,y+1,C)
            elif ok(x,y) and ok(x+1,y+1): c.fill(x,y,x+1,y+1,C)
    return c
def email():
    c=Cv(W,H); c.frame(0,6,39,25,C,2,2)
    c.line(2,8,16,16,M,3); c.line(37,8,23,16,M,3); c.fill(17,17,22,17,M)
    c.line(2,23,11,16,C,2); c.line(36,23,27,16,C,2)
    return c
def ftp():
    c=Cv(W,H); c.frame(0,2,39,28,Wh,2,2)
    for k,y in enumerate(range(6,13)): c.fill(12-k,y,12+k,y,C)
    c.fill(10,13,14,24,C)
    c.fill(25,6,29,17,M)
    for k,y in enumerate(range(24,17,-1)): c.fill(27-k,y,27+k,y,M)
    return c
def bubble(c,x0,y0,x1,y1,col):
    c.frame(x0,y0,x1,y1,col,2,2)
    for (x,y,dx,dy) in ((x0,y0,1,1),(x1,y0,-1,1),(x0,y1,1,-1),(x1,y1,-1,-1)):
        c.px(x,y,B); c.px(x+dx,y,B); c.px(x,y+dy,B); c.px(x+dx*2,y+dy*2,col)
def irc():
    c=Cv(W,H); bubble(c,1,3,38,20,M)
    c.fill(7,19,12,20,B)
    for k,y in enumerate(range(19,27)):
        c.fill(5,y,6,y,M); x=13-k; c.fill(x,y,x+1,y,M)
    for y in (7,11,15):
        for x in (10,15,20,25,30): c.fill(x,y,x+1,y+1,C)
    return c
def netconf():
    c=Cv(W,H)
    c.frame(13,4,26,9,C,1,1); c.fill(19,10,20,12,C)
    c.fill(5,13,34,14,C)
    for bx in (5,19,33): c.fill(bx,15,bx+1,17,C)
    for bx in (0,14,28):
        c.frame(bx,18,bx+11,25,Wh,3,2); c.fill(bx+3,20,bx+8,23,M)
    return c
def processes():
    c=Cv(W,H)
    c.frame(14,4,38,17,C,1,1)
    c.fill(8,7,32,21,B); c.frame(8,7,32,21,M,1,1); c.fill(8,7,30,7,C)
    c.fill(0,10,27,28,B); c.frame(0,10,27,28,M,2,2)
    for y,l in ((13,9),(16,18),(19,18),(22,9)): c.fill(5,y,5+l-1,y+1,C)
    return c
def shell():
    c=Cv(W,H); c.frame(0,4,39,27,Wh,2,2)
    c.fill(2,6,37,6,M); c.fill(2,25,37,25,C)
    c.pat(5,10,["###.......",".###......","..####....","....###...",".....####.",".......###",".....####.","....###...","..####....",".###......","###......."],{'#':Wh})
    c.fill(18,19,27,20,Wh)
    return c
def mon(c,y0,y1,col):
    c.frame(1,y0,38,y1,col,2,2)
    for (x,y,dx,dy) in ((1,y0,1,1),(38,y0,-1,1),(1,y1,1,-1),(38,y1,-1,-1)):
        c.px(x,y,B); c.px(x+dx*2,y+dy*2,col)
def telnet():
    c=Cv(W,H); mon(c,4,20,Wh)
    c.fill(7,8,26,9,C); c.fill(7,11,21,12,C); c.fill(7,14,11,15,C); c.fill(14,14,27,15,C)
    c.fill(16,21,23,22,C); c.fill(8,23,31,23,Wh); c.fill(8,24,31,25,M)
    return c
def telnetd():
    c=Cv(W,H); mon(c,3,20,C); c.fill(3,18,36,18,Wh)
    c.fill(12,10,27,12,M)
    for i in range(7):
        h=i*0.85
        c.fill(5+i,round(11-h),5+i,round(11+h),M); c.fill(34-i,round(11-h),34-i,round(11+h),M)
    c.fill(16,21,23,23,M); c.fill(9,24,30,25,M); c.px(11,24,Wh); c.px(28,24,Wh)
    return c

def vnc():
    c=Cv(W,H)
    c.frame(14,4,38,19,C,1,1); c.fill(16,6,36,6,M); c.fill(36,6,36,17,M)
    c.fill(0,9,29,28,B); c.frame(0,9,29,28,C,2,2); c.fill(3,12,26,12,Wh); c.fill(3,12,3,25,Wh); c.fill(3,25,26,25,M); c.fill(26,12,26,25,M)
    for y in range(14,24):
        for x in range(7,25):
            r=(y-14)%6; xs=(x-7)%6
            if xs==r or xs==5-r: c.px(x,y,C)
    return c
def www():
    c=Cv(W,H); brk(c,3,29,Wh); cx,cy,rx,ry=19.5,16,15.5,12.6
    c.ell(cx,cy,rx,ry,C,tx=2,ty=1.6)
    for y in (9,22):
        for x in range(40):
            if c.inell(cx,cy,rx-0.5,ry-0.5,x,y): c.px(x,y,C)
    c.ell(cx,cy,7.5,ry,C,tx=1.6,ty=1.1)
    c.fill(19,4,20,28,C)
    for y in (15,16):
        for x in range(40):
            if c.inell(cx,cy,rx-0.3,ry,x,y): c.px(x,y,M)
    return c
def ring(c,cx,cy,rx,ry,angles,dw,dh,col):
    # dots placed by hand-chosen angles from the top, mirrored pixel for pixel
    for a in angles:
        t=math.radians(a); x=round(cx+rx*math.sin(t)-dw/2); y=round(cy-ry*math.cos(t)-dh/2)
        xm=int(2*cx)-x-dw+1
        if a==0:
            w=dw if (dw%2==0)==(cx%1==0.5) else dw+1
            x=int(round(cx-w/2)); c.fill(x,y,x+w-1,y+dh-1,col); continue
        c.fill(x,y,x+dw-1,y+dh-1,col); c.fill(xm,y,xm+dw-1,y+dh-1,col)
WS_OUT=[(0.598,.130),(0.685,.146),(0.761,.197),(0.810,.264),(0.839,.348),(0.868,.442),(0.866,.545),(0.831,.639),(0.787,.728),(0.717,.801)]
WS_IN=[(0.546,.275,0),(0.630,.319,0),(0.679,.382,0),(0.690,.493,1),(0.655,.603,0),(0.608,.656,0)]
def ws_dots(c,sx,ox,sy,oy,cx,ow,oh,iw,ih,tw,th,ocol,icols,OUT=None,IN=None):
    OUT=OUT or WS_OUT; IN=IN or WS_IN
    def put(nx,ny,w,h,col,mirror=True):
        x=ox+nx*sx; y=oy+ny*sy
        d=x-cx; xr=round(cx+abs(d)-w/2); yr=round(y-h/2)
        c.fill(xr,yr,xr+w-1,yr+h-1,col)
        if mirror:
            xm=int(2*cx)-xr-w+1; c.fill(xm,yr,xm+w-1,yr+h-1,col)
    cw=ow if (ow%2==0)==(cx%1==0.5) else ow+1
    xr=int(cx-cw/2+0.5); yr=round(oy+0.129*sy-oh/2); c.fill(xr,yr,xr+cw-1,yr+oh-1,ocol)
    for nx,ny in OUT: put(nx,ny,ow,oh,ocol)
    for k,(nx,ny,tall) in enumerate(IN): put(nx,ny,tw if tall else iw,th if tall else ih,icols[k%len(icols)])
def webserver():
    c=Cv(W,H); c.frame(0,2,39,28,Wh,2,2)
    ws_dots(c,40,0,27,2,19.5,2,2,3,1,2,2,M,[Wh,M,M,Wh],
        OUT=[(0.603,.140),(0.706,.177),(0.785,.244),(0.835,.345),(0.869,.440),(0.872,.556),(0.834,.663),(0.778,.752),(0.706,.825)],
        IN=[(0.564,.288,0),(0.669,.386,1),(0.696,.553,1),(0.625,.645,0)])
    c.fill(18,14,21,16,Wh); c.fill(19,18,20,20,Wh)
    c.frame(17,21,22,23,Wh,2,1); c.fill(15,24,24,25,Wh); c.px(15,26,Wh); c.px(16,26,Wh); c.px(23,26,Wh); c.px(24,26,Wh)
    return c

ICONS=[about,calculator,dhcp,directory,editor,email,ftp,irc,netconf,processes,shell,telnet,telnetd,vnc,www,webserver]
