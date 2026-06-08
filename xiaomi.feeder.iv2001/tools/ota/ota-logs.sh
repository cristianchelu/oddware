# shellcheck shell=bash
# Shared OTA bench log paths — sourced by ota-ab-roundtrip.sh and friends.
#
# Logs are kept under tools/ota/logs/<run-id>/ (override with OTA_LOG_DIR).
# Set OTA_LOG_KEEP=0 to delete the run directory on successful exit only.

ota_logs_repo_root() {
    local lib_dir
    lib_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    cd "$lib_dir/../.." && pwd
}

ota_logs_new_run() {
    local device_id="${1:?device_id required}"
    local repo_root="${2:-$(ota_logs_repo_root)}"
    local base dir run_id

    base="${OTA_LOG_DIR:-${repo_root}/tools/ota/logs}"
    run_id="$(date -u +%Y%m%dT%H%M%SZ)-${device_id}-$$"
    dir="${base}/${run_id}"
    mkdir -p "$dir"
    printf '%s\n' "$dir"
}

ota_logs_hop_uart() {
    local run_dir="${1:?run_dir required}"
    local from_bank="${2:?from required}"
    local to_bank="${3:?to required}"
    printf '%s/hop-%s-to-%s-uart.log\n' "$run_dir" "$from_bank" "$to_bank"
}

ota_logs_http() {
    local run_dir="${1:?run_dir required}"
    printf '%s/http.log\n' "$run_dir"
}

ota_logs_write_meta() {
    local run_dir="${1:?run_dir required}"
    shift
    {
        echo "started_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "pid=$$"
        while [ $# -gt 0 ]; do
            echo "$1"
            shift
        done
    } >"${run_dir}/meta.txt"
}

ota_logs_maybe_cleanup() {
    local run_dir="${1:?run_dir required}"
    local exit_code="${2:-0}"

    if [ "${OTA_LOG_KEEP:-1}" = "0" ] && [ "$exit_code" -eq 0 ]; then
        rm -rf "$run_dir"
        return
    fi
    echo "Logs retained: ${run_dir}"
}
