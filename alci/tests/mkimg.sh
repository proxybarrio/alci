#!/bin/bash
# usage: mkimg.sh <src tree> <label>  -> ${ALCI_QT:-/home/claude/qt}/<label>-{fd720,fd1440,hd32,hd20t}.img with serial console + shell,
# and <label>-fd720b.img (second disk for functional-fd.txt)
mkdir -p ${ALCI_QT:-/home/claude/qt}
set -e
T=$1; L=$2; cd $T; source ./env.sh >/dev/null
B=elkscmd/rootfs_template/bootopts
# always start from the committed bootopts and put it back, even on failure
git checkout -- $B; trap 'git checkout -- $B' EXIT
cp $B /tmp/bootopts.save
if [ "$CONSOLE" = screen ]; then
  { echo '## alci screen test'; echo 'init=/bin/sh'; grep -v '^#' /tmp/bootopts.save; } > $B
else
  { echo '## alci qemu test'; echo 'console=ttyS0,19200'; echo 'init=/bin/sh';
    [ -n "$BOOTOPTS_EXTRA" ] && printf '%b\n' "$BOOTOPTS_EXTRA"; grep -v '^#' /tmp/bootopts.save; } > $B
fi
make all > ${ALCI_QT:-/home/claude/qt}/build-$L.log 2>&1
make -C image fd720-minix fd1440-minix hd32-minix >> ${ALCI_QT:-/home/claude/qt}/build-$L.log 2>&1
# 20 MB image with the Toshiba T1200 BIOS geometry (611 cyl, 4 heads, 17 sectors)
( cd image && { echo CONFIG_APPS_HD=y; echo CONFIG_IMG_HD=y; echo CONFIG_IMG_BLOCKS=20774;
    echo CONFIG_IMG_SECT=17; echo CONFIG_IMG_HEAD=4; echo CONFIG_IMG_CYL=611;
    echo CONFIG_IMG_MINIX=y; echo CONFIG_IMG_DEV=y; echo CONFIG_IMG_BOOT=y;
    sed -n -e '/CONFIG_TIME_/p' < ../.config; } > Config &&
  make -f Make.image DESTDIR=$T/target "CONFIG=$PWD/Config" NAME=hd20t >> ${ALCI_QT:-/home/claude/qt}/build-$L.log 2>&1; rm -f Config )
cp image/fd720-minix.img ${ALCI_QT:-/home/claude/qt}/$L-fd720.img; cp image/fd1440-minix.img ${ALCI_QT:-/home/claude/qt}/$L-fd1440.img; cp image/hd32-minix.img ${ALCI_QT:-/home/claude/qt}/$L-hd32.img; cp image/hd20t.img ${ALCI_QT:-/home/claude/qt}/$L-hd20t.img
# 200-line text file used by the screen benchmarks
python3 -c "
for i in range(200): print('Line %03d: the quick brown fox jumps over the lazy dog 0123456789 abcdefghij' % i)" > /tmp/alci-b.txt
for I in fd720 fd1440 hd32 hd20t; do elks/tools/bin/mfs ${ALCI_QT:-/home/claude/qt}/$L-$I.img cp /tmp/alci-b.txt /b.txt > /dev/null; done
# second 720K disk for the floppy suite (functional-fd.txt): the programs it
# needs that are not on the 720K base disk, mounted read-only on /mnt
D2=/tmp/alci-d2.$$; rm -rf $D2; mkdir -p $D2/bin
for P in misc_utils/tar misc_utils/compress file_utils/md5sum test/libc/test_libc test/other/test_float; do cp elkscmd/$P $D2/bin/; done
rm -f ${ALCI_QT:-/home/claude/qt}/$L-fd720b.img
elks/tools/bin/mfs ${ALCI_QT:-/home/claude/qt}/$L-fd720b.img genfs -1 -n14 -i64 -s720 $D2 > /dev/null; rm -rf $D2
echo IMG $L OK
