/* The file manager on the desktop: its description and icon (the drawing
   is given by main.c from the Alci icon set). Copyright (c) 2026, the Alci
   contributors. BSD 3-clause licence, the terms of Contiki 1.x. */
#include "dsc.h"

extern struct ctk_icon files_icon;

DSC(files_dsc, "Files", "files.prg", files_init, &files_icon);

struct ctk_icon files_icon = {CTK_ICON("Files", NULL, NULL)};
