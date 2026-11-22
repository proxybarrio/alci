# Contiki 1.x CTK desktop for ELKS/Alci, CGA prototype (task D7)
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
	-I$(CONTIKI)/ek -I$(CONTIKI)/ctk -I$(CONTIKI)/lib -I$(CONTIKI)/apps -I icons/out
LDFLAGS = -melks-libc -mtune=i8086 -mcmodel=small -mno-segment-relocation-stuff -maout-heap=0x2000 -maout-stack=0x1000
VPATH = $(CONTIKI)/ctk $(CONTIKI)/ek $(CONTIKI)/lib $(CONTIKI)/apps
OBJS = main.o ctk-cga.o theme-default.o ctk.o ek.o arg.o timer.o \
	program-handler.o petsciiconv.o about.o about-dsc.o calc.o calc-dsc.o
ctk: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
fetch:
	[ -d contiki-1.x ] || git clone $(CONTIKI_REPO) contiki-1.x
	git -C contiki-1.x checkout -q $(CONTIKI_REV)
	# the icons are declared extern in the headers and static in the .c files;
	# current compilers reject that, so drop "static" (no other change)
	sed -i 's/^static struct ctk_icon \([a-z_]*_icon\)/struct ctk_icon \1/' contiki-1.x/contiki/apps/*-dsc.c
	# Alci icons are 10x4 cells (80x32 pixels): place them one column further
	# from the right edge and leave a free row between them
	sed -i -e 's/^#define ICONX_START  (width - 6)/#define ICONX_START  (width - 12)/' \
	       -e 's/^#define ICONY_DELTA  -5/#define ICONY_DELTA  -6/' contiki-1.x/contiki/ctk/ctk.c
clean:
	rm -f ctk *.o
