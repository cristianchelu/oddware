#!/usr/bin/env bash
# Build the pod parts headless with FreeCAD: mech/pod.sh [503040|103450] [--check]
set -euo pipefail
cd "$(dirname "$0")"
export POD_BATTERY=503040 POD_CHECK=0
for a in "$@"; do case "$a" in 503040|103450) POD_BATTERY=$a;; --check) POD_CHECK=1;; *) echo "bad arg $a"; exit 2;; esac; done
timeout 900 flatpak run --env=POD_BATTERY=$POD_BATTERY --env=POD_CHECK=$POD_CHECK --command=freecadcmd org.freecad.FreeCAD pod.py 2>&1 \
    | grep -v -E '^FreeCAD |^\(C\)|licensed|^\s*$'
