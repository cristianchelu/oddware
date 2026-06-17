#!/usr/bin/env bash
# Remote MiniCLI over minimal telnet (spec/30-processes/uart-console.md § Remote telnet).
#
#   ./tools/remote-console.sh 192.168.1.42
#   telnet 192.168.1.42 2323
#   REMOTE_CLI_PORT=2323 ./tools/remote-console.sh 192.168.1.42
#
# Disconnect: type exit at the prompt, or press Ctrl+C.
# Requires nc, telnet, or socat. Device must have REMOTE_CLI_ENABLE=y and STA IP.
set -euo pipefail

PORT="${REMOTE_CLI_PORT:-2323}"

if [ $# -lt 1 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Usage: $0 <device-ip> [port]"
    echo "  REMOTE_CLI_PORT  default 2323"
    echo ""
    echo "Disconnect: type exit at the MiniCLI prompt, or press Ctrl+C."
    echo "Avoid typing on UART0 while connected (preempts telnet)."
    echo ""
    echo "At the \$ prompt: ? lists commands; <group> ? lists subcommands."
    exit 0
fi

HOST="$1"
if [ $# -ge 2 ]; then
    PORT="$2"
fi

echo "remote-console: ${HOST}:${PORT}" >&2
echo "disconnect: type exit, or press Ctrl+C" >&2
echo "tip: avoid UART0 input while connected (switches CLI back to serial)" >&2

# Prefer clients that exit when the device closes the socket after 'exit'.
if command -v nc >/dev/null 2>&1; then
    if nc -h 2>&1 | grep -q -- '-N'; then
        exec nc -N "$HOST" "$PORT"
    fi
    if nc -h 2>&1 | grep -q -- '-q'; then
        exec nc -q 0 "$HOST" "$PORT"
    fi
    exec nc "$HOST" "$PORT"
fi

if command -v telnet >/dev/null 2>&1; then
    exec telnet "$HOST" "$PORT"
fi

if command -v socat >/dev/null 2>&1; then
    exec socat -,raw,echo=0,crnl "TCP:${HOST}:${PORT}"
fi

echo "error: install nc, telnet, or socat" >&2
exit 1
