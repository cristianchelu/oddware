#!/usr/bin/env bash
# Bench UART capture at 115200 8N1 (boot log / MiniCLI).
#
#   ./tools/uart-console.sh /dev/ttyUSB0
#   ./tools/uart-console.sh -l /dev/ttyUSB0
#   UART_CONSOLE_SECS=10 ./tools/uart-console.sh -l /dev/ttyUSB0
#
# Interactive: install picocom (dnf install picocom) or use dump mode above.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LOG_BASE="${OTA_LOG_DIR:-${SCRIPT_DIR}/ota/logs}"

usage() {
    echo "Usage: $0 [-l] /dev/ttyDEVICE [baud]"
    echo "  -l, --log          Write to tools/ota/logs/uart-console-<UTC>.log"
    echo "  UART_CONSOLE_SECS=N  capture for N seconds (default 30 in non-tty stdin)"
    exit "${1:-0}"
}

ENABLE_LOG=0
POSITIONAL=()

while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help) usage 0 ;;
        -l|--log) ENABLE_LOG=1; shift ;;
        --) shift; POSITIONAL+=("$@"); break ;;
        -*) echo "error: unknown option: $1" >&2; usage 1 ;;
        *) POSITIONAL+=("$1"); shift ;;
    esac
done

if [ ${#POSITIONAL[@]} -lt 1 ]; then
    usage 0
fi

TTY="$(readlink -f "${POSITIONAL[0]}")"
BAUD="${POSITIONAL[1]:-${UART_BAUD:-115200}}"
SECS="${UART_CONSOLE_SECS:-}"
LOG_FILE=""

if [ "$ENABLE_LOG" -eq 1 ]; then
    mkdir -p "$LOG_BASE"
    LOG_FILE="${LOG_BASE}/uart-console-$(date -u +%Y%m%dT%H%M%SZ).log"
    : >"$LOG_FILE"
fi

stty -F "$TTY" "$BAUD" cs8 -cstopb -parenb raw -echo min 0 time 1
echo "uart-console: $TTY @ $BAUD" >&2
if [ -n "$LOG_FILE" ]; then
    echo "uart-console: logging to $LOG_FILE" >&2
fi

if [ -n "$SECS" ] || [ ! -t 0 ]; then
    if [ -n "$LOG_FILE" ]; then
        exec timeout "${SECS:-30}" cat "$TTY" | tee "$LOG_FILE"
    fi
    exec timeout "${SECS:-30}" cat "$TTY"
fi

if command -v picocom >/dev/null 2>&1; then
    picocom_args=(-b "$BAUD")
    if [ -n "$LOG_FILE" ]; then
        picocom_args+=(--logfile "$LOG_FILE")
    fi
    exec picocom "${picocom_args[@]}" "$TTY"
fi

echo "error: picocom not found — use dump mode: UART_CONSOLE_SECS=30 $0 $TTY" >&2
echo "  Fedora: sudo dnf install picocom" >&2
exit 1
