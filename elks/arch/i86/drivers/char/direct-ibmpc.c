/*
 * direct console video output for IBM PC.
 */

static void SetDisplayPage(Console * C)
{
    outw((C->crtc_offset & 0xff00) | 0x0c, C->crtc_base);
    outw((C->crtc_offset << 8) | 0x0d, C->crtc_base);
    /* publish the display start (bytes) in the BIOS data area, as the BIOS does,
     * for programs that read video memory directly */
    pokew(0x4E, 0x40, C->crtc_offset << 1);
}

static void PositionCursor(Console * C)
{
    unsigned int Pos = C->cx + C->Width * C->cy + C->crtc_offset;

    if (C->cursoroff)           /* hide by moving just past end of page */
        Pos = C->Width * C->Height + C->crtc_offset;
    outb(14, C->crtc_base);
    outb(Pos >> 8, C->crtc_base + 1);
    outb(15, C->crtc_base);
    outb(Pos, C->crtc_base + 1);
}

static void DisplayCursor(Console * C, int onoff)
{
    /* Hide cursor by moving it offscreen rather than changing its shape.
     * The correct shape can't be known on all adapters (BDA 0x0460 is
     * emulated on EGA/VGA and wrong on the Compaq Portable), so the
     * shape set by the BIOS at boot is left untouched.
     */
    C->cursoroff = !onoff;
    PositionCursor(C);
}

static void VideoWrite(Console * C, int c)
{
    /* direct far store: avoids a pokew() call per character */
    *(unsigned short __far *)_MK_FP(C->vseg,
        ((C->cx + C->cy * C->Width) << 1) + C->vseg_offset) = (C->attr << 8) | (c & 255);
}

static void ClearRange(Console * C, int x, int y, int x2, int y2)
{
    int vp;

    x2 = x2 - x + 1;
    vp = ((x + y * C->Width) << 1) + C->vseg_offset;
    if (x2 == C->Width) {       /* whole rows: one fill for the block */
        fmemsetw((void *)vp, C->vseg, (C->attr << 8) | ' ', x2 * (y2 - y + 1));
        return;
    }
    do {
        fmemsetw((void *)vp, C->vseg, (C->attr << 8) | ' ', x2);
        vp += C->Width << 1;
    } while (++y <= y2);
}

static void ScrollUp(Console * C, int y)
{
    int vp;
    int MaxRow = C->Height - 1;
    int MaxCol = C->Width - 1;

#ifndef CONFIG_CONSOLE_AMSTRAD_IGA
    /* Hardware scroll: move the display start down one line within this VC's
     * share of video RAM. At the end of the share, copy the screen back to the
     * start once ("rebase") instead of copying it on every line.
     */
    if (y == 0 && C->crtc_limit) {
        unsigned int next = C->crtc_offset + C->Width;
        if (next + C->Width * C->Height > C->crtc_limit) {
            fmemcpyw((void *)(C->crtc_home << 1), C->vseg,
                     (void *)(next << 1), C->vseg, MaxRow * C->Width);
            next = C->crtc_home;
        }
        C->crtc_offset = next;
        C->vseg_offset = next << 1;
        ClearRange(C, 0, MaxRow, MaxCol, MaxRow);
        if (C == Visible[C->display])
            SetDisplayPage(C);
        return;
    }
#endif
    vp = (y * (C->Width << 1)) + C->vseg_offset;
    if ((unsigned int)y < MaxRow)
        fmemcpyw((void *)vp, C->vseg,
                 (void *)(vp + (C->Width << 1)), C->vseg, (MaxRow - y) * C->Width);
    ClearRange(C, 0, MaxRow, MaxCol, MaxRow);
}

#ifdef CONFIG_EMUL_ANSI
static void ScrollDown(Console * C, int y)
{
    int vp;
    int yy = C->Height - 1;

    vp = (yy * (C->Width << 1)) + C->vseg_offset;
    while (--yy >= y) {
        fmemcpyw((void *)vp, C->vseg, (void *)(vp - (C->Width << 1)), C->vseg, C->Width);
        vp -= C->Width << 1;
    }
    ClearRange(C, 0, y, C->Width - 1, y);
}
#endif
