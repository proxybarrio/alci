/* File dialog for the CTK desktop on ELKS/Alci (filedlg.c). */
#ifndef FILEDLG_H
#define FILEDLG_H

#include "ek.h"

#define FILEDLG_NAMELEN 40              /* a name typed in the dialog */
#define FILEDLG_PATHLEN 79              /* the full path given back */

/* what filedlg_event() did with an event */
#define FILEDLG_NOTMINE 0               /* not for the dialog (or none open) */
#define FILEDLG_BUSY    1               /* taken, the dialog stays */
#define FILEDLG_OK      2               /* closed: the name is in "name" */
#define FILEDLG_CANCEL  3               /* closed, nothing chosen */

/* title ("Open a file"), the button ("Open", "Save"), the path to start
   with (its directory is shown, its name is in the entry) and to give back
   as a full path (FILEDLG_PATHLEN + 1 bytes): the dialog changes the
   desktop's directory, so a name alone would not stay right */
void filedlg_open(char *title, char *button, char *name);
unsigned char filedlg_event(ek_event_t ev, ek_data_t data);

#endif
