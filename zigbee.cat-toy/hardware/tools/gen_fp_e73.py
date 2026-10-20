#!/usr/bin/env python3
"""Build the Ebyte E73-2G4M08S1C KiCad footprint from LCSC/EasyEDA data (C356849).

Source: easyeda/C356849.json (EasyEDA package WIRELM-SMD_E73-2G4M08S1C, 1 unit = 10 mil).
Pad geometry is taken from each pad's polygon bounding box. Origin = module centre.
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from sexpr import Sym as S, dump  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
U = 0.254  # mm per EasyEDA unit


def main():
    d = json.load(open(os.path.join(HERE, 'easyeda', 'C356849.json')))['result']['packageDetail']['dataStr']
    pads = []
    outline = None
    for s in d['shape']:
        f = s.split('~')
        if f[0] == 'PAD':
            pts = list(map(float, f[10].split()))
            xs, ys = pts[0::2], pts[1::2]
            pads.append((f[8], min(xs), max(xs), min(ys), max(ys)))
        elif f[0] == 'SOLIDREGION' and f[1] == '99':      # component outline
            nums = [float(t) for t in f[3].replace('M', ' ').replace('L', ' ').replace('Z', ' ').split()]
            xs, ys = nums[0::2], nums[1::2]
            outline = (min(xs), max(xs), min(ys), max(ys))
    assert len(pads) == 43 and outline
    cx, cy = (outline[0] + outline[1]) / 2, (outline[2] + outline[3]) / 2

    def mm(v, c):
        return round((v - c) * U, 3)

    ox1, ox2, oy1, oy2 = mm(outline[0], cx), mm(outline[1], cx), mm(outline[2], cy), mm(outline[3], cy)
    fp = [S('footprint'), 'Ebyte_E73-2G4M08S1C',
          [S('version'), 20250114], [S('generator'), 'gen_fp_e73.py'],
          [S('layer'), 'F.Cu'],
          [S('descr'), 'Ebyte E73-2G4M08S1C nRF52840 module, 13x18 mm, ceramic antenna. Pads from LCSC C356849 EasyEDA footprint.'],
          [S('tags'), 'nRF52840 BLE Zigbee module Ebyte'],
          [S('property'), 'Reference', 'REF**', [S('at'), 0, oy1 - 1.5, 0], [S('layer'), 'F.SilkS'],
           [S('effects'), [S('font'), [S('size'), 1, 1], [S('thickness'), 0.15]]]],
          [S('property'), 'Value', 'E73-2G4M08S1C', [S('at'), 0, oy2 + 1.5, 0], [S('layer'), 'F.Fab'],
           [S('effects'), [S('font'), [S('size'), 1, 1], [S('thickness'), 0.15]]]],
          [S('attr'), S('smd')]]

    def line(x1, y1, x2, y2, layer, w):
        return [S('fp_line'), [S('start'), x1, y1], [S('end'), x2, y2],
                [S('stroke'), [S('width'), w], [S('type'), S('solid')]], [S('layer'), layer]]

    def rect(x1, y1, x2, y2, layer, w):
        return [S('fp_rect'), [S('start'), x1, y1], [S('end'), x2, y2],
                [S('stroke'), [S('width'), w], [S('type'), S('solid')]],
                [S('fill'), S('no')], [S('layer'), layer]]

    fp.append(rect(ox1, oy1, ox2, oy2, 'F.Fab', 0.1))
    # antenna end = the edge without pads (bottom, +Y)
    pad_max_y = max(mm(p[4], cy) for p in pads)
    ant_y = round(pad_max_y + 0.4, 2)
    fp.append(line(ox1, ant_y, ox2, ant_y, 'F.Fab', 0.1))
    fp.append([S('fp_text'), S('user'), 'ANTENNA', [S('at'), 0, (ant_y + oy2) / 2, 0], [S('layer'), 'F.Fab'],
               [S('effects'), [S('font'), [S('size'), 0.8, 0.8], [S('thickness'), 0.12]]]])
    # silkscreen: antenna-end bracket only (edges are covered by castellated pads)
    fp.append(line(ox1 - 0.15, ant_y, ox1 - 0.15, oy2 + 0.15, 'F.SilkS', 0.12))
    fp.append(line(ox1 - 0.15, oy2 + 0.15, ox2 + 0.15, oy2 + 0.15, 'F.SilkS', 0.12))
    fp.append(line(ox2 + 0.15, oy2 + 0.15, ox2 + 0.15, ant_y, 'F.SilkS', 0.12))
    # courtyard around pads + body
    px1 = min(mm(p[1], cx) for p in pads)
    px2 = max(mm(p[2], cx) for p in pads)
    py1 = min(mm(p[3], cy) for p in pads)
    fp.append(rect(round(min(px1, ox1) - 0.25, 2), round(min(py1, oy1) - 0.25, 2),
                   round(max(px2, ox2) + 0.25, 2), round(oy2 + 0.25, 2), 'F.CrtYd', 0.05))
    for num, x1, x2, y1, y2 in sorted(pads, key=lambda p: int(p[0])):
        x, y = mm((x1 + x2) / 2, cx), mm((y1 + y2) / 2, cy)
        w, h = round((x2 - x1) * U, 3), round((y2 - y1) * U, 3)
        fp.append([S('pad'), num, S('smd'), S('rect'), [S('at'), x, y], [S('size'), w, h],
                   [S('layers'), 'F.Cu', 'F.Paste', 'F.Mask']])
        if num == '1':
            # pin-1 marker on silk, outside the pad
            fp.append([S('fp_circle'), [S('center'), round(x + w / 2 + 0.5, 2), y],
                       [S('end'), round(x + w / 2 + 0.75, 2), y],
                       [S('stroke'), [S('width'), 0.12], [S('type'), S('solid')]],
                       [S('fill'), S('yes')], [S('layer'), 'F.SilkS']])
    # antenna keepout: no copper of any kind under/around the ceramic antenna
    ky1 = ant_y
    fp.append([S('zone'), [S('layers'), 'F.Cu', 'B.Cu', 'In1.Cu', 'In2.Cu'],
               [S('name'), 'ANTENNA_KEEPOUT'], [S('hatch'), S('edge'), 0.5],
               [S('connect_pads'), [S('clearance'), 0]], [S('min_thickness'), 0.25],
               [S('keepout'), [S('tracks'), S('not_allowed')], [S('vias'), S('not_allowed')],
                [S('pads'), S('not_allowed')], [S('copperpour'), S('not_allowed')],
                [S('footprints'), S('allowed')]],
               [S('fill'), [S('thermal_gap'), 0.5], [S('thermal_bridge_width'), 0.5]],
               [S('polygon'), [S('pts'), [S('xy'), ox1 - 1, ky1], [S('xy'), ox2 + 1, ky1],
                                [S('xy'), ox2 + 1, oy2 + 4], [S('xy'), ox1 - 1, oy2 + 4]]]])
    # 3D body from tools/gen_3d.py
    fp.append([S('model'), '${KIPRJMOD}/zigbee_toy.3dshapes/Ebyte_E73-2G4M08S1C.step',
               [S('offset'), [S('xyz'), 0, 0, 0]], [S('scale'), [S('xyz'), 1, 1, 1]],
               [S('rotate'), [S('xyz'), 0, 0, 0]]])
    out = os.path.join(HERE, '..', 'zigbee_toy', 'zigbee_toy.pretty')
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, 'Ebyte_E73-2G4M08S1C.kicad_mod'), 'w') as f:
        f.write(dump(fp) + '\n')
    print(f'outline {ox2 - ox1:.2f} x {oy2 - oy1:.2f} mm, pads {len(pads)}, antenna keepout from y={ant_y}')
    for num, x1, x2, y1, y2 in sorted(pads, key=lambda p: int(p[0]))[:3]:
        print(num, mm((x1 + x2) / 2, cx), mm((y1 + y2) / 2, cy), round((x2 - x1) * U, 3), round((y2 - y1) * U, 3))


if __name__ == '__main__':
    main()
