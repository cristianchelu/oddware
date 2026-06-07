#!/usr/bin/env bash
# Build bootloader + application via LinkIt SDK build.sh.
#
#   ./tools/build-firmware.sh
#
# Requires: ./tools/fetch-sdk.sh and source tools/build-env.sh (or bootstrap.sh).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# shellcheck disable=SC1091
source "$SCRIPT_DIR/build-env.sh"

cd "$SDK_ROOT"
./build.sh mt7682_hdk petfeeder bl

OUTPUT="$SDK_ROOT/out/mt7682_hdk/petfeeder"
FLASH_DIR="$FIRMWARE_DIR/flash"

# MediaTek Flash Tool resolves rom `file:` paths relative to the .cfg directory.
# Keep symlinks next to the committed cfg so the tool finds the build artifacts.
for bin in mt7682_bootloader.bin petfeeder.bin mt768x_default_PerRate_TxPwr.bin; do
    if [ ! -f "$OUTPUT/$bin" ]; then
        rm -f "$FLASH_DIR/$bin"
        echo "ERROR: missing $OUTPUT/$bin" >&2
        if [ "$bin" = "mt7682_bootloader.bin" ]; then
            echo "  Flash Tool shows BootLoader in red when this file is absent." >&2
        fi
        exit 1
    fi
    ln -sfn "$OUTPUT/$bin" "$FLASH_DIR/$bin"
done

echo ""
echo "Build output: $OUTPUT"
echo "  ${OUTPUT}/petfeeder.bin"
echo "  ${OUTPUT}/mt7682_bootloader.bin"
echo "  ${OUTPUT}/mt768x_default_PerRate_TxPwr.bin"
echo ""
echo "Flash package (load this .cfg in MediaTek IoT Flash Tool):"
echo "  ${FLASH_DIR}/flash_download.cfg"
