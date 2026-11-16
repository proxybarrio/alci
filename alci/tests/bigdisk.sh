#!/bin/bash
# usage: bigdisk.sh <hd32 image>
# Checks INT 13h extensions (EDD) on QEMU: a 1 GB second disk with a 4 MB
# partition at cylinder 1190, written in one boot and read back in another.
set -e
D=$(dirname $0); IMG=$1; W=$(mktemp -d)
cp $IMG $W/boot.img; truncate -s 1G $W/big.img
python3 -c "
import struct
f=open('$W/big.img','r+b'); m=bytearray(512)
m[446:462]=bytes([0,0xFE,0xFF,0xFF,0x80,0xFE,0xFF,0xFF])+struct.pack('<II',1200000,8192)
m[510:512]=b'\x55\xaa'; f.write(m)"
printf 'mkfs /dev/hdb1 4096\nmount /dev/hdb1 /mnt\ncp /bin/sh /mnt/s; cp /bin/vi /mnt/v; sync\numount /mnt; sync\n' > $W/w.txt
printf 'dmesg | grep hdb\nmount /dev/hdb1 /mnt\ncmp /bin/sh /mnt/s && cmp /bin/vi /mnt/v && echo big-ok\numount /mnt\n' > $W/r.txt
NOSNAP=1 EXTRA_HD=$W/big.img python3 $D/qtest.py $W/boot.img hd 486 640 $W/w.txt > /dev/null
NOSNAP=1 EXTRA_HD=$W/big.img python3 $D/qtest.py $W/boot.img hd 486 640 $W/r.txt
rm -rf $W
