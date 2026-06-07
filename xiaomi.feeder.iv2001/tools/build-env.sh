#!/usr/bin/env bash
# Source this file to set up the build environment:
#   source tools/build-env.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
FIRMWARE_DIR="$REPO_ROOT/firmware"
PATCHES_DIR="$FIRMWARE_DIR/patches"

export SDK_ROOT="${SDK_ROOT:-$REPO_ROOT/external/airoha-iot-sdk}"

if [ ! -d "$SDK_ROOT" ]; then
    echo "ERROR: SDK not found at $SDK_ROOT"
    echo "Run: ./tools/fetch-sdk.sh"
    return 1 2>/dev/null || exit 1
fi

# --- SDK app symlink (firmware/ -> project/aw7698_evk/apps/petfeeder) ---
PETFEEDER_LINK="$SDK_ROOT/project/aw7698_evk/apps/petfeeder"

if [ -L "$PETFEEDER_LINK" ]; then
    current_target="$(readlink -f "$PETFEEDER_LINK")"
    if [ "$current_target" != "$(readlink -f "$FIRMWARE_DIR")" ]; then
        echo "Updating petfeeder symlink -> $FIRMWARE_DIR"
        ln -sfn "$FIRMWARE_DIR" "$PETFEEDER_LINK"
    fi
elif [ -e "$PETFEEDER_LINK" ]; then
    echo "ERROR: $PETFEEDER_LINK exists and is not a symlink"
    return 1 2>/dev/null || exit 1
else
    echo "Creating petfeeder symlink -> $FIRMWARE_DIR"
    ln -sfn "$FIRMWARE_DIR" "$PETFEEDER_LINK"
fi

# --- Apply SDK patches (idempotent) ---
if [ -d "$PATCHES_DIR" ]; then
    shopt -s nullglob
    patches=("$PATCHES_DIR"/*.patch)
    shopt -u nullglob

    if [ "${#patches[@]}" -gt 0 ]; then
        echo "Applying SDK patches from $PATCHES_DIR ..."
        for patch_file in "${patches[@]}"; do
            echo "  $(basename "$patch_file")"
            if patch -p1 --forward --dry-run -d "$SDK_ROOT" -i "$patch_file" >/dev/null 2>&1; then
                patch -p1 --forward -d "$SDK_ROOT" -i "$patch_file"
            else
                echo "    already applied or not applicable — skipping"
            fi
        done
    fi
fi

# --- Toolchain ---
GCC_DIR="$SDK_ROOT/tools/gcc/linux/gcc-arm-none-eabi"
if [ -d "$GCC_DIR/bin" ]; then
    export PATH="$GCC_DIR/bin:$PATH"
    echo "GCC toolchain added to PATH from SDK"
else
    echo "WARNING: GCC toolchain not found at $GCC_DIR/bin"
    echo "You may need to install arm-none-eabi-gcc separately."
fi

echo "SDK_ROOT=$SDK_ROOT"
echo "FIRMWARE_DIR=$FIRMWARE_DIR"
echo "Build environment ready."
