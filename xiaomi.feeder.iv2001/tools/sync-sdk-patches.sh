#!/usr/bin/env bash
# Reset the gitignored LinkIt SDK tree and apply only firmware/patches/*.patch.
#
#   ./tools/sync-sdk-patches.sh              # always reset + apply
#   ./tools/sync-sdk-patches.sh --if-needed  # skip when stamp matches and SDK is clean
#
# Exit status: 0. Prints SYNC_RAN=1 when the SDK was reset (caller may clean build obj).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PATCHES_DIR="$REPO_ROOT/firmware/patches"
SERIES_FILE="$PATCHES_DIR/series"
STAMP_FILE="$REPO_ROOT/.sdk-patches-stamp"
SDK_ROOT="${SDK_ROOT:-$REPO_ROOT/external/linkit-sdk-v4.6.2-houndify}"

IF_NEEDED=0
for arg in "$@"; do
    case "$arg" in
        --if-needed) IF_NEEDED=1 ;;
        -h|--help)
            echo "Usage: $0 [--if-needed]"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            exit 2
            ;;
    esac
done

if [ ! -d "$SDK_ROOT/.git" ]; then
    echo "ERROR: SDK not found at $SDK_ROOT (run ./tools/fetch-sdk.sh)" >&2
    exit 1
fi

if [ ! -f "$SERIES_FILE" ]; then
    echo "ERROR: missing $SERIES_FILE" >&2
    exit 1
fi

compute_fingerprint() {
  {
    cat "$SERIES_FILE"
    while IFS= read -r line || [ -n "$line" ]; do
        line="${line%%#*}"
        line="$(echo "$line" | xargs)"
        [ -z "$line" ] && continue
        patch_path="$PATCHES_DIR/$line"
        if [ ! -f "$patch_path" ]; then
            echo "ERROR: patch listed in series not found: $patch_path" >&2
            exit 1
        fi
        sha256sum "$patch_path"
    done < "$SERIES_FILE"
    git -C "$SDK_ROOT" rev-parse HEAD
  } | sha256sum | awk '{print $1}'
}

sdk_tree_dirty() {
    ! git -C "$SDK_ROOT" diff --quiet || \
    ! git -C "$SDK_ROOT" diff --cached --quiet
}

SYNC_RAN=0
fingerprint="$(compute_fingerprint)"

if [ "$IF_NEEDED" -eq 1 ] && [ -f "$STAMP_FILE" ]; then
    if [ "$(cat "$STAMP_FILE")" = "$fingerprint" ] && ! sdk_tree_dirty; then
        echo "SDK patches in sync (stamp ok, tree clean)"
        echo "SYNC_RAN=0"
        exit 0
    fi
fi

echo "Syncing SDK patches (reset vendor tree, apply firmware/patches/) ..."
git -C "$SDK_ROOT" reset --hard HEAD

while IFS= read -r line || [ -n "$line" ]; do
    line="${line%%#*}"
    line="$(echo "$line" | xargs)"
    [ -z "$line" ] && continue
    patch_file="$PATCHES_DIR/$line"
    echo "  $line"
    if command -v patch >/dev/null 2>&1; then
        patch -p1 --forward -d "$SDK_ROOT" -i "$patch_file"
    else
        git -C "$SDK_ROOT" apply "$patch_file"
    fi
done < "$SERIES_FILE"

printf '%s\n' "$fingerprint" > "$STAMP_FILE"
SYNC_RAN=1

# git reset --hard drops the executable bit on SDK helper scripts (shallow clone).
if [ ! -x "$SDK_ROOT/build.sh" ]; then
    chmod +x "$SDK_ROOT/build.sh" 2>/dev/null || true
    find "$SDK_ROOT/middleware" "$SDK_ROOT/tools" "$SDK_ROOT/driver" -name '*.sh' -type f ! -perm -111 \
        -exec chmod +x {} + 2>/dev/null || true
fi

echo "SDK patch sync complete."
echo "SYNC_RAN=1"
