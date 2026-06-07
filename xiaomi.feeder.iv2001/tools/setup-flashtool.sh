#!/usr/bin/env bash
# One-time setup: extract MediaTek IoT Flash Tool, init Wine prefix, write FlashTool.ini.
#
#   ./tools/setup-flashtool.sh
#   ./tools/setup-flashtool.sh --force
#
# Requires LinkIt SDK (./tools/bootstrap.sh or ./tools/fetch-sdk.sh).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
FORCE=0

for arg in "$@"; do
    case "$arg" in
        --force) FORCE=1 ;;
        -h|--help)
            echo "Usage: $0 [--force]"
            echo ""
            echo "  --force   Re-extract PC_tool.zip and regenerate FlashTool.ini"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            exit 2
            ;;
    esac
done

# shellcheck disable=SC1091
source "$SCRIPT_DIR/lib/flashtool-wine.sh"

cd "$REPO_ROOT"

"$SCRIPT_DIR/check-prereqs.sh" flash

resolve_paths

if [ ! -f "$SDK_PC_TOOL_ZIP" ]; then
    echo "error: SDK PC_tool.zip not found at $SDK_PC_TOOL_ZIP" >&2
    echo "Run: ./tools/bootstrap.sh" >&2
    exit 1
fi

if [ "$FORCE" -eq 1 ]; then
    rm -rf "$FT_WIN"
    rm -rf "$WINEPREFIX"
fi

ensure_flashtool_extracted
stop_wineserver
ensure_wine_prefix
write_flashtool_ini

echo ""
echo "Smoke-testing coda.exe ..."
cd "$FT_WIN"
set +e
wine ./coda.exe 2>&1 | head -5
coda_status=$?
set -e
stop_wineserver

if [ "$coda_status" -gt 1 ]; then
    echo "error: coda.exe smoke test failed (exit $coda_status)" >&2
    exit 1
fi

echo ""
echo "IoT Flash Tool ready:"
echo "  $FT_WIN"
echo "  WINEPREFIX=$WINEPREFIX"
echo ""
echo "Flash firmware:"
echo "  ./tools/iot-flash.sh download /dev/ttyUSB0"
echo "  ./tools/iot-flash.sh gui /dev/ttyUSB0"
