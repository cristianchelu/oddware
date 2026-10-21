#!/usr/bin/env bash
# Regenerate the schematic + libraries, then verify with KiCad ERC and a netlist diff.
# The .kicad_sch is GENERATED: edit tools/design.py (placement + nets), not the schematic.
set -euo pipefail
cd "$(dirname "$0")"
K="flatpak run --command=kicad-cli org.kicad.kicad"
python3 gen_fp.py
python3 gen_sch.py
cd ../zigbee_toy
mkdir -p build
$K sch erc --severity-all --exit-code-violations -o build/erc.rpt zigbee_toy.kicad_sch
$K sch export netlist --format kicadsexpr -o build/zigbee_toy.net zigbee_toy.kicad_sch >/dev/null
python3 ../tools/check_netlist.py build/zigbee_toy.net netlist_intent.txt
$K sch export pdf -o zigbee_toy_schematic.pdf zigbee_toy.kicad_sch >/dev/null
$K sch export bom --fields 'Value,${QUANTITY},Reference,Footprint,LCSC' \
    --labels 'Comment,Qty,Designator,Footprint,LCSC Part #' --group-by 'Value,Footprint,LCSC' \
    --ref-range-delimiter '' --exclude-dnp -o jlc_bom.csv zigbee_toy.kicad_sch >/dev/null
echo "OK: ERC clean, netlist matches intent, PDF + JLC BOM written"
