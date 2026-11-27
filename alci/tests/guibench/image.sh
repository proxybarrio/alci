#!/bin/bash
# Test image: image.sh OUT.img BASE.img src:/dest ... (directories made as needed)
set -e
E=${ELKS_ROOT:-$(cd $(dirname $0)/../../.. && pwd)}; M=$E/elks/tools/bin/mfs; OUT=$1; BASE=$2; shift 2
cp $BASE $OUT
{ echo '## alci screen test'; echo 'init=/bin/sh'; grep -v '^#' $E/elkscmd/rootfs_template/bootopts; } > /tmp/bootopts.scr
$M $OUT rm /bootopts >/dev/null 2>&1 || true
$M $OUT cp /tmp/bootopts.scr /bootopts > /dev/null
for A in "$@"; do S=${A%%:*}; D=${A#*:}; d=$(dirname $D)
  p=""; for c in $(echo $d | tr / ' '); do p=$p/$c; $M $OUT mkdir $p >/dev/null 2>&1 || true; done
  $M $OUT rm $D >/dev/null 2>&1 || true; $M $OUT cp $S $D > /dev/null; done
echo ok
