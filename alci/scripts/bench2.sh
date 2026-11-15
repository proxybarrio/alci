#!/bin/bash
# usage: bench2.sh <image-label> [prof_from]  -- everyday-use benchmarks on an existing bench-<label>.img
ROOT=${ALCI_ROOT:-/home/claude}
L=$1
cd $ROOT/mpc
IMG=$ROOT/work/b2-$L.img; cp bench-$L.img $IMG
python3 -c "
for i in range(400): print('%05d the quick brown fox jumps over the lazy dog' % ((i*7919)%400))" > $ROOT/work/s.txt
$ROOT/elks/elks/tools/bin/mfs $IMG cp $ROOT/work/s.txt /s.txt >/dev/null
C='cat /bin/sh > /dev/null|||cat /bin/vi > /dev/null|||for i in 1 2 3 4 5 6 7 8 9 10; do pwd > /dev/null; done|||cat /s.txt | cat | cat > /dev/null|||sort /s.txt > /dev/null|||grep fox /s.txt > /dev/null'
P=""; [ -n "$2" ] && P="ELKS_PROF=$ROOT/work/b2p-$L.txt ELKS_PROF_FROM=$2 ELKS_SAMPLE=97"
env ELKS_FD=$IMG ELKS_MAXSEC=600 $P ELKS_CMDS="$C" timeout 1800 $ROOT/martypc/target/release/martypc_headless --config_file martypc.toml --base_dir . --benchmark-mode 2>&1 | grep "^ELKS: cmd\|timeout\|boot to" | sed "s/^ELKS: /$L /"
