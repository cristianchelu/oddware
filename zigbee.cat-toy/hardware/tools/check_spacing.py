#!/usr/bin/env python3
"""Assembly spacing between neighbouring parts, for pairs whose courtyards overlap (KiCad flags those, but
the library courtyards carry margins this board can't afford). Fails on part bodies (F.Fab outline) closer
than 0.2 mm, or pads of different nets closer than 0.15 mm. Runs inside KiCad's Python."""
import itertools
import os
import sys

import pcbnew

MIN_BODY, MIN_PAD = 0.2, 0.15
T = pcbnew.ToMM
b = pcbnew.LoadBoard(os.path.join(os.path.dirname(__file__), '..', 'zigbee_toy', 'zigbee_toy.kicad_pcb'))


def rect(bb):
    return (T(bb.GetLeft()), T(bb.GetTop()), T(bb.GetRight()), T(bb.GetBottom()))


def body(fp):
    bbs = [rect(g.GetBoundingBox()) for g in fp.GraphicalItems()
           if g.GetLayer() == pcbnew.F_Fab and isinstance(g, pcbnew.PCB_SHAPE)]
    return (min(r[0] for r in bbs), min(r[1] for r in bbs), max(r[2] for r in bbs), max(r[3] for r in bbs)) if bbs else None


def gap(a, c):
    dx = max(a[0] - c[2], c[0] - a[2], 0)
    dy = max(a[1] - c[3], c[1] - a[3], 0)
    return (dx * dx + dy * dy) ** 0.5


fps = [f for f in b.GetFootprints() if not f.IsFlipped() and not f.GetReference().startswith(('TP', 'H'))]
bad, worst = [], (9, '')
for f1, f2 in itertools.combinations(fps, 2):
    a = pcbnew.SHAPE_POLY_SET(f1.GetCourtyard(pcbnew.F_Cu))
    a.BooleanIntersection(f2.GetCourtyard(pcbnew.F_Cu))
    if not (a.OutlineCount() and a.Area() > 1e3):
        continue
    pair = f'{f1.GetReference()}-{f2.GetReference()}'
    b1, b2 = body(f1), body(f2)
    if b1 and b2:
        g = gap(b1, b2)
        worst = min(worst, (g, pair + ' bodies'))
        if g < MIN_BODY:
            bad.append(f'{pair}: bodies {g:.2f} mm apart')
    for p in f1.Pads():
        for q in f2.Pads():
            if p.GetNetname() and p.GetNetname() == q.GetNetname():
                continue
            g = gap(rect(p.GetBoundingBox()), rect(q.GetBoundingBox()))
            if g < MIN_PAD:
                bad.append(f'{pair}: pads {p.GetNumber()}/{q.GetNumber()} {g:.2f} mm apart')
for m in bad:
    print('SPACING:', m)
print(f'spacing: {"FAIL" if bad else "ok"}, closest bodies {worst[0]:.2f} mm ({worst[1]})')
sys.exit(1 if bad else 0)
