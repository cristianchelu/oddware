#!/usr/bin/env bash
# Produce the JLCPCB order files in zigbee_toy/build/jlc/:
#   gerbers.zip   Gerber + Excellon drill (zones refilled first, so the copper matches a fresh fill)
#   cpl.csv       placement file in JLC's own footprint frame (rotation/origin per part fitted against the
#                 EasyEDA footprint of each LCSC number), limited to the refs in jlc_bom.csv
#   bom.csv       copy of jlc_bom.csv
set -euo pipefail
cd "$(dirname "$0")/../zigbee_toy"
K="flatpak run --command=kicad-cli org.kicad.kicad"
OUT=build/jlc
rm -rf "$OUT" && mkdir -p "$OUT/gerbers"
# The board must sit next to its .kicad_pro/.kicad_dru/fp-lib-table, or KiCad falls back to default rules (0.2 mm clearance, 0.5 mm edge).
mkdir -p "$OUT/work" && cp zigbee_toy.kicad_pcb zigbee_toy.kicad_pro zigbee_toy.kicad_dru fp-lib-table "$OUT/work/" && cp -r zigbee_toy.pretty "$OUT/work/"
$K pcb drc --refill-zones --save-board --exit-code-violations -o "$OUT/drc.rpt" "$OUT/work/zigbee_toy.kicad_pcb" >/dev/null
$K pcb export gerbers --layers F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts \
    --subtract-soldermask --no-x2 --use-drill-file-origin -o "$OUT/gerbers/" "$OUT/work/zigbee_toy.kicad_pcb" >/dev/null
$K pcb export drill --format excellon --excellon-separate-th --excellon-units mm --drill-origin plot \
    --generate-map --map-format gerberx2 -o "$OUT/gerbers/" "$OUT/work/zigbee_toy.kicad_pcb" >/dev/null
# CPL in JLC's footprint frame (see tools/jlc_cpl.py). EasyEDA footprint data is cached in tools/easyeda/.
EE=../tools/easyeda; mkdir -p "$EE"
UA='Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 Chrome/120 Safari/537.36'
for c in $(tail -n +2 jlc_bom.csv | awk -F'","' '{print $5}' | tr -d '"' | sort -u); do
    [ -s "$EE/$c.json" ] || curl -s -m 40 -A "$UA" -H 'Accept: application/json' \
        "https://easyeda.com/api/products/$c/components?version=6.4.19.5" -o "$EE/$c.json"
    grep -q '"success":true' "$EE/$c.json" || { echo "no EasyEDA footprint for $c"; rm -f "$EE/$c.json"; exit 1; }
done
flatpak run --command=python3 org.kicad.kicad ../tools/jlc_cpl.py "$OUT/work/zigbee_toy.kicad_pcb" jlc_bom.csv "$EE" "$OUT/cpl.csv" \
    2>&1 | grep -v -E 'm_choices|swig|Debug: Adding'
cp jlc_bom.csv "$OUT/bom.csv"
rm -rf "$OUT/work"
( cd "$OUT/gerbers" && zip -q ../gerbers.zip ./* )
echo "OK: $OUT/{gerbers.zip,cpl.csv,bom.csv}"
