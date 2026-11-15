#!/bin/bash
# Alci: prepare a fresh workspace. Usage: alci/scripts/setup.sh  (run from anywhere)
# Layout: $ROOT/elks (this fork, branch alci), $ROOT/martypc (fork, branch elks-bench),
#         $ROOT/mpc (MartyPC run directory), $ROOT/work (scratch)
set -e
ROOT=${ALCI_ROOT:-/home/claude}
ELKS=$ROOT/elks; MPC=$ROOT/mpc
mkdir -p $ROOT/work
# host packages
command -v xxd >/dev/null || sudo apt-get install -y xxd || apt-get install -y xxd
dpkg -s libudev-dev >/dev/null 2>&1 || sudo apt-get install -y libudev-dev pkg-config || apt-get install -y libudev-dev pkg-config
# toolchain (prebuilt, branch 'toolchain' of the fork)
if [ ! -x $ELKS/cross/bin/ia16-elf-gcc ]; then
  rm -rf $ROOT/work/tc
  git clone -q --depth 1 -b toolchain https://github.com/proxybarrio/elks $ROOT/work/tc
  mkdir -p $ELKS/cross && tar -C $ELKS/cross -xJf $ROOT/work/tc/ia16-toolchain.tar.xz
  [ -f $ELKS/.config ] || cp $ROOT/work/tc/elks.config $ELKS/.config
fi
[ -f $ELKS/.config ] || cp $ELKS/alci/elks.config $ELKS/.config
# MartyPC
if [ ! -d $ROOT/martypc ]; then
  git clone -q --depth 1 -b elks-bench https://github.com/proxybarrio/martypc $ROOT/martypc
fi
if [ ! -x $ROOT/martypc/target/release/martypc_headless ]; then
  command -v cargo >/dev/null || { curl -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal; . $HOME/.cargo/env; }
  (cd $ROOT/martypc && RUSTUP_TOOLCHAIN=stable cargo build --release -p martypc_headless)
fi
if [ ! -d $MPC ]; then cp -r $ROOT/martypc/install $MPC; cp $MPC/martypc-elks.toml $MPC/martypc.toml; fi
# first ELKS build (tools, kernel, apps)
cd $ELKS && source ./env.sh >/dev/null && make all > $ROOT/work/build-setup.log 2>&1 && echo "ALCI SETUP OK"
