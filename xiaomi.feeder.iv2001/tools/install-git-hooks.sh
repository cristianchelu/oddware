#!/usr/bin/env bash
# Install oddware git hooks that re-sync SDK patches after checkout/merge.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FEEDER_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
ODDWARE_ROOT="$(git -C "$FEEDER_ROOT" rev-parse --show-toplevel)"
HOOKS_DIR="$ODDWARE_ROOT/.git/hooks"

install_hook() {
    local name="$1"
    local hook_path="$HOOKS_DIR/$name"
    cat > "$hook_path" <<EOF
#!/usr/bin/env bash
# Auto-installed by xiaomi.feeder.iv2001/tools/install-git-hooks.sh
set -euo pipefail
FEEDER_ROOT="\$(git rev-parse --show-toplevel)/xiaomi.feeder.iv2001"
if [ -x "\$FEEDER_ROOT/tools/sync-sdk-patches.sh" ]; then
    "\$FEEDER_ROOT/tools/sync-sdk-patches.sh" --if-needed || true
fi
EOF
    chmod +x "$hook_path"
    echo "Installed $hook_path"
}

mkdir -p "$HOOKS_DIR"
install_hook post-checkout
install_hook post-merge
echo "Git hooks ready. SDK patches re-sync after checkout/merge when the tree drifts."
