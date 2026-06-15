#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SDK_DIR="$REPO_ROOT/external/linkit-sdk-v4.6.2-houndify"

SDK_REPO="https://github.com/luvinland/linkit-sdk-v4.6.2-houndify.git"
SDK_BRANCH="master"

if [ -d "$SDK_DIR/.git" ]; then
    echo "SDK already present at $SDK_DIR"
    echo "To update: cd $SDK_DIR && git pull"
    exit 0
fi

echo "Cloning LinkIt SDK v4.6.2 into $SDK_DIR ..."
echo "  repo:   $SDK_REPO"
echo "  branch: $SDK_BRANCH"
echo ""

git clone --depth 1 --branch "$SDK_BRANCH" "$SDK_REPO" "$SDK_DIR"

echo ""
echo "Done. SDK available at: $SDK_DIR"
echo ""
echo "Next:"
echo "  source tools/build-env.sh"
echo "Or run full setup:"
echo "  ./tools/bootstrap.sh"
