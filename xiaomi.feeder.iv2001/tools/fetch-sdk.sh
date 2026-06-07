#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SDK_DIR="$REPO_ROOT/external/airoha-iot-sdk"

SDK_REPO="https://github.com/Kamwing1992/SDK_For_MT5932-MT7682-MT7686-MT7687-MT7697-AW7698.git"
SDK_BRANCH="main"

if [ -d "$SDK_DIR/.git" ]; then
    echo "SDK already present at $SDK_DIR"
    echo "To update: cd $SDK_DIR && git pull"
    exit 0
fi

echo "Cloning Airoha IoT SDK into $SDK_DIR ..."
echo "  repo:   $SDK_REPO"
echo "  branch: $SDK_BRANCH"
echo ""

git clone --depth 1 --branch "$SDK_BRANCH" "$SDK_REPO" "$SDK_DIR"

echo ""
echo "Done. SDK available at: $SDK_DIR"
echo "Set SDK_ROOT=$SDK_DIR in your environment or use tools/build-env.sh"
