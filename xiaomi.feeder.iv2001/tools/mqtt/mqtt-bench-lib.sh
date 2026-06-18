# shellcheck shell=bash
# Shared MQTT bench helpers — spec/30-processes/mqtt-protocol.md

mqtt_bench_load_env() {
    local script_dir="$1"
    local ota_env="${script_dir}/../ota/.env"

    if [ -f "$ota_env" ]; then
        # shellcheck disable=SC1090
        source "$ota_env"
    fi

    MQTT_HOST="${MQTT_HOST:-127.0.0.1}"
    MQTT_PORT="${MQTT_PORT:-1883}"
    MQTT_USER="${MQTT_USER:-petfeeder-flasher}"
    MQTT_PASS="${MQTT_PASS:-petfeeder}"
    DEVICE_ID="${DEVICE_ID:-ddeeff}"
}

mqtt_bench_topic() {
    echo "petfeeder/${DEVICE_ID}/$1"
}

mqtt_bench_pub() {
    local topic="$1"
    local payload="$2"
    local retain_flag=()

    if [ "${3:-}" = "--retain" ]; then
        retain_flag=(-r)
    fi

    mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "$topic" -m "$payload" -q 1 "${retain_flag[@]}"
}

mqtt_bench_sub() {
    local pattern="${1:-petfeeder/${DEVICE_ID}/#}"

    mosquitto_sub -h "$MQTT_HOST" -p "$MQTT_PORT" \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "$pattern" -v
}

mqtt_bench_payload_file() {
    local file="$1"
    if [ ! -f "$file" ]; then
        echo "error: payload file not found: $file" >&2
        return 1
    fi
    cat "$file"
}

mqtt_bench_substitute_device_id() {
    sed "s/DEVICE_ID/${DEVICE_ID}/g"
}

mqtt_bench_validate_json() {
    local payload="$1"
    local label="${2:-payload}"

    if ! printf '%s' "$payload" | python3 -m json.tool >/dev/null 2>&1; then
        echo "error: invalid JSON for $label" >&2
        printf '%s\n' "$payload" >&2
        return 1
    fi
}

mqtt_bench_ha_bowl_error_payload() {
    mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/ha-bowl_error.json" \
        | mqtt_bench_substitute_device_id
}

mqtt_bench_ha_bowl_weight_payload() {
    mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/ha-bowl_weight.json" \
        | mqtt_bench_substitute_device_id
}

mqtt_bench_ha_battery_payload() {
    mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/ha-battery.json" \
        | mqtt_bench_substitute_device_id
}

mqtt_bench_ha_battery_voltage_payload() {
    mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/ha-battery_voltage.json" \
        | mqtt_bench_substitute_device_id
}

mqtt_bench_ha_mains_payload() {
    mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/ha-mains.json" \
        | mqtt_bench_substitute_device_id
}

mqtt_bench_bowl_weight() {
    local mode="$1"
    local payload

    case "$mode" in
        42)
            payload="$(mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/bowl_weight-42")"
            ;;
        empty)
            payload="$(mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/bowl_weight-empty")"
            ;;
        *)
            echo "error: bowl_weight expects 42|empty" >&2
            return 1
            ;;
    esac

    mqtt_bench_pub "$(mqtt_bench_topic bowl_weight)" "$payload" --retain
}

mqtt_bench_battery() {
    local mode="$1"
    local payload

    case "$mode" in
        75)
            payload="$(mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/battery-75")"
            ;;
        unknown)
            payload="$(mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/battery-unknown")"
            ;;
        *)
            echo "error: battery expects 75|unknown" >&2
            return 1
            ;;
    esac

    mqtt_bench_pub "$(mqtt_bench_topic battery)" "$payload" --retain
}

mqtt_bench_battery_voltage() {
    local mv="$1"

    case "$mv" in
        ''|*[!0-9]*)
            echo "error: battery_voltage expects plain integer mV" >&2
            return 1
            ;;
    esac

    mqtt_bench_pub "$(mqtt_bench_topic battery_voltage)" "$mv" --retain
}

mqtt_bench_mains() {
    local mode="$1"

    case "$mode" in
        ON|OFF) ;;
        *)
            echo "error: mains expects ON|OFF" >&2
            return 1
            ;;
    esac

    mqtt_bench_pub "$(mqtt_bench_topic mains)" "$mode" --retain
}

mqtt_bench_session_online() {
    mqtt_bench_pub "$(mqtt_bench_topic connection)" "online" --retain
}

mqtt_bench_session_offline() {
    mqtt_bench_pub "$(mqtt_bench_topic connection)" "offline" --retain
}

mqtt_bench_ota_idle() {
    local bank="${1:-A}"
    local payload

    case "$bank" in
        A|B) ;;
        *)
            echo "error: bank must be A or B" >&2
            return 1
            ;;
    esac

    if [ "$bank" = "B" ]; then
        payload="$(mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/ota-idle-b.json")"
    else
        payload="$(mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/ota-idle-a.json")"
    fi

    mqtt_bench_pub "$(mqtt_bench_topic ota/status)" "$payload" --retain
}

mqtt_bench_state_bowl_error() {
    local mode="$1"
    local payload

    case "$mode" in
        on)  payload="$(mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/state-bowl-error.json")" ;;
        off) payload="$(mqtt_bench_payload_file "$MQTT_BENCH_PAYLOADS/state-ok.json")" ;;
        *)
            echo "error: state bowl_error expects on|off" >&2
            return 1
            ;;
    esac

    mqtt_bench_pub "$(mqtt_bench_topic state)" "$payload" --retain
}

mqtt_bench_ha_discovery() {
    local entity="$1"
    local topic payload

    case "$entity" in
        bowl_error)
            topic="homeassistant/binary_sensor/petfeeder_${DEVICE_ID}/bowl_error/config"
            payload="$(mqtt_bench_ha_bowl_error_payload)"
            ;;
        bowl_weight)
            topic="homeassistant/sensor/petfeeder_${DEVICE_ID}/bowl_weight/config"
            payload="$(mqtt_bench_ha_bowl_weight_payload)"
            ;;
        battery)
            topic="homeassistant/sensor/petfeeder_${DEVICE_ID}/battery/config"
            payload="$(mqtt_bench_ha_battery_payload)"
            ;;
        battery_voltage)
            topic="homeassistant/sensor/petfeeder_${DEVICE_ID}/battery_voltage/config"
            payload="$(mqtt_bench_ha_battery_voltage_payload)"
            ;;
        mains)
            topic="homeassistant/binary_sensor/petfeeder_${DEVICE_ID}/mains/config"
            payload="$(mqtt_bench_ha_mains_payload)"
            ;;
        dispense)
            topic="homeassistant/button/petfeeder_${DEVICE_ID}/dispense/config"
            payload="{\"name\":\"Dispense\",\"unique_id\":\"petfeeder_${DEVICE_ID}_dispense\",\"command_topic\":\"petfeeder/${DEVICE_ID}/cmd/dispense\",\"payload_press\":\"{}\",\"availability_topic\":\"petfeeder/${DEVICE_ID}/connection\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\",\"device\":{\"identifiers\":[\"petfeeder_${DEVICE_ID}\"],\"name\":\"Pet Feeder ${DEVICE_ID}\",\"manufacturer\":\"Xiaomi\",\"model\":\"Smart Pet Food Feeder 2\"}}"
            ;;
        *)
            echo "error: unknown HA entity: $entity" >&2
            return 1
            ;;
    esac

    mqtt_bench_validate_json "$payload" "ha discovery $entity"
    mqtt_bench_pub "$topic" "$payload" --retain
    echo "published retained HA discovery:"
    echo "  topic: $topic"
    echo "  device_id: $DEVICE_ID broker: $MQTT_HOST:$MQTT_PORT"
}

mqtt_bench_clean() {
    local slice="${1:-all}"

    case "$slice" in
        state|all)
            mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "$(mqtt_bench_topic state)" -n -r -q 1 2>/dev/null || true
            ;;
    esac

    case "$slice" in
        ota|all)
            mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "$(mqtt_bench_topic ota/status)" -n -r -q 1 2>/dev/null || true
            ;;
    esac

    case "$slice" in
        ha|all)
            mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "homeassistant/binary_sensor/petfeeder_${DEVICE_ID}/bowl_error/config" \
                -n -r -q 1 2>/dev/null || true
            mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "homeassistant/sensor/petfeeder_${DEVICE_ID}/bowl_weight/config" \
                -n -r -q 1 2>/dev/null || true
            mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "homeassistant/sensor/petfeeder_${DEVICE_ID}/battery/config" \
                -n -r -q 1 2>/dev/null || true
            mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "homeassistant/sensor/petfeeder_${DEVICE_ID}/battery_voltage/config" \
                -n -r -q 1 2>/dev/null || true
            mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "homeassistant/binary_sensor/petfeeder_${DEVICE_ID}/mains/config" \
                -n -r -q 1 2>/dev/null || true
            mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "homeassistant/button/petfeeder_${DEVICE_ID}/dispense/config" \
                -n -r -q 1 2>/dev/null || true
            ;;
    esac

    case "$slice" in
        power|all)
            mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "$(mqtt_bench_topic battery)" -n -r -q 1 2>/dev/null || true
            mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "$(mqtt_bench_topic battery_voltage)" -n -r -q 1 2>/dev/null || true
            mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "$(mqtt_bench_topic mains)" -n -r -q 1 2>/dev/null || true
            ;;
    esac

    case "$slice" in
        connection|session|all)
            mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "$(mqtt_bench_topic connection)" -n -r -q 1 2>/dev/null || true
            ;;
    esac
}
