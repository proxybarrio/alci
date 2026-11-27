/*
 * The Amstrad PC1512/1640 mouse driver of Gem-elks left out, for the D2
 * benchmark build only (alci/tests/guibench): with the CGA font fix
 * (gem-elks-cga-font.patch) the server's code passes the 128 KB of the
 * medium model by 32 bytes, and the benchmark runs without a mouse.
 * Copyright (c) 2026 the Alci contributors; GPL-2.0 like Gem-elks.
 */
#include "drivers/gem_mouse_amstrad.h"

void gem_mouse_amstrad_reset(void) { }
GEM_VDI_WORD gem_mouse_amstrad_poll(GEM_VDI_COORD *delta_x,
	GEM_VDI_COORD *delta_y) { *delta_x = 0; *delta_y = 0; return 0; }
GEM_VDI_WORD gem_mouse_amstrad_key_byte(GEM_VDI_UBYTE byte)
	{ (void) byte; return 0; }
GEM_VDI_WORD gem_mouse_amstrad_buttons(void) { return 0; }
