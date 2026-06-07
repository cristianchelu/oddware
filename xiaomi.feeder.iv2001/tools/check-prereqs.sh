#!/usr/bin/env bash
# Verify host or target build prerequisites.
#   ./tools/check-prereqs.sh host
#   ./tools/check-prereqs.sh target
set -euo pipefail

mode="${1:-host}"
missing=()

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        missing+=("$1")
    fi
}

case "$mode" in
    host)
        require_cmd git
        require_cmd make
        require_cmd gcc
        ;;
    sdk)
        require_cmd patch
        ;;
    target)
        require_cmd arm-none-eabi-gcc
        require_cmd arm-none-eabi-g++
        require_cmd arm-none-eabi-objcopy
        sysroot="$(arm-none-eabi-gcc -print-sysroot 2>/dev/null || true)"
        if [ -z "$sysroot" ] || [ ! -f "$sysroot/include/stdio.h" ]; then
            missing+=("arm-none-eabi newlib headers (stdio.h in sysroot)")
        fi
        ;;
    flash)
        require_cmd wine
        require_cmd wineserver
        require_cmd unzip
        ;;
    *)
        echo "Usage: $0 {host|sdk|target|flash}" >&2
        exit 2
        ;;
esac

if [ "${#missing[@]}" -gt 0 ]; then
    echo "ERROR: missing required tools for '$mode' setup:" >&2
    printf '  - %s\n' "${missing[@]}" >&2
    echo "" >&2
    case "$mode" in
        host)
            echo "Install host tools, then re-run bootstrap." >&2
            echo "  Fedora:  sudo dnf install git make gcc" >&2
            echo "  Debian:  sudo apt install git make gcc" >&2
            ;;
        sdk)
            echo "Install patch for SDK patch application (full bootstrap)." >&2
            echo "  Fedora:  sudo dnf install patch" >&2
            echo "  Debian:  sudo apt install patch" >&2
            ;;
        target)
            echo "Install the ARM GCC toolchain for firmware builds." >&2
            echo "  Fedora:  sudo dnf install arm-none-eabi-gcc-cs arm-none-eabi-gcc-cs-c++ arm-none-eabi-newlib" >&2
            echo "  Debian:  sudo apt install gcc-arm-none-eabi" >&2
            ;;
        flash)
            echo "Install Wine and helpers for Linux UART flashing." >&2
            echo "  Fedora:  sudo dnf install wine wineserver unzip" >&2
            echo "  Debian:  sudo apt install wine unzip" >&2
            ;;
    esac
    exit 1
fi

echo "Prerequisites OK ($mode)"
