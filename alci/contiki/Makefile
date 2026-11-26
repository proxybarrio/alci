# Contiki 1.x CTK desktop for ELKS/Alci (task D7), one program per screen type,
# and "desk", which starts the one that fits the machine:
#   ctk       CGA 640x200, 2 colours       ctk-cga4  CGA 320x200, 4 colours
#   ctk-ega   EGA 640x350, 16 colours      ctk-vga   VGA 640x480, 16 colours
#   ctk-mcga  MCGA 320x200, 256 colours
# Needs the ELKS cross toolchain (. ./env.sh in the ELKS tree).
# "make fetch" gets Contiki 1.x at the commit this prototype was built with.
CONTIKI_REPO = https://github.com/adamdunkels/contiki-1.x
CONTIKI_REV = bfc34cec508399cb34b5da1944cd9d7af7548146
CONTIKI ?= contiki-1.x/contiki
TOPDIR ?= ../..
CC = ia16-elf-gcc
CFLAGS = -melks-libc -mtune=i8086 -mcmodel=small -mno-segment-relocation-stuff \
	-Os -fno-inline -Wall -Wno-unused-parameter \
	-I elks -I . -I$(TOPDIR)/include -I$(TOPDIR)/libc/include -I$(TOPDIR)/elks/include \
	-I$(CONTIKI)/ek -I$(CONTIKI)/ctk -I$(CONTIKI)/lib -I$(CONTIKI)/apps
LDFLAGS = -melks-libc -mtune=i8086 -mcmodel=small -mno-segment-relocation-stuff -maout-heap=0x2000 -maout-stack=0x1000
VPATH = $(CONTIKI)/ctk $(CONTIKI)/ek $(CONTIKI)/lib $(CONTIKI)/apps
COMMON = main.c ctk.c ek.c arg.c timer.c \
	program-handler.c petsciiconv.c about.c about-dsc.c calc.c calc-dsc.c mouse.c \
	edit.c edit-dsc.c filedlg.c term.c term-dsc.c
SRCS_col = $(COMMON) ctk-gfx.c
# icons built into the desktop
ICONS = about calculator editor shell
TARGETS = cga cga4 ega vga mcga
PROGS = desk ctk ctk-cga4 ctk-ega ctk-vga ctk-mcga ctk-tandy ctk-tandy4 ctk-herc ctk-att ctk-ega2 ctk-vga2

all: $(PROGS)

define target
obj-$(1)/gfx-icons.h: icons/tools/mkgfx.py
	mkdir -p obj-$(1)
	python3 icons/tools/mkgfx.py $(1) $$@ $(ICONS)
obj-$(1)/%.o: %.c obj-$(1)/gfx-icons.h gfx.h elks/ctk-conf.h elks/ctk-arch.h mouse.h filedlg.h watch.h
	$(CC) $(CFLAGS) -DGFX_$(shell echo $(1) | tr a-z A-Z) -I obj-$(1) -include gfx.h -c -o $$@ $$<
$(2): $(patsubst %.c,obj-$(1)/%.o,$(SRCS_$(3)))
	$(CC) $(LDFLAGS) -o $$@ $$^
endef
$(eval $(call target,cga,ctk,col))
$(eval $(call target,cga4,ctk-cga4,col))
$(eval $(call target,ega,ctk-ega,col))
$(eval $(call target,vga,ctk-vga,col))
$(eval $(call target,mcga,ctk-mcga,col))
$(eval $(call target,tandy,ctk-tandy,col))
$(eval $(call target,tandy4,ctk-tandy4,col))
$(eval $(call target,herc,ctk-herc,col))
$(eval $(call target,att,ctk-att,col))
$(eval $(call target,ega2,ctk-ega2,col))
$(eval $(call target,vga2,ctk-vga2,col))

# desk: runs the build that fits the screen (asks the video BIOS)
desk: desk.c
	$(CC) $(CFLAGS) -o $@ $<

fetch:
	[ -d contiki-1.x ] || git clone $(CONTIKI_REPO) contiki-1.x
	git -C contiki-1.x checkout -q -f $(CONTIKI_REV)
	# the icons are declared extern in the headers and static in the .c files;
	# current compilers reject that, so drop "static" (no other change)
	sed -i 's/^static struct ctk_icon \([a-z_]*_icon\)/struct ctk_icon \1/' contiki-1.x/contiki/apps/*-dsc.c
	# the icon column follows the icon size of the screen type (gfx.h)
	sed -i -e 's/^#define ICONX_START  (width - 6)/#define ICONX_START  (width - CTK_ICONX_FROM_RIGHT)/' \
	       -e 's/^#define ICONY_START  (height - 7)/#define ICONY_START  (height - CTK_ICONY_FROM_BOTTOM)/' \
	       -e 's/^#define ICONX_DELTA  -16/#define ICONX_DELTA  (-CTK_ICONX_FROM_RIGHT - 4)/' \
	       -e 's/^#define ICONY_DELTA  -5/#define ICONY_DELTA  (-CTK_ICONY_STEP)/' contiki-1.x/contiki/ctk/ctk.c
	# the calculator shows 0 when its display is empty
	git -C contiki-1.x apply ../patches/calc-zero.patch
	# opening a window redraws that window and the one it covers, closing
	# one the rows it took, instead of the whole screen
	git -C contiki-1.x apply ../patches/ctk-partial-redraw.patch

clean:
	rm -rf $(PROGS) obj-*
