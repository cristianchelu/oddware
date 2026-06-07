#!/usr/bin/env bash
# Bench UART capture at 115200 8N1 (boot log / MiniCLI).
#
#   ./tools/uart-console.sh /dev/ttyUSB0
#   UART_CONSOLE_SECS=10 ./tools/uart-console.sh /dev/ttyUSB0
#
# Interactive: install picocom (dnf install picocom) or use dump mode above.
set -euo pipefail

if [ $# -lt 1 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Usage: $0 /dev/ttyDEVICE [baud]"
    echo "  UART_CONSOLE_SECS=N  capture for N seconds (default 30 in non-tty stdin)"
    exit 0
fi

TTY="$(readlink -f "$1")"
BAUD="${2:-${UART_BAUD:-115200}}"
SECS="${UART_CONSOLE_SECS:-}"

stty -F "$TTY" "$BAUD" cs8 -cstopb -parenb raw -echo min 0 time 1
echo "uart-console: $TTY @ $BAUD" >&2

if [ -n "$SECS" ] || [ ! -t 0 ]; then
    exec timeout "${SECS:-30}" cat "$TTY"
fi

if command -v picocom >/dev/null 2>&1; then
    exec picocom -b "$BAUD" "$TTY"
fi

echo "error: picocom not found — use dump mode: UART_CONSOLE_SECS=30 $0 $TTY" >&2
echo "  Fedora: sudo dnf install picocom" >&2
exit 1
