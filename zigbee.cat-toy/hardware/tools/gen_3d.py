#!/usr/bin/env python3
"""3D models (STEP, coloured) for the parts whose KiCad models are missing.

Runs in FreeCAD's GUI, offscreen - colours only export from view providers:
    tools/gen_3d.sh

KiCad model space: mm, X right, Y up (footprint Y negated), Z up from the board's top face,
origin = footprint origin.  Dimensions:
  * Espressif ESP32-C6-MINI-1U: datasheet v1.2, fig. 9 (13.2 x 12.5 x 2.4 mm, shield outline,
    u.FL 1.70 / 1.55 mm from the top-left corner).
  * Alps SKRTLAE010: Alps dimension drawing (4.5 x 2.56 x 3.3 body, D2 plunger 0.84 proud,
    axis 1.7 above the PCB, D0.7 guide bosses 0.45 below).
"""
import os
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


def c6_mini_1u():
    """Espressif ESP32-C6-MINI-1U: 13.2 x 12.5 mm PCB 0.8 thick, 2.4 mm overall, u.FL in the top-left notch."""
    W, L, T, H = 13.2, 12.5, 0.8, 2.4

    def m(x, y):                         # datasheet top view (origin top-left) -> footprint frame
        return x - W / 2, y - L / 2
    pcb = box(-W / 2, -L / 2, 0, W / 2, L / 2, T)
    # shield can: 12.25 x 8.70 full width, the right 9.18 mm runs up to 11.55, leaving the u.FL corner free
    x1, y1 = m(0.47, 3.32)
    x2, y2 = m(12.72, 12.02)
    shield = box(x1, y1, T, x2, y2, H)
    x3, y3 = m(3.54, 0.47)
    shield = shield.fuse(box(x3, y3, T, x2, y1 + 0.01, H)).removeSplitter()
    # castellation-free LGA: show the pad row as a gold rim on the underside edge
    rim = box(-W / 2, -L / 2, -0.02, W / 2, L / 2, 0).cut(box(-W / 2 + 0.6, -L / 2 + 0.6, -0.03, W / 2 - 0.6, L / 2 - 0.6, 0.01))
    ux, uy = m(1.70, 1.55)
    ufl = box(ux - 1.0, uy - 1.0, T, ux + 1.0, uy + 1.0, T + 0.2).fuse(cyl(0.95, ux, uy, T + 0.2, 1.05)).removeSplitter()
    ufl_pin = cyl(0.25, ux, uy, T + 0.2, 1.06)
    export('ESP32-C6-MINI-1U', [
        ('pcb', pcb, MODULE_PCB), ('pads', rim, GOLD), ('shield', shield, TIN),
        ('ufl', ufl, GOLD), ('ufl_pin', ufl_pin, WHITE)])


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
    c6_mini_1u()
    skrtlae010()
except Exception:
    import traceback
    LOG.write(traceback.format_exc())
finally:
    LOG.close()
    os._exit(0)          # the offscreen GUI never quits on its own
