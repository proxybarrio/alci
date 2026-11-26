/*
 * Serial and PS/2 mouse for the CTK desktop on ELKS/Alci (task D7).
 *
 * The protocol parsers are the ELKS user mode mouse driver
 * (elkscmd/gui/mouse.c), without its messages and test program, since the
 * desktop is in graphics mode when the mouse is read.
 *
 * Mouse driver from Microwindows
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>

#include "mouse.h"

#define MOUSE_PORT  "/dev/ttyS0"    /* default port unless MOUSE_PORT= env specified */
#define MOUSE_QEMU  "/dev/ttyS1"    /* default port on QEMU */
#define MOUSE_PS2   "/dev/psaux"    /* default port for PS/2 mouse */
#define MOUSE_TYPE          "ms"    /* default mouse is microsoft mouse */

/* states for the mouse */
#define IDLE                0       /* start of byte sequence */
#define XSET                1       /* setting x delta */
#define YSET                2       /* setting y delta */
#define XADD                3       /* adjusting x delta */
#define YADD                4       /* adjusting y delta */

/* pc and ms button codes*/
#define PC_LEFT_BUTTON      4
#define PC_MIDDLE_BUTTON    2
#define PC_RIGHT_BUTTON     1
#define MS_LEFT_BUTTON      2
#define MS_RIGHT_BUTTON     1

/* ps/2 button codes */
#define PS2_LEFT_BUTTON     1
#define PS2_RIGHT_BUTTON    2

/* Bit fields in the bytes sent by the mouse */
#define TOP_FIVE_BITS       0xf8
#define BOTTOM_THREE_BITS   0x07
#define TOP_BIT             0x80
#define SIXTH_BIT           0x40
#define BOTTOM_TWO_BITS     0x03
#define THIRD_FOURTH_BITS   0x0c
#define BOTTOM_SIX_BITS     0x3f
#define PS2_CTRL_BYTE       0x08

int     mouse_fd = -1;      /* file descriptor for mouse */

static int      state;      /* IDLE, XSET, ... */
static int      buttons;    /* current mouse buttons pressed*/
static int      xd;         /* change in x */
static int      yd;         /* change in y */

static int      left;       /* because the button values change */
static int      middle;     /* between mice, the buttons are */
static int      right;      /* redefined */

static unsigned char    *bp;/* buffer pointer */
static int      nbytes;     /* number of bytes left */
static int      (*parse)(int); /* parse routine */
static int      parsePC(int);       /* routine to interpret PC mouse */
static int      parseMS(int);       /* routine to interpret MS mouse */
static int      parsePS2(int);      /* routine to interpret PS/2 mouse */

/*
 * NOTE: max_bytes can't be larger than 1 mouse read packet or select() can fail,
 * as mouse driver would be storing unprocessed data not seen by select.
 */
#define PC_MAX_BYTES   5
#define MS_MAX_BYTES   3
#define PS2_MAX_BYTES  3
#define MAX_BYTES      5
static int max_bytes = MS_MAX_BYTES;

/*
 * Open up the mouse device: MOUSE_TYPE ms (default), pc or ps2, or none;
 * MOUSE_PORT for the serial port. Returns the fd, or negative.
 */
int open_mouse(void)
{
    char *port, *type;
    struct termios termios;
    struct stat st, tst;

    if( !(type = getenv("MOUSE_TYPE")))
        type = MOUSE_TYPE;

    if(!strcmp(type, "pc")) {
        left = PC_LEFT_BUTTON;
        middle = PC_MIDDLE_BUTTON;
        right = PC_RIGHT_BUTTON;
        parse = parsePC;
        max_bytes = PC_MAX_BYTES;
    } else if (strcmp(type, "ms") == 0) {
        left = MS_LEFT_BUTTON;
        right = MS_RIGHT_BUTTON;
        middle = 0;
        parse = parseMS;
        max_bytes = MS_MAX_BYTES;
    } else if (strcmp(type, "ps2") == 0) {
        left = PS2_LEFT_BUTTON;
        right = PS2_RIGHT_BUTTON;
        middle = 0;
        parse = parsePS2;
        max_bytes = PS2_MAX_BYTES;
    } else {
        return -1;                      /* "none", or unknown */
    }

    if (parse == parsePS2)
        port = MOUSE_PS2;
    else if (!(port = getenv("MOUSE_PORT")))
        port = getenv("QEMU")? MOUSE_QEMU: MOUSE_PORT;
    mouse_fd = open(port, O_RDWR | O_EXCL | O_NOCTTY | O_NONBLOCK);
    if (mouse_fd < 0)
        return -1;

    /* not a port this program talks on (a serial console): leave it as it is */
    if (fstat(mouse_fd, &st) == 0) {
        int fd;
        for (fd = 0; fd < 3; fd++) {
            if (fstat(fd, &tst) == 0 && tst.st_rdev == st.st_rdev) {
                close(mouse_fd);
                mouse_fd = -1;
                return -1;
            }
        }
    }

    if (tcgetattr(mouse_fd, &termios) < 0) {
        close(mouse_fd);
        mouse_fd = -1;
        return -1;
    }
    if(cfgetispeed(&termios) != B1200)
        cfsetispeed(&termios, (speed_t)B1200);
    termios.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    termios.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT | IGNBRK);
    termios.c_cflag &= ~(CSIZE | PARENB);
    termios.c_cflag |= CREAD | CS8;
    termios.c_cc[VMIN] = 0;
    termios.c_cc[VTIME] = 0;
    if(tcsetattr(mouse_fd, TCSAFLUSH, &termios) < 0) {
        close(mouse_fd);
        mouse_fd = -1;
        return -1;
    }

    state = IDLE;
    nbytes = 0;
    buttons = 0;
    xd = 0;
    yd = 0;
    return mouse_fd;
}

void close_mouse(void)
{
    if (mouse_fd >= 0)
        close(mouse_fd);
    mouse_fd = -1;
}

/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns -1 on error, 0 if either no bytes were read or not enough
 * was read for a complete state, or 1 if the new state was read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 */
int read_mouse(int *dx, int *dy, int *dz, int *bptr)
{
    int b;
    static unsigned char buffer[MAX_BYTES];

    if (nbytes <= 0) {
        bp = buffer;
        nbytes = read(mouse_fd, bp, max_bytes);
        if (nbytes < 0) {
            if (errno == EINTR || errno == EAGAIN)
                return 0;
            return -1;
        }
    }
    while (nbytes-- > 0) {
        if ((*parse)((int) *bp++)) {
            *dx = xd;
            *dy = yd;
            *dz = 0;
            b = 0;
            if(buttons & left)
                b |= BUTTON_L;
            if(buttons & right)
                b |= BUTTON_R;
            if(buttons & middle)
                b |= BUTTON_M;
            *bptr = b;
            return 1;
        }
    }
    return 0;
}

/*
 * Input routine for PC mouse.
 * Returns nonzero when a new mouse state has been completed.
 */
static int parsePC(int byte)
{
    int sign;           /* sign of movement */

    switch (state) {
        case IDLE:
            if ((byte & TOP_FIVE_BITS) == TOP_BIT) {
                buttons = ~byte & BOTTOM_THREE_BITS;
                state = XSET;
            }
            break;

        case XSET:
            sign = 1;
            if (byte > 127) {
                byte = 256 - byte;
                sign = -1;
            }
            xd = byte * sign;
            state = YSET;
            break;

        case YSET:
            sign = 1;
            if (byte > 127) {
                byte = 256 - byte;
                sign = -1;
            }
            yd = -byte * sign;
            state = XADD;
            break;

        case XADD:
            sign = 1;
            if (byte > 127) {
                byte = 256 - byte;
                sign = -1;
            }
            xd += byte * sign;
            state = YADD;
            break;

        case YADD:
            sign = 1;
            if (byte > 127) {
                byte = 256 - byte;
                sign = -1;
            }
            yd -= byte * sign;
            state = IDLE;
            return 1;
    }
    return 0;
}

/*
 * Input routine for Microsoft mouse.
 * Returns nonzero when a new mouse state has been completed.
 */
static int parseMS(int byte)
{
    /* only the first byte of a packet has the sixth bit: one seen in the
       middle starts a new packet (the mouse's "M" when the port opens) */
    if (byte & SIXTH_BIT)
        state = IDLE;
    switch (state) {
        case IDLE:
            if (byte & SIXTH_BIT) {
                buttons = (byte >> 4) & BOTTOM_TWO_BITS;
                yd = ((byte & THIRD_FOURTH_BITS) << 4);
                xd = ((byte & BOTTOM_TWO_BITS) << 6);
                state = XADD;
            }
            break;

        case XADD:
            xd |= (byte & BOTTOM_SIX_BITS);
            state = YADD;
            break;

        case YADD:
            yd |= (byte & BOTTOM_SIX_BITS);
            state = IDLE;
            if (xd > 127)
                xd -= 256;
            if (yd > 127)
                yd -= 256;
            return 1;
    }
    return 0;
}

/*
 * Input routine for PS/2 mouse.
 * Returns nonzero when a new mouse state has been completed.
 */
static int parsePS2(int byte)
{
    switch (state) {
    case IDLE:
        if (byte & PS2_CTRL_BYTE) {
            buttons = byte & (PS2_LEFT_BUTTON|PS2_RIGHT_BUTTON);
            state = XSET;
        }
        break;

    case XSET:
        if(byte > 127)
                byte -= 256;
        xd = byte;
        state = YSET;
        break;

    case YSET:
        if(byte > 127)
                byte -= 256;
        yd = -byte;
        state = IDLE;
        return 1;
    }
    return 0;
}
