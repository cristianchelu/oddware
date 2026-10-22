#!/usr/bin/env bash
# Render the board and summarise DRC. Usage: tools/pcb_check.sh [layers]
set -euo pipefail
cd "$(dirname "$0")/../zigbee_toy"
K="flatpak run --command=kicad-cli org.kicad.kicad"
L=${1:-F.Cu,B.Cu,F.Fab,F.Courtyard,Edge.Cuts,F.Silkscreen}
$K pcb export svg -l "$L" --mode-single --page-size-mode 2 --exclude-drawing-sheet --sp -o build/board.svg zigbee_toy.kicad_pcb >/dev/null
magick -density 600 build/board.svg -background white -flatten build/board.png
$K pcb drc --severity-all -o build/drc.rpt zigbee_toy.kicad_pcb >/dev/null || true
grep -E '^\[' build/drc.rpt | sed 's/:.*//' | sort | uniq -c
flatpak run --command=python3 org.kicad.kicad ../tools/check_spacing.py 2>&1 | grep -v -E 'm_choices|swig|Debug' || true
