#!/usr/bin/env python3
"""Build the ESP32-C6-MINI-1U KiCad footprint from LCSC/EasyEDA data (C20627095).

Source: easyeda/C20627095.json (EasyEDA package WIFIM-SMD_61P-L13.2-W12.5-P0.80, 1 unit = 10 mil).
Origin = module body centre. Same orientation as Espressif's top view: pins 1-11 on the left
(pin 1 at the top), 12-24 along the bottom, 25-35 up the right, GND row 36-48 along the top,
u.FL connector in the top-left corner (1.70 mm in from the left edge, 1.55 mm down from the top).
The pads also fit the ESP32-C6-MINI-1 (PCB antenna); its antenna area is a copper keepout here.
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from sexpr import Sym as S, dump  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
U = 0.254  # mm per EasyEDA unit
NAME = 'ESP32-C6-MINI-1U'
UFL = (-6.6 + 1.70, -6.25 + 1.55)       # u.FL centre from the datasheet top view
MINI1_EXTRA = 16.6 - 12.5               # the PCB-antenna MINI-1 is this much longer, at the u.FL end


def line(x1, y1, x2, y2, layer, w):
    return [S('fp_line'), [S('start'), x1, y1], [S('end'), x2, y2],
            [S('stroke'), [S('width'), w], [S('type'), S('solid')]], [S('layer'), layer]]


def rect(x1, y1, x2, y2, layer, w):
    return [S('fp_rect'), [S('start'), x1, y1], [S('end'), x2, y2],
            [S('stroke'), [S('width'), w], [S('type'), S('solid')]],
            [S('fill'), S('no')], [S('layer'), layer]]


def main():
    d = json.load(open(os.path.join(HERE, 'easyeda', 'C20627095.json')))['result']['packageDetail']['dataStr']
    hx, hy = d['head']['x'], d['head']['y']
    pads, outline = [], None
    for s in d['shape']:
        f = s.split('~')
        if f[0] == 'PAD':
            pads.append((f[8], float(f[2]), float(f[3]), float(f[4]), float(f[5])))
        elif f[0] == 'SOLIDREGION' and f[1] == '99':      # component outline
            nums = [float(t) for t in f[3].replace('M', ' ').replace('L', ' ').replace('Z', ' ').split()]
            outline = (min(nums[0::2]), max(nums[0::2]), min(nums[1::2]), max(nums[1::2]))
    assert len(pads) == 61 and outline
    cx, cy = (outline[0] + outline[1]) / 2, (outline[2] + outline[3]) / 2

    def mm(v, c):
        return round((v - c) * U, 3)

    bx, by = round((outline[1] - outline[0]) * U / 2, 2), round((outline[3] - outline[2]) * U / 2, 2)
    assert abs(bx - 6.6) < 0.05 and abs(by - 6.25) < 0.05, (bx, by)
    fp = [S('footprint'), NAME,
          [S('version'), 20250114], [S('generator'), 'gen_fp.py'],
          [S('layer'), 'F.Cu'],
          [S('descr'), 'Espressif ESP32-C6-MINI-1U, 13.2x12.5 mm, u.FL antenna. Pads from LCSC C20627095 EasyEDA footprint.'],
          [S('tags'), 'ESP32-C6 Zigbee Thread WiFi module Espressif'],
          [S('property'), 'Reference', 'REF**', [S('at'), 0, -by - 1.2, 0], [S('layer'), 'F.SilkS'],
           [S('effects'), [S('font'), [S('size'), 1, 1], [S('thickness'), 0.15]]]],
          [S('property'), 'Value', NAME, [S('at'), 0, by + 1.2, 0], [S('layer'), 'F.Fab'],
           [S('effects'), [S('font'), [S('size'), 1, 1], [S('thickness'), 0.15]]]],
          [S('attr'), S('smd')]]
    fp.append(rect(-bx, -by, bx, by, 'F.Fab', 0.1))
    fp.append([S('fp_circle'), [S('center'), *UFL], [S('end'), UFL[0] + 1.0, UFL[1]],
               [S('stroke'), [S('width'), 0.1], [S('type'), S('solid')]], [S('fill'), S('no')], [S('layer'), 'F.Fab']])
    fp.append([S('fp_text'), S('user'), 'u.FL', [S('at'), UFL[0] + 2.2, UFL[1], 0], [S('layer'), 'F.Fab'],
               [S('effects'), [S('font'), [S('size'), 0.6, 0.6], [S('thickness'), 0.1]]]])
    m = 0.15
    fp.append(rect(-bx - m, -by - m, bx + m, by + m, 'F.SilkS', 0.12))
    fp.append(rect(-bx - 0.25, -by - 0.25, bx + 0.25, by + 0.25, 'F.CrtYd', 0.05))
    for num, x, y, w, h in sorted(pads, key=lambda p: int(p[0])):
        px, py = mm(x, cx), mm(y, cy)
        w, h = round(w * U, 3), round(h * U, 3)
        layers = ['F.Cu', 'F.Paste', 'F.Mask']
        fp.append([S('pad'), num, S('smd'), S('rect'), [S('at'), px, py], [S('size'), w, h], [S('layers'), *layers]])
        if num == '1':      # pin-1 dot outside the silk outline
            fp.append([S('fp_circle'), [S('center'), round(-bx - 0.55, 2), py], [S('end'), round(-bx - 0.35, 2), py],
                       [S('stroke'), [S('width'), 0.12], [S('type'), S('solid')]],
                       [S('fill'), S('yes')], [S('layer'), 'F.SilkS']])
    # Same pads as the ESP32-C6-MINI-1 (PCB antenna), whose body runs 4.1 mm further at this end.
    # Keep copper out from just above the top pad row to the MINI-1's antenna end, on both layers,
    # so the antenna version drops onto the same footprint (its antenna then overhangs the board edge).
    top_pad = min(mm(y, cy) - h * U / 2 for _, x, y, w, h in pads)
    k_y2, k_y1 = round(top_pad - 0.25, 2), round(-by - MINI1_EXTRA, 2)
    fp.append([S('fp_rect'), [S('start'), -bx, k_y1], [S('end'), bx, -by],
               [S('stroke'), [S('width'), 0.1], [S('type'), S('dash')]], [S('fill'), S('no')], [S('layer'), 'Dwgs.User']])
    fp.append([S('fp_text'), S('user'), 'MINI-1 antenna', [S('at'), 0, round((k_y1 - by) / 2, 2), 0], [S('layer'), 'Dwgs.User'],
               [S('effects'), [S('font'), [S('size'), 0.6, 0.6], [S('thickness'), 0.1]]]])
    fp.append([S('zone'), [S('layers'), 'F.Cu', 'B.Cu'], [S('name'), 'MINI1_ANTENNA_KEEPOUT'], [S('hatch'), S('edge'), 0.5],
               [S('connect_pads'), [S('clearance'), 0]], [S('min_thickness'), 0.25],
               [S('keepout'), [S('tracks'), S('not_allowed')], [S('vias'), S('not_allowed')],
                [S('pads'), S('not_allowed')], [S('copperpour'), S('not_allowed')], [S('footprints'), S('allowed')]],
               [S('fill'), [S('thermal_gap'), 0.5], [S('thermal_bridge_width'), 0.5]],
               [S('polygon'), [S('pts'), [S('xy'), -bx, k_y1], [S('xy'), bx, k_y1], [S('xy'), bx, k_y2], [S('xy'), -bx, k_y2]]]])
    fp.append([S('model'), f'${{KIPRJMOD}}/zigbee_toy.3dshapes/{NAME}.step',
               [S('offset'), [S('xyz'), 0, 0, 0]], [S('scale'), [S('xyz'), 1, 1, 1]],
               [S('rotate'), [S('xyz'), 0, 0, 0]]])
    out = os.path.join(HERE, '..', 'zigbee_toy', 'zigbee_toy.pretty')
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, NAME + '.kicad_mod'), 'w') as f:
        f.write(dump(fp) + '\n')
    p1 = next(p for p in pads if p[0] == '1')
    print(f'{NAME}: body {2 * bx:.2f} x {2 * by:.2f} mm, {len(pads)} pads, pin 1 at ({mm(p1[1], cx)}, {mm(p1[2], cy)}), MINI-1 antenna keepout y {k_y1}..{k_y2}')


if __name__ == '__main__':
    main()
