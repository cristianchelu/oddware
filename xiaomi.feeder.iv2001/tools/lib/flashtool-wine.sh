#!/usr/bin/env bash
# Shared Wine COM mapping and IoT Flash Tool path helpers.
# Sourced by setup-flashtool.sh and iot-flash.sh — do not execute directly.
set -euo pipefail

resolve_paths() {
    if [ -z "${REPO_ROOT:-}" ]; then
        echo "error: REPO_ROOT must be set before sourcing flashtool-wine.sh" >&2
        return 1
    fi

    SDK_ROOT="${SDK_ROOT:-$REPO_ROOT/external/linkit-sdk-v4.7.1}"
    SDK_PC_TOOL_ZIP="$SDK_ROOT/tools/PC_tool.zip"
    FT_WIN="${IOT_FLASH_TOOL_ROOT:-$REPO_ROOT/external/iot-flash-tool/win}"
    export WINEPREFIX="${WINEPREFIX:-$REPO_ROOT/external/.wine-iot-flash}"
    export WINEDEBUG="${WINEDEBUG:--all}"
    COM_PORT="${IOT_COM_PORT:-3}"
    COM_NAME="COM${COM_PORT}"
    DA_BIN="$FT_WIN/MTK_AllInOne_DA.bin"
}

ensure_flashtool_extracted() {
    resolve_paths

    if [ -f "$FT_WIN/FlashTool.exe" ] && [ -f "$FT_WIN/coda.exe" ]; then
        return 0
    fi

    if [ ! -f "$SDK_PC_TOOL_ZIP" ]; then
        echo "error: $SDK_PC_TOOL_ZIP not found" >&2
        echo "Run: ./tools/bootstrap.sh   (or ./tools/fetch-sdk.sh)" >&2
        return 1
    fi

    echo "Extracting IoT Flash Tool from PC_tool.zip ..."
    mkdir -p "$(dirname "$FT_WIN")"
    rm -rf "$FT_WIN"
    unzip -q -o "$SDK_PC_TOOL_ZIP" "IoT_Flash_Tool/win/*" -d "$(dirname "$FT_WIN")/.."
    mv "$(dirname "$FT_WIN")/../IoT_Flash_Tool/win" "$FT_WIN"
    rmdir "$(dirname "$FT_WIN")/../IoT_Flash_Tool" 2>/dev/null || true
}

stop_wineserver() {
    if command -v wineserver >/dev/null 2>&1 && pgrep -u "$(id -u)" -x wineserver >/dev/null 2>&1; then
        echo "Stopping wineserver..."
        wineserver -k || true
        sleep 0.5
    fi
}

ensure_wine_prefix() {
    if [ ! -d "$WINEPREFIX" ]; then
        echo "Creating Wine prefix at $WINEPREFIX ..."
        wineboot --init
    fi
}

map_com_port() {
    local uart="$1"
    local com_link="$WINEPREFIX/dosdevices/com${COM_PORT}"
    local serial_key='\\Device\\Serial2'

    mkdir -p "$WINEPREFIX/dosdevices"
    rm -f "$com_link"
    ln -s "$uart" "$com_link"

    wine reg add 'HKLM\Software\Wine\Ports' \
        /v "$COM_NAME" /t REG_SZ /d "$uart" /f >/dev/null
    wine reg add 'HKLM\Hardware\Devicemap\SERIALCOMM' \
        /v "$serial_key" /t REG_SZ /d "$COM_NAME" /f >/dev/null

    echo "Mapped ${COM_NAME}:"
    echo "  $com_link -> $uart"
    echo "  HKLM\\Software\\Wine\\Ports\\${COM_NAME} = $uart"
}

# Copy flash_download.cfg + ROM bins next to coda.exe (Wine follows local paths reliably).
stage_flash_package() {
    local cfg_linux="$1"
    local cfg_dir bin

    resolve_paths
    cfg_dir="$(dirname "$(readlink -f "$cfg_linux")")"
    cp -f "$cfg_linux" "$FT_WIN/flash_download.cfg"
    while IFS= read -r bin; do
        [ -n "$bin" ] || continue
        if [ -f "$cfg_dir/$bin" ]; then
            cp -Lf "$cfg_dir/$bin" "$FT_WIN/$bin" 2>/dev/null \
                || cp -f "$cfg_dir/$bin" "$FT_WIN/$bin"
        else
            echo "error: missing ROM file $cfg_dir/$bin" >&2
            return 1
        fi
    done < <(grep -E '^\s+file:' "$cfg_linux" | sed -E 's/.*file:[[:space:]]*//')
}

linux_to_wine_zpath() {
    local linux_path="$1"
    local abs_path
    abs_path="$(readlink -f "$linux_path")"
    winepath -w "$abs_path" 2>/dev/null | tr -d '\r' | tail -1
}

write_flashtool_ini() {
    local cfg_path="${1:-$REPO_ROOT/firmware/flash/flash_download.cfg}"
    local ini_path="$FT_WIN/FlashTool.ini"
    local da_wine cfg_wine log_wine

    da_wine="$(linux_to_wine_zpath "$DA_BIN")"
    cfg_wine="$(linux_to_wine_zpath "$cfg_path")"
    log_wine="$(linux_to_wine_zpath "$FT_WIN")/"

    cat >"$ini_path" <<EOF
[global]
UsedUSB=0
ComName=${COM_NAME}
EnableUSB2.0=0
LongPressPowerKey=0
DALoggingChannel=0
LogPath=${log_wine}
DAPath=${da_wine}
FormatType=1
FormatAddressType=16
FormatAddress=0
FormatLength=1024
OtpOperation=0
OtpAddress=0
OtpLength=0
OtpFilePath=
UsbSwitchToolFilterVid=3725
UsbSwitchToolFilterPid=35
UsbSwitchTool=0
UsbSwitchToolbyUART=false
PowerOffCompletely=0
ResetCompletely=0
UID=
CertPath=certificate.crt
SecBootPath=

[ui]
CurrentWidget=0
CFGPath=${cfg_wine}

[ReadBackList]
size=0
EOF

    echo "Wrote $ini_path"
}

warn_dialout() {
    if ! groups | grep -qw dialout; then
        echo "warning: user not in dialout group — serial open may fail" >&2
        echo "  Fedora:  sudo usermod -aG dialout \$USER && newgrp dialout" >&2
    fi
}

resolve_tty_device() {
    local dev="$1"
    local resolved

    resolved="$(readlink -f "$dev")"
    if [[ "$resolved" != /dev/tty* ]]; then
        echo "error: device must resolve to /dev/tty*, got: $resolved" >&2
        return 1
    fi
    if [ ! -e "$resolved" ]; then
        echo "error: $resolved does not exist" >&2
        return 1
    fi
    printf '%s\n' "$resolved"
}

# Optional arg: CODA log file — countdown ends early only after a successful Done.
flash_log_flash_succeeded() {
    local log_file="${1:-}"

    [ -n "$log_file" ] && [ -f "$log_file" ] && grep -qF "Done." "$log_file"
}

flash_reset_prompt() {
    local log_file="${1:-}"
    local sec="${IOT_FLASH_RESET_WAIT_SEC:-30}"
    local remaining="$sec"

    echo ""
    printf '─%.0s' {1..60}
    echo
    echo "  Reset the feeder to enter BROM download mode:"
    echo "    • Power-cycle the supply, or"
    echo "    • Pulse TP15 (SW3 RESET / CHIP_EN)"
    echo "  The flash tool is listening — reset within ${sec}s."
    printf '─%.0s' {1..60}
    echo ""
    while [ "$remaining" -gt 0 ]; do
        if flash_log_flash_succeeded "$log_file"; then
            echo ""
            echo "  Flash completed."
            return 0
        fi
        printf '\r  %2ds remaining… ' "$remaining"
        sleep 1
        remaining=$((remaining - 1))
    done
    echo ""
    if flash_log_flash_succeeded "$log_file"; then
        echo "  Flash completed."
    else
        echo "  Reset window ended."
    fi
}

# Start Wine/CODA, prompt for manual reset, wait for completion.
run_with_manual_reset() {
    local uart="$1"
    shift
    local listen_ms="${IOT_FLASH_LISTEN_MS:-2000}"
    local log pid status=0

    resolve_paths
    ensure_flashtool_extracted
    stop_wineserver
    ensure_wine_prefix
    map_com_port "$uart"

    log="$(mktemp "${TMPDIR:-/tmp}/iot-flash.XXXXXX")"
    trap "rm -f '$log'" RETURN

    (cd "$FT_WIN" && "$@") 2>&1 | tee "$log" &
    pid=$!
    sleep "$(awk "BEGIN { printf \"%.3f\", $listen_ms / 1000 }")"
    flash_reset_prompt "$log"

    if kill -0 "$pid" 2>/dev/null; then
        echo "  Waiting for flash tool to finish..."
        wait "$pid" || status=$?
    else
        wait "$pid" 2>/dev/null || status=$?
    fi

    stop_wineserver
    return "$status"
}

with_wine_uart() {
    run_with_manual_reset "$@"
}
