#!/bin/bash
# usage: bench.sh <label>  -- builds ELKS (shell on boot), makes 720K image with bench file, times on MartyPC
set -e
L=$1
ROOT=${ALCI_ROOT:-/home/claude}
cd ${ELKS_TREE:-$ROOT/elks} && source ./env.sh >/dev/null
sed -i 's/^#init=\/bin\/sh/init=\/bin\/sh/' elkscmd/rootfs_template/bootopts
make all > $ROOT/work/build-$L.log 2>&1 || { git checkout -- elkscmd/rootfs_template/bootopts; echo BUILD FAILED; grep -n "error" $ROOT/work/build-$L.log | head; exit 1; }
make -C image fd720-minix > $ROOT/work/img-$L.log 2>&1
git checkout -- elkscmd/rootfs_template/bootopts
IMG=$ROOT/mpc/bench-$L.img; cp image/fd720-minix.img $IMG
python3 -c "
for i in range(200): print('Line %03d: the quick brown fox jumps over the lazy dog 0123456789 abcdefghij' % i)" > $ROOT/work/bench.txt
elks/tools/bin/mfs $IMG cp $ROOT/work/bench.txt /b.txt >/dev/null
cd $ROOT/mpc
ELKS_FD=$IMG ELKS_MAXSEC=600 ELKS_CMDS='cat /b.txt > /dev/null|||cat /b.txt|||cat /b.txt|||ls -l /bin|||ls -l /bin' \
  timeout 1800 $ROOT/martypc/target/release/martypc_headless --config_file martypc.toml --base_dir . --benchmark-mode 2>&1 | grep "^ELKS: cmd" | sed "s/^ELKS: /$L /"
