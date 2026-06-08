#!/usr/bin/env bash
# Run two back-to-back OTAs to validate A<->B fallback (inactive-bank image each hop).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# shellcheck disable=SC1091
source "$SCRIPT_DIR/ota-logs.sh"
# shellcheck disable=SC1091
source "$SCRIPT_DIR/uart-capture.sh"
# shellcheck disable=SC1091
source "$SCRIPT_DIR/ota-common.sh"

ota_common_load_env "$SCRIPT_DIR"

DEVICE_ID="${1:-768722}"
EXIT_CODE=0
RUN_DIR=""
HTTP_LOG=""
OTA_HTTP_PID=""

cleanup() {
    uart_capture_release
    ota_common_stop_http
    if [ -n "$RUN_DIR" ]; then
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
RUN_DIR="$(ota_logs_new_run "$DEVICE_ID" "$REPO_ROOT")"
HTTP_LOG="$(ota_logs_http "$RUN_DIR")"

if [ -e "$UART_DEV" ]; then
    if ! uart_capture_acquire "$UART_DEV"; then
        EXIT_CODE=1
        exit 1
    fi
fi

ota_logs_write_meta "$RUN_DIR" \
    "device_id=${DEVICE_ID}" \
    "mqtt_host=${MQTT_HOST}:${MQTT_PORT}" \
    "http=${LAN_IP}:${HTTP_PORT}" \
    "hop_timeout=${HOP_TIMEOUT}" \
    "progress_timeout=${PROGRESS_TIMEOUT}" \
    "uart_dev=${UART_DEV}"

ota_common_reset_ota_status "$DEVICE_ID"

echo "Device: ${DEVICE_ID}  broker: ${MQTT_HOST}:${MQTT_PORT}  HTTP: ${LAN_IP}:${HTTP_PORT}"
echo "Timeouts: hop=${HOP_TIMEOUT}s stall=${PROGRESS_TIMEOUT}s"
echo "Log directory: ${RUN_DIR}"
echo "Starting HTTP server (${REPO_ROOT}/firmware/flash)..."
ota_common_start_http "$SCRIPT_DIR" "${REPO_ROOT}/firmware/flash" "$HTTP_LOG" "$LAN_IP"

boot_uart="$(ota_logs_hop_uart "$RUN_DIR" "boot" "wait")"
uart_capture_start "$boot_uart" "$$"
if ! "$SCRIPT_DIR/wait-mqtt-online.sh" --device-id "$DEVICE_ID" \
    --uart-log "$boot_uart" --timeout 120; then
    echo "error: device not MQTT-ready" >&2
    EXIT_CODE=1
    exit 1
fi
uart_capture_stop

start_bank="$(ota_common_read_bank "$DEVICE_ID")"
echo "Current bank: ${start_bank}"
echo "start_bank=${start_bank}" >>"${RUN_DIR}/meta.txt"

hop_args=(
    --device-id "$DEVICE_ID"
    --run-dir "$RUN_DIR"
    --http-log "$HTTP_LOG"
    --no-start-http
    --shared-uart
    --skip-build
    --hop-timeout "$HOP_TIMEOUT"
    --progress-timeout "$PROGRESS_TIMEOUT"
    --uart "$UART_DEV"
)

"$SCRIPT_DIR/ota-hop.sh" "${hop_args[@]}" || EXIT_CODE=1
"$SCRIPT_DIR/ota-hop.sh" "${hop_args[@]}" || EXIT_CODE=1

end_bank="$(ota_common_read_bank "$DEVICE_ID")"
echo ""
echo "Round-trip complete: ${start_bank} -> ... -> ${end_bank}"
echo "end_bank=${end_bank}" >>"${RUN_DIR}/meta.txt"
echo "exit_code=${EXIT_CODE}" >>"${RUN_DIR}/meta.txt"

if [ "$EXIT_CODE" -eq 0 ] && [ "$start_bank" = "$end_bank" ]; then
    echo "SUCCESS: both directions validated (back on bank ${end_bank})"
else
    if [ "$EXIT_CODE" -ne 0 ]; then
        echo "FAILED: see logs under ${RUN_DIR}" >&2
    else
        echo "PARTIAL: ended on bank ${end_bank} (started ${start_bank})"
    fi
fi

exit "$EXIT_CODE"
