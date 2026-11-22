#!/bin/bash
# mkrelease.sh <output dir> <version>    e.g. mkrelease.sh /tmp/rel 0.1-preview
# Builds the Alci release images from the current tree:
#   floppies (360K, 720K disk 1 and 2, 1.2M, 1.44M) with alci/elks.config;
#   32 MB hard disk (MBR) with alci/elks.config plus CONFIG_EXEC_OS2 and
#   Bobcat (decided 2026-11-22: OS/2 loader, +1696 resident bytes, on the
#   hard disk image only).
# Needs OpenWatcom for Bobcat: WATCOM=<dir> and its binl64 on PATH.
# Leaves .config and the kernel as alci/elks.config builds them.
set -e
OUT=$1; VER=$2
[ -n "$OUT" ] && [ -n "$VER" ] || { echo "usage: $0 <outdir> <version>"; exit 1; }
[ -n "$WATCOM" ] || { echo "WATCOM not set (OpenWatcom needed for Bobcat)"; exit 1; }
BOBCAT_REPO=https://github.com/rafael2k/bobcat
BOBCAT_REV=d642c3fba7668b223a673b71041c2f7c1418e822
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd $ROOT
. ./env.sh > /dev/null
MFS=$ROOT/elks/tools/bin/mfs
N=alci-$VER
mkdir -p $OUT
LOG=$OUT/build.log; : > $LOG

kernel() {   # kernel <config file>
  cp "$1" .config
  yes '' | make config >> $LOG 2>&1
  make -C elks clean >> $LOG 2>&1
  make all >> $LOG 2>&1
}
restore() { kernel alci/elks.config; }
trap 'echo "failed, see $LOG"; restore' ERR

echo "floppy images"
kernel alci/elks.config
make -C image fd360-minix fd720-minix fd1200-minix fd1440-minix >> $LOG 2>&1
cp image/fd360-minix.img  $OUT/$N-fd360.img
cp image/fd720-minix.img  $OUT/$N-fd720-disk1.img
cp image/fd1200-minix.img $OUT/$N-fd1200.img
cp image/fd1440-minix.img $OUT/$N-fd1440.img
alci/scripts/mkdisk2.sh $OUT/$N-fd720-disk1.img $OUT/$N-fd1440.img $OUT/$N-fd720-disk2.img >> $LOG

echo "Bobcat"
mkdir -p extapps
[ -d extapps/bobcat ] || git clone -q $BOBCAT_REPO extapps/bobcat
git -C extapps/bobcat fetch -q origin
git -C extapps/bobcat checkout -q $BOBCAT_REV
./buildext.sh owc_libc >> $LOG 2>&1
( cd extapps/bobcat/src && make -f Makefile.elks clean && make -f Makefile.elks ) >> $LOG 2>&1

echo "hard disk image"
sed -e 's/^# CONFIG_EXEC_OS2 is not set/CONFIG_EXEC_OS2=y/' alci/elks.config > /tmp/alci-hd.config
grep -q '^CONFIG_EXEC_OS2=y' /tmp/alci-hd.config
kernel /tmp/alci-hd.config
make -C image hd32-minix >> $LOG 2>&1
H=image/hd32-minix.img
$MFS $H cp extapps/bobcat/src/bobcat /bin/bobcat > /dev/null
$MFS $H cp extapps/bobcat/lynx.cfg /etc/lynx.cfg > /dev/null
$MFS $H cat /bootopts > /tmp/alci-bootopts
cat >> /tmp/alci-bootopts <<'EOF'
#buf=16 for bobcat with network: 48K less disk cache
EOF
$MFS $H rm /bootopts > /dev/null
$MFS $H cp /tmp/alci-bootopts /bootopts > /dev/null
dd if=/dev/zero bs=512 count=63 2>/dev/null | cat - $H > $OUT/$N-hd32.img
setboot $OUT/$N-hd32.img -P63,16,63 -Sm bootblocks/mbr.bin >> $LOG
( cd extapps/bobcat && git archive --format=tar.gz --prefix=bobcat-$BOBCAT_REV/ $BOBCAT_REV ) > $OUT/bobcat-source-$BOBCAT_REV.tar.gz

trap - ERR
restore
( cd $OUT && sha256sum *.img *.tar.gz > SHA256SUMS )
cat $OUT/SHA256SUMS
echo "release $N built from $(git rev-parse --short HEAD)"
