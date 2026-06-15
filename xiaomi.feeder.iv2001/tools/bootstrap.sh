#!/usr/bin/env bash
# First-time setup and smoke verification for new clones.
#
#   ./tools/bootstrap.sh              # host tests + fetch SDK + build-env
#   ./tools/bootstrap.sh --host-only    # host tests only (no SDK clone)
#
# Run from anywhere inside xiaomi.feeder.iv2001/.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
HOST_ONLY=0
WITH_FLASH_TOOL=0

for arg in "$@"; do
    case "$arg" in
        --host-only) HOST_ONLY=1 ;;
        --with-flash-tool) WITH_FLASH_TOOL=1 ;;
        -h|--help)
            echo "Usage: $0 [--host-only] [--with-flash-tool]"
            echo ""
            echo "  --host-only        Run host unit tests only; skip SDK fetch and build-env."
            echo "  --with-flash-tool  After SDK fetch, set up Wine IoT Flash Tool (Linux UART flash)."
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            exit 2
            ;;
    esac
done

cd "$REPO_ROOT"

echo "==> Checking host prerequisites"
"$SCRIPT_DIR/check-prereqs.sh" host

echo ""
echo "==> Running host unit tests"
make -C firmware/test test-host

if [ "$HOST_ONLY" -eq 1 ]; then
    echo ""
    echo "Host setup verified. To prepare firmware builds:"
    echo "  ./tools/bootstrap.sh          # includes SDK fetch"
    echo "  source tools/build-env.sh     # per-shell PATH for arm-none-eabi-gcc"
    exit 0
fi

echo ""
echo "==> Checking SDK prerequisites"
"$SCRIPT_DIR/check-prereqs.sh" sdk

echo ""
echo "==> Fetching Airoha IoT SDK v4.7.1 (large clone, one-time)"
"$SCRIPT_DIR/fetch-sdk.sh"

echo ""
echo "==> Configuring SDK app bridge, board symlinks, and patches"
(
    # shellcheck disable=SC1091
    source "$SCRIPT_DIR/build-env.sh"
)

echo ""
echo "==> Checking target toolchain"
"$SCRIPT_DIR/check-prereqs.sh" target

if [ "$WITH_FLASH_TOOL" -eq 1 ]; then
    echo ""
    echo "==> Setting up IoT Flash Tool (Wine)"
    "$SCRIPT_DIR/setup-flashtool.sh"
fi

echo ""
echo "==> Installing git hooks (SDK patch re-sync on checkout/merge)"
"$SCRIPT_DIR/install-git-hooks.sh"

echo ""
echo "Setup complete."
echo ""
echo "Next steps:"
echo "  make test-host                  # re-run host tests anytime"
echo "  source tools/build-env.sh       # per-shell ARM toolchain PATH"
echo "  ./tools/build-firmware.sh       # once GCC/Makefile exists (Step 1)"
if [ "$WITH_FLASH_TOOL" -eq 1 ]; then
    echo "  ./tools/iot-flash.sh download /dev/ttyUSB0   # UART flash (manual reset prompt)"
else
    echo "  ./tools/bootstrap.sh --with-flash-tool         # optional Linux UART flash tooling"
fi
echo ""
echo "Read AGENTS.md for spec-driven TDD workflow."
