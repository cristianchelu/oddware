#!/usr/bin/env bash
# Single OTA hop: MQTT-ready check, publish inactive-bank image, wait for bank swap.
#
#   ./tools/ota/ota-hop.sh --device-id 768722
#   ./tools/ota/ota-hop.sh --device-id 768722 --firmware firmware/flash/petfeeder_b.bin
#
# Publish-only (same as mqtt-ota.sh --publish-only):
#   ./tools/ota/ota-hop.sh --device-id 768722 --publish-only [--serve-foreground]
#
# Called by ota-ab-roundtrip.sh with --run-dir, --http-log, --shared-uart.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FLASH_DIR="$REPO_ROOT/firmware/flash"

# shellcheck disable=SC1091
source "$SCRIPT_DIR/ota-logs.sh"
# shellcheck disable=SC1091
source "$SCRIPT_DIR/uart-capture.sh"
# shellcheck disable=SC1091
source "$SCRIPT_DIR/ota-common.sh"

ota_common_load_env "$SCRIPT_DIR"

DEVICE_ID=""
FIRMWARE=""
RUN_DIR=""
HTTP_LOG=""
SKIP_BUILD=0
SHARED_UART=0
MANAGE_HTTP=1
PUBLISH_ONLY=0
SERVE_FOREGROUND=0
UART_LOG=""

usage() {
    cat <<EOF
Usage: $(basename "$0") --device-id ID [options]

Options:
  --device-id ID         MQTT device id (required)
  --firmware PATH        Image to serve (default: inactive bank from state topic)
  --run-dir DIR          Log directory (default: new under tools/ota/logs/)
  --http-log FILE        Use existing HTTP log (implies --no-start-http)
  --no-start-http        Do not start/stop the range HTTP server
  --shared-uart          UART lock held by parent (ota-ab-roundtrip.sh)
  --publish-only         Publish OTA only; do not wait for completion
  --serve-foreground     With --publish-only: keep HTTP server until Ctrl+C
  --skip-build           Do not run build-firmware.sh when .bin is missing
  --hop-timeout SEC      Max wait for bank swap (default: $HOP_TIMEOUT)
  --progress-timeout SEC Stall limit (default: $PROGRESS_TIMEOUT)
  --uart DEV             Serial port (default: $UART_DEV)
  -h, --help             Show this help
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --device-id) DEVICE_ID="${2:?}"; shift 2 ;;
        --firmware) FIRMWARE="${2:?}"; shift 2 ;;
        --run-dir) RUN_DIR="${2:?}"; shift 2 ;;
        --http-log) HTTP_LOG="${2:?}"; MANAGE_HTTP=0; shift 2 ;;
        --no-start-http) MANAGE_HTTP=0; shift ;;
        --shared-uart) SHARED_UART=1; shift ;;
        --publish-only) PUBLISH_ONLY=1; shift ;;
        --serve-foreground) SERVE_FOREGROUND=1; shift ;;
        --skip-build) SKIP_BUILD=1; shift ;;
        --hop-timeout) HOP_TIMEOUT="${2:?}"; shift 2 ;;
        --progress-timeout) PROGRESS_TIMEOUT="${2:?}"; shift 2 ;;
        --uart) UART_DEV="${2:?}"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "error: unknown argument: $1" >&2; usage >&2; exit 1 ;;
    esac
done

if [ -z "$DEVICE_ID" ]; then
    echo "error: --device-id is required" >&2
    usage >&2
    exit 1
fi

EXIT_CODE=0
OTA_HTTP_PID=""
OWN_RUN_DIR=0
OWN_UART_LOCK=0

cleanup() {
    uart_capture_stop
    if [ "$OWN_UART_LOCK" -eq 1 ]; then
        uart_capture_release
    fi
    if [ "$MANAGE_HTTP" -eq 1 ]; then
        ota_common_stop_http
    fi
    if [ "$OWN_RUN_DIR" -eq 1 ] && [ -n "$RUN_DIR" ]; then
        ota_logs_maybe_cleanup "$RUN_DIR" "$EXIT_CODE"
    fi
}

on_signal() {
    EXIT_CODE=130
    cleanup
    exit "$EXIT_CODE"
}

trap cleanup EXIT
trap on_signal INT TERM HUP

LAN_IP="$(ota_common_pick_lan_ip)"

if [ -z "$FIRMWARE" ]; then
    FIRMWARE="$(ota_common_pick_firmware "$DEVICE_ID" "$FLASH_DIR")"
    echo "Inactive-bank image: $(basename "$FIRMWARE")"
fi

if [ ! -f "$FIRMWARE" ]; then
    if [ "$SKIP_BUILD" -eq 1 ]; then
        echo "error: firmware not found: $FIRMWARE" >&2
        exit 1
    fi
    echo "Building firmware..."
    "$SCRIPT_DIR/../build-firmware.sh"
fi

if [ ! -f "$FIRMWARE" ]; then
    echo "error: firmware not found: $FIRMWARE" >&2
    exit 1
fi

if [ -z "$RUN_DIR" ]; then
    RUN_DIR="$(ota_logs_new_run "$DEVICE_ID" "$REPO_ROOT")"
    OWN_RUN_DIR=1
fi

if [ -z "$HTTP_LOG" ]; then
    HTTP_LOG="$(ota_logs_http "$RUN_DIR")"
fi

if [ "$OWN_RUN_DIR" -eq 1 ]; then
    ota_logs_write_meta "$RUN_DIR" \
        "device_id=${DEVICE_ID}" \
        "mqtt_host=${MQTT_HOST}:${MQTT_PORT}" \
        "http=${LAN_IP}:${HTTP_PORT}" \
        "hop_timeout=${HOP_TIMEOUT}" \
        "progress_timeout=${PROGRESS_TIMEOUT}" \
        "uart_dev=${UART_DEV}"
    ota_common_reset_ota_status "$DEVICE_ID"
    echo "Log directory: ${RUN_DIR}"
fi

if [ "$SHARED_UART" -eq 0 ] && [ -e "$UART_DEV" ]; then
    if ! uart_capture_acquire "$UART_DEV"; then
        EXIT_CODE=1
        exit 1
    fi
    OWN_UART_LOCK=1
elif [ "$SHARED_UART" -eq 1 ] && [ -e "$UART_DEV" ]; then
    # Parent (ota-ab-roundtrip.sh) holds the flock; we only start/stop capture.
    UART_CAPTURE_DEV="$(readlink -f "$UART_DEV")"
fi

if [ "$MANAGE_HTTP" -eq 1 ]; then
    echo "Starting HTTP server (${FLASH_DIR})..."
    ota_common_start_http "$SCRIPT_DIR" "$FLASH_DIR" "$HTTP_LOG" "$LAN_IP"
fi

if [ "$PUBLISH_ONLY" -eq 1 ]; then
    ota_common_publish_ota "$DEVICE_ID" "$FIRMWARE" "$LAN_IP" "${RUN_DIR}/meta.txt"
    echo "Published. Watch petfeeder/${DEVICE_ID}/ota/status on the broker."
    if [ "$SERVE_FOREGROUND" -eq 1 ]; then
        echo "HTTP server stays up until you Ctrl+C."
        wait "$OTA_HTTP_PID"
    else
        echo "HTTP server pid ${OTA_HTTP_PID}; stop with: kill ${OTA_HTTP_PID}"
        trap - EXIT INT TERM HUP
    fi
    exit 0
fi

active="$(ota_common_read_bank "$DEVICE_ID")"
if [ "$active" = "?" ]; then
    echo "error: device not online on MQTT" >&2
    EXIT_CODE=1
    exit 1
fi
target="$([ "$active" = "A" ] && echo B || echo A)"

uart_log="$(ota_logs_hop_uart "$RUN_DIR" "$active" "$target")"
hop_meta="${RUN_DIR}/hop-${active}-to-${target}.txt"
{
    echo "hop=${active}->${target}"
    echo "firmware=$(basename "$FIRMWARE")"
    echo "started_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} >"$hop_meta"

if [ -e "$UART_DEV" ]; then
    uart_capture_start "$uart_log" "$$"
    UART_LOG="$uart_log"
    echo "  UART capture: $UART_DEV -> $UART_LOG (pid $UART_CAPTURE_PID)"
else
    echo "  UART capture: skipped ($UART_DEV not found)"
fi

echo ""
echo "=== OTA hop: ${active} -> ${target} ($(basename "$FIRMWARE")) ==="

wait_uart_args=()
if [ -n "$UART_LOG" ]; then
    wait_uart_args=(--uart-log "$UART_LOG")
elif [ -e "$UART_DEV" ]; then
    wait_uart_args=(--uart "$UART_DEV")
fi
if ! "$SCRIPT_DIR/wait-mqtt-online.sh" --device-id "$DEVICE_ID" \
    "${wait_uart_args[@]}" --timeout 120; then
    echo "error: device not MQTT-ready before OTA publish" >&2
    EXIT_CODE=1
    exit 1
fi

http_before="$(grep -c ' 206 ' "$HTTP_LOG" 2>/dev/null || true)"
http_before="${http_before:-0}"
t0=$(date +%s)
ota_common_publish_ota "$DEVICE_ID" "$FIRMWARE" "$LAN_IP" "${RUN_DIR}/meta.txt"

if ! "$SCRIPT_DIR/ota-wait-hop.sh" \
    --device-id "$DEVICE_ID" \
    --want-bank "$target" \
    --http-log "$HTTP_LOG" \
    --uart-log "$uart_log" \
    --hop-timeout "$HOP_TIMEOUT" \
    --progress-timeout "$PROGRESS_TIMEOUT" | tee -a "$hop_meta"; then
    echo "result=FAIL" >>"$hop_meta"
    echo "--- UART tail ($uart_log) ---"
    strings "$uart_log" | grep -E '^\[(ota|mqtt)\]' | tail -20 || true
    echo "--- HTTP tail ($HTTP_LOG) ---"
    tail -10 "$HTTP_LOG" || true
    EXIT_CODE=1
    exit 1
fi

t1=$(date +%s)
http_after="$(grep -c ' 206 ' "$HTTP_LOG" 2>/dev/null || true)"
http_after="${http_after:-0}"
hop_http=$((http_after - http_before))
{
    echo "ended_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "wall_seconds=$((t1 - t0))"
    echo "http_206_count=${hop_http}"
    echo "uart_log=${uart_log}"
    echo "result=OK"
} >>"$hop_meta"
echo "  hop log: ${hop_meta}"
echo "  OK: bank ${target} (${hop_http} HTTP ranges, $((t1 - t0))s)"

exit 0
