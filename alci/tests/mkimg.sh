#!/bin/bash
# usage: mkimg.sh <src tree> <label>  -> ${ALCI_QT:-/home/claude/qt}/<label>-{fd1440,hd32}.img with serial console + shell
mkdir -p ${ALCI_QT:-/home/claude/qt}
set -e
T=$1; L=$2; cd $T; source ./env.sh >/dev/null
B=elkscmd/rootfs_template/bootopts; cp $B /tmp/bootopts.save
{ echo '## alci qemu test'; echo 'console=ttyS0,19200'; echo 'init=/bin/sh'; grep -v '^#' /tmp/bootopts.save; } > $B
make all > ${ALCI_QT:-/home/claude/qt}/build-$L.log 2>&1
make -C image fd720-minix fd1440-minix hd32-minix >> ${ALCI_QT:-/home/claude/qt}/build-$L.log 2>&1
cp /tmp/bootopts.save $B
cp image/fd720-minix.img ${ALCI_QT:-/home/claude/qt}/$L-fd720.img; cp image/fd1440-minix.img ${ALCI_QT:-/home/claude/qt}/$L-fd1440.img; cp image/hd32-minix.img ${ALCI_QT:-/home/claude/qt}/$L-hd32.img
echo IMG $L OK
