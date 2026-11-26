/* The terminal on the desktop: its description and icon (the drawing is
   given by main.c from the Alci icon set). Copyright (c) 2026, the Alci
   contributors. BSD 3-clause licence, the terms of Contiki 1.x. */
#include "dsc.h"

extern struct ctk_icon term_icon;

DSC(term_dsc, "Shell", "term.prg", term_init, &term_icon);

struct ctk_icon term_icon = {CTK_ICON("Shell", NULL, NULL)};
