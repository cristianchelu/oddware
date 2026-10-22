#!/usr/bin/env python3
"""Build zigbee_toy.kicad_pcb from the schematic netlist + placement.py.

Runs inside KiCad's Python:
    flatpak run --command=python3 org.kicad.kicad tools/gen_pcb.py place
    flatpak run --command=python3 org.kicad.kicad tools/gen_pcb.py dsn
    flatpak run --command=python3 org.kicad.kicad tools/gen_pcb.py ses
"""
import json
import os
import sys

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from sexpr import parse, find, findall  # noqa: E402
from placement import PLACE, BOARD_W, BOARD_H, CORNER_R, EDGE_KEEPOUT, EDGE_PARTS  # noqa: E402

PROJ = os.path.join(HERE, '..', 'zigbee_toy')
PCB = os.path.join(PROJ, 'zigbee_toy.kicad_pcb')
NETLIST = os.path.join(PROJ, 'build', 'zigbee_toy.net')
FP_DIRS = {'zigbee_toy': os.path.join(PROJ, 'zigbee_toy.pretty')}
SYS_FP = '/app/share/kicad/footprints'

MM = pcbnew.FromMM

# Net classes (also written to the .kicad_pro so DRC and the DSN export see them)
NETCLASSES = {
    'Default': dict(track=0.2, clearance=0.15, via=0.7, drill=0.3),
    'Power': dict(track=0.5, clearance=0.15, via=0.8, drill=0.4),
    'Supply': dict(track=0.3, clearance=0.15, via=0.7, drill=0.3),
}
NET_CLASS_OF = {
    'Power': ['CELL_P', 'CELL_N', 'VBAT', 'MOT_A', 'MOT_B', 'SPK_P', 'SPK_N', 'GND'],
    'Supply': ['VBUS', '3V3'],
}


# 3D models for footprints whose library model is missing: project copies in
# zigbee_toy.3dshapes (tools/gen_3d.sh), or an equivalent generic library body.
PROJECT_MODELS = {
    'U1': '${KIPRJMOD}/zigbee_toy.3dshapes/ESP32-C6-MINI-1U.step',
    'J1': '${KIPRJMOD}/zigbee_toy.3dshapes/USB_C_Receptacle_HRO_TYPE-C-31-M-12.step',
    'SW1': '${KIPRJMOD}/zigbee_toy.3dshapes/SW_Push_1P1T-MP_NO_Horizontal_Alps_SKRTLAE010.step',
    # TI's DSG0008A is the generic 2x2 mm WSON-8 with a 0.9x1.6 mm pad
    'U2': '${KICAD10_3DMODEL_DIR}/Package_SON.3dshapes/WSON-8-1EP_2x2mm_P0.5mm_EP0.9x1.6mm.step',
}


def set_models(board):
    for fp in board.GetFootprints():
        name = PROJECT_MODELS.get(fp.GetReference())
        if name:
            m = pcbnew.FP_3DMODEL()
            m.m_Filename = name
            fp.Models().clear()
            fp.Add3DModel(m)


def v(x, y):
    return pcbnew.VECTOR2I(MM(x), MM(y))


def write_project_rules():
    pro_path = os.path.join(PROJ, 'zigbee_toy.kicad_pro')
    pro = json.load(open(pro_path))
    classes = []
    for name, c in NETCLASSES.items():
        classes.append({
            'name': name, 'clearance': c['clearance'], 'track_width': c['track'],
            'via_diameter': c['via'], 'via_drill': c['drill'],
            'microvia_diameter': 0.3, 'microvia_drill': 0.1,
            'diff_pair_gap': 0.25, 'diff_pair_via_gap': 0.25, 'diff_pair_width': 0.2,
            'wire_width': 6, 'bus_width': 12, 'line_style': 0,
            'pcb_color': 'rgba(0, 0, 0, 0.000)', 'schematic_color': 'rgba(0, 0, 0, 0.000)',
            'priority': 2147483647 if name == 'Default' else 0,
        })
    patterns = [{'netclass': nc, 'pattern': n} for nc, nets in NET_CLASS_OF.items() for n in nets]
    pro['net_settings'] = {'classes': classes, 'meta': {'version': 4}, 'net_colors': None,
                           'netclass_assignments': None, 'netclass_patterns': patterns}
    # JLCPCB 2-layer standard capabilities, with a little margin
    pro.setdefault('board', {}).setdefault('design_settings', {})['rules'] = {
        'min_clearance': 0.127, 'min_track_width': 0.127, 'min_connection': 0.127,
        'min_via_diameter': 0.5, 'min_via_annular_width': 0.1, 'min_through_hole_diameter': 0.3,
        'min_hole_clearance': 0.25, 'min_hole_to_hole': 0.5, 'min_copper_edge_clearance': 0.3,
        'min_silk_clearance': 0.0, 'min_text_height': 0.8, 'min_text_thickness': 0.12,
        'solder_mask_to_copper_clearance': 0.0, 'min_microvia_diameter': 0.2,
        'min_microvia_drill': 0.1, 'max_error': 0.005, 'min_resolved_spokes': 2,
        'use_height_for_length_calcs': True, 'allow_blind_buried_vias': False,
        'allow_microvias': False, 'min_groove_width': 0.0,
    }
    # gen_silk.py clips footprint silk at the board edge on purpose, so the board copies differ
    pro['board']['design_settings'].setdefault('rule_severities', {})['lib_footprint_mismatch'] = 'ignore'
    # courtyards overlap by their library margins on this board; tools/check_spacing.py checks the real gaps
    pro['board']['design_settings']['rule_severities']['courtyards_overlap'] = 'warning'
    with open(pro_path, 'w') as f:
        json.dump(pro, f, indent=2)


def read_netlist():
    t = parse(open(NETLIST).read())
    comps = {}
    for c in findall(find(t, 'components'), 'comp'):
        ref = find(c, 'ref')[1]
        ts = find(c, 'tstamps')
        props = [find(pr, 'name')[1] for pr in findall(c, 'property')]
        comps[ref] = dict(value=find(c, 'value')[1], footprint=find(c, 'footprint')[1],
                          uuid=ts[1] if ts else None,
                          excl_bom='exclude_from_bom' in props)   # hand-soldered parts: keep them out of the JLC CPL too
    nets = {}
    for n in findall(find(t, 'nets'), 'net'):
        name = find(n, 'name')[1].lstrip('/')
        for node in findall(n, 'node'):
            ref, pin = find(node, 'ref')[1], find(node, 'pin')[1]
            if ref.startswith('#'):
                continue
            if name.startswith('unconnected-'):
                continue
            nets.setdefault(name, []).append((ref, pin))
    return comps, nets


def load_fp(fpid):
    lib, name = fpid.split(':')
    path = FP_DIRS.get(lib, os.path.join(SYS_FP, lib + '.pretty'))
    fp = pcbnew.FootprintLoad(path, name)
    if fp is None:
        raise SystemExit(f'footprint not found: {fpid}')
    fp.SetFPID(pcbnew.LIB_ID(lib, name))
    return fp


def outline(board):
    w, h, r = BOARD_W, BOARD_H, CORNER_R
    layer = pcbnew.Edge_Cuts

    def seg(a, b):
        s = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_SEGMENT)
        s.SetStart(v(*a)); s.SetEnd(v(*b)); s.SetLayer(layer); s.SetWidth(MM(0.1))
        board.Add(s)

    def arc(c, start, mid, end):
        s = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_ARC)
        s.SetArcGeometry(v(*start), v(*mid), v(*end)); s.SetLayer(layer); s.SetWidth(MM(0.1))
        board.Add(s)

    k = r * (1 - 0.7071067811865476)
    seg((r, 0), (w - r, 0)); seg((w, r), (w, h - r)); seg((w - r, h), (r, h)); seg((0, h - r), (0, r))
    arc(None, (w - r, 0), (w - k, k), (w, r))
    arc(None, (w, h - r), (w - k, h - k), (w - r, h))
    arc(None, (r, h), (k, h - k), (0, h - r))
    arc(None, (0, r), (k, k), (r, 0))


def gnd_zone(board, net, layer, prio):
    z = pcbnew.ZONE(board)
    z.SetLayer(layer)
    z.SetNet(net)
    z.SetAssignedPriority(prio)
    z.SetLocalClearance(MM(0.18))         # lets the pour squeeze between the back-side test pad traces
    z.SetMinThickness(MM(0.15))
    z.SetThermalReliefGap(MM(0.25))
    z.SetThermalReliefSpokeWidth(MM(0.3))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THT_THERMAL)   # SMD solid, THT thermal
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    poly = z.Outline()
    poly.NewOutline()
    for x, y in [(0, 0), (BOARD_W, 0), (BOARD_W, BOARD_H), (0, BOARD_H)]:
        poly.Append(MM(x), MM(y))
    z.SetZoneName(f'GND_{pcbnew.LayerName(layer)}')
    board.Add(z)


def place():
    write_project_rules()
    comps, nets = read_netlist()
    # Always start from an empty stub: FootprintLoad misbehaves after loading a populated board.
    fps = {ref: load_fp(c['footprint']) for ref, c in comps.items()}
    with open(PCB, 'w') as f:
        f.write('(kicad_pcb (version 20241229) (generator "pcbnew") (generator_version "9.0"))\n')
    board = pcbnew.LoadBoard(PCB)
    board.SetCopperLayerCount(2)
    netinfo = {}
    for name in sorted(nets):
        ni = pcbnew.NETINFO_ITEM(board, name)
        board.Add(ni)
        netinfo[name] = ni
    pad_net = {}
    for name, nodes in nets.items():
        for ref, pin in nodes:
            pad_net[(ref, pin)] = name
    missing = sorted(set(comps) - set(PLACE))
    if missing:
        raise SystemExit(f'no placement for {missing}')
    for ref, c in sorted(comps.items()):
        fp = fps[ref]
        fp.SetReference(ref)
        fp.SetValue(c['value'])
        if c['uuid']:
            fp.SetPath(pcbnew.KIID_PATH('/' + c['uuid']))
        x, y, rot, *side = PLACE[ref]
        if c['excl_bom']:
            fp.SetAttributes(fp.GetAttributes() | pcbnew.FP_EXCLUDE_FROM_BOM | pcbnew.FP_EXCLUDE_FROM_POS_FILES)
        board.Add(fp)
        fp.SetPosition(v(x, y))
        fp.SetOrientationDegrees(rot)
        if ref == 'SW1':
            fp.SetDuplicatePadNumbersAreJumpers(True)   # both '1' pads are the same switch terminal
        if ref.startswith('TP'):
            # bare probe pads, not parts: the library's 2 mm courtyard would forbid a tight grid
            for g in fp.GraphicalItems():
                if g.GetLayer() == pcbnew.F_CrtYd:
                    g.SetEnd(g.GetCenter() + v(0.6, 0))
        if side == ['B']:
            fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
        fp.Reference().SetTextSize(v(0.8, 0.8))
        fp.Reference().SetTextThickness(MM(0.12))
        fp.Value().SetVisible(False)
        if ref[0] in 'RC' or ref.startswith('H') or ref.startswith('TP'):
            fp.Reference().SetVisible(False)
        for pad in fp.Pads():
            if ref == 'J1' and pad.GetNumber() == 'SH':
                pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_FULL)
            n = pad_net.get((ref, pad.GetNumber()))
            if n:
                pad.SetNet(netinfo[n])
    set_models(board)
    check_edge_keepout(board, comps)
    outline(board)
    gnd_zone(board, netinfo['GND'], pcbnew.F_Cu, 0)
    gnd_zone(board, netinfo['GND'], pcbnew.B_Cu, 0)
    pcbnew.SaveBoard(PCB, board)
    print('placed', len(comps), 'footprints,', len(nets), 'nets')


def check_edge_keepout(board, comps):
    """JLC assembly: every assembled body >= EDGE_KEEPOUT from the board edge (F.Fab outline = body).
    Parts in EDGE_PARTS may reach their own edge only."""
    bad = []
    for fp in board.GetFootprints():
        ref = fp.GetReference()
        if comps[ref]['excl_bom'] or fp.IsFlipped():
            continue
        boxes = [g.GetBoundingBox() for g in fp.GraphicalItems()
                 if g.GetLayer() == pcbnew.F_Fab and isinstance(g, pcbnew.PCB_SHAPE)]
        if not boxes:
            boxes = [fp.GetCourtyard(pcbnew.F_Cu).BBox()]
        x1 = min(pcbnew.ToMM(b.GetLeft()) for b in boxes); x2 = max(pcbnew.ToMM(b.GetRight()) for b in boxes)
        y1 = min(pcbnew.ToMM(b.GetTop()) for b in boxes); y2 = max(pcbnew.ToMM(b.GetBottom()) for b in boxes)
        d = {'top': y1, 'bottom': BOARD_H - y2, 'left': x1, 'right': BOARD_W - x2}
        if ref in EDGE_PARTS:
            d.pop(EDGE_PARTS[ref])
        for side, dist in d.items():
            if dist < EDGE_KEEPOUT - 1e-3:
                bad.append(f'{ref} body {dist:.2f} mm from the {side} edge')
    if bad and os.environ.get('PLACE_DRAFT'):          # floorplanning: report, keep going
        print('DRAFT, edge keepout violated:\n  ' + '\n  '.join(bad))
        return
    if bad:
        raise SystemExit('edge keepout (' + str(EDGE_KEEPOUT) + ' mm) violated:\n  ' + '\n  '.join(bad))
    print(f'edge keepout ok: assembled bodies >= {EDGE_KEEPOUT} mm from the edges')


_LABEL_BOXES = []


def label_boxes():
    """Back-silk legend and test pad label areas (board mm): no vias there."""
    if _LABEL_BOXES:
        return _LABEL_BOXES
    import silk
    from design import TEST_POINTS
    label = {ref: text for ref, _, text in TEST_POINTS}
    boxes = [silk.text_box(x, y, rot, text, size) for text, x, y, rot, size in silk.BACK_TEXT]
    px, py, ps = silk.PAW
    boxes.append((px - ps / 2, py - ps / 2, px + ps / 2, py + ps / 2))
    for ref, side in silk.TP_LABEL_SIDE.items():
        x, y, *_ = PLACE[ref]
        boxes.append(silk.tp_label_box(x, y, label[ref], side))
    _LABEL_BOXES.extend(boxes)
    return boxes


def export_dsn():
    board = pcbnew.LoadBoard(PCB)
    # Autorouter only: forbid F.Cu tracks over the module's centre ground pad.
    u1 = board.FindFootprintByReference('U1')
    c = u1.GetPosition()
    cx, cy = pcbnew.ToMM(c.x), pcbnew.ToMM(c.y)
    ko = pcbnew.ZONE(board)
    ko.SetIsRuleArea(True)
    ko.SetDoNotAllowTracks(True)
    ko.SetDoNotAllowVias(False)
    ko.SetDoNotAllowPads(False)
    ko.SetDoNotAllowZoneFills(False)
    ko.SetDoNotAllowFootprints(False)
    ko.SetLayer(pcbnew.F_Cu)
    o = ko.Outline()
    o.NewOutline()
    for dx, dy in [(-3.3, -2.75), (3.3, -2.75), (3.3, 4.05), (-3.3, 4.05)]:  # the centre ground pad; the ring outside it takes escape vias
        o.Append(MM(cx + dx), MM(cy + dy))
    board.Add(ko)

    def no_tracks(layer, x1, y1, x2, y2):
        z = pcbnew.ZONE(board)
        z.SetIsRuleArea(True)
        z.SetDoNotAllowTracks(True)
        z.SetDoNotAllowVias(False)
        z.SetDoNotAllowPads(False)
        z.SetDoNotAllowZoneFills(False)
        z.SetDoNotAllowFootprints(False)
        z.SetLayer(layer)
        o = z.Outline()
        o.NewOutline()
        for px, py in [(x1, y1), (x2, y1), (x2, y2), (x1, y2)]:
            o.Append(MM(px), MM(py))
        board.Add(z)

    # footprint keepouts (the MINI-1 antenna area under U1) as board rule areas, so the DSN carries them
    for fp in board.GetFootprints():
        for fz in fp.Zones():
            if not fz.GetIsRuleArea():
                continue
            bb = fz.Outline().BBox()
            for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
                z = pcbnew.ZONE(board)
                z.SetIsRuleArea(True)
                z.SetDoNotAllowTracks(True)
                z.SetDoNotAllowVias(True)
                z.SetDoNotAllowPads(False)
                z.SetDoNotAllowZoneFills(True)
                z.SetDoNotAllowFootprints(False)
                z.SetLayer(layer)
                o = z.Outline()
                o.NewOutline()
                for px, py in [(bb.GetLeft(), bb.GetTop()), (bb.GetRight(), bb.GetTop()),
                               (bb.GetRight(), bb.GetBottom()), (bb.GetLeft(), bb.GetBottom())]:
                    o.Append(px, py)
                board.Add(z)
    # keep autorouted copper 0.5 mm off the board edge (both layers)
    e = 0.5
    for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
        no_tracks(layer, 0, 0, BOARD_W, e)
        no_tracks(layer, 0, BOARD_H - e, BOARD_W, BOARD_H)
        no_tracks(layer, 0, 0, e, BOARD_H)
        no_tracks(layer, BOARD_W - e, 0, BOARD_W, BOARD_H)
    # nothing on B.Cu under the 32 kHz crystal and its traces to the module
    y1 = board.FindFootprintByReference('Y1').GetPosition()
    yx, yy = pcbnew.ToMM(y1.x), pcbnew.ToMM(y1.y)
    no_tracks(pcbnew.B_Cu, yx - 1.65, cy + 5.7, yx + 2.05, yy + 1.3)
    # nothing under the USB-C receptacle: on F.Cu only solder mask would separate a track from the
    # shell; B.Cu stays solid so the shell ties into the ground plane
    # (F.Cu from the body's back edge; B.Cu only right of the D+/D- escape vias, which routes.py puts at x 30.45/31.15)
    j1fp = board.FindFootprintByReference('J1')
    j1 = j1fp.GetCourtyard(pcbnew.F_Cu).BBox()
    body_x = min(pcbnew.ToMM(g.GetBoundingBox().GetLeft()) for g in j1fp.GraphicalItems()
                 if g.GetLayer() == pcbnew.F_Fab and isinstance(g, pcbnew.PCB_SHAPE))
    no_tracks(pcbnew.F_Cu, body_x, pcbnew.ToMM(j1.GetTop()), BOARD_W, pcbnew.ToMM(j1.GetBottom()))
    no_tracks(pcbnew.B_Cu, 31.6, pcbnew.ToMM(j1.GetTop()), BOARD_W, pcbnew.ToMM(j1.GetBottom()))
    out = os.path.join(PROJ, 'build', 'zigbee_toy.dsn')
    ok = pcbnew.ExportSpecctraDSN(board, out)
    print('dsn', ok, out)


def import_ses():
    board = pcbnew.LoadBoard(PCB)
    ses = os.path.join(PROJ, 'build', 'zigbee_toy.ses')
    ok = pcbnew.ImportSpecctraSES(board, ses)
    pcbnew.SaveBoard(PCB, board)
    print('ses', ok)


def fill():
    board = pcbnew.LoadBoard(PCB)
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    pcbnew.SaveBoard(PCB, board)




def ratsnest():
    """Write build/ratsnest.kicad_pcb: the board plus per-net MST lines on User.1 (GND skipped)."""
    import math
    board = pcbnew.LoadBoard(PCB)
    pads = {}
    for fp in board.GetFootprints():
        for p in fp.Pads():
            n = p.GetNetname()
            if n and n != 'GND':
                pads.setdefault(n, []).append(p.GetPosition())
    for n, pts in pads.items():
        done = [pts[0]]
        todo = pts[1:]
        while todo:
            best = min(((math.dist((a.x, a.y), (b.x, b.y)), a, b) for a in done for b in todo),
                       key=lambda t: t[0])
            _, a, b = best
            s = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_SEGMENT)
            s.SetStart(a); s.SetEnd(b); s.SetLayer(pcbnew.User_1); s.SetWidth(MM(0.08))
            board.Add(s)
            done.append(b)
            todo.remove(b)
    pcbnew.SaveBoard(os.path.join(PROJ, 'build', 'ratsnest.kicad_pcb'), board)


def add_routes():
    """Add the hand routes from routes.py as locked tracks/vias."""
    import importlib
    import routes
    import routes_frozen
    importlib.reload(routes)
    all_routes = routes.ROUTES + routes_frozen.ROUTES
    all_vias = routes.VIAS + routes_frozen.VIAS
    # Expects a freshly placed board (no tracks): run via the 'layout' command.
    board = pcbnew.LoadBoard(PCB)
    pads = {}
    for fp in board.GetFootprints():
        for p in fp.Pads():
            pads.setdefault(f'{fp.GetReference()}.{p.GetNumber()}', p)
    nets = board.GetNetsByName()

    def pt(p):
        if isinstance(p, str):
            return pads[p].GetPosition()
        return v(*p)

    layer = {'F': pcbnew.F_Cu, 'B': pcbnew.B_Cu}

    def mm_pt(p):
        q = pt(p)
        return pcbnew.ToMM(q.x), pcbnew.ToMM(q.y)

    def octilinear(pts):
        """Insert elbows so every segment is 0/45/90 deg: leave pads straight, then 45."""
        out = [pts[0]]
        for a, b in zip(pts, pts[1:]):
            (ax, ay), (bx, by) = mm_pt(a), mm_pt(b)
            dx, dy = bx - ax, by - ay
            adx, ady = abs(dx), abs(dy)
            if min(adx, ady) < 0.06 or abs(adx - ady) < 0.06:
                out.append(b)
                continue
            m = min(adx, ady)
            sx, sy = (1 if dx > 0 else -1), (1 if dy > 0 else -1)
            if isinstance(a, str) or not isinstance(b, str):
                mid = (round(bx - sx * m, 4), round(by - sy * m, 4))   # straight, then 45 into b
            else:
                mid = (round(ax + sx * m, 4), round(ay + sy * m, 4))   # 45 first, straight into the pad
            out += [mid, b]
        return out

    def chamfer(pts, width, anchors):
        """Cut every corner of 90 degrees or sharper with a 45-degree chamfer. Vertices that other
        polylines or vias attach to (anchors) keep their corner, so nothing gets disconnected."""
        import math
        out = [pts[0]]
        for i in range(1, len(pts) - 1):
            p = pts[i]
            if isinstance(p, str) or (round(p[0], 3), round(p[1], 3)) in anchors:
                out.append(p)
                continue
            (ax, ay), (px, py), (bx, by) = mm_pt(out[-1]), mm_pt(p), mm_pt(pts[i + 1])
            ux, uy, wx, wy = px - ax, py - ay, bx - px, by - py
            li, lo = math.hypot(ux, uy), math.hypot(wx, wy)
            if li < 1e-6 or lo < 1e-6 or (-ux * wx - uy * wy) / (li * lo) < -0.01:   # interior angle > 90
                out.append(p)
                continue
            c = min(max(width, 0.3), 0.45 * li, 0.45 * lo)
            if any(math.hypot(qx - px, qy - py) < c + width for qx, qy in anchors):   # a tap near the corner
                out.append(p)
                continue
            out += [(round(px - ux / li * c, 4), round(py - uy / li * c, 4)),
                    (round(px + wx / lo * c, 4), round(py + wy / lo * c, 4))]
        out.append(pts[-1])
        return out

    polys = [(net, lyr, width, octilinear(list(pts))) for net, lyr, width, pts in all_routes]
    vias = {(round(x, 3), round(y, 3)) for _, x, y, _, _ in all_vias}
    anchors = {'F': set(vias), 'B': set(vias)}
    for _, lyr, _, pts in polys:
        for p in (pts[0], pts[-1]):
            if not isinstance(p, str):
                anchors[lyr].add((round(p[0], 3), round(p[1], 3)))
    for net, lyr, width, pts in polys:
        ni = nets[net]
        pts = chamfer(pts, width, anchors[lyr])
        for a, b in zip(pts, pts[1:]):
            t = pcbnew.PCB_TRACK(board)
            t.SetStart(pt(a)); t.SetEnd(pt(b)); t.SetWidth(MM(width))
            t.SetLayer(layer[lyr]); t.SetNet(ni); t.SetLocked(True)
            board.Add(t)
    for net, x, y, size, drill in all_vias:
        via = pcbnew.PCB_VIA(board)
        via.SetPosition(v(x, y)); via.SetWidth(MM(size)); via.SetDrill(MM(drill))
        via.SetNet(nets[net]); via.SetLocked(True)
        board.Add(via)
    pcbnew.SaveBoard(PCB, board)
    print('routes:', len(all_routes), 'polylines,', len(all_vias), 'vias')


def edge_dist(x, y):
    """Distance from (x, y) to the board outline, rounded corners included."""
    w, h, r = BOARD_W, BOARD_H, CORNER_R
    cx, cy = min(max(x, r), w - r), min(max(y, r), h - r)
    if (cx, cy) != (x, y):
        return r - ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5
    return min(x, y, w - x, h - y)


def in_label(x, y, m=0.45):
    return any(x1 - m < x < x2 + m and y1 - m < y < y2 + m for x1, y1, x2, y2 in label_boxes())


def via_holes_ok(board, p, drill):
    """JLC hole-to-hole (0.5 mm) against every via already on the board."""
    for t in board.GetTracks():
        if t.GetClass() == 'PCB_VIA' and (t.GetPosition() - p).EuclideanNorm() < \
                MM(drill / 2 + 0.5) + t.GetDrill() / 2:
            return False
    return True


def stitch(pitch=2.5, size=0.6, drill=0.3):
    """Drop GND stitching vias on a grid wherever they clear everything else."""
    board = pcbnew.LoadBoard(PCB)
    gnd = board.GetNetsByName()['GND']
    clear = MM(0.2)
    avoid_fp = {'U1', 'J1', 'SW1', 'J3'}
    items = [p for fp in board.GetFootprints() for p in fp.Pads()] + list(board.GetTracks())
    keepouts = [z for z in board.Zones() if z.GetIsRuleArea()]
    for fp in board.GetFootprints():
        keepouts += [z for z in fp.Zones() if z.GetIsRuleArea()]
    courts = [fp.GetCourtyard(pcbnew.F_Cu) for fp in board.GetFootprints()
              if fp.GetReference() in avoid_fp]
    labels = [(x1 - 0.5, y1 - 0.5, x2 + 0.5, y2 + 0.5) for x1, y1, x2, y2 in label_boxes()]
    added = 0
    x = 1.5
    while x < BOARD_W - 1.6:
        y = 1.5
        while y < BOARD_H - 1.6:
            p = v(x, y)
            ok = not any(x1 < x < x2 and y1 < y < y2 for x1, y1, x2, y2 in labels)
            for z in keepouts:
                if z.Outline().Contains(p):
                    ok = False
                    break
            if ok:
                for c in courts:
                    if c.OutlineCount() and c.Contains(p):
                        ok = False
                        break
            if ok:
                for it in items:
                    if it.HitTest(p, MM(size / 2) + clear):
                        ok = False
                        break
            if ok:
                for fp in board.GetFootprints():
                    for p2 in fp.Pads():
                        if p2.HasHole() and p2.HitTest(p, MM(size / 2 + 0.5)):
                            ok = False
                    if fp.GetReference().startswith('H') and \
                            (fp.GetPosition() - p).EuclideanNorm() < MM(2.6):
                        ok = False
            if ok and via_holes_ok(board, p, drill):
                via = pcbnew.PCB_VIA(board)
                via.SetPosition(p); via.SetWidth(MM(size)); via.SetDrill(MM(drill)); via.SetNet(gnd)
                board.Add(via)
                items.append(via)
                added += 1
            y += pitch
        x += pitch
    # every pour fragment must reach the other layer: fill with islands kept (an island with no
    # via would otherwise just vanish), give via-less fragments their own via, then refill
    pours = [z for z in board.Zones() if not z.GetIsRuleArea()]
    for z in pours:
        z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_NEVER)
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    vias = [t for t in board.GetTracks() if t.GetClass() == 'PCB_VIA' and t.GetNetname() == 'GND']
    tht = [p for fp in board.GetFootprints() for p in fp.Pads() if p.HasHole() and p.GetNetname() == 'GND']
    fills = {z.GetLayer(): z.GetFilledPolysList(z.GetLayer()) for z in pours}
    fixed = 0
    for z in pours:
        fill = z.GetFilledPolysList(z.GetLayer())
        other = [f for lyr, f in fills.items() if lyr != z.GetLayer()][0]
        for i in range(fill.OutlineCount()):
            poly = pcbnew.SHAPE_POLY_SET()
            poly.AddOutline(fill.Outline(i))
            if any(poly.Contains(it.GetPosition()) for it in vias + tht):
                continue
            bb = fill.Outline(i).BBox()
            step = MM(0.2)
            done = False
            yy = bb.GetTop()
            while yy < bb.GetBottom() and not done:
                xx = bb.GetLeft()
                while xx < bb.GetRight() and not done:
                    p = pcbnew.VECTOR2I(xx, yy)
                    holes_ok = all((h.GetPosition() - p).EuclideanNorm() >
                                   h.GetDrillSize().x / 2 + MM(size / 2 + 0.5)
                                   for fp in board.GetFootprints() for h in fp.Pads() if h.HasHole())
                    px, py = pcbnew.ToMM(p.x), pcbnew.ToMM(p.y)
                    edge_ok = edge_dist(px, py) > size / 2 + 0.6
                    in_other = any(other.Outline(k).PointInside(p, MM(size / 2 + 0.05))
                                   for k in range(other.OutlineCount()))
                    if edge_ok and in_other and via_holes_ok(board, p, drill) and poly.Contains(p) and poly.Outline(0).PointInside(p, MM(size / 2 + 0.05)) and \
                            holes_ok and not any(it.HitTest(p, MM(size / 2) + clear) for it in items):
                        via = pcbnew.PCB_VIA(board)
                        via.SetPosition(p); via.SetWidth(MM(size)); via.SetDrill(MM(drill)); via.SetNet(gnd)
                        board.Add(via); items.append(via); vias.append(via)
                        fixed += 1
                        done = True
                    xx += step
                yy += step
    for z in pours:
        z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    # first pass with stitching-size vias; whatever stays unbridged gets a second chance with
    # 0.5 mm vias at JLC's 0.15 mm clearance (small pockets walled in by signal tracks)
    fixed += bridge_ground_groups(board, gnd, items, size, drill, clear, warn=False)
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    fixed += bridge_ground_groups(board, gnd, items, 0.5, drill, MM(0.15))
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    # drop added vias that ended up in no pour on one of the layers (their sliver was removed)
    fills = [z.GetFilledPolysList(z.GetLayer()) for z in pours]
    dangling = [t for t in board.GetTracks() if t.GetClass() == 'PCB_VIA' and t.GetNetname() == 'GND'
                and not t.IsLocked() and not all(f.Contains(t.GetPosition()) for f in fills)]
    for t in dangling:
        board.Remove(t)          # last edit before saving: no proxies are used after this
    if dangling:
        pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    pcbnew.SaveBoard(PCB, board)
    fixed -= len(dangling)
    print('stitching vias:', added, '+ island vias:', fixed)


def bridge_ground_groups(board, gnd, items, size, drill, clear, warn=True):
    """Union-find over GND pour fragments joined by vias/THT pads; bridge stray groups."""
    frags = []
    for z in board.Zones():
        if z.GetIsRuleArea():
            continue
        f = z.GetFilledPolysList(z.GetLayer())
        for i in range(f.OutlineCount()):
            ps = pcbnew.SHAPE_POLY_SET()
            ps.AddOutline(f.Outline(i))
            frags.append((z.GetLayer(), ps, f.Outline(i).Area()))
    parent = list(range(len(frags)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    links = [t.GetPosition() for t in board.GetTracks() if t.GetClass() == 'PCB_VIA' and t.GetNetname() == 'GND']
    links += [p.GetPosition() for fp in board.GetFootprints() for p in fp.Pads() if p.HasHole() and p.GetNetname() == 'GND']
    for pos in links:
        hit = [i for i, (_, ps, _) in enumerate(frags) if ps.Contains(pos)]
        for i in hit[1:]:
            parent[find(i)] = find(hit[0])
    groups = {}
    for i in range(len(frags)):
        groups.setdefault(find(i), []).append(i)
    main = max(groups, key=lambda g: sum(frags[i][2] for i in groups[g]))
    added = 0
    holes = [h for fp in board.GetFootprints() for h in fp.Pads() if h.HasHole()]
    for g, members in groups.items():
        if g == main:
            continue
        done = False
        for i in members:
            for j in groups[main]:
                if frags[i][0] == frags[j][0] or done:
                    continue
                inter = pcbnew.SHAPE_POLY_SET(frags[i][1])
                inter.BooleanIntersection(frags[j][1])
                if inter.OutlineCount() == 0:
                    continue
                bb = inter.BBox()
                yy = bb.GetTop()
                while yy < bb.GetBottom() and not done:
                    xx = bb.GetLeft()
                    while xx < bb.GetRight() and not done:
                        p = pcbnew.VECTOR2I(xx, yy)
                        px, py = pcbnew.ToMM(p.x), pcbnew.ToMM(p.y)
                        if edge_dist(px, py) > size / 2 + 0.6 and \
                                any(inter.Outline(k).PointInside(p, MM(size / 2 + 0.05))
                                    for k in range(inter.OutlineCount())) and \
                                all((h.GetPosition() - p).EuclideanNorm() > h.GetDrillSize().x / 2 + MM(size / 2 + 0.5)
                                    for h in holes) and \
                                not any(it.HitTest(p, MM(size / 2) + clear) for it in items) and \
                                via_holes_ok(board, p, drill):
                            via = pcbnew.PCB_VIA(board)
                            via.SetPosition(p); via.SetWidth(MM(size)); via.SetDrill(MM(drill)); via.SetNet(gnd)
                            board.Add(via); items.append(via)
                            added += 1
                            done = True
                        xx += MM(0.2)
                    yy += MM(0.2)
        if not done and warn:
            for i in members:
                bb = frags[i][1].BBox()
                print('WARNING: unbridged GND fragment', pcbnew.LayerName(frags[i][0]),
                      [round(pcbnew.ToMM(c), 2) for c in (bb.GetLeft(), bb.GetTop(), bb.GetRight(), bb.GetBottom())])
    return added


def models():
    board = pcbnew.LoadBoard(PCB)
    set_models(board)
    pcbnew.SaveBoard(PCB, board)
    print('3D models set for', ', '.join(PROJECT_MODELS))


COMMANDS = {'stitch': stitch, 'layout': lambda: (place(), add_routes()), 'route': add_routes, 'place': place, 'dsn': export_dsn, 'ses': import_ses, 'fill': fill, 'ratsnest': ratsnest, 'models': models}


if __name__ == '__main__':
    COMMANDS[sys.argv[1]]()
