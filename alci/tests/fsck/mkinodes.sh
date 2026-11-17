#!/bin/sh
# mkinodes.sh - make a MINIX v1 image with many in-use inodes for fsck tests
#
# usage: mkinodes.sh <image> [nfiles] [blocks] [filler_kb]
#   nfiles     small files to create (default 2000; > 1024 inodes in use
#              exercises fsck's inode block offsets past inode block 31)
#   blocks     filesystem size in 1K blocks (default 32768, max 65535)
#   filler_kb  size of a large file of random data written first, so the small files'
#              data lands at high zone numbers (e.g. 40000 puts it past 32767)
#
# Needs mfs from elks/tools/bin in PATH.
set -e
IMG=${1:?usage: mkinodes.sh <image> [nfiles] [blocks] [filler_kb]}
N=${2:-2000}; BLOCKS=${3:-32768}; FILLER=${4:-0}
T=$(mktemp -d)
# the filler goes in first with genfs, the small files are added after it
[ "$FILLER" -gt 0 ] && dd if=/dev/urandom of=$T/filler bs=1024 count=$FILLER 2>/dev/null
echo "file" > $T/f
INODES=$((N + N / 100 + 64))
rm -f $IMG
mfs $IMG genfs -1 -n14 -i$INODES -s$BLOCKS $T > /dev/null
i=0
while [ $i -lt $N ]; do
    d=/d$((i / 100))
    [ $((i % 100)) -eq 0 ] && mfs $IMG mkdir $d
    mfs $IMG cp $T/f $d/f$i
    i=$((i + 1))
done
rm -rf $T
echo "$IMG: $N files, $INODES inodes, $BLOCKS blocks"
