"""Alci icons for CGA 640x200 (two colours, pixels 2.4 times taller than wide).
Each function draws one 80x32 icon (10x4 text cells). 1 = ink, 0 = paper."""
import math
from px import Cv
W,H=80,32; K=1; Wt=0
def about():
    c=Cv(W,H); x0,y0,x1,y1=4,1,75,30
    c.clip=(x0+4,y0+2,x1-4,y1-2)
    cx,cy=39.5,15.5
    c.ell(cx,cy,42,17.6,K,tx=3,ty=1.2)
    for y in (6,8,11,13,16,18,21,23,25):
        for x in range(8,72):
            if c.inell(cx,cy,26.5,12.4,x,y): c.px(x,y,K)
    c.ell(cx,cy,31,13.4,K,tx=2.5,ty=1.0)
    c.ell(73,32.5,37,16.8,Wt,fill=True)
    c.ell(73,32.5,34.5,15.6,K,fill=True)
    c.ell(60.5,25,7.5,3.2,Wt,fill=True)
    c.clip=(0,0,W-1,H-1); c.frame(x0,y0,x1,y1,K,4,2)
    return c
DH=["1-17 24-52 64-67 74-78","1-17 24-52 64-67 74-78","1-4 22-51","1-13 20-48","1-5 19-47 75-78","1-4 18-47 75-78",
"1-12 16-47","1-5 16-46","1-4 16-42","1-8 13-42 46-60 75-78","1-4 13-16 28-42 47-60 75-78","1-4 13-15 28-40 50-56",
"1-4 10-19 25-39 51-55","1-4 10-39","1-4 10-39","10-37","6-35","3-35","3-17 19-50","19-50","1-11 16-46","1-4 16-46",
"1-10 17-57","1-12 25-57","1-4 21-52","1-13 21-54","1-13 22-54","1-4 24-57 75-78","1-18 26-41 45-63 67-69"]
def dhcp():
    c=Cv(W,H)
    for r,sp in enumerate(DH):
        for t in sp.split():
            a,b=map(int,t.split('-')); c.fill(a,r+1,b,r+1,K)
    return c

def calculator():
    c=Cv(W,H); x0,y0,x1,y1=0,1,79,30
    c.frame(x0,y0,x1,y1,K,4,2); c.fill(38,y0,41,y1,K); c.fill(x0,15,x1,16,K)
    # plus
    c.fill(12,8,29,9,K); c.fill(19,5,22,12,K)
    # minus
    c.fill(50,8,67,9,K)
    # times
    c.line(13,19,27,26,K,3); c.line(27,19,13,26,K,3)
    # divide
    c.fill(50,22,67,23,K); c.fill(57,19,60,20,K); c.fill(57,25,60,26,K)
    return c

def directory():
    c=Cv(W,H); t=3
    c.fill(2,t,26,t,K); c.line(26,t,31,t+2,K,2); c.fill(31,t+2,77,t+2,K)
    c.fill(2,t,4,27,K); c.fill(75,t+2,77,27,K); c.fill(2,26,77,27,K)
    c.fill(2,8,77,9,K)
    for y in (13,17,21):
        for i in range(5):
            x=19+i*10; c.fill(x,y,x+3,y,K)
    return c
def email():
    c=Cv(W,H); c.frame(0,5,79,25,K,4,2)
    c.line(4,7,32,15,K,2); c.line(75,7,47,15,K,2)
    c.fill(33,16,46,16,K)
    c.line(4,23,25,14,K,2); c.line(75,23,54,14,K,2)
    return c
def ftp():
    c=Cv(W,H); c.frame(1,2,78,28,K,4,2)
    for k,y in enumerate(range(6,13)): c.fill(26-1-k*2,y,26+k*2,y,K)
    c.fill(21,13,30,24,K)
    c.fill(49,6,58,17,K)
    for k,y in enumerate(range(24,17,-1)): c.fill(53-1-k*2,y,53+k*2,y,K)
    return c
def bubble(c,x0,y0,x1,y1):
    c.frame(x0,y0,x1,y1,K,3,1)
    for (x,y,dx,dy) in ((x0,y0,1,1),(x1,y0,-1,1),(x0,y1,1,-1),(x1,y1,-1,-1)):
        c.fill(x,y,x+dx*2,y,0); c.px(x,y+dy,0)
        c.fill(x+dx*2,y+dy,x+dx*4,y+dy,K); c.fill(x+dx*1,y+dy*2,x+dx*1,y+dy*2,K)
def irc():
    c=Cv(W,H); bubble(c,2,2,77,20)
    c.fill(11,20,21,20,0); c.fill(7,19,9,27,K); c.line(9,27,21,20,K,3)
    for y in (6,10,14):
        for i in range(5):
            x=19+i*10; c.fill(x,y,x+3,y,K)
    return c

def netconf():
    c=Cv(W,H)
    c.frame(30,4,49,9,K,4,2); c.fill(38,10,41,12,K)
    c.fill(10,13,69,14,K)
    for bx in (10,38,66): c.fill(bx,15,bx+3,17,K)
    for bx in (2,30,58): c.frame(bx,18,bx+19,24,K,4,2)
    return c
def processes():
    c=Cv(W,H)
    c.frame(26,3,77,20,K,3,1)
    c.fill(14,6,65,23,0); c.frame(14,6,65,23,K,3,1)
    c.fill(2,9,53,27,0); c.frame(2,9,53,27,K,3,2)
    for y,l in ((13,26),(16,40),(19,40),(22,26)):
        c.fill(9,y,9+l-1,y,K)
    return c
def shell():
    c=Cv(W,H); c.frame(1,4,78,26,K,4,2); c.fill(8,7,71,23,K)
    for r in range(10,21):
        k=5-abs(r-15); x=14+k*2; c.fill(x,r,x+6,r,0)
    c.fill(29,19,45,20,0)
    return c

def monitor(c,y0,y1,yb):
    c.frame(3,y0,76,y1,K,3,1)
    for (x,y,dx,dy) in ((3,y0,1,1),(76,y0,-1,1),(3,y1,1,-1),(76,y1,-1,-1)):
        c.fill(x,y,x+dx*1,y,0); c.px(x,y+dy,0); c.px(x+dx*3,y+dy,K); c.px(x+dx*1,y+dy*1,K) ; c.px(x+dx*2,y+dy*1,K)
    c.fill(9,y0+2,70,y1-2,K)
    c.fill(34,y1+1,45,yb-1,K); c.frame(14,yb,65,yb+3,K,3,1)
def telnet():
    c=Cv(W,H); monitor(c,3,21,24)
    c.fill(16,8,51,8,0); c.fill(16,12,41,12,0); c.fill(16,16,30,16,0); c.fill(36,16,57,16,0)
    return c

def telnetd():
    c=Cv(W,H); monitor(c,3,22,25)
    c.fill(27,11,52,14,0)
    for i in range(5):
        c.fill(17+i*2,12-i,18+i*2,13+i,0); c.fill(61-i*2,12-i,62-i*2,13+i,0)
    return c
def vnc():
    c=Cv(W,H)
    c.frame(24,3,77,20,K,3,1)
    c.fill(2,8,61,27,0); c.frame(2,8,61,27,K,4,2); c.frame(9,11,55,24,K,2,1)
    t=['##....##','..#..#..','...##...','..#..#..']
    for y in range(13,23):
        for x in range(19,53):
            if t[(y-13)%4][(x-19)%8]=='#': c.px(x,y,K)
    return c
def brk(c,y0,y1):
    for (x,y,dx,dy) in ((0,y0,1,1),(79,y0,-1,1),(0,y1,1,-1),(79,y1,-1,-1)):
        c.fill(x,y,x+dx*10,y+dy*1,K); c.fill(x,y,x+dx*3,y+dy*4,K)
def www():
    c=Cv(W,H); cx,cy,rx,ry=39.5,15.5,31,12.8
    brk(c,2,29)
    c.ell(cx,cy,rx,ry,K,tx=4,ty=1.2)
    for y in (8,9,15,16,22,23):
        for x in range(80):
            if c.inell(cx,cy,rx-1,ry-0.5,x,y): c.px(x,y,K)
    c.fill(38,3,41,28,K)
    c.ell(cx,cy,15,ry,K,tx=4,ty=1.2)
    return c
def editor():
    c=Cv(W,H); brk(c,2,29); cx,cy,rx,ry=39.5,15.5,31,12.8
    c.ell(cx,cy,rx,ry,K,tx=4,ty=1.2)
    ok=lambda a,b: c.inell(cx,cy,rx-5,ry-1.3,a,b)
    for j,y in enumerate(range(5,27,2)):
        off=0 if j%2==0 else 4
        for x in range(8+off,74,8):
            if ok(x,y) and ok(x+3,y): c.fill(x,y,x+3,y,K)
            elif ok(x+1,y) and ok(x+2,y): c.fill(x+1,y,x+2,y,K)
    return c
def ring(c,cx,cy,rx,ry,angles,dw,dh,col):
    for a in angles:
        t=math.radians(a); x=round(cx+rx*math.sin(t)-dw/2); y=round(cy-ry*math.cos(t)-dh/2)
        xm=int(2*cx)-x-dw+1
        if a==0:
            w=dw if (dw%2==0)==(cx%1==0.5) else dw+1
            x=int(round(cx-w/2)); c.fill(x,y,x+w-1,y+dh-1,col); continue
        c.fill(x,y,x+dw-1,y+dh-1,col); c.fill(xm,y,xm+dw-1,y+dh-1,col)
WS_OUT=[(0.598,.130),(0.685,.146),(0.761,.197),(0.810,.264),(0.839,.348),(0.868,.442),(0.866,.545),(0.831,.639),(0.787,.728),(0.717,.801)]
WS_IN=[(0.546,.275,0),(0.630,.319,0),(0.679,.382,0),(0.690,.493,1),(0.655,.603,0),(0.608,.656,0)]
def ws_dots(c,sx,ox,sy,oy,cx,ow,oh,iw,ih,tw,th,ocol,icols):
    def put(nx,ny,w,h,col,mirror=True):
        x=ox+nx*sx; y=oy+ny*sy
        d=x-cx; xr=round(cx+abs(d)-w/2); yr=round(y-h/2)
        c.fill(xr,yr,xr+w-1,yr+h-1,col)
        if mirror:
            xm=int(2*cx)-xr-w+1; c.fill(xm,yr,xm+w-1,yr+h-1,col)
    cw=ow if (ow%2==0)==(cx%1==0.5) else ow+1
    xr=int(cx-cw/2+0.5); yr=round(oy+0.129*sy-oh/2); c.fill(xr,yr,xr+cw-1,yr+oh-1,ocol)
    for nx,ny in WS_OUT: put(nx,ny,ow,oh,ocol)
    for k,(nx,ny,tall) in enumerate(WS_IN): put(nx,ny,tw if tall else iw,th if tall else ih,icols[k%len(icols)])
def webserver():
    c=Cv(W,H); c.frame(2,2,77,28,K,4,2)
    ws_dots(c,76,2,27,2,39.5,3,1,4,1,3,2,K,[K])
    c.fill(36,14,43,16,K); c.fill(38,18,41,20,K)
    c.frame(34,21,45,23,K,4,1); c.fill(30,24,49,25,K); c.fill(30,26,33,26,K); c.fill(46,26,49,26,K)
    return c

ICONS=[about,calculator,dhcp,directory,editor,email,ftp,irc,netconf,processes,shell,telnet,telnetd,vnc,www,webserver]
