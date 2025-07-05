#!/bin/bash

#
# Download zyc clang 10.0.1, if needed
#

KERNEL_DEFCONFIG=vendor/kona-perf_defconfig
DIR=$PWD
export ARCH=arm64
export CLANG_PATH="/$HOME/clang/bin"
export PATH="$CLANG_PATH:$PATH"
export CROSS_COMPILE=aarch64-linux-gnu-
export KBUILD_BUILD_USER=root
export KBUILD_BUILD_HOST=dg02-pool06-kvm10

echo
echo "Kernel is going to be built using $KERNEL_DEFCONFIG"
echo

make CC=clang AR=llvm-ar NM=llvm-nm OBJDUMP=llvm-objdump STRIP=llvm-strip O=out $KERNEL_DEFCONFIG

make CC=clang AR=llvm-ar NM=llvm-nm OBJDUMP=llvm-objdump STRIP=llvm-strip O=out -j$(nproc --all)
