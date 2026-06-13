# shellcheck shell=bash
# UART capture for OTA bench scripts — releases the port when the parent exits.
#
#   source tools/ota/uart-capture.sh
#   uart_capture_acquire /dev/ttyUSB0
#   uart_capture_start /path/to/log.log
#   uart_capture_stop
#   uart_capture_release

UART_CAPTURE_DEV=""
UART_CAPTURE_LOG=""
UART_CAPTURE_PID=""
UART_CAPTURE_WATCHDOG_PID=""
UART_CAPTURE_LOCK_FILE=""
UART_CAPTURE_LOCK_FD=""

uart_capture_lock_path() {
    local dev="${1:?}"
    local base="${OTA_LOG_DIR:-$(dirname "${BASH_SOURCE[0]}")/logs}"
    mkdir -p "$base"
    printf '%s/.uart-%s.lock\n' "$base" "$(basename "$dev")"
}

uart_capture_stale_cleanup() {
    local lock_file="$1"
    local line main_pid cap_pid dev

    [ -f "$lock_file" ] || return 0

    # shellcheck disable=SC1090
    source "$lock_file" 2>/dev/null || true

    if [ -n "${main_pid:-}" ] && kill -0 "$main_pid" 2>/dev/null; then
        echo "error: UART ${dev:-device} in use by ota bench pid ${main_pid} (lock: ${lock_file})" >&2
        return 1
    fi

    if [ -n "${cap_pid:-}" ] && kill -0 "$cap_pid" 2>/dev/null; then
        kill -TERM "$cap_pid" 2>/dev/null || true
        wait "$cap_pid" 2>/dev/null || true
    fi

    if [ -n "${dev:-}" ] && command -v fuser >/dev/null 2>&1; then
        fuser -k "$dev" 2>/dev/null || true
    fi

    rm -f "$lock_file"
    return 0
}

uart_capture_acquire() {
    local dev="$1"
    local lock_file

    dev="$(readlink -f "$dev")"
    lock_file="$(uart_capture_lock_path "$dev")"

    if ! uart_capture_stale_cleanup "$lock_file"; then
        return 1
    fi

    exec {UART_CAPTURE_LOCK_FD}>>"$lock_file"
    if ! flock -n "$UART_CAPTURE_LOCK_FD"; then
        echo "error: UART lock busy: ${lock_file}" >&2
        exec {UART_CAPTURE_LOCK_FD}>&-
        UART_CAPTURE_LOCK_FD=""
        return 1
    fi

    {
        echo "main_pid=$$"
        echo "dev=$dev"
        echo "started_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    } >"$lock_file"

    UART_CAPTURE_DEV="$dev"
    UART_CAPTURE_LOCK_FILE="$lock_file"
    return 0
}

uart_capture_start() {
    local log_path="$1"
    local parent_pid="${2:-$$}"
    local cat_cmd=(cat)

    uart_capture_stop

    if [ -z "$UART_CAPTURE_DEV" ] || [ ! -e "$UART_CAPTURE_DEV" ]; then
        UART_CAPTURE_LOG=""
        return 1
    fi

    UART_CAPTURE_LOG="$log_path"
    : >"$log_path"
    stty -F "$UART_CAPTURE_DEV" 115200 cs8 -cstopb -parenb raw -echo 2>/dev/null || true

    if command -v stdbuf >/dev/null 2>&1; then
        cat_cmd=(stdbuf -o0 -e0 cat)
    fi

    (
        trap 'exit 0' TERM INT
        while true; do
            "${cat_cmd[@]}" "$UART_CAPTURE_DEV" >>"$log_path" 2>/dev/null || sleep 0.2
        done
    ) &
    UART_CAPTURE_PID=$!

    (
        while kill -0 "$parent_pid" 2>/dev/null; do
            sleep 1
        done
        if [ -n "${UART_CAPTURE_PID:-}" ] && kill -0 "$UART_CAPTURE_PID" 2>/dev/null; then
            kill -TERM "$UART_CAPTURE_PID" 2>/dev/null || true
            wait "$UART_CAPTURE_PID" 2>/dev/null || true
        fi
    ) &
    UART_CAPTURE_WATCHDOG_PID=$!

    if [ -n "$UART_CAPTURE_LOCK_FILE" ]; then
        {
            echo "main_pid=$parent_pid"
            echo "cap_pid=$UART_CAPTURE_PID"
            echo "watchdog_pid=$UART_CAPTURE_WATCHDOG_PID"
            echo "dev=$UART_CAPTURE_DEV"
            echo "log=$log_path"
            echo "started_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
        } >"$UART_CAPTURE_LOCK_FILE"
    fi

    return 0
}

uart_capture_stop() {
    local i

    if [ -n "${UART_CAPTURE_WATCHDOG_PID:-}" ] && kill -0 "$UART_CAPTURE_WATCHDOG_PID" 2>/dev/null; then
        kill -TERM "$UART_CAPTURE_WATCHDOG_PID" 2>/dev/null || true
        for i in $(seq 1 20); do
            kill -0 "$UART_CAPTURE_WATCHDOG_PID" 2>/dev/null || break
            sleep 0.1
        done
        if kill -0 "$UART_CAPTURE_WATCHDOG_PID" 2>/dev/null; then
            kill -KILL "$UART_CAPTURE_WATCHDOG_PID" 2>/dev/null || true
        fi
        wait "$UART_CAPTURE_WATCHDOG_PID" 2>/dev/null || true
    fi
    UART_CAPTURE_WATCHDOG_PID=""

    if [ -n "${UART_CAPTURE_PID:-}" ] && kill -0 "$UART_CAPTURE_PID" 2>/dev/null; then
        kill -TERM "$UART_CAPTURE_PID" 2>/dev/null || true
        for i in $(seq 1 20); do
            kill -0 "$UART_CAPTURE_PID" 2>/dev/null || break
            sleep 0.1
        done
        if kill -0 "$UART_CAPTURE_PID" 2>/dev/null; then
            kill -KILL "$UART_CAPTURE_PID" 2>/dev/null || true
        fi
        wait "$UART_CAPTURE_PID" 2>/dev/null || true
    fi
    UART_CAPTURE_PID=""

    # Kill any cat child that survived (SIGTERM to subshell doesn't reach children).
    if [ -n "${UART_CAPTURE_DEV:-}" ] && command -v fuser >/dev/null 2>&1; then
        fuser -k "${UART_CAPTURE_DEV}" 2>/dev/null || true
    fi
}

uart_capture_release() {
    uart_capture_stop

    if [ -n "${UART_CAPTURE_LOCK_FD:-}" ]; then
        flock -u "$UART_CAPTURE_LOCK_FD" 2>/dev/null || true
        exec {UART_CAPTURE_LOCK_FD}>&-
        UART_CAPTURE_LOCK_FD=""
    fi

    if [ -n "${UART_CAPTURE_LOCK_FILE:-}" ]; then
        rm -f "$UART_CAPTURE_LOCK_FILE"
        UART_CAPTURE_LOCK_FILE=""
    fi

    UART_CAPTURE_DEV=""
    UART_CAPTURE_LOG=""
}

uart_capture_on_exit() {
    uart_capture_release
}
