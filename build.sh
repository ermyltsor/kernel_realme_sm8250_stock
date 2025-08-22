#!/bin/bash

# history build output
if [ -d out ]; then
    echo "Delete the existing out folder."
    rm -rf out
else
    echo "Not found out folder."
fi

# find history anykernel.zip
if find anykernel -maxdepth 1 -type f -name "*.zip" | grep -q .; then
    echo "Delete the history anykernel.zip file."
    find anykernel -maxdepth 1 -type f -name "*.zip" -delete
else
    echo "No history exists anykernel.zip."
fi

# Fake username
#export KBUILD_BUILD_USER= 

# Fake Hostname
export KBUILD_BUILD_HOST=$(tr -dc 'A-Za-z0-9' </dev/urandom | head -c 12)

# Custom Clang Path
if [ -d clang-tools ]; then
    # I'm a noob, if errors please manual operation.
    echo "Found clang-tools folder."
else
    echo "Create clang-tools folder."
    mkdir -p clang-tools

    if [ -f Clang-10.0.1-20220724.tar.gz ]; then
        echo "Clang-10.0.1-20220724.tar.gz exists."
    else
        # get the zyc clang from github,
        wget -c https://github.com/ZyCromerZ/Clang/releases/download/10.0.1-20220724-release/Clang-10.0.1-20220724.tar.gz
    fi

    tar -xzf Clang-10.0.1-20220724.tar.gz -C clang-tools
    rm -f Clang-10.0.1-20220724.tar.gz
fi

export CLANG_PATH=$PWD/clang-tools/bin
export PATH=$CLANG_PATH:$PATH
export CROSS_COMPILE=aarch64-linux-gnu-

# Create output directory
mkdir -p out

KERNEL_DEFCONFIG=vendor/kona-perf_defconfig
ANYKERNEL_NAME=rui3.0-stock

echo "The kernel will be built in 5 seconds."
sleep 5

echo
echo "Kernel is going to be built using $KERNEL_DEFCONFIG."
echo

MAKE_FLAGS="ARCH=arm64 AR=llvm-ar CC=clang NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip LLVM=1 O=out"

make $MAKE_FLAGS $KERNEL_DEFCONFIG

make $MAKE_FLAGS -j$(nproc)

echo "Build Complete."

echo "Enter the anykernel directory after 5 seconds."
sleep 5
cd anykernel

if [ -f ../out/arch/arm64/boot/Image ] && [ -f ../out/arch/arm64/boot/dtb ] && [ -f ../out/arch/arm64/boot/dtbo.img ]; then
    echo "Copy Image, dtb, dtbo.img from output to the anykernel folder."
    cp ../out/arch/arm64/boot/Image .
    cp ../out/arch/arm64/boot/dtb .
    cp ../out/arch/arm64/boot/dtbo.img .

    echo "Compress everything in the anykernel folder into a zip file."
    zip -qr "$ANYKERNEL_NAME.zip" *

    echo "Return to the root directory of the kernel tree."
    cd ..

   sleep 2

    if [ -f anykernel/Image ] && [ -f anykernel/dtb ] && [ -f anykernel/dtbo.img ]; then
        echo "Delete Image, dtb, and dtbo.img from the anykernel folder."
        rm anykernel/Image anykernel/dtb anykernel/dtbo.img
    fi

    echo "You can find the $ANYKERNEL_NAME.zip in anykernel folder."
else
    exit 1
fi