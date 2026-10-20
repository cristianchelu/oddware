#!/usr/bin/env bash
# Build the ball parts headless with FreeCAD: mech/ball.sh [803040|103450] [--check]
set -euo pipefail
cd "$(dirname "$0")"
export BALL_CELL=803040 BALL_CHECK=0
for a in "$@"; do case "$a" in 803040|103450) BALL_CELL=$a;; --check) BALL_CHECK=1;; *) echo "bad arg $a"; exit 2;; esac; done
cp ball_view.scad build/ball_$BALL_CELL/ 2>/dev/null || true
timeout 1200 flatpak run --env=BALL_CELL=$BALL_CELL --env=BALL_CHECK=$BALL_CHECK --command=freecadcmd org.freecad.FreeCAD ball.py 2>&1 \
    | grep -v -E '^FreeCAD |^\(C\)|licensed|^\s*$|saving|%\)|\*\*\*|Transfer|WorkSession|Step File|\[32'
