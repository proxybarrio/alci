/* File dialog for the CTK desktop on ELKS/Alci (filedlg.c). */
#ifndef FILEDLG_H
#define FILEDLG_H

#include "ek.h"

#define FILEDLG_NAMELEN 40              /* a path, relative to the directory */

/* what filedlg_event() did with an event */
#define FILEDLG_NOTMINE 0               /* not for the dialog (or none open) */
#define FILEDLG_BUSY    1               /* taken, the dialog stays */
#define FILEDLG_OK      2               /* closed: the name is in "name" */
#define FILEDLG_CANCEL  3               /* closed, nothing chosen */

/* title ("Open a file"), the button ("Open", "Save"), the name to start
   with and to give back (FILEDLG_NAMELEN + 1 bytes) */
void filedlg_open(char *title, char *button, char *name);
unsigned char filedlg_event(ek_event_t ev, ek_data_t data);

#endif
