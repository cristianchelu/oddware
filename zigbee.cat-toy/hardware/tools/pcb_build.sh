#!/usr/bin/env bash
# Full board pipeline: place -> routes (hand + frozen) -> stitch/fill -> silk -> DRC.
set -euo pipefail
cd "$(dirname "$0")/.."
KPY="flatpak run --command=python3 org.kicad.kicad"
quiet() { grep -v -E 'm_choices|swig/python|Debug: Adding' || true; }
$KPY tools/gen_pcb.py layout 2>&1 | quiet
# All copper is hand-placed now: routes.py + routes_frozen.py (the 2026-10-20 Freerouting result,
# frozen). To autoroute again: gen_pcb.py dsn, run tools/freerouting/Route.java, gen_pcb.py ses.
$KPY tools/gen_pcb.py stitch 2>&1 | quiet
$KPY tools/gen_silk.py 2>&1 | quiet
tools/pcb_check.sh F.Cu,B.Cu,Edge.Cuts,F.Silkscreen
