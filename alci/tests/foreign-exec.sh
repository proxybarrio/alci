#!/bin/bash
# usage: foreign-exec.sh <alci fd720 image> <elks hd32 image> <workdir>
# Copies every LZSA1-compressed program of an Alci floppy image into /z of
# an ELKS image built from another kernel (for example upstream ELKS with
# its default configuration, made with mkimg.sh), runs each one on QEMU
# with stdin from /dev/null and prints its exit status, then runs a few
# checks that are compared with a clean boot of the same image (task F4e:
# a kernel without the LZSA1 decoder must refuse these programs and stay
# intact).
set -e
A=$1; E=$2; W=$3
T=$(cd $(dirname $0)/../.. && pwd); M=$T/elks/tools/bin/mfs
mkdir -p $W/prog
P=""
for p in $($M $A ls /bin | grep -v '^\.'); do
  $M $A cat /bin/$p > $W/prog/$p 2>/dev/null || continue
  python3 -c "
import struct,sys;d=open('$W/prog/$p','rb').read()
sys.exit(0 if len(d)>64 and d[4]==64 and struct.unpack_from('<H',d,62)[0]==0x3153 else 1)" && P="$P $p"
done
chmod 755 $W/prog/*
cp $E $W/run.img; cp $E $W/clean.img
$M $W/run.img mkdir /z > /dev/null
: > $W/suite.txt
for p in $P; do
  $M $W/run.img cp $W/prog/$p /z/$p > /dev/null
  echo "/z/$p </dev/null >/dev/null 2>&1; echo $p=\$?" >> $W/suite.txt
done
printf '%s\n' "ls /bin | wc -l" "md5sum /bin/sh /bin/ls /etc/passwd" \
  "cat /etc/passwd | wc -c" "meminfo" "echo alive" > $W/check.txt
cat $W/check.txt >> $W/suite.txt
python3 $T/alci/tests/sq.py $W/run.img hd 486 640 $W/suite.txt $W/run.raw > $W/run.out 2>&1 || true
python3 $T/alci/tests/sq.py $W/clean.img hd 486 640 $W/check.txt $W/clean.raw > $W/clean.out 2>&1 || true
echo "programs: $(echo $P | wc -w)"
grep "^[a-z0-9_]*=[0-9]*$" $W/run.out | tr '\n' ' '; echo
sed -n '/^=== ls \/bin/,$p' $W/run.out | diff - $W/clean.out > /dev/null && echo "system intact: checks identical to a clean boot" || echo "CHECKS DIFFER: see $W/run.out and $W/clean.out"
