#!/usr/bin/env bash
# Build bootloader + application via LinkIt SDK build.sh.
#
#   ./tools/build-firmware.sh
#
# Produces petfeeder_a.bin (Bank A @ 0x08012000) and petfeeder_b.bin (Bank B @ 0x08100000).
#
# Requires: ./tools/fetch-sdk.sh and source tools/build-env.sh (or bootstrap.sh).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SDK_ROOT="${SDK_ROOT:-$REPO_ROOT/external/linkit-sdk-v4.7.1}"

if [ ! -d "$SDK_ROOT" ]; then
    echo "ERROR: SDK not found at $SDK_ROOT (run ./tools/fetch-sdk.sh)" >&2
    exit 1
fi

# Reset SDK to known state, re-apply patches, then recreate app/board symlinks.
export FORCE_SDK_SYNC=1
# shellcheck disable=SC1091
source "$SCRIPT_DIR/build-env.sh"

SDK_PARENT="$(cd "$SDK_ROOT/.." && pwd)"
BUILD_OUT="${LINKIT_SDK_OUT:-$SDK_PARENT/LinkitSDK_OUT}"

echo "Cleaning SDK build objects after patch sync ..."
rm -rf "$BUILD_OUT/mt7682_hdk/petfeeder/obj"

cd "$SDK_ROOT"
./build.sh mt7682_hdk petfeeder bl

OUTPUT="$BUILD_OUT/mt7682_hdk/petfeeder"
FLASH_DIR="$REPO_ROOT/firmware/flash"
A_BIN="$OUTPUT/petfeeder.bin"
B_BIN="$OUTPUT/binary_B/petfeeder.bin"

if [ ! -f "$A_BIN" ]; then
    echo "ERROR: missing Bank A build: $A_BIN" >&2
    exit 1
fi

if [ ! -f "$B_BIN" ]; then
    echo "ERROR: missing Bank B build: $B_BIN" >&2
    echo "  Dual-image build should produce binary_B/petfeeder.bin" >&2
    exit 1
fi

for bin in mt7682_bootloader.bin mt768x_default_PerRate_TxPwr.bin; do
    if [ ! -f "$OUTPUT/$bin" ]; then
        echo "ERROR: missing $OUTPUT/$bin" >&2
        exit 1
    fi
    rm -f "$FLASH_DIR/$bin"
    cp -f "$OUTPUT/$bin" "$FLASH_DIR/$bin"
done

rm -f "$FLASH_DIR/petfeeder_a.bin" "$FLASH_DIR/petfeeder_b.bin" "$FLASH_DIR/petfeeder.bin"
cp -f "$A_BIN" "$FLASH_DIR/petfeeder_a.bin"
cp -f "$B_BIN" "$FLASH_DIR/petfeeder_b.bin"
ln -sf petfeeder_a.bin "$FLASH_DIR/petfeeder.bin"

echo ""
echo "Build output: $OUTPUT"
echo "  ${FLASH_DIR}/petfeeder_a.bin  (Bank A, 0x08012000)"
echo "  ${FLASH_DIR}/petfeeder_b.bin  (Bank B, 0x08100000)"
echo "  ${FLASH_DIR}/mt7682_bootloader.bin"
echo "  ${FLASH_DIR}/mt768x_default_PerRate_TxPwr.bin"
echo ""
echo "Flash package (load this .cfg in MediaTek IoT Flash Tool):"
echo "  ${FLASH_DIR}/flash_download.cfg"
