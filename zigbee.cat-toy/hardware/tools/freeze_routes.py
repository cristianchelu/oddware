#!/usr/bin/env python3
"""One-off: turn the autorouted (unlocked) copper of the current board into tools/routes_frozen.py.
Run on a board that has hand routes + an imported SES but no stitching:
    gen_pcb.py layout && gen_pcb.py ses && freeze_routes.py
"""
import os
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
b = pcbnew.LoadBoard(os.path.join(HERE, '..', 'zigbee_toy', 'zigbee_toy.kicad_pcb'))
T = lambda v: round(pcbnew.ToMM(v), 4)  # noqa: E731
segs, vias = {}, []
for t in b.GetTracks():
    if t.IsLocked():
        continue
    if t.GetClass() == 'PCB_VIA':
        vias.append((t.GetNetname(), T(t.GetPosition().x), T(t.GetPosition().y), T(t.GetWidth(pcbnew.F_Cu)), T(t.GetDrill())))
        continue
    key = (t.GetNetname(), 'F' if t.GetLayer() == pcbnew.F_Cu else 'B', T(t.GetWidth()))
    segs.setdefault(key, []).append(((T(t.GetStart().x), T(t.GetStart().y)), (T(t.GetEnd().x), T(t.GetEnd().y))))


def chains(edges):
    """Greedy: join segments end to end into polylines."""
    edges = list(edges)
    out = []
    while edges:
        a, b2 = edges.pop()
        line = [a, b2]
        grown = True
        while grown:
            grown = False
            for i, (p, q) in enumerate(edges):
                if p == line[-1] or q == line[-1]:
                    line.append(q if p == line[-1] else p); edges.pop(i); grown = True; break
                if p == line[0] or q == line[0]:
                    line.insert(0, q if p == line[0] else p); edges.pop(i); grown = True; break
        out.append(line)
    return out


lines = ['"""Frozen copy of the Freerouting result (2026-10-20, rev A floorplan v9). GENERATED ONCE by',
         'freeze_routes.py; edit by hand from here on. Loaded after routes.py\'s hand routes."""',
         '', 'ROUTES = [']
n = 0
for (net, lyr, w), e in sorted(segs.items()):
    for line in chains(e):
        lines.append(f'    ({net!r}, {lyr!r}, {w}, {tuple(line)!r}),')
        n += 1
lines += [']', '', 'VIAS = [']
for v in sorted(vias):
    lines.append(f'    {v!r},')
lines += [']', '']
open(os.path.join(HERE, 'routes_frozen.py'), 'w').write('\n'.join(lines))
print('frozen', n, 'polylines,', len(vias), 'vias')
