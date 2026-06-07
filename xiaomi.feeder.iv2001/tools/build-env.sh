#!/usr/bin/env bash
# Source this file to set up the build environment:
#   source tools/build-env.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
FIRMWARE_DIR="$REPO_ROOT/firmware"
PATCHES_DIR="$FIRMWARE_DIR/patches"

export SDK_ROOT="${SDK_ROOT:-$REPO_ROOT/external/linkit-sdk-v4.6.2-houndify}"

if [ ! -d "$SDK_ROOT" ]; then
    echo "ERROR: SDK not found at $SDK_ROOT"
    echo "Run: ./tools/fetch-sdk.sh"
    return 1 2>/dev/null || exit 1
fi

# Shallow git clones often drop the executable bit on SDK helper scripts.
if [ ! -x "$SDK_ROOT/build.sh" ]; then
    echo "Fixing execute permission on SDK build scripts"
    chmod +x "$SDK_ROOT/build.sh" 2>/dev/null || true
    find "$SDK_ROOT/middleware" "$SDK_ROOT/tools" "$SDK_ROOT/driver" -name '*.sh' -type f ! -perm -111 \
        -exec chmod +x {} + 2>/dev/null || true
fi

# --- SDK app bridge (project/mt7682_hdk/apps/petfeeder) ---
# build.sh discovers projects via `find project | grep GCC/Makefile` which does
# not descend into a symlinked app directory. Use a real petfeeder/ stub with
# symlinks into firmware/ instead.
PETFEEDER_APP="$SDK_ROOT/project/mt7682_hdk/apps/petfeeder"

if [ -L "$PETFEEDER_APP" ]; then
    rm -f "$PETFEEDER_APP"
fi
mkdir -p "$PETFEEDER_APP/GCC"
ln -sfn "$FIRMWARE_DIR/src" "$PETFEEDER_APP/src"
ln -sfn "$FIRMWARE_DIR/inc" "$PETFEEDER_APP/inc"
ln -sfn "$FIRMWARE_DIR/flash" "$PETFEEDER_APP/flash"
for gcc_file in Makefile feature.mk mt7682_flash.ld; do
    if [ -f "$FIRMWARE_DIR/GCC/$gcc_file" ]; then
        ln -sfn "$FIRMWARE_DIR/GCC/$gcc_file" "$PETFEEDER_APP/GCC/$gcc_file"
    fi
done
echo "SDK app bridge: $PETFEEDER_APP -> $FIRMWARE_DIR"

# --- IV2001 board config (config/board + driver/board symlinks into SDK) ---
IV2001_BOARD_CFG="$SDK_ROOT/config/board/iv2001"
mkdir -p "$(dirname "$IV2001_BOARD_CFG")"
ln -sfn "$FIRMWARE_DIR/board/iv2001" "$IV2001_BOARD_CFG"

IV2001_DRIVER_BOARD="$SDK_ROOT/driver/board/iv2001"
mkdir -p "$IV2001_DRIVER_BOARD"
ln -sfn "$SDK_ROOT/driver/board/mt7682_hdk/util" "$IV2001_DRIVER_BOARD/util"
ln -sfn "$SDK_ROOT/driver/board/mt7686_hdk/ept" "$IV2001_DRIVER_BOARD/ept"
echo "Board config: iv2001 (pinmux in firmware/, BSP via mt7682_hdk/mt7686_hdk)"

# Bootloader uses IV2001 memory_map + dual-image flash_map from firmware/
BOOTLOADER_INC="$SDK_ROOT/project/mt7682_hdk/apps/bootloader/inc"
mkdir -p "$BOOTLOADER_INC"
ln -sfn "$FIRMWARE_DIR/inc/memory_map.h" "$BOOTLOADER_INC/memory_map.h"
ln -sfn "$FIRMWARE_DIR/inc/flash_map.h" "$BOOTLOADER_INC/flash_map.h"
ln -sfn "$FIRMWARE_DIR/inc/bl_dual_image.h" "$BOOTLOADER_INC/bl_dual_image.h"
ln -sfn "$FIRMWARE_DIR/inc/boot_bank.h" "$BOOTLOADER_INC/boot_bank.h"
ln -sfn "$FIRMWARE_DIR/src/bl_dual_image.c" "$SDK_ROOT/project/mt7682_hdk/apps/bootloader/src/bl_dual_image.c"
ln -sfn "$FIRMWARE_DIR/src/boot_bank.c" "$SDK_ROOT/project/mt7682_hdk/apps/bootloader/src/boot_bank.c"

# copy_firmware.sh reads tools/config/iv2001/download/default/flash_download.cfg
IV2001_DOWNLOAD_CFG="$SDK_ROOT/tools/config/iv2001/download/default"
mkdir -p "$IV2001_DOWNLOAD_CFG"
ln -sfn "$FIRMWARE_DIR/flash/flash_download.cfg" "$IV2001_DOWNLOAD_CFG/flash_download.cfg"

# --- Apply SDK patches (idempotent) ---
if [ -d "$PATCHES_DIR" ]; then
    shopt -s nullglob
    patches=("$PATCHES_DIR"/*.patch)
    shopt -u nullglob

    if [ "${#patches[@]}" -gt 0 ]; then
        echo "Applying SDK patches from $PATCHES_DIR ..."
        for patch_file in "${patches[@]}"; do
            echo "  $(basename "$patch_file")"
            applied=0
            if command -v patch >/dev/null 2>&1; then
                if patch -p1 --forward --dry-run -d "$SDK_ROOT" -i "$patch_file" >/dev/null 2>&1; then
                    patch -p1 --forward -d "$SDK_ROOT" -i "$patch_file"
                    applied=1
                fi
            elif git -C "$SDK_ROOT" apply --check "$patch_file" >/dev/null 2>&1; then
                git -C "$SDK_ROOT" apply "$patch_file"
                applied=1
            fi
            if [ "$applied" -eq 0 ]; then
                echo "    already applied or not applicable — skipping"
            fi
        done
    fi
fi

# --- Toolchain (LinkIt does not bundle Linux GCC) ---
if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    echo "ERROR: arm-none-eabi-gcc not found on PATH"
    echo "Run: ./tools/check-prereqs.sh target"
    return 1 2>/dev/null || exit 1
fi

ARM_GCC_SYSROOT="$(arm-none-eabi-gcc -print-sysroot 2>/dev/null || true)"
if [ -z "$ARM_GCC_SYSROOT" ] || [ ! -f "$ARM_GCC_SYSROOT/include/stdio.h" ]; then
    echo "ERROR: arm-none-eabi-gcc sysroot lacks newlib headers (stdio.h)"
    echo "Run: ./tools/check-prereqs.sh target"
    return 1 2>/dev/null || exit 1
fi

export ARM_GCC_BIN="$(dirname "$(command -v arm-none-eabi-gcc)")"
export PATH="$ARM_GCC_BIN:$PATH"

# SDK link step calls tools/gcc/gcc-arm-none-eabi/bin/arm-none-eabi-g++.
# Distro tools live in /usr/bin — build a stub tree with bin/ symlinks.
ARM_TOOLCHAIN_STUB="$REPO_ROOT/external/arm-none-eabi-stub"
mkdir -p "$ARM_TOOLCHAIN_STUB/bin"
for tool in gcc g++ objcopy objdump size nm ranlib strip; do
    if command -v "arm-none-eabi-$tool" >/dev/null 2>&1; then
        ln -sfn "$(command -v "arm-none-eabi-$tool")" "$ARM_TOOLCHAIN_STUB/bin/arm-none-eabi-$tool"
    fi
done

mkdir -p "$SDK_ROOT/tools/gcc/linux" "$SDK_ROOT/tools/gcc"
ln -sfn "$ARM_TOOLCHAIN_STUB" "$SDK_ROOT/tools/gcc/linux/gcc-arm-none-eabi"
ln -sfn "$ARM_TOOLCHAIN_STUB" "$SDK_ROOT/tools/gcc/gcc-arm-none-eabi"
echo "Using arm-none-eabi-gcc from $ARM_GCC_BIN"

echo "SDK_ROOT=$SDK_ROOT"
echo "FIRMWARE_DIR=$FIRMWARE_DIR"
echo "Build environment ready."
