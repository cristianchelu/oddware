#!/usr/bin/env bash
# Trigger OTA via MQTT (thin wrapper around ota-hop.sh).
#
# Monitored hop (wait for bank swap, logs under tools/ota/logs/):
#   ./tools/ota/mqtt-ota.sh --device-id 768722 --skip-build
#
# Publish-only (HTTP server left running in background):
#   ./tools/ota/mqtt-ota.sh --device-id 768722 --publish-only --skip-build
#
# Publish-only with HTTP until Ctrl+C (--serve-foreground):
#   ./tools/ota/mqtt-ota.sh --device-id 768722 --publish-only --serve-foreground
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

args=()
publish_only=0
serve_foreground=0

while [ $# -gt 0 ]; do
    case "$1" in
        --wait)
            serve_foreground=1
            publish_only=1
            shift
            ;;
        --publish-only)
            publish_only=1
            shift
            ;;
        --serve-foreground)
            serve_foreground=1
            shift
            ;;
        *)
            args+=("$1")
            shift
            ;;
    esac
done

if [ "$publish_only" -eq 0 ]; then
    exec "$SCRIPT_DIR/ota-hop.sh" "${args[@]}"
fi

hop_args=(--publish-only "${args[@]}")
if [ "$serve_foreground" -eq 1 ]; then
    hop_args+=(--serve-foreground)
fi
exec "$SCRIPT_DIR/ota-hop.sh" "${hop_args[@]}"
