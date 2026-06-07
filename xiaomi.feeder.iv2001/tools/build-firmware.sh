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
echo ""
echo "Build output: $OUTPUT"
echo "  ${OUTPUT}/petfeeder.bin"
echo "  ${OUTPUT}/mt7682_bootloader.bin"
echo "  ${OUTPUT}/mt768x_default_PerRate_TxPwr.bin"

if [ -f "$FIRMWARE_DIR/flash/flash_download.cfg" ]; then
    cp -f "$FIRMWARE_DIR/flash/flash_download.cfg" "$OUTPUT/flash_download.cfg"
    echo "  ${OUTPUT}/flash_download.cfg  (IV2001 2 MB layout)"
fi
