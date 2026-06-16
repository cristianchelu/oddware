#!/usr/bin/env bash
# Wait for an OTA hop to finish (bank swap + reconnect).
#
# Fails fast if download makes no progress for PROGRESS_TIMEOUT seconds.
# Absolute ceiling is HOP_TIMEOUT seconds.
#
#   ./tools/ota/ota-wait-hop.sh --want-bank B --http-log /tmp/http.log
#   ./tools/ota/ota-wait-hop.sh --want-bank A --http-log /tmp/http.log --uart-log /tmp/uart.log
#
# Environment: MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS (or tools/ota/.env)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f "$SCRIPT_DIR/.env" ]; then
    # shellcheck disable=SC1091
    source "$SCRIPT_DIR/.env"
fi

MQTT_HOST="${MQTT_HOST:-192.168.100.52}"
MQTT_PORT="${MQTT_PORT:-1883}"
MQTT_USER="${MQTT_USER:-petfeeder-flasher}"
MQTT_PASS="${MQTT_PASS:-petfeeder}"
HOP_TIMEOUT="${HOP_TIMEOUT:-300}"
PROGRESS_TIMEOUT="${PROGRESS_TIMEOUT:-90}"
REBOOT_GRACE_TIMEOUT="${REBOOT_GRACE_TIMEOUT:-180}"
POLL_INTERVAL="${POLL_INTERVAL:-5}"

DEVICE_ID=""
WANT_BANK=""
HTTP_LOG=""
UART_LOG=""

usage() {
    cat <<EOF
Usage: $(basename "$0") --device-id ID --want-bank A|B --http-log FILE [options]

Options:
  --device-id ID       MQTT device id (default: 768722)
  --want-bank A|B      Expected bank after OTA
  --http-log FILE      Range HTTP server log (counts 206 GETs as progress)
  --uart-log FILE      Optional UART capture (parses [ota] NN% tagged lines)
  --hop-timeout SEC    Absolute max wait (default: $HOP_TIMEOUT)
  --progress-timeout SEC  Stall limit with no progress (default: $PROGRESS_TIMEOUT)
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --device-id) DEVICE_ID="${2:?}"; shift 2 ;;
        --want-bank) WANT_BANK="${2:?}"; shift 2 ;;
        --http-log) HTTP_LOG="${2:?}"; shift 2 ;;
        --uart-log) UART_LOG="${2:?}"; shift 2 ;;
        --hop-timeout) HOP_TIMEOUT="${2:?}"; shift 2 ;;
        --progress-timeout) PROGRESS_TIMEOUT="${2:?}"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "error: unknown argument: $1" >&2; usage >&2; exit 1 ;;
    esac
done

DEVICE_ID="${DEVICE_ID:-768722}"

if [ -z "$WANT_BANK" ] || [ -z "$HTTP_LOG" ]; then
    echo "error: --want-bank and --http-log are required" >&2
    usage >&2
    exit 1
fi

if [ "$WANT_BANK" != "A" ] && [ "$WANT_BANK" != "B" ]; then
    echo "error: --want-bank must be A or B" >&2
    exit 1
fi

STATE_TOPIC="petfeeder/${DEVICE_ID}/state"
OTA_STATUS_TOPIC="petfeeder/${DEVICE_ID}/ota/status"
last_ota_status=""

read_bank() {
    local status
    status="$(mosquitto_sub -h "$MQTT_HOST" -p "$MQTT_PORT" \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "$OTA_STATUS_TOPIC" -C 1 -W 3 2>/dev/null || true)"
    case "$status" in
        *'"bank":"B"'*|*'"bank": "B"'*) echo "B" ;;
        *'"bank":"A"'*|*'"bank": "A"'*) echo "A" ;;
        *) echo "?" ;;
    esac
}

http_hits() {
    local count=0
    if [ ! -f "$HTTP_LOG" ]; then
        echo 0
        return
    fi
    count="$(grep -c ' 206 ' "$HTTP_LOG" 2>/dev/null || true)"
    echo "${count:-0}"
}

uart_pct() {
    local pct=-1
    if [ -z "$UART_LOG" ] || [ ! -f "$UART_LOG" ]; then
        echo -1
        return
    fi
    pct="$(strings "$UART_LOG" 2>/dev/null \
        | grep -oE '\[ota\] [0-9]+%' \
        | tail -1 \
        | grep -oE '[0-9]+' || true)"
    echo "${pct:--1}"
}

uart_ota_started() {
    if [ -z "$UART_LOG" ] || [ ! -f "$UART_LOG" ]; then
        return 1
    fi
    strings "$UART_LOG" 2>/dev/null | grep -qE '\[mqtt\] cmd ota topic=|\[ota\] download started'
}

uart_reboot_phase() {
    if [ -z "$UART_LOG" ] || [ ! -f "$UART_LOG" ]; then
        return 1
    fi
    strings "$UART_LOG" 2>/dev/null \
        | grep -qE 'bank swap pending|download complete|\[ota\] 100%'
}

deadline=$((SECONDS + HOP_TIMEOUT))
last_progress_at=$SECONDS
http_baseline="$(http_hits)"
last_http="$http_baseline"
last_pct=-1
saw_activity=0
saw_uart_start=0
reboot_phase=0

echo "waiting for bank ${WANT_BANK} (hop max ${HOP_TIMEOUT}s, stall ${PROGRESS_TIMEOUT}s, reboot_grace ${REBOOT_GRACE_TIMEOUT}s)"

while [ "$SECONDS" -lt "$deadline" ]; do
    bank="$(read_bank)"
    hits="$(http_hits)"
    hop_hits=$((hits - http_baseline))
    pct="$(uart_pct)"
    progressed=0
    ota_status="$(mosquitto_sub -h "$MQTT_HOST" -p "$MQTT_PORT" \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "$OTA_STATUS_TOPIC" -C 1 -W 1 2>/dev/null || true)"
    if [ -n "$ota_status" ] && [ "$ota_status" != "$last_ota_status" ]; then
        last_ota_status="$ota_status"
        if [[ "$ota_status" == *'"state": "error"'* ]] || [[ "$ota_status" == *'"state":"error"'* ]]; then
            echo "  FAIL: device reported ${ota_status}" >&2
            exit 1
        fi
    fi

    if [ "$bank" = "$WANT_BANK" ] && [ "$saw_activity" -eq 1 ]; then
        echo "  OK: online on bank ${bank} (http_hop=${hop_hits}, uart_pct=${pct})"
        exit 0
    fi

    if [ "$hits" != "$last_http" ] && [ "$hop_hits" -gt 0 ]; then
        progressed=1
        last_http="$hits"
    fi

    if [ "$pct" -ge 0 ] && [ "$pct" != "$last_pct" ]; then
        progressed=1
        last_pct="$pct"
    fi

    if [ "$saw_uart_start" -eq 0 ] && uart_ota_started; then
        progressed=1
        saw_uart_start=1
    fi

    if uart_reboot_phase; then
        if [ "$reboot_phase" -eq 0 ]; then
            reboot_phase=1
            last_progress_at=$SECONDS
            echo "  reboot phase: download done, waiting for bank ${WANT_BANK} (grace ${REBOOT_GRACE_TIMEOUT}s)"
        fi
        progressed=1
    fi

    if [ "$bank" = "offline" ]; then
        progressed=1
        saw_activity=1
    fi

    if [ "$progressed" -eq 1 ]; then
        last_progress_at=$SECONDS
        saw_activity=1
        echo "  progress: bank=${bank} http_hop=${hop_hits} uart=${pct}% reboot=${reboot_phase}"
    fi

    stall_limit="$PROGRESS_TIMEOUT"
    if [ "$reboot_phase" -eq 1 ]; then
        stall_limit="$REBOOT_GRACE_TIMEOUT"
    fi

    if [ "$saw_activity" -eq 1 ] && [ $((SECONDS - last_progress_at)) -ge "$stall_limit" ]; then
        echo "  FAIL: no progress for ${stall_limit}s (bank=${bank}, http_hop=${hop_hits}, uart=${pct}%, reboot=${reboot_phase})" >&2
        exit 1
    fi

    if [ "$saw_activity" -eq 0 ] && [ $((SECONDS - last_progress_at)) -ge "$PROGRESS_TIMEOUT" ]; then
        echo "  FAIL: OTA never started within ${PROGRESS_TIMEOUT}s (bank=${bank}, http_hop=${hop_hits})" >&2
        exit 1
    fi

    sleep "$POLL_INTERVAL"
done

bank="$(read_bank)"
echo "  FAIL: hop timeout ${HOP_TIMEOUT}s (bank=${bank}, want=${WANT_BANK}, http=$(http_hits))" >&2
exit 1
