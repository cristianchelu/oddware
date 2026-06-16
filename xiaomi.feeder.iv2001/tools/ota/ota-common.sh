# shellcheck shell=bash
# Shared OTA bench helpers — sourced by ota-hop.sh and ota-ab-roundtrip.sh.

ota_common_load_env() {
    local script_dir="$1"
    if [ -f "${script_dir}/.env" ]; then
        # shellcheck disable=SC1091
        source "${script_dir}/.env"
    fi
    MQTT_HOST="${MQTT_HOST:-192.168.100.52}"
    MQTT_PORT="${MQTT_PORT:-1883}"
    MQTT_USER="${MQTT_USER:-petfeeder-flasher}"
    MQTT_PASS="${MQTT_PASS:-petfeeder}"
    HTTP_PORT="${HTTP_PORT:-8080}"
    HTTP_BIND="${HTTP_BIND:-0.0.0.0}"
    HOP_TIMEOUT="${HOP_TIMEOUT:-300}"
    PROGRESS_TIMEOUT="${PROGRESS_TIMEOUT:-90}"
    UART_DEV="${UART_DEV:-/dev/ttyUSB0}"
}

ota_common_pick_lan_ip() {
    python3 - <<'PY'
import socket

def candidates():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        yield s.getsockname()[0]
        s.close()
    except OSError:
        pass

for ip in candidates():
    if ip and not ip.startswith("127."):
        print(ip)
        break
else:
    print("127.0.0.1")
PY
}

ota_common_read_bank() {
    local device_id="$1"
    local status

    status="$(mosquitto_sub -h "$MQTT_HOST" -p "$MQTT_PORT" \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "petfeeder/${device_id}/ota/status" -C 1 -W 8 2>/dev/null || true)"
    case "$status" in
        *'"bank":"B"'*|*'"bank": "B"'*) echo "B" ;;
        *'"bank":"A"'*|*'"bank": "A"'*) echo "A" ;;
        *) echo "?" ;;
    esac
}

ota_common_pick_firmware() {
    local device_id="$1"
    local flash_dir="$2"
    local active

    active="$(ota_common_read_bank "$device_id")"
    if [ "$active" = "B" ]; then
        echo "${flash_dir}/petfeeder_a.bin"
    else
        echo "${flash_dir}/petfeeder_b.bin"
    fi
}

ota_common_publish_ota() {
    local device_id="$1"
    local firmware="$2"
    local lan_ip="$3"
    local meta_file="${4:-}"

    local name sha url payload topic

    name="$(basename "$firmware")"
    sha="$(sha512sum "$firmware" | awk '{print $1}')"
    url="http://${lan_ip}:${HTTP_PORT}/${name}"
    payload="{\"url\":\"${url}\",\"sha512\":\"${sha}\"}"
    topic="petfeeder/${device_id}/cmd/ota"

    echo "  publish OTA: ${name}"
    if [ -n "$meta_file" ]; then
        echo "  firmware_sha512=${sha}" >>"$meta_file"
    fi
    mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "$topic" -m "$payload" -q 1
}

ota_common_start_http() {
    local script_dir="$1"
    local flash_dir="$2"
    local http_log="$3"
    local lan_ip="$4"

    fuser -k "${HTTP_PORT}/tcp" 2>/dev/null || true
    sleep 1
    : >"$http_log"
    python3 "${script_dir}/range-http-server.py" "$HTTP_PORT" --bind "$HTTP_BIND" \
        --directory "$flash_dir" >>"$http_log" 2>&1 &
    OTA_HTTP_PID=$!
    sleep 1
    local curl_ok=0
    if curl -sf --max-time 3 -H 'Range: bytes=0-63' \
        "http://${lan_ip}:${HTTP_PORT}/petfeeder_a.bin" >/dev/null; then
        curl_ok=1
    fi
    if [ "$curl_ok" -eq 0 ]; then
        echo "error: HTTP Range server not reachable" >&2
        cat "$http_log" >&2
        return 1
    fi
}

ota_common_stop_http() {
    if [ -n "${OTA_HTTP_PID:-}" ] && kill -0 "$OTA_HTTP_PID" 2>/dev/null; then
        kill "$OTA_HTTP_PID" 2>/dev/null || true
        wait "$OTA_HTTP_PID" 2>/dev/null || true
    fi
    OTA_HTTP_PID=""
    fuser -k "${HTTP_PORT}/tcp" 2>/dev/null || true
}

ota_common_reset_ota_status() {
    local device_id="$1"
    mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "petfeeder/${device_id}/ota/status" \
        -m '{"state":"idle","pct":0,"error":"","bank":"A"}' -r -q 1 2>/dev/null || true
}
