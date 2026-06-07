#!/usr/bin/env bash
# Source this file to set up the build environment:
#   source tools/build-env.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

export SDK_ROOT="${SDK_ROOT:-$REPO_ROOT/external/airoha-iot-sdk}"

if [ ! -d "$SDK_ROOT" ]; then
    echo "ERROR: SDK not found at $SDK_ROOT"
    echo "Run: ./tools/fetch-sdk.sh"
    return 1 2>/dev/null || exit 1
fi

GCC_DIR="$SDK_ROOT/tools/gcc/linux/gcc-arm-none-eabi"
if [ -d "$GCC_DIR/bin" ]; then
    export PATH="$GCC_DIR/bin:$PATH"
    echo "GCC toolchain added to PATH from SDK"
else
    echo "WARNING: GCC toolchain not found at $GCC_DIR/bin"
    echo "You may need to install arm-none-eabi-gcc separately."
fi

echo "SDK_ROOT=$SDK_ROOT"
echo "Build environment ready."
