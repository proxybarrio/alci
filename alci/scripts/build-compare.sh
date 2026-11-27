#!/bin/bash
# usage: build-compare.sh <old-rev>  -- build <old-rev> in a git worktree with the
# current .config and compare the kernel and every file of target/ with the
# current tree (both built here with make all). Used after an upstream
# merge: identical files mean identical behaviour and timings; any difference is
# listed and the checks of ACCEPTANCE.md (V1 to V5) must then be run.
# Known harmless differences, listed but not counted: the version string in
# the kernel, the build time in ktcp (sntp.c BUILD_TIME), comment lines in
# bootopts.
set -e
OLD=$1; T=${ELKS_TREE:-/home/claude/elks}; W=/tmp/alci-cmp-wt
cd $T
# both trees built the same way here: an image build (make -C image) prunes target/
git diff --quiet -- elkscmd/rootfs_template/bootopts || { echo "bootopts modified, restore it first"; exit 1; }
( source ./env.sh > /dev/null && make all > /tmp/alci-cmp-build-new.log 2>&1 ) ||
  { echo "BUILD OF THE CURRENT TREE FAILED, see /tmp/alci-cmp-build-new.log"; exit 1; }
rm -rf $W; git worktree prune; git worktree add -q --detach $W $OLD
cp .config $W/; ln -s $T/cross $W/cross
cp elks/arch/i86/drivers/char/KeyMaps/config.in elks/arch/i86/drivers/char/KeyMaps/keymaps.h \
   $W/elks/arch/i86/drivers/char/KeyMaps/ 2>/dev/null || true
mkdir -p $W/elks/tools/bin; cp elks/tools/bin/lzsa $W/elks/tools/bin/ 2>/dev/null || true
( cd $W && source ./env.sh > /dev/null && make all > /tmp/alci-cmp-build.log 2>&1 ) ||
  { echo "BUILD OF $OLD FAILED, see /tmp/alci-cmp-build.log"; exit 1; }
(cd $W/target && find . -type f | sort | xargs md5sum) > /tmp/alci-cmp-old.md5
(cd $T/target && find . -type f | sort | xargs md5sum) > /tmp/alci-cmp-new.md5
n=0
for f in $(diff <(awk '{print $2}' /tmp/alci-cmp-old.md5) <(awk '{print $2}' /tmp/alci-cmp-new.md5) | grep '^[<>]' | awk '{print $2}'); do
  echo "ONLY IN ONE BUILD: $f"; n=$((n+1)); done
for f in $(join -j 2 <(sort -k2 /tmp/alci-cmp-old.md5) <(sort -k2 /tmp/alci-cmp-new.md5) | awk '$2 != $3 {print $1}') ./KERNEL; do
  if [ $f = ./KERNEL ]; then o=$W/elks/arch/i86/boot/Image; c=elks/arch/i86/boot/Image; else o=$W/target/$f; c=$T/target/$f; fi
  r=$(python3 - $o $c $f <<'PY'
import re, sys
a, b = open(sys.argv[1], 'rb').read(), open(sys.argv[2], 'rb').read()
f = sys.argv[3]
ver = lambda d: re.sub(rb'commit [0-9a-f]+ +\d+ \w+ \d{4} [\d:]+', b'COMMIT', d)
nocomment = lambda d: [l for l in d.splitlines() if l.strip() and not l.lstrip().startswith(b'#')]
if a == b: print('same')
elif f in ('./KERNEL', './linux') and ver(a) == ver(b): print('version string only')
elif f == './bin/ktcp' and len(a) == len(b) and sum(x != y for x, y in zip(a, b)) <= 8: print('build time only')
elif f == './bootopts' and nocomment(a) == nocomment(b): print('comment lines only')
else: print('DIFFERS')
PY
)
  case $r in same) ;; DIFFERS) echo "DIFFERS: $f"; n=$((n+1)) ;; *) echo "$f: $r" ;; esac
done
git worktree remove --force $W
[ $n = 0 ] && echo "BUILD COMPARE OK: same files as $OLD" || { echo "BUILD COMPARE: $n difference(s), run the checks"; exit 2; }
