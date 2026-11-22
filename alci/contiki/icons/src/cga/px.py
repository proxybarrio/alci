"""Small pixel canvas used to draw the Alci icons by hand, one pixel at a time."""
import numpy as np
from PIL import Image
class Cv:
    def __init__(s,w,h,bg=0): s.a=np.full((h,w),bg,np.int8); s.w=w; s.h=h; s.clip=(0,0,w-1,h-1)
    def px(s,x,y,c):
        x0,y0,x1,y1=s.clip
        if x0<=x<=x1 and y0<=y<=y1: s.a[y,x]=c
    def fill(s,x0,y0,x1,y1,c):
        x0,x1=sorted((int(x0),int(x1))); y0,y1=sorted((int(y0),int(y1)))
        cx0,cy0,cx1,cy1=s.clip
        x0=max(x0,cx0); y0=max(y0,cy0); x1=min(x1,cx1); y1=min(y1,cy1)
        if x1>=x0 and y1>=y0: s.a[y0:y1+1,x0:x1+1]=c
    def frame(s,x0,y0,x1,y1,c,tx=1,ty=1):
        s.fill(x0,y0,x1,y0+ty-1,c); s.fill(x0,y1-ty+1,x1,y1,c); s.fill(x0,y0,x0+tx-1,y1,c); s.fill(x1-tx+1,y0,x1,y1,c)
    def line(s,x0,y0,x1,y1,c,w=1):
        # row-spans so shallow diagonals stay continuous; w = horizontal thickness
        if y0>y1: x0,y0,x1,y1=x1,y1,x0,y0
        n=y1-y0
        for y in range(y0,y1+1):
            if n==0: xa,xb=x0,x1
            else:
                t0=max(0,(y-y0-0.5)/n); t1=min(1,(y-y0+0.5)/n)
                xa=x0+(x1-x0)*t0; xb=x0+(x1-x0)*t1
            lo=int(round(min(xa,xb))); hi=int(round(max(xa,xb)))
            if hi>lo and n>0: hi-=1 if abs(x1-x0)>n else 0
            s.fill(lo,y,hi+w-1,y,c)
    def ell(s,cx,cy,rx,ry,c,fill=False,tx=1,ty=1,mask=None):
        for y in range(s.h):
            for x in range(s.w):
                dx=(x+0.5-cx); dy=(y+0.5-cy)
                o=(dx/rx)**2+(dy/ry)**2<=1
                if not o: continue
                if fill: inside=False
                else:
                    ri=rx-tx; rj=ry-ty
                    inside= ri>0 and rj>0 and (dx/ri)**2+(dy/rj)**2<=1
                if not inside and (mask is None or mask(x,y)): s.px(x,y,c)
    def inell(s,cx,cy,rx,ry,x,y): return ((x+0.5-cx)/rx)**2+((y+0.5-cy)/ry)**2<=1
    def pat(s,x0,y0,rows,cmap):
        for j,r in enumerate(rows):
            for i,ch in enumerate(r):
                if ch in cmap: s.px(x0+i,y0+j,cmap[ch])
    def img(s,pal): return Image.fromarray(np.array(pal,np.uint8)[s.a])
    def ascii(s,chars='#.'): return '\n'.join(''.join(chars[v] for v in r) for r in s.a)
