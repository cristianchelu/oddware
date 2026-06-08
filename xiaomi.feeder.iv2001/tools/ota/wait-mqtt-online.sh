#!/usr/bin/env bash
# Wait until the device has finished boot and is listening on MQTT.
#
# Retained state/online:true is not enough — it survives power loss until the
# device reconnects. Prefer UART "[mqtt] subscribed" when a serial port is given.
#
#   ./tools/ota/wait-mqtt-online.sh --device-id 768722
#   ./tools/ota/wait-mqtt-online.sh --device-id 768722 --uart /dev/ttyUSB0
#
# Environment: MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS (or tools/ota/.env)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "$SCRIPT_DIR/uart-capture.sh"
if [ -f "$SCRIPT_DIR/.env" ]; then
    # shellcheck disable=SC1091
    source "$SCRIPT_DIR/.env"
fi

MQTT_HOST="${MQTT_HOST:-192.168.100.52}"
MQTT_PORT="${MQTT_PORT:-1883}"
MQTT_USER="${MQTT_USER:-petfeeder-flasher}"
MQTT_PASS="${MQTT_PASS:-petfeeder}"
DEVICE_ID="${DEVICE_ID:-768722}"
UART_DEV="${UART_DEV:-}"
UART_LOG_IN=""
TIMEOUT="${TIMEOUT:-120}"
POLL_INTERVAL="${POLL_INTERVAL:-2}"

usage() {
    cat <<EOF
Usage: $(basename "$0") [options]

Options:
  --device-id ID   MQTT device id (default: $DEVICE_ID)
  --uart DEV       Open serial port and capture boot log (recommended)
  --uart-log FILE  Poll an existing UART capture (do not open the port)
  --timeout SEC    Max wait (default: $TIMEOUT)
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --device-id) DEVICE_ID="${2:?}"; shift 2 ;;
        --uart) UART_DEV="${2:?}"; shift 2 ;;
        --uart-log) UART_LOG_IN="${2:?}"; shift 2 ;;
        --timeout) TIMEOUT="${2:?}"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "error: unknown argument: $1" >&2; usage >&2; exit 1 ;;
    esac
done

STATE_TOPIC="petfeeder/${DEVICE_ID}/state"
UART_LOG=""
UART_OWNS_PORT=0

cleanup() {
    if [ "$UART_OWNS_PORT" -eq 1 ]; then
        uart_capture_release
    else
        uart_capture_stop
    fi
    if [ -n "${UART_LOG:-}" ] && [[ "$UART_LOG" == /tmp/wait-mqtt-uart-* ]]; then
        rm -f "$UART_LOG"
    fi
}
trap cleanup EXIT INT TERM HUP

read_bank() {
    local state
    state="$(mosquitto_sub -h "$MQTT_HOST" -p "$MQTT_PORT" \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "$STATE_TOPIC" -C 1 -W 3 2>/dev/null || true)"
    case "$state" in
        *'"bank": "B"'*|*'"bank":"B"'*) echo "B" ;;
        *'"bank": "A"'*|*'"bank":"A"'*) echo "A" ;;
        *'"online": true'*|*'"online":true'*) echo "?" ;;
        *'"online": false'*|*'"online":false'*) echo "offline" ;;
        *) echo "?" ;;
    esac
}

uart_log_path() {
    if [ -n "$UART_LOG_IN" ]; then
        echo "$UART_LOG_IN"
    else
        echo "${UART_LOG:-}"
    fi
}

uart_mqtt_ready() {
    local log
    log="$(uart_log_path)"
    if [ -z "$log" ] || [ ! -f "$log" ]; then
        return 1
    fi
    strings "$log" 2>/dev/null | grep -qE '\[mqtt\] subscribed '
}

uart_boot_seen() {
    local log
    log="$(uart_log_path)"
    if [ -z "$log" ] || [ ! -f "$log" ]; then
        return 1
    fi
    strings "$log" 2>/dev/null | grep -qE 'Leaving the BROM|FreeRTOS Running'
}

start_uart_capture() {
    if [ -n "$UART_LOG_IN" ]; then
        return
    fi
    if [ -z "$UART_DEV" ] || [ ! -e "$UART_DEV" ]; then
        return
    fi
    if ! uart_capture_acquire "$UART_DEV"; then
        exit 1
    fi
    UART_OWNS_PORT=1
    UART_LOG="$(mktemp /tmp/wait-mqtt-uart-XXXXXX.log)"
    uart_capture_start "$UART_LOG" "$$"
}

deadline=$((SECONDS + TIMEOUT))
start_uart_capture

echo "waiting for MQTT ready (device ${DEVICE_ID}, max ${TIMEOUT}s)..."

while [ "$SECONDS" -lt "$deadline" ]; do
    if uart_mqtt_ready; then
        bank="$(read_bank)"
        echo "  ready: UART subscribed, broker bank=${bank}"
        sleep 2
        exit 0
    fi

    bank="$(read_bank)"
    if [ "$bank" = "A" ] || [ "$bank" = "B" ]; then
        # No UART: broker bank field means a live publish happened.
        if [ -z "$UART_DEV" ] || [ ! -e "$UART_DEV" ]; then
            echo "  ready: broker bank=${bank} (no UART)"
            exit 0
        fi
        # With UART but boot in progress, retained state may lie — keep waiting.
        if ! uart_boot_seen; then
            echo "  ready: broker bank=${bank}, no boot log (already online)"
            exit 0
        fi
    fi

    sleep "$POLL_INTERVAL"
done

bank="$(read_bank)"
echo "error: MQTT not ready within ${TIMEOUT}s (broker bank=${bank})" >&2
exit 1
