#!/bin/sh
# Build Nano-X (Microwindows) for ELKS outside its tree: objects and
# programs go to $OUT; the source tree is only read.
# usage: nxbuild.sh SRC OUT cga|vga (SRC: microwindows/src; ELKS_ROOT: the tree)
set -e
SRC=$1; OUT=$2; SCR=$3
T=${ELKS_ROOT:-$(cd $(dirname $0)/../../.. && pwd)}
mkdir -p $OUT/obj $OUT/bin
CF="-mcmodel=small -melks-libc -mtune=i8086 -Os -mno-segment-relocation-stuff -fno-inline -fno-builtin-printf -fno-builtin-fprintf
 -w -I$T/include -I$T/libc/include -I$T/elks/include
 -O2 -fno-align-jumps -fno-align-functions -fno-align-loops -fno-align-labels
 -DELKS=1 -DUNIX=1 -DNDEBUG=1 -DMWCONFIG_FILE=\"mwconfig.elks\"
 -DMWPIXEL_FORMAT=MWPF_PALETTE -DSCREEN_PIXTYPE=MWPF_PALETTE -I$SRC/include
 -ffunction-sections -DNONETWORK=0"
if [ $SCR = cga ]; then
  CF="$CF -DUSE_SMALL_CURSOR=1"; DRV="scr_cga vgaplan4_cga vgaplan4_mem"
else
  CF="$CF -DUSE_VGA_XOR_CURSOR=1"; DRV="scr_vga vgaplan4_vga vgaplan4_mem"
fi
cc() { o=$OUT/obj/$(echo $1 | tr / _).o; X=""; [ $2 = c ] && X="-include stddef.h"; ia16-elf-gcc $CF $X -c -o $o $SRC/$1.$2; echo $o; }
SERVER=""; for f in nanox/srvmain nanox/srvfunc nanox/srvutil nanox/srvevent nanox/nxutil nanox/nxdraw nanox/nxpaintnc nanox/srvclip1 \
  nanox/wmaction nanox/wmclients nanox/wmevents nanox/wmutil nanox/srvnet drivers/osdep \
  engine/devdraw engine/devmouse engine/devkbd engine/devclip1 engine/devopen engine/devfont engine/devlist engine/devblit engine/devpal1 engine/devpal4 \
  drivers/genfont drivers/genmem drivers/fb fonts/compiled/winFreeSansSerif11x13 fonts/compiled/X6x13 drivers/mou_ser drivers/kbd_tty; do
  SERVER="$SERVER $(cc $f c)"; done
for d in $DRV; do SERVER="$SERVER $(cc drivers/$d c)"; done
SERVER="$SERVER $(cc drivers/vgaplan4_asm S)"
ia16-elf-gcc -mcmodel=small -melks-libc -Wl,--gc-sections -maout-heap=0x2000 -o $OUT/bin/nano-X $SERVER
CLIENT=""; for f in nanox/client nanox/nxproto; do CLIENT="$CLIENT $(cc $f c)"; done
CLIENT="$CLIENT $OUT/obj/nanox_nxutil.o $OUT/obj/drivers_osdep.o"
rm -f $OUT/libnano-X.a; ia16-elf-ar rc $OUT/libnano-X.a $CLIENT
for p in ${PROGS:-nxcalc nxterm nxclock nxstart nxedit}; do
  ia16-elf-gcc $CF -DCONFIG_HW_VGA=1 -I$SRC/include -c -o $OUT/obj/$p.o $SRC/demos/nanox/$p.c
  ia16-elf-gcc -mcmodel=small -melks-libc -Wl,--gc-sections -o $OUT/bin/$p $OUT/obj/$p.o $OUT/libnano-X.a
done
# the benchmark client, from this directory
B=$(cd $(dirname $0) && pwd)
ia16-elf-gcc $CF -c -o $OUT/obj/nxbench.o $B/nxbench.c
ia16-elf-gcc -mcmodel=small -melks-libc -Wl,--gc-sections -o $OUT/bin/nxbench $OUT/obj/nxbench.o $OUT/libnano-X.a
ls -l $OUT/bin
