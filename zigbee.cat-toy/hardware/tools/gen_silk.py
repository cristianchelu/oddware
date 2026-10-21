#!/usr/bin/env python3
"""Silkscreen pass on the routed board (spec in silk.py). Runs inside KiCad's Python:
    flatpak run --command=python3 org.kicad.kicad tools/gen_silk.py

  * clips footprint silk that crosses the board edge (overhanging JSTs, USB-C, module)
  * passive refs -> F.Fab; selected refs placed on F.Silkscreen clear of everything
  * front legends auto-placed near their anchor; back legends + title near their nominal spot
"""
import math
import os
import sys

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from gen_pcb import PCB, MM, v, BOARD_W, BOARD_H, CORNER_R  # noqa: E402
import silk  # noqa: E402

T = pcbnew.ToMM
EDGE = 0.25          # silk-to-board-edge
PAD_CLR = 0.15       # silk-to-pad / via / hole
SILK_CLR = 0.12      # silk-to-silk
BODY_CLR = 0.1      # silk-to-component body (F.Fab outline)
THICK = 0.15


def inside_board(x, y, m=EDGE):
    if not (m <= x <= BOARD_W - m and m <= y <= BOARD_H - m):
        return False
    r = CORNER_R
    for cx in (r, BOARD_W - r):
        for cy in (r, BOARD_H - r):
            if (x - cx) * (cx - BOARD_W / 2) > 0 and (y - cy) * (cy - BOARD_H / 2) > 0:
                if math.hypot(x - cx, y - cy) > r - m:
                    return False
    return True


def bbox_mm(item):
    b = item.GetBoundingBox()
    return [T(b.GetLeft()), T(b.GetTop()), T(b.GetRight()), T(b.GetBottom())]


def grow(r, d):
    return [r[0] - d, r[1] - d, r[2] + d, r[3] + d]


def hit(a, b):
    return a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]


# ------------------------------------------------------------------ edge clipping
def clip_footprint_silk(board, footprints):
    n_cut = 0
    # snapshot first: the SWIG proxies go stale once a footprint's drawings are edited
    snap = [(fp, list(fp.GraphicalItems())) for fp in footprints]
    for fp, items in snap:
        for g in items:
            if g.GetLayer() != pcbnew.F_SilkS or not isinstance(g, pcbnew.PCB_SHAPE):
                continue
            x1, y1, x2, y2 = bbox_mm(g)
            if all(inside_board(x, y, m=EDGE - 0.05) for x in (x1, x2) for y in (y1, y2)):
                continue
            n_cut += 1
            w = g.GetWidth()
            segs = []
            if g.GetShape() == pcbnew.SHAPE_T_SEGMENT:
                segs = [((T(g.GetStart().x), T(g.GetStart().y)), (T(g.GetEnd().x), T(g.GetEnd().y)))]
            elif g.GetShape() == pcbnew.SHAPE_T_RECT:
                c = [(x1, y1), (x2, y1), (x2, y2), (x1, y2)]
                segs = [(c[i], c[(i + 1) % 4]) for i in range(4)]
            fp.Remove(g)
            for (ax, ay), (bx, by) in segs:
                n = max(2, int(math.hypot(bx - ax, by - ay) / 0.02))
                run = None
                for i in range(n + 1):
                    t = i / n
                    p = (ax + (bx - ax) * t, ay + (by - ay) * t)
                    ok = inside_board(*p, m=EDGE + T(w) / 2)
                    if ok and run is None:
                        run = [p, p]
                    elif ok:
                        run[1] = p
                    if (not ok or i == n) and run is not None:
                        if math.hypot(run[1][0] - run[0][0], run[1][1] - run[0][1]) > 0.2:
                            s = pcbnew.PCB_SHAPE(fp, pcbnew.SHAPE_T_SEGMENT)
                            s.SetLayer(pcbnew.F_SilkS); s.SetWidth(w)
                            s.SetStart(v(*run[0])); s.SetEnd(v(*run[1]))
                            fp.Add(s)
                        run = None
    return n_cut


# ------------------------------------------------------------------ obstacles
def body_box(fp):
    """Component body: the F.Fab outline (courtyards carry assembly margin we can't afford)."""
    boxes = [bbox_mm(g) for g in fp.GraphicalItems()
             if g.GetLayer() == pcbnew.F_Fab and isinstance(g, pcbnew.PCB_SHAPE)]
    if not boxes:
        b = fp.GetCourtyard(pcbnew.F_Cu).BBox()
        return [T(b.GetLeft()), T(b.GetTop()), T(b.GetRight()), T(b.GetBottom())]
    return [min(b[0] for b in boxes), min(b[1] for b in boxes), max(b[2] for b in boxes), max(b[3] for b in boxes)]


def obstacles(board, footprints, side):
    """(name, rect) silk on `side` must stay out of: copper openings, holes, bodies, other silk."""
    cu = pcbnew.F_Cu if side == 'F' else pcbnew.B_Cu
    sl = pcbnew.F_SilkS if side == 'F' else pcbnew.B_SilkS
    obs = []
    for fp in footprints:
        ref = fp.GetReference()
        for p in fp.Pads():
            if p.IsOnLayer(cu) or p.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH:
                obs.append((f'{ref}.{p.GetNumber()}', grow(bbox_mm(p), PAD_CLR)))
        if ref.startswith('H'):          # screw head
            b = fp.GetCourtyard(pcbnew.F_Cu).BBox()
            obs.append((ref, [T(b.GetLeft()), T(b.GetTop()), T(b.GetRight()), T(b.GetBottom())]))
        elif side == 'F':
            obs.append((ref + ' body', grow(body_box(fp), BODY_CLR)))
        for g in fp.GraphicalItems():
            if g.GetLayer() == sl:
                obs.append((ref + ' silk', grow(bbox_mm(g), SILK_CLR)))
    for t in board.GetTracks():
        # vias are tented both sides: back legends may cross them, front refs keep clear for legibility
        if isinstance(t, pcbnew.PCB_VIA) and side == 'F':
            obs.append((f'via ({T(t.GetPosition().x):.2f}, {T(t.GetPosition().y):.2f})', grow(bbox_mm(t), PAD_CLR)))
    return obs


def text_box(t):
    """Ink extent (strokes incl. width); GetBoundingBox() pads text by ~0.25 mm a side."""
    b = t.GetEffectiveTextShape().BBox()
    return [T(b.GetLeft()), T(b.GetTop()), T(b.GetRight()), T(b.GetBottom())]


def blocker(r, obs):
    if not all(inside_board(x, y) for x in (r[0], r[2]) for y in (r[1], r[3])):
        return 'board edge'
    return next((n for n, o in obs if hit(r, o)), None)


def fits(r, obs):
    return blocker(r, obs) is None


def size_text(t, size):
    t.SetTextSize(v(size, size))
    t.SetTextThickness(MM(THICK if size >= 0.8 else 0.1))
    t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_CENTER)
    t.SetVertJustify(pcbnew.GR_TEXT_V_ALIGN_CENTER)


def extents(t, rot):
    """Text box relative to its anchor at this rotation (centre-justified)."""
    t.SetTextAngleDegrees(rot)
    t.SetPosition(v(0, 0))
    return text_box(t)


def try_at(t, x, y, rot, obs, ext):
    e = ext[rot]
    r = [x + e[0], y + e[1], x + e[2], y + e[3]]
    if not fits(grow(r, SILK_CLR / 2), obs):
        return None
    t.SetTextAngleDegrees(rot)
    t.SetPosition(v(x, y))
    return r


def place_near_box(t, body, obs, rots=(0, 90)):
    """Try positions hugging each side of `body`; nearest, horizontal first."""
    best = None
    ext = {rot: extents(t, rot) for rot in rots}
    for rot in rots:
        b = ext[rot]
        w, h = b[2] - b[0], b[3] - b[1]
        cx, cy = (body[0] + body[2]) / 2, (body[1] + body[3]) / 2
        for gi in range(12):
            g = 0.05 + gi * 0.15
            for si in range(0, 21):
                s = (si + 1) // 2 * 0.2 * (1 if si % 2 else -1)
                for side, (x, y) in (('top', (cx + s, body[1] - g - h / 2)),
                                     ('bottom', (cx + s, body[3] + g + h / 2)),
                                     ('left', (body[0] - g - w / 2, cy + s)),
                                     ('right', (body[2] + g + w / 2, cy + s))):
                    score = g + abs(s) * 0.6 + (0.5 if rot else 0)
                    if best and score >= best[0]:
                        continue
                    r = try_at(t, x, y, rot, obs, ext)
                    if r:
                        best = (score, x, y, rot, r)
    if best:
        t.SetTextAngleDegrees(best[3]); t.SetPosition(v(best[1], best[2]))
        return best[4]
    return None


def place_near_point(t, x0, y0, obs, rots=(0,), rmax=2.5):
    ext = {rot: extents(t, rot) for rot in rots}
    for ri in range(int(rmax / 0.1) + 1):
        r = ri * 0.1
        steps = 1 if r == 0 else 16
        for rot in rots:
            for k in range(steps):
                a = 2 * math.pi * k / steps
                rr = try_at(t, x0 + r * math.cos(a), y0 + r * math.sin(a), rot, obs, ext)
                if rr:
                    return rr
    return None


def paw(board, cx, cy, size, obs):
    """Pad + four toes, filled circles on B.Silkscreen (mirrored view reads the same)."""
    s = size / 2.6
    parts = [(0, 0.45, 0.62), (-0.78, -0.25, 0.3), (-0.3, -0.72, 0.3), (0.3, -0.72, 0.3), (0.78, -0.25, 0.3)]
    circles = [(cx + dx * s, cy + dy * s, r * s) for dx, dy, r in parts]
    circles += [(cx + dx * s, cy + 0.55 * s, 0.5 * s) for dx in (-0.22, 0.22)]   # wider pad
    for x, y, r in circles:
        b = blocker(grow([x - r, y - r, x + r, y + r], SILK_CLR), obs)
        if b:
            print(f'WARNING: paw print hits {b}')
    for x, y, r in circles:
        c = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_CIRCLE)
        c.SetLayer(pcbnew.B_SilkS); c.SetCenter(v(x, y)); c.SetEnd(v(x + r - 0.05, y))
        c.SetWidth(MM(0.1)); c.SetFilled(True)
        board.Add(c)
        obs.append(('paw', grow([x - r, y - r, x + r, y + r], SILK_CLR)))


def main():
    board = pcbnew.LoadBoard(PCB)
    # Any BOARD/FOOTPRINT Remove() leaves pcbnew's SWIG proxies stale - even across a fresh
    # LoadBoard - so phase 1 (removals: old board silk, edge clipping) snapshots everything up
    # front, saves, and re-execs this script for phase 2 (placement, add-only).
    if not os.environ.get('SILK_PHASE2'):
        footprints = list(board.GetFootprints())
        old = [d for d in board.GetDrawings() if d.GetLayer() in (pcbnew.F_SilkS, pcbnew.B_SilkS)]
        n = clip_footprint_silk(board, footprints)
        for d in old:
            board.Remove(d)
        print('clipped', n, 'footprint silk shapes at the board edge; removed', len(old), 'old board silk items')
        pcbnew.SaveBoard(PCB, board)
        sys.stdout.flush()
        os.environ['SILK_PHASE2'] = '1'
        os.execv(sys.executable, [sys.executable, os.path.abspath(__file__)])
    footprints = list(board.GetFootprints())

    fps = {fp.GetReference(): fp for fp in footprints}
    pads = {f'{r}.{p.GetNumber()}': p for r, fp in fps.items() for p in fp.Pads()}

    # passives / holes: refs onto the fab layer, centred on the part
    for ref, fp in fps.items():
        f = fp.Reference()
        if ref in silk.FRONT_REFS:
            continue
        if ref.startswith('H'):
            f.SetVisible(False)
            continue
        f.SetLayer(pcbnew.B_Fab if fp.IsFlipped() else pcbnew.F_Fab); f.SetVisible(True)
        size_text(f, 0.5)
        f.SetTextAngleDegrees(fp.GetOrientationDegrees() % 180)
        f.SetPosition(fp.GetPosition())

    def add_text(layer, s, size):
        t = pcbnew.PCB_TEXT(board)
        t.SetText(s); t.SetLayer(layer); size_text(t, size)
        if layer == pcbnew.B_SilkS:
            t.SetMirrored(True)
        board.Add(t)
        return t

    def put(t, what, x, y, rot, obs):
        """Fixed position: place it and report anything it lands on."""
        t.SetTextAngleDegrees(rot); t.SetPosition(v(x, y))
        r = text_box(t)
        hit_by = blocker(grow(r, SILK_CLR / 2), obs)
        if hit_by:
            print(f'WARNING: {what} at ({x}, {y}) hits {hit_by}')
        obs.append((what, grow(r, SILK_CLR)))

    obs = obstacles(board, footprints, 'F')
    # front legends first (they're anchored to a pin), then the refs: connectors, then crowded small parts
    for s, anchor, size in silk.FRONT_LABELS:
        t = add_text(pcbnew.F_SilkS, s, size)
        if len(anchor) == 3:
            put(t, f'front {s!r}', *anchor, obs)
            continue
        x, y = anchor if isinstance(anchor, tuple) else \
            (T(pads[anchor].GetPosition().x), T(pads[anchor].GetPosition().y))
        r = place_near_point(t, x, y, obs, rots=(0,), rmax=3.0)
        if r is None:
            print(f'WARNING: front label {s!r} near {anchor} dropped')
            board.Remove(t)
        else:
            obs.append((f'front {s!r}', grow(r, SILK_CLR)))
    order = sorted(silk.FRONT_REFS, key=lambda r: (r not in silk.REF_AT, r[0] != 'J', r))
    for ref in order:
        fp = fps[ref]
        f = fp.Reference()
        f.SetLayer(pcbnew.F_SilkS); f.SetVisible(True)
        size_text(f, 0.8)
        if ref in silk.REF_AT:
            put(f, ref, *silk.REF_AT[ref], obs)
            continue
        body = body_box(fp)
        # clamp the search box to the board for overhanging connectors
        body = [max(body[0], 0), max(body[1], 0), min(body[2], BOARD_W), min(body[3], BOARD_H)]
        r = place_near_box(f, grow(body, BODY_CLR), obs)
        if r is None:
            print('WARNING: no room for', ref, '- moved to F.Fab')
            f.SetLayer(pcbnew.F_Fab); size_text(f, 0.5)
            f.SetTextAngleDegrees(0); f.SetPosition(fp.GetPosition())
        else:
            obs.append((ref, grow(r, SILK_CLR)))

    obs = obstacles(board, footprints, 'B')
    paw(board, *silk.PAW, obs)
    for s, x, y, rot, size in silk.BACK_TEXT:
        put(add_text(pcbnew.B_SilkS, s, size), f'back {s!r}', x, y, rot, obs)
    # test pad labels: the side given in silk.py (export_dsn kept vias out of that spot), else nearest side
    for ref, fp in sorted(fps.items(), key=lambda kv: int(kv[0][2:]) if kv[0].startswith('TP') else 0):
        if not ref.startswith('TP'):
            continue
        t = add_text(pcbnew.B_SilkS, fp.GetValue(), silk.TP_LABEL_SIZE)
        ring = [bbox_mm(g) for g in fp.GraphicalItems() if g.GetLayer() == pcbnew.B_SilkS][0]
        e = extents(t, 0)
        cx, cy = (ring[0] + ring[2]) / 2, (ring[1] + ring[3]) / 2
        side = silk.TP_LABEL_SIDE.get(ref, 'right')
        w, h = e[2] - e[0], e[3] - e[1]
        x, y = {'right': (ring[2] + 0.25 + w / 2, cy), 'left': (ring[0] - 0.25 - w / 2, cy),
                'below': (cx, ring[3] + 0.25 + h / 2), 'above': (cx, ring[1] - 0.25 - h / 2)}[side]
        r = try_at(t, x, y, 0, obs, {0: e})
        if r is None:
            print(f'WARNING: {ref} label {fp.GetValue()!r} at its spot hits {blocker(grow([x + e[0], y + e[1], x + e[2], y + e[3]], SILK_CLR / 2), obs)}')
            r = place_near_box(t, grow(ring, 0.05), obs, rots=(0,))
        if r is None:
            print(f'WARNING: no room for the {ref} label {fp.GetValue()!r}')
            board.Remove(t)
        else:
            obs.append((f'{ref} label', grow(r, SILK_CLR)))

    pcbnew.SaveBoard(PCB, board)
    print('silkscreen done')


if __name__ == '__main__':
    main()
