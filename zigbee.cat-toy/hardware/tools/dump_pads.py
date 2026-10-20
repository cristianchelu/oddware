"""Print pad positions grouped by net (run in KiCad python)."""
import os, sys, pcbnew
b = pcbnew.LoadBoard(os.path.join(os.path.dirname(__file__), '..', 'zigbee_toy', 'zigbee_toy.kicad_pcb'))
nets = {}
for fp in b.GetFootprints():
    for p in fp.Pads():
        n = p.GetNetname()
        if not n:
            continue
        pos = p.GetPosition(); sz = p.GetSize()
        nets.setdefault(n, []).append(f"{fp.GetReference()}.{p.GetNumber()}@({pcbnew.ToMM(pos.x):.2f},{pcbnew.ToMM(pos.y):.2f})"
                                     f"[{pcbnew.ToMM(sz.x):.2f}x{pcbnew.ToMM(sz.y):.2f}{'T' if p.HasHole() else ''}]")
for n in sorted(nets):
    print(n, ' '.join(sorted(nets[n])))
