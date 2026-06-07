#!/usr/bin/env bash
# MediaTek IoT Flash Tool on Linux (Wine + CODA), manual reset prompt.
#
#   ./tools/iot-flash.sh download /dev/ttyUSB0
#   ./tools/iot-flash.sh probe /dev/ttyUSB0
#   ./tools/uart-console.sh /dev/ttyUSB0
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ -f "$SCRIPT_DIR/iot-flash.env" ]; then
    # shellcheck disable=SC1091
    source "$SCRIPT_DIR/iot-flash.env"
fi

FLASH_DIR="$REPO_ROOT/firmware/flash"

# shellcheck disable=SC1091
source "$SCRIPT_DIR/lib/flashtool-wine.sh"

FORMAT=0
CFG_PATH=""

usage() {
    cat <<EOF
Usage:
  $(basename "$0") gui DEVICE [flash-tool-args...]
  $(basename "$0") download DEVICE [--cfg PATH] [--format]
  $(basename "$0") probe DEVICE

Bench UART: ./tools/uart-console.sh DEVICE

Starts the flash tool, then prompts you to reset the feeder (power-cycle or
pulse TP15). Timeout: IOT_FLASH_RESET_WAIT_SEC (default 30) in tools/iot-flash.env.

Linux/Wine supports download and BROM probe only. Full-chip readback is not
available headlessly — use IoT Flash Tool on Windows (see spec/10-hardware/flash.md).
EOF
}

resolve_flash_cfg() {
    if [ -n "${IOT_FLASH_CFG:-}" ]; then
        printf '%s\n' "$IOT_FLASH_CFG"
        return 0
    fi

    local fw_cfg="$FLASH_DIR/flash_download.cfg"
    if [ -f "$fw_cfg" ] && [ -f "$FLASH_DIR/petfeeder.bin" ]; then
        printf '%s\n' "$fw_cfg"
        return 0
    fi

    echo "error: flash_download.cfg not found — build firmware first:" >&2
    echo "  source tools/build-env.sh && ./tools/build-firmware.sh" >&2
    return 1
}

cmd_probe() {
    local tty="$1"
    local wait_sec="${IOT_FLASH_RESET_WAIT_SEC:-30}"
    local deadline remaining i ok=0 resp

    echo "tty=$tty  probe_window=${wait_sec}s"
    echo ""
    printf '─%.0s' {1..60}
    echo
    echo "  Reset the feeder (power-cycle or pulse TP15), probing for ${wait_sec}s…"
    printf '─%.0s' {1..60}
    echo ""

    exec 3<>"$tty"
    stty -F "$tty" 115200 cs8 -cstopb -parenb raw -echo min 0 time 1

    deadline=$((SECONDS + wait_sec))
    while [ "$SECONDS" -lt "$deadline" ]; do
        remaining=$((deadline - SECONDS))
        printf '\r  %2ds remaining… ' "$remaining"
        for ((i = 0; i < 20; i++)); do
            printf '\xa0' >&3
            if IFS= read -r -n 1 -t 0.02 -u 3 resp; then
                if [ "$resp" = $'\x5f' ]; then
                    ok=1
                    break 2
                fi
            fi
        done
    done
    echo ""
    exec 3<&-

    if [ "$ok" -eq 1 ]; then
        echo "BROM sync: PASS (0x5F)"
        return 0
    fi
    echo "BROM sync: FAIL (no 0x5F) — check TX→TP2, RX→TP1, GND→TP27; reset during window" >&2
    return 1
}

cmd_gui() {
    local tty
    local -a ft_args=()
    tty="$(resolve_tty_device "$1")"
    shift

    while [ $# -gt 0 ]; do
        case "$1" in
            -h|--help) usage; exit 0 ;;
            *) ft_args+=("$1") ;;
        esac
        shift
    done

    warn_dialout
    run_with_manual_reset "$tty" wine FlashTool.exe "${ft_args[@]}"
}

cmd_download() {
    local tty
    local -a coda_args=()

    tty="$(resolve_tty_device "$1")"
    shift

    while [ $# -gt 0 ]; do
        case "$1" in
            --cfg) CFG_PATH="${2:-}"; shift ;;
            --format) FORMAT=1 ;;
            -h|--help) usage; exit 0 ;;
            *) echo "error: unknown download option: $1" >&2; exit 2 ;;
        esac
        shift
    done

    [ -n "$CFG_PATH" ] || CFG_PATH="$(resolve_flash_cfg)"
    [ -f "$CFG_PATH" ] || { echo "error: cfg not found: $CFG_PATH" >&2; exit 1; }

    stage_flash_package "$CFG_PATH"
    if [ "$FORMAT" -eq 1 ]; then
        coda_args=(./coda.exe flash_download.cfg --UART "$COM_NAME" -f -d --reset)
    else
        coda_args=(./coda.exe flash_download.cfg --UART "$COM_NAME" -d --reset)
    fi

    warn_dialout
    local status=0
    run_with_manual_reset "$tty" wine "${coda_args[@]}" || status=$?
    return "$status"
}

main() {
    if [ $# -eq 0 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
        usage
        exit 0
    fi
    [ $# -ge 2 ] || { usage; exit 2; }

    local subcmd="$1"
    shift

    resolve_paths
    command -v wine >/dev/null 2>&1 || { echo "error: wine not found" >&2; exit 1; }
    ensure_flashtool_extracted

    case "$subcmd" in
        gui) cmd_gui "$@" ;;
        download) cmd_download "$@" ;;
        readback)
            echo "error: CODA readback is not supported on Linux/Wine." >&2
            echo "  Use IoT Flash Tool on Windows for full-chip readback." >&2
            exit 1
            ;;
        probe) cmd_probe "$(resolve_tty_device "$1")" ;;
        *) echo "error: unknown subcommand: $subcmd" >&2; usage; exit 2 ;;
    esac
}

main "$@"
