/* The editor on the desktop: its description and icon (the drawing is
   given by main.c from the Alci icon set). Copyright (c) 2026, the Alci
   contributors. BSD 3-clause licence, the terms of Contiki 1.x. */
#include "dsc.h"

extern struct ctk_icon edit_icon;

DSC(edit_dsc, "Text editor", "edit.prg", edit_init, &edit_icon);

struct ctk_icon edit_icon = {CTK_ICON("Editor", NULL, NULL)};
