#!/usr/bin/env python3
"""3D models (STEP, coloured) for the parts whose KiCad models are missing.

Runs in FreeCAD's GUI, offscreen - colours only export from view providers:
    tools/gen_3d.sh

KiCad model space: mm, X right, Y up (footprint Y negated), Z up from the board's top face,
origin = footprint origin.  Dimensions:
  * Ebyte E73-2G4M08S1C: Ebyte user manual v1.9, ch. 3 (13 x 18 x 3.0 mm, shield and antenna
    positions scaled off the top view); castellation positions from our footprint.
  * Alps SKRTLAE010: Alps dimension drawing (4.5 x 2.56 x 3.3 body, D2 plunger 0.84 proud,
    axis 1.7 above the PCB, D0.7 guide bosses 0.45 below).
"""
import os
import re
import sys

import FreeCAD as App
import FreeCADGui as Gui
import ImportGui
import Part

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.join(HERE, '..', 'zigbee_toy')
OUT = os.path.join(PROJ, 'zigbee_toy.3dshapes')

GOLD = (0.86, 0.69, 0.33)
TIN = (0.62, 0.63, 0.66)
STEEL = (0.72, 0.73, 0.75)
MODULE_PCB = (0.10, 0.12, 0.16)
CERAMIC = (0.93, 0.92, 0.88)
BLACK = (0.12, 0.12, 0.12)
WHITE = (0.95, 0.95, 0.93)
V = App.Vector


def box(x1, y1, z1, x2, y2, z2):
    """Box from footprint-space corners (Y down) into model space (Y up)."""
    xa, xb = sorted((x1, x2))
    ya, yb = sorted((-y1, -y2))
    za, zb = sorted((z1, z2))
    return Part.makeBox(xb - xa, yb - ya, zb - za, V(xa, ya, za))


def cyl(r, x, y, z, h, axis=(0, 0, 1)):
    """Cylinder at footprint-space (x, y, z) along a model-space axis."""
    return Part.makeCylinder(r, h, V(x, -y, z), V(*axis))


def export(name, parts):
    doc = App.newDocument(name)
    objs = []
    for label, shape, color in parts:
        o = doc.addObject('Part::Feature', label)
        o.Shape = shape
        o.ViewObject.ShapeColor = color
        objs.append(o)
    path = os.path.join(OUT, name + '.step')
    ImportGui.export(objs, path)
    print('wrote', os.path.relpath(path, PROJ))
    App.closeDocument(doc.Name)


def e73():
    W, L, T = 13.0, 18.0, 1.0          # PCB; total height 3.0 with the shield
    pcb = box(-W / 2, -L / 2, 0, W / 2, L / 2, T)
    # castellations: every footprint pad sitting on the module edge
    fp = open(os.path.join(PROJ, 'zigbee_toy.pretty', 'Ebyte_E73-2G4M08S1C.kicad_mod')).read()
    edge = []
    for x, y in re.findall(r'\(pad "\d+" smd rect\s+\(at ([-\d.]+) ([-\d.]+)\)', fp):
        x, y = float(x), float(y)
        if abs(abs(x) - W / 2) < 0.1:
            edge.append((W / 2 * (1 if x > 0 else -1), y, 'x'))
        elif abs(y + L / 2) < 0.1:
            edge.append((x, -L / 2, 'y'))
    plating = None
    for x, y, side in edge:
        hole = cyl(0.3, x, y, -0.1, T + 0.2)
        pcb = pcb.cut(hole)
        if side == 'x':
            s = 1 if x > 0 else -1
            ring = box(x - s * 0.65, y - 0.4, -0.02, x, y + 0.4, T + 0.02)
        else:
            ring = box(x - 0.4, y, -0.02, x + 0.4, y + 0.65, T + 0.02)
        ring = ring.cut(hole)
        plating = ring if plating is None else plating.fuse(ring)
    shield = box(-5.1, -7.6, T, 5.1, 4.45, 3.0)
    shield = shield.cut(box(-4.9, -7.4, T, 4.9, 4.25, 2.85))      # open can, 0.15 mm walls
    antenna = box(-3.8, 6.4, T, 4.6, 8.3, T + 0.8)
    mark = box(-3.4, 7.1, T + 0.8, -2.6, 7.6, T + 0.82)
    export('Ebyte_E73-2G4M08S1C', [
        ('pcb', pcb, MODULE_PCB), ('castellations', plating.removeSplitter(), GOLD),
        ('shield', shield, TIN), ('antenna', antenna, CERAMIC), ('antenna_mark', mark, BLACK)])


def skrtlae010():
    H, AXIS = 3.3, 1.7
    housing = box(-2.25, -1.35, 0, 2.25, 0.95, H)
    frame = box(-2.25, 0.95, 0, 2.25, 1.21, H)                    # steel front plate
    frame = frame.cut(cyl(1.2, 0, 0.9, AXIS, 0.4, axis=(0, -1, 0)))
    plunger = cyl(1.0, 0, 1.0, AXIS, 1.04, axis=(0, -1, 0))       # tip at y = 2.04
    bosses = cyl(0.35, -2.125, 0, -0.45, 0.45).fuse(cyl(0.35, 2.125, 0, -0.45, 0.45))
    leads = None
    for x, w in ((-1.225, 0.5), (0.0, 0.4), (1.225, 0.5)):          # signal terminals
        lead = box(x - w / 2, -1.7, 0, x + w / 2, -1.35, 0.15)
        leads = lead if leads is None else leads.fuse(lead)
    for x in (-1.85, 1.85):                                          # mounting tabs
        leads = leads.fuse(box(x - 0.5, 0.75, 0, x + 0.5, 1.21, 0.15))
    export('SW_Push_1P1T-MP_NO_Horizontal_Alps_SKRTLAE010', [
        ('housing', housing, BLACK), ('frame', frame, STEEL), ('plunger', plunger, WHITE),
        ('bosses', bosses, BLACK), ('terminals', leads, TIN)])


# FreeCAD runs this file without __name__ == '__main__'; its GUI also swallows stdout,
# so progress goes to a log that gen_3d.sh prints.
LOG = open(os.path.join(PROJ, 'build', 'gen_3d.log'), 'w')
print = lambda *a: (LOG.write(' '.join(map(str, a)) + '\n'), LOG.flush())  # noqa: E731
os.makedirs(OUT, exist_ok=True)
try:
    e73()
    skrtlae010()
except Exception:
    import traceback
    LOG.write(traceback.format_exc())
finally:
    LOG.close()
    os._exit(0)          # the offscreen GUI never quits on its own
