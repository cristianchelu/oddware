#!/usr/bin/env bash
# MQTT bench — fake retained topics + HA discovery for broker validation.
# See tools/mqtt/README.md and spec/30-processes/mqtt-protocol.md

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/mqtt-bench-lib.sh"

mqtt_bench_load_env "$SCRIPT_DIR"
MQTT_BENCH_PAYLOADS="${SCRIPT_DIR}/payloads"

usage() {
    cat <<EOF
Usage: $(basename "$0") <command> [args...]

Commands:
  pub <topic-suffix> <payload|-f file> [--retain]
  sub [pattern]
  session online|offline
  ota idle <A|B>
  state bowl_error on|off
  bowl_weight 42|empty
  battery 75|unknown
  battery_voltage <mV>
  mains ON|OFF
  feed_mode open_loop|compensated
  ha discovery <entity>              # publish retained discovery config
  ha rediscover <entity>             # clear config topic, wait, republish (entity_category updates)
  ha rediscover-categories           # rediscover all diagnostic + config entities
  verify ha <entity>                 # print topic + JSON (no publish)
  clean [--slice state|ota|ha|connection|power|all]

Environment:
  DEVICE_ID   MQTT device id (default: ddeeff)
  MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS — from tools/ota/.env when present

Examples:
  export DEVICE_ID=ddeeff
  $0 session online
  $0 ota idle A
  $0 ha discovery bowl_error
  $0 state bowl_error on
  $0 pub state -f payloads/state-ok.json --retain
EOF
}

cmd="${1:-}"
shift || true

case "$cmd" in
    pub)
        suffix="${1:?topic suffix required}"
        arg="${2:?payload or -f file required}"
        shift 2
        retain=""
        if [ "${1:-}" = "--retain" ]; then
            retain="--retain"
        fi
        if [ "$arg" = "-f" ]; then
            file="${1:?file path required after -f}"
            if [[ "$file" != /* ]]; then
                file="${SCRIPT_DIR}/${file#payloads/}"
                if [ ! -f "$file" ]; then
                    file="${MQTT_BENCH_PAYLOADS}/$(basename "$file")"
                fi
            fi
            payload="$(mqtt_bench_payload_file "$file")"
        else
            payload="$arg"
        fi
        mqtt_bench_pub "$(mqtt_bench_topic "$suffix")" "$payload" $retain
        ;;

    sub)
        mqtt_bench_sub "${1:-}"
        ;;

    session)
        mode="${1:?online|offline required}"
        case "$mode" in
            online)  mqtt_bench_session_online ;;
            offline) mqtt_bench_session_offline ;;
            *) echo "error: session expects online|offline" >&2; exit 1 ;;
        esac
        ;;

    ota)
        sub="${1:?idle required}"
        bank="${2:-A}"
        if [ "$sub" != "idle" ]; then
            echo "error: ota subcommand: idle <A|B>" >&2
            exit 1
        fi
        mqtt_bench_ota_idle "$bank"
        ;;

    state)
        sub="${1:?bowl_error required}"
        mode="${2:?on|off required}"
        if [ "$sub" != "bowl_error" ]; then
            echo "error: state subcommand: bowl_error on|off" >&2
            exit 1
        fi
        mqtt_bench_state_bowl_error "$mode"
        ;;

    bowl_weight)
        mode="${1:?42|empty required}"
        mqtt_bench_bowl_weight "$mode"
        ;;

    battery)
        mode="${1:?75|unknown required}"
        mqtt_bench_battery "$mode"
        ;;

    battery_voltage)
        mv="${1:?mV required}"
        mqtt_bench_battery_voltage "$mv"
        ;;

    mains)
        mode="${1:?ON|OFF required}"
        mqtt_bench_mains "$mode"
        ;;

    feed_mode)
        mode="${1:?open_loop|compensated required}"
        mqtt_bench_feed_mode "$mode"
        ;;

    ha)
        sub="${1:?subcommand required}"
        shift || true
        case "$sub" in
            discovery)
                entity="${1:?entity required}"
                mqtt_bench_ha_discovery "$entity"
                ;;
            rediscover)
                entity="${1:?entity required}"
                mqtt_bench_ha_rediscover "$entity"
                ;;
            rediscover-categories)
                mqtt_bench_ha_rediscover_categories
                ;;
            *)
                echo "error: ha subcommand: discovery|rediscover|rediscover-categories" >&2
                exit 1
                ;;
        esac
        ;;

    verify)
        sub="${1:?ha required}"
        entity="${2:?entity required}"
        if [ "$sub" != "ha" ]; then
            echo "error: verify subcommand: ha <entity>" >&2
            exit 1
        fi
        if ! mqtt_bench_ha_discovery_lookup "$entity"; then
            exit 1
        fi
        mqtt_bench_validate_json "$MQTT_BENCH_HA_PAYLOAD" "ha $entity"
        echo "topic: $MQTT_BENCH_HA_TOPIC"
        printf '%s\n' "$MQTT_BENCH_HA_PAYLOAD" | python3 -m json.tool
        ;;

    clean)
        slice="all"
        if [ "${1:-}" = "--slice" ]; then
            slice="${2:-all}"
        fi
        mqtt_bench_clean "$slice"
        ;;

    ""|-h|--help|help)
        usage
        ;;

    *)
        echo "error: unknown command: $cmd" >&2
        usage >&2
        exit 1
        ;;
esac
