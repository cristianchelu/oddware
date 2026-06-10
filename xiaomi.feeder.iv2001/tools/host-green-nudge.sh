#!/usr/bin/env bash
# After make test-host: nudge agents to cross-compile when ARM-only paths changed.
# See AGENTS.md § Green = host + cross-compile.

set -euo pipefail

IV2001_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$IV2001_ROOT"

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    exit 0
fi

GIT_ROOT="$(git rev-parse --show-toplevel)"
IV2001_REL="${IV2001_ROOT#"$GIT_ROOT"/}"
if [[ "$IV2001_REL" == "$IV2001_ROOT" ]]; then
    IV2001_REL=""
fi

iv2001_relpath() {
    local path="$1"

    if [[ -n "$IV2001_REL" ]]; then
        if [[ "$path" == "$IV2001_REL"/* ]]; then
            printf '%s\n' "${path#"$IV2001_REL"/}"
            return 0
        fi
        return 1
    fi

    printf '%s\n' "$path"
    return 0
}

mapfile -t HOST_SRC_NAMES < <(
    grep -E '[[:space:]]+\.\./src/[^[:space:]]+' firmware/test/Makefile \
        | sed -E 's|.*/([^[:space:]]+)$|\1|' \
        | sort -u
)

host_src_is_linked() {
    local base="$1"
    local name

    for name in "${HOST_SRC_NAMES[@]}"; do
        if [[ "$name" == "$base" ]]; then
            return 0
        fi
    done
    return 1
}

mapfile -t CHANGED < <(
    {
        git diff --name-only HEAD 2>/dev/null || true
        git diff --cached --name-only 2>/dev/null || true
        git ls-files --others --exclude-standard 2>/dev/null || true
    } | sort -u | grep -v '^$' || true
)

if ((${#CHANGED[@]} == 0)); then
    exit 0
fi

mapfile -t TRIGGER_PATHS < <(
    for path in "${CHANGED[@]}"; do
        local_path=""
        if ! local_path="$(iv2001_relpath "$path")"; then
            continue
        fi
        case "$local_path" in
            firmware/adapters/*|firmware/GCC/*|firmware/patches/*|firmware/ports/*|firmware/inc/*)
                printf '%s\n' "$local_path"
                ;;
            firmware/src/*)
                if ! host_src_is_linked "$(basename "$local_path")"; then
                    printf '%s\n' "$local_path"
                fi
                ;;
        esac
    done | sort -u
)

if ((${#TRIGGER_PATHS[@]} == 0)); then
    exit 0
fi

printf '\n'
printf 'HOST OK — cross-compile not run.\n'
printf 'Working tree touches ARM-only firmware (host tests use fakes for these):\n'
for path in "${TRIGGER_PATHS[@]}"; do
    printf '  %s\n' "$path"
done
printf 'Before flash, OTA, or claiming done, run:\n'
printf '  source tools/build-env.sh && ./tools/build-firmware.sh\n'
printf 'Do not report ready to flash until TOTAL BUILD: PASS.\n'
