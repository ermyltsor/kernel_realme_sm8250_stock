#!/bin/bash

# Kernel source path
KERNEL_ROOT=$PWD

# Remove out, if exist
if [ -d "$KERNEL_ROOT/out" ]; then
    echo "Delete the existing out folder."
    rm -rf "$KERNEL_ROOT/out"
fi

# AnyKernel3
if [[ ! -d "$KERNEL_ROOT/anykernel" ]]; then
    git clone --depth=1 https://github.com/ermyltsor/AnyKernel3.git "$KERNEL_ROOT/anykernel"
fi

# Remove anykernel packages, if exist
if find "$KERNEL_ROOT/anykernel" -maxdepth 1 -type f -name "*.zip" | grep -q .; then
    echo "Remove any existing anykernel packages."
    find "$KERNEL_ROOT/anykernel" -maxdepth 1 -type f -name "*.zip" -delete
fi

if [ -f "$KERNEL_ROOT/anykernel/Image" ] && [ -f "$KERNEL_ROOT/anykernel/dtb" ] && [ -f "$KERNEL_ROOT/anykernel/dtbo.img" ]; then
    rm -f "$KERNEL_ROOT/anykernel/Image"
    rm -f "$KERNEL_ROOT/anykernel/dtb"
    rm -f "$KERNEL_ROOT/anykernel/dtbo.img"
fi

# Clang
if [[ ! -d "$KERNEL_ROOT/clang-10" ]]; then
    mkdir -p "$KERNEL_ROOT/clang-10"

    if [[ ! -d "$KERNEL_ROOT/Clang-10.0.1-20220724.tar.gz" ]]; then
        wget -c https://github.com/ZyCromerZ/Clang/releases/download/10.0.1-20220724-release/Clang-10.0.1-20220724.tar.gz
    fi

    tar -xzf "$KERNEL_ROOT/Clang-10.0.1-20220724.tar.gz" -C "$KERNEL_ROOT/clang-10"
    rm -f "$KERNEL_ROOT/Clang-10.0.1-20220724.tar.gz"
fi

export CLANG_PATH=$KERNEL_ROOT/clang-10/bin
export PATH=$CLANG_PATH:$PATH
export CROSS_COMPILE=aarch64-linux-gnu-

ANYKERNEL_NAME=C15PORT

KERNEL_DEFCONFIG=vendor/kona-perf_defconfig

echo
echo "Kernel is going to be built using $KERNEL_DEFCONFIG."
echo

MAKE_FLAGS="ARCH=arm64 AR=llvm-ar CC=clang NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip LLVM=1 O=out"

make $MAKE_FLAGS $KERNEL_DEFCONFIG

make $MAKE_FLAGS -j$(nproc)

echo "Build Complete."

IMAGE_OUT=$KERNEL_ROOT/out/arch/arm64/boot

if [ -f "$IMAGE_OUT/Image" ] && [ -f "$IMAGE_OUT/dtb" ] && [ -f "$IMAGE_OUT/dtbo.img" ]; then
    cp "$IMAGE_OUT/Image"       "$KERNEL_ROOT/anykernel"
    cp "$IMAGE_OUT/dtb"         "$KERNEL_ROOT/anykernel"
    cp "$IMAGE_OUT/dtbo.img"    "$KERNEL_ROOT/anykernel"

    cd "$KERNEL_ROOT/anykernel"
    zip -r "$ANYKERNEL_NAME.zip" .
    echo "Find the package in $KERNEL_ROOT/anykernel/$ANYKERNEL_NAME.zip."
    cd ../
else
    exit 1
fi
