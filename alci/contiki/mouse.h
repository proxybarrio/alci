/* Serial and PS/2 mouse (mouse.c, from the ELKS user mode mouse driver). */
#ifndef MOUSE_H
#define MOUSE_H

#define BUTTON_L            0x01    /* left button*/
#define BUTTON_R            0x02    /* right button*/
#define BUTTON_M            0x10    /* middle*/

extern int mouse_fd;
int open_mouse(void);
int read_mouse(int *dx, int *dy, int *dz, int *bptr);
void close_mouse(void);

#endif
