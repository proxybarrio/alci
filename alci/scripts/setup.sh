#!/bin/bash
# Alci: prepare a fresh workspace. Usage: alci/scripts/setup.sh  (run from anywhere)
# Layout: $ROOT/elks (this fork, branch alci), $ROOT/martypc (fork, branch elks-bench),
#         $ROOT/mpc (MartyPC run directory), $ROOT/work (scratch)
set -e
ROOT=${ALCI_ROOT:-/home/claude}
ELKS=$ROOT/elks; MPC=$ROOT/mpc
mkdir -p $ROOT/work
# host packages
command -v xxd >/dev/null || apt-get install -y xxd || { apt-get update -q >/dev/null 2>&1; apt-get install -y xxd; }
dpkg -s libudev-dev >/dev/null 2>&1 || sudo apt-get install -y libudev-dev pkg-config || apt-get install -y libudev-dev pkg-config
# LZSA compressor (zlib licence) for the LZSA1 executable packer (elks/tools/elks-compress/elks-pack.py)
if [ ! -x $ELKS/elks/tools/bin/lzsa ] && mkdir -p $ELKS/elks/tools/bin; then
  (rm -rf $ROOT/work/lzsa && git clone -q --depth 1 https://github.com/emmanuel-marty/lzsa $ROOT/work/lzsa && \
   make -C $ROOT/work/lzsa > /dev/null 2>&1 && cp $ROOT/work/lzsa/lzsa $ELKS/elks/tools/bin/) || true
fi
# toolchain (prebuilt, branch 'toolchain')
if [ ! -x $ELKS/cross/bin/ia16-elf-gcc ]; then
  rm -rf $ROOT/work/tc
  git clone -q --depth 1 -b toolchain https://github.com/proxybarrio/alci $ROOT/work/tc
  mkdir -p $ELKS/cross && tar -C $ELKS/cross -xJf $ROOT/work/tc/ia16-toolchain.tar.xz
  [ -f $ELKS/.config ] || cp $ROOT/work/tc/elks.config $ELKS/.config
fi
[ -f $ELKS/.config ] || cp $ELKS/alci/elks.config $ELKS/.config
# MartyPC
if [ ! -d $ROOT/martypc ]; then
  git clone -q --depth 1 -b elks-bench https://github.com/proxybarrio/martypc $ROOT/martypc
elif [ "$(git -C $ROOT/martypc rev-parse --abbrev-ref HEAD)" != elks-bench ]; then
  git -C $ROOT/martypc fetch -q origin elks-bench:elks-bench && git -C $ROOT/martypc checkout -q elks-bench
fi
if [ ! -x $ROOT/martypc/target/release/martypc_headless ]; then
  command -v cargo >/dev/null || { curl -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal; . $HOME/.cargo/env; }
  (cd $ROOT/martypc && RUSTUP_TOOLCHAIN=stable cargo build --release -p martypc_headless)
fi
if [ ! -d $MPC ]; then cp -r $ROOT/martypc/install $MPC; cp $MPC/martypc-elks.toml $MPC/martypc.toml; fi
# 86Box with the automation socket, and its ROM set (functional tests on real BIOSes)
if [ ! -x $ROOT/86box/86box/build/src/86Box ]; then
  apt-get install -y -q cmake ninja-build libsdl2-dev libslirp-dev libsndfile1-dev libserialport-dev \
    libevdev-dev libxkbcommon-dev libopenal-dev libpng-dev libfreetype-dev libzstd-dev libarchive-dev qemu-system-x86 > $ROOT/work/apt86.log 2>&1 || \
    { apt-get update -q > /dev/null 2>&1; apt-get install -y -q cmake ninja-build libsdl2-dev libslirp-dev libsndfile1-dev \
      libserialport-dev libevdev-dev libxkbcommon-dev libopenal-dev libpng-dev libfreetype-dev libzstd-dev libarchive-dev qemu-system-x86 > $ROOT/work/apt86.log 2>&1; }
  mkdir -p $ROOT/86box
  # our fork (branch alci, PicoMEM patch already in) if it exists, else upstream patched below
  [ -d $ROOT/86box/86box ] || GIT_LFS_SKIP_SMUDGE=1 git clone -q --depth 1 -b alci https://github.com/proxybarrio/86box $ROOT/86box/86box 2>/dev/null || \
    GIT_LFS_SKIP_SMUDGE=1 git clone -q --depth 1 https://github.com/86Box/86box $ROOT/86box/86box
  [ -d $ROOT/86box/roms ] || GIT_LFS_SKIP_SMUDGE=1 git clone -q --depth 1 https://github.com/86Box/roms $ROOT/86box/roms
  (cd $ROOT/86box/86box && cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DQT=OFF -DSDL2=ON -DAUTOMATION=ON \
     -DRTMIDI=OFF -DFLUIDSYNTH=OFF -DMUNT=OFF -DSOUNDCANVAS=OFF -DDISCORD=OFF > $ROOT/work/cmake86.log 2>&1 && \
   ninja -C build > $ROOT/work/ninja86.log 2>&1) || echo "86Box build failed, see $ROOT/work/ninja86.log"
fi
# PicoMEM for 86Box: os8088's 86Box patch and libpmhost (MIT; see alci/RELATED.md)
if [ ! -f $ROOT/os8088/build/picomem/v2/libpmhost-v2.so ]; then
  [ -d $ROOT/os8088 ] || { GIT_LFS_SKIP_SMUDGE=1 git clone -q --depth 1 --filter=blob:none --sparse \
      https://github.com/jggonz/os8088 $ROOT/os8088 && git -C $ROOT/os8088 sparse-checkout set tools/86box tools/picomem; }
  (cd $ROOT/os8088 && OS88_NO_PREFLIGHT=1 bash tools/picomem/build.sh > $ROOT/work/picomem.log 2>&1) || echo "libpmhost build failed, see $ROOT/work/picomem.log"
  if ! grep -q picomem $ROOT/86box/86box/src/device/CMakeLists.txt; then
    (cd $ROOT/86box/86box && git apply $ROOT/os8088/tools/86box/patches/07-picomem.patch && ninja -C build > $ROOT/work/ninja86.log 2>&1) || echo "86Box PicoMEM patch failed"
  fi
fi
export PICOMEM_LIB=$ROOT/os8088/build/picomem/v2/libpmhost-v2.so PICOMEM_SD=$ROOT/os8088/build/picomem/sd.img
export B86=$ROOT/86box/86box/build/src/86Box R86=$ROOT/86box/roms
# first ELKS build (tools, kernel, apps)
cd $ELKS && source ./env.sh >/dev/null
[ -f elks/arch/i86/drivers/char/KeyMaps/keymaps.h ] || (yes '' | make config > $ROOT/work/config-setup.log 2>&1)
make all > $ROOT/work/build-setup.log 2>&1 && echo "ALCI SETUP OK"
