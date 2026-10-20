#!/usr/bin/env bash
# Build the project's own 3D models (tools/gen_3d.py) in FreeCAD's GUI, offscreen.
# USB_C_Receptacle_HRO_TYPE-C-31-M-12.step is vendored as-is from KiCad's bundled
# royalblue54L_feather demo (same footprint, zero offset): KiCad's library ships none.
set -euo pipefail
cd "$(dirname "$0")/.."
timeout 300 flatpak run --env=QT_QPA_PLATFORM=offscreen --command=FreeCAD org.freecad.FreeCAD \
    tools/gen_3d.py >/dev/null 2>&1 || true
cat zigbee_toy/build/gen_3d.log
! grep -q Traceback zigbee_toy/build/gen_3d.log
