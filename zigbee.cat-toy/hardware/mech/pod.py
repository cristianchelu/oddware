#!/usr/bin/env python3
"""Shared electronics pod for the zigbee toys: cradle + faceplate, lid, battery box.

Run headless:  hardware/mech/pod.sh [503040|103450] [--check]   (env POD_BATTERY / POD_CHECK=1)
Frame = KiCad's STEP frame: X right, Y up (= -board y), Z up, board bottom at z=0, top at z=1.6,
board origin = top-left corner of the PCB outline (same as zigbee_toy.kicad_pcb).

Parts
  cradle       board carrier: floor plate, walls, two M2 bosses, edge ledges, antenna notch, open
               connector bay on the left (JST plugs + wire exits), faceplate on the right with the
               USB-C slot, LED window and a flexure tab over the side button.
  lid          flat lid with an inner lip, friction fit (bench unit / dev box).
  battery_box  open-top box with a rim; the cradle's floor plate drops into the rim like a lid.
               Sized by BATTERY: 503040 (5x30x40, 500 mAh) or 103450 (10x34x50, 1000 mAh).

Toy bodies: cut a window of FACE_WINDOW (printed by the script) where the faceplate shows
through, mount the cradle on the battery box or on their own ribs, route wires out of the bay.
"""
import os
import sys

import FreeCAD as App
import Part
import MeshPart

V = App.Vector
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))
import placement  # noqa: E402  (board outline and part positions come from the KiCad generator's placement)

P = placement.PLACE

# ---- parameters (mm) -------------------------------------------------------------------------
BATTERY = os.environ.get('POD_BATTERY', '503040')      # set by pod.sh (freecadcmd eats argv)
CHECK = os.environ.get('POD_CHECK', '') == '1'
assert BATTERY in ('503040', '103450')
CELL = {'503040': (5.0, 30.0, 40.0), '103450': (10.0, 34.0, 50.0)}[BATTERY]   # t, w, l
CELL_CLR = (1.0, 1.0, 1.0)            # swelling / slide clearance per axis (t, w, l)

PCB_W, PCB_H, PCB_T, PCB_R = placement.BOARD_W, placement.BOARD_H, 1.6, placement.CORNER_R
HOLES = [(P[h][0], -P[h][1]) for h in ('H1', 'H2')]   # M2, board frame (Y up)
PCB_CLR = 0.25                        # board outline clearance to the walls (JLC +-0.2 routing)
FACE_GAP = 0.15                       # board edge to faceplate inner face
WALL, FLOOR, FACE = 1.2, 1.2, 1.0     # 3 perimeters, floor, faceplate
STANDOFF = 2.0                        # board bottom above floor top: the battery JST-PH's pins protrude 1.81
Z_WALL = 7.0                          # JST-PH housing top is 6.44
LID_T, LIP_H, LIP_W, LIP_CLR = 1.2, 1.4, 0.8, 0.2
BOSS_D, BOSS_HOLE = 4.4, 1.7          # M2 thread-forming
# connector bay beyond the left board edge: 12 mm for the plugs, longer when the cell needs it (the
# battery box hangs under the whole cradle, so the cradle sets its length)
BAY_L = max(12.0, CELL[2] + CELL_CLR[2] + 2 * WALL + LIP_W + LIP_CLR / 2 + WALL - (PCB_W + FACE_GAP + FACE))
LEDGE_W = 1.0
U1_X = P['U1'][0]
MINI1_ANT = (U1_X - 6.6, U1_X + 6.6, 3.1)   # MINI-1 antenna past the top edge: board x from..to, overhang
ANT_X = (U1_X - 7.3, U1_X + 7.2)            # antenna notch: the 1U's u.FL lead (x U1-4.9) leaves here; a MINI-1's antenna overhangs here

USB_C = (-P['J1'][1], PCB_T + 3.33 / 2)     # (Y, z) centre of the receptacle face
USB_SLOT = (10.0, 4.6)                # w x h, full-round ends
LED_WIN = ((-P['D2'][1], PCB_T + 0.7), (3.0, 1.6))
BTN = (-P['SW1'][1], PCB_T + 1.7)           # plunger axis (Y, z)
BTN_TIP_X = P['SW1'][0] + 2.04              # plunger tip, from the STEP
TAB_W, TAB_L, TAB_T, TAB_SLOT = 5.0, 7.0, 0.6, 0.6
TAB_NUB_D, TAB_BUMP_D, TAB_BUMP_H = 2.5, 3.0, 0.6
WIRE_NOTCH = [(-P['J4'][1], 3.2), (-P['J5'][1], 3.2)]   # (Y centre, width) in the bay end wall: motor, speaker
J3_Y = P['J3'][1]                                       # battery JST-PH pin 1 (pin 2 is 2 mm further down)
LEAD_SLOT = (-8.0, -2.0, -(J3_Y + 2.8), -(J3_Y - 0.2))  # x0,x1,Y0,Y1 through the floor for the battery lead

# derived
Z_FLOOR_TOP = -STANDOFF
Z_FLOOR_BOT = Z_FLOOR_TOP - FLOOR
X_IN0, X_IN1 = -BAY_L, PCB_W + FACE_GAP
Y_IN0, Y_IN1 = -PCB_H - PCB_CLR, PCB_CLR
X_OUT0, X_OUT1 = X_IN0 - WALL, X_IN1 + FACE
Y_OUT0, Y_OUT1 = Y_IN0 - WALL, Y_IN1 + WALL
Z_TOP = Z_WALL + LID_T
YC = -PCB_H / 2
BOX_W_IN = CELL[1] + CELL_CLR[1]
PLATE_Y0, PLATE_Y1 = YC - (BOX_W_IN + 2 * WALL - 2 * LIP_W - LIP_CLR) / 2, YC + (BOX_W_IN + 2 * WALL - 2 * LIP_W - LIP_CLR) / 2
PLATE_Y0, PLATE_Y1 = min(PLATE_Y0, Y_OUT0), max(PLATE_Y1, Y_OUT1)


def box(x0, y0, z0, x1, y1, z1):
    return Part.makeBox(x1 - x0, y1 - y0, z1 - z0, V(x0, y0, z0))


def rrect(x0, y0, z0, x1, y1, z1, r):
    """Box with rounded vertical (Z) edges."""
    b = box(x0, y0, z0, x1, y1, z1)
    edges = [e for e in b.Edges if abs(e.tangentAt(e.FirstParameter).z) > 0.99]
    return b.makeFillet(r, edges)


def slot_x(yc, zc, w, h, x0, x1):
    """Stadium-shaped cutter through X: w along Y, h along Z, full-round ends."""
    r = min(w, h) / 2
    if w >= h:
        s = box(x0, yc - w / 2 + r, zc - r, x1, yc + w / 2 - r, zc + r)
        s = s.fuse(Part.makeCylinder(r, x1 - x0, V(x0, yc - w / 2 + r, zc), V(1, 0, 0)))
        s = s.fuse(Part.makeCylinder(r, x1 - x0, V(x0, yc + w / 2 - r, zc), V(1, 0, 0)))
    else:
        s = box(x0, yc - r, zc - h / 2 + r, x1, yc + r, zc + h / 2 - r)
        s = s.fuse(Part.makeCylinder(r, x1 - x0, V(x0, yc, zc - h / 2 + r), V(1, 0, 0)))
        s = s.fuse(Part.makeCylinder(r, x1 - x0, V(x0, yc, zc + h / 2 - r), V(1, 0, 0)))
    return s


def cradle():
    # floor plate (wide enough to sit in the battery box rim) + walls
    body = rrect(X_OUT0, PLATE_Y0, Z_FLOOR_BOT, X_OUT1, PLATE_Y1, Z_FLOOR_TOP, 1.0)
    walls = rrect(X_OUT0, Y_OUT0, Z_FLOOR_TOP, X_OUT1, Y_OUT1, Z_WALL, 1.5)
    walls = walls.cut(box(X_IN0, Y_IN0, Z_FLOOR_TOP, X_IN1, Y_IN1, Z_WALL + 1))
    body = body.fuse(walls)
    # faceplate, full height, on the USB-C edge
    body = body.fuse(box(X_IN1, Y_OUT0, Z_FLOOR_BOT, X_OUT1, Y_OUT1, Z_TOP))
    # board ledges along the long walls
    body = body.fuse(box(0, Y_IN0, Z_FLOOR_TOP, X_IN1, Y_IN0 + LEDGE_W, 0))
    body = body.fuse(box(0, Y_IN1 - LEDGE_W, Z_FLOOR_TOP, ANT_X[0], Y_IN1, 0))
    body = body.fuse(box(ANT_X[1], Y_IN1 - LEDGE_W, Z_FLOOR_TOP, X_IN1, Y_IN1, 0))
    # antenna notch: the FPC antenna cable leaves over the top edge here, or a MINI-1's antenna overhangs it
    body = body.cut(box(ANT_X[0], -0.25, Z_FLOOR_BOT - 1, ANT_X[1], Y_OUT1 + 5, Z_TOP + 1))
    # M2 bosses
    for x, y in HOLES:
        body = body.fuse(Part.makeCylinder(BOSS_D / 2, STANDOFF, V(x, y, Z_FLOOR_TOP)))
        body = body.cut(Part.makeCylinder(BOSS_HOLE / 2, FLOOR + STANDOFF + 2, V(x, y, Z_FLOOR_BOT - 1)))
    # battery lead slot through the floor, in the connector bay
    x0, x1, y0, y1 = LEAD_SLOT
    body = body.cut(slot_x(0, 0, 1, 1, 0, 1).translate(V(0, 0, 0)) if False else
                    rrect(x0, y0, Z_FLOOR_BOT - 1, x1, y1, Z_FLOOR_TOP + 1, 1.4))
    # wire notches in the bay end wall
    for yc, w in WIRE_NOTCH:
        body = body.cut(box(X_OUT0 - 1, yc - w / 2, 2.0, X_IN0 + 1, yc + w / 2, Z_TOP + 1))
    # --- faceplate features ---
    yc, zc = USB_C
    body = body.cut(slot_x(yc, zc, USB_SLOT[0], USB_SLOT[1], X_IN1 - 1, X_OUT1 + 1))
    (yc, zc), (w, h) = LED_WIN
    body = body.cut(slot_x(yc, zc, w, h, X_IN1 - 1, X_OUT1 + 1))
    # button flexure: tab hinged at its top, thinned from the inside, nub reaching the plunger
    yb, zb = BTN
    tab_y0, tab_y1 = yb - TAB_W / 2, yb + TAB_W / 2
    tab_z0 = zb - 2.3
    tab_z1 = tab_z0 + TAB_L
    for y0, y1 in ((tab_y0 - TAB_SLOT, tab_y0), (tab_y1, tab_y1 + TAB_SLOT)):
        body = body.cut(box(X_IN1 - 1, y0, tab_z0 - TAB_SLOT, X_OUT1 + 1, y1, tab_z1))
    body = body.cut(box(X_IN1 - 1, tab_y0 - TAB_SLOT, tab_z0 - TAB_SLOT, X_OUT1 + 1, tab_y1 + TAB_SLOT, tab_z0))
    body = body.cut(box(X_IN1 - 1, tab_y0, tab_z0, X_OUT1 - TAB_T, tab_y1, tab_z1))        # thin the tab
    nub_x0 = min(X_IN1, BTN_TIP_X + 0.05)                                                   # reaches to just short of the plunger tip
    body = body.fuse(Part.makeCylinder(TAB_NUB_D / 2, (X_OUT1 - TAB_T) - nub_x0, V(nub_x0, yb, zb), V(1, 0, 0)))
    body = body.fuse(Part.makeCylinder(TAB_BUMP_D / 2, TAB_BUMP_H, V(X_OUT1, yb, zb), V(1, 0, 0)))
    return body


def lid():
    plate = rrect(X_OUT0, Y_OUT0, Z_WALL, X_IN1, Y_OUT1, Z_TOP, 1.5)
    lip = box(X_IN0 + LIP_CLR, Y_IN0 + LIP_CLR, Z_WALL - LIP_H, X_IN1 - LIP_CLR, Y_IN1 - LIP_CLR, Z_WALL)
    lip = lip.cut(box(X_IN0 + LIP_CLR + LIP_W, Y_IN0 + LIP_CLR + LIP_W, Z_WALL - LIP_H - 1,
                      X_IN1 - LIP_CLR - LIP_W, Y_IN1 - LIP_CLR - LIP_W, Z_WALL + 1))
    return plate.fuse(lip)


def battery_box():
    t, w, l = (c + d for c, d in zip(CELL, CELL_CLR))
    h_in = t
    x0, x1 = X_OUT0 - LIP_W - LIP_CLR / 2, X_OUT1          # open rim on the faceplate end: box ends flush
    y0, y1 = PLATE_Y0 - LIP_W - LIP_CLR / 2, PLATE_Y1 + LIP_W + LIP_CLR / 2
    z_top, z_bot = Z_FLOOR_BOT + FLOOR, Z_FLOOR_BOT - h_in - FLOOR
    body = rrect(x0, y0, z_bot, x1, y1, z_top, 1.5)
    body = body.cut(box(x0 + WALL, y0 + WALL, z_bot + FLOOR, x1 - WALL, y1 - WALL, Z_FLOOR_BOT))   # cavity
    body = body.cut(box(x0 + LIP_W, y0 + LIP_W, Z_FLOOR_BOT - 0.01, x1 + 1, y1 - LIP_W, z_top + 1))   # rim seat
    assert (x1 - x0 - 2 * WALL) >= l and (y1 - y0 - 2 * WALL) >= w, 'cell does not fit'
    # stop rib: the cell sits against the faceplate end, under the board; the lead loops up in the bay
    rib_x1 = (x1 - WALL) - l
    if rib_x1 - 1.0 > x0 + WALL + 0.5:
        body = body.fuse(box(rib_x1 - 1.0, y0 + WALL - 0.01, z_bot + FLOOR - 0.01, rib_x1, y1 - WALL + 0.01, Z_FLOOR_BOT - 1.0))
    return body, (x0, x1, y0, y1, z_bot, z_top, h_in)


def export(doc, name, shape, outdir):
    o = doc.addObject('Part::Feature', name)
    o.Shape = shape
    Part.export([o], os.path.join(outdir, f'{name}.step'))
    m = MeshPart.meshFromShape(Shape=shape, LinearDeflection=0.05, AngularDeflection=0.3, Relative=False)
    m.write(os.path.join(outdir, f'{name}.stl'))
    bb = shape.BoundBox
    print(f'{name:12} {bb.XLength:6.2f} x {bb.YLength:6.2f} x {bb.ZLength:6.2f} mm  '
          f'[{bb.XMin:.2f}..{bb.XMax:.2f}, {bb.YMin:.2f}..{bb.YMax:.2f}, {bb.ZMin:.2f}..{bb.ZMax:.2f}]  '
          f'vol {shape.Volume / 1000:.1f} cm3')
    return o


def main():
    outdir = os.path.join(HERE, 'build', f'pod_{BATTERY}')
    os.makedirs(outdir, exist_ok=True)
    doc = App.newDocument('pod')
    c = cradle()
    export(doc, 'cradle', c, outdir)
    export(doc, 'lid', lid(), outdir)
    bx, dims = battery_box()
    export(doc, 'battery_box', bx, outdir)
    print(f'battery {BATTERY}: cavity {dims[1]-dims[0]-2*WALL:.1f} x {dims[3]-dims[2]-2*WALL:.1f} x {dims[6]:.1f} mm')
    print(f'FACE_WINDOW (toy body cutout for the faceplate, Y x Z): {Y_OUT1 - Y_OUT0:.2f} x {Z_TOP - Z_FLOOR_BOT:.2f} mm '
          f'at Y {Y_OUT0:.2f}..{Y_OUT1:.2f}, z {Z_FLOOR_BOT:.2f}..{Z_TOP:.2f}; outer face at x={X_OUT1:.2f}')
    print(f'stack height floor-bottom to lid-top: {Z_TOP - Z_FLOOR_BOT:.2f} mm; with battery box: {Z_TOP - dims[4]:.2f} mm')
    if CHECK:
        # Part.read expands the STEP assembly into placed solids (Import.insert would also give the
        # un-placed prototypes at the origin, which collide with everything).
        step = os.path.join(HERE, '..', 'zigbee_toy', 'build', 'zigbee_toy.step')
        board = Part.read(step)
        pod = c.fuse(lid())
        hits = []
        for sol in board.Solids:
            bb = sol.BoundBox
            if bb.ZMax <= 0.001 and bb.ZMin > -0.1:   # bottom copper/mask skin, 35 um below the laminate
                continue
            if not bb.intersect(pod.BoundBox):
                continue
            com = sol.common(pod)
            if com.Volume > 0.05:
                cb = com.BoundBox
                hits.append((com.Volume, f'solid at x {bb.XMin:.1f}..{bb.XMax:.1f} Y {bb.YMin:.1f}..{bb.YMax:.1f} z {bb.ZMin:.2f}..{bb.ZMax:.2f}'
                             f' overlaps x {cb.XMin:.2f}..{cb.XMax:.2f} Y {cb.YMin:.2f}..{cb.YMax:.2f} z {cb.ZMin:.2f}..{cb.ZMax:.2f}'))
        print('collision check vs board STEP:', 'CLEAN' if not hits else f'{len(hits)} HITS')
        for v, desc in sorted(hits, reverse=True):
            print(f'   {v:7.2f} mm3  {desc}')
        # a MINI-1 (PCB antenna) on the same pads: its antenna end overhangs the top edge (not in the STEP)
        ant = Part.makeBox(MINI1_ANT[1] - MINI1_ANT[0], MINI1_ANT[2], 0.8, V(MINI1_ANT[0], 0, 1.6))
        v = ant.common(pod).Volume
        print('MINI-1 antenna overhang vs pod:', 'CLEAR' if v < 0.01 else f'HITS {v:.2f} mm3')
    App.closeDocument(doc.Name)


main()
