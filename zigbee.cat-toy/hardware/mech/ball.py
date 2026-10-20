#!/usr/bin/env python3
"""Rolling-ball toy: two screw-together fuzzy domes, free-spinning tail ring, and a closed "core"
(pendulum chassis in its own shell) hanging from the N20 gearmotor that forms the axle.

Run headless:  hardware/mech/ball.sh [--check]            (env BALL_CELL=803040, BALL_CHECK=1)

Frame: Z is the pole axis (dome B, the one you unscrew to charge, at +Z; the motor dome A at -Z).
Gravity pulls the pendulum toward -Y. The board lies flat under the motor, components down, its
USB-C edge toward +Z, where the core cap has the port window and the pinhole for the button.

Parts (STEP + STL in build/ball_<cell>/)
  dome_a    motor side: internal thread in its rim, D-socket hub, fabric tuck slot + stitch holes
  dome_b    lip with external thread + ring land, hub bore for the pin, tuck slot + stitch holes
  ring      tail ring with a lug (3 mm cord hole)
  core_cup  the capsule: cylinder R31 rounded by a sphere, open at +Z, hole at the bottom round the
            motor hub, bosses for the frame plate and the cap
  core_cap  closes the cup: USB-C window, pinhole for the pairing button, hole for the pin post
  frame     plate with the motor cradle/face mount, standoffs for the PCB, pin post
  tray      battery tray under the board, with a ballast pocket on its underside
  motor / battery / board / ballast are envelopes for --check and renders, not printed.
"""
import math
import os
import traceback

import FreeCAD as App
import Part
import MeshPart

V = App.Vector
HERE = os.path.dirname(os.path.abspath(__file__))
CELL_NAME = os.environ.get('BALL_CELL', '803040')
CHECK = os.environ.get('BALL_CHECK', '') == '1'
CELLS = {'803040': (8.0, 30.0, 40.0, 20.0)}            # t, w, l, grams
CELL = CELLS[CELL_NAME]

# ---- sphere and joint ---------------------------------------------------------------------------
R_OUT = 40.0                 # 80 mm ball
WALL = 1.6
R_IN = R_OUT - WALL          # 38.4
POLE_FLAT = R_OUT - 0.4
R_LIP = 35.0                 # dome B lip core = ring land radius
LIP_WALL = 2.0               # lip inner radius 33
PITCH, TURNS, TH_DEPTH, TH_CLR = 3.0, 2.0, 1.6, 0.3
TH_BASE, TH_TOP = 1.8, 0.9
Z_A_RIM = 0.0
RING_W, RING_CLR, RING_R_OUT = 4.2, 0.3, R_OUT - 2.0
Z_B_BODY = RING_W + 2 * RING_CLR            # 4.8 mm groove
LIP_Z0 = -(TURNS * PITCH + 2.0)             # -8
RIM_THICK_Z = LIP_Z0 - 1.0
SHOULDER_H = 3.5
STITCH_D, STITCH_N = 1.2, 24
TUCK_R0, TUCK_R1, TUCK_DEPTH = R_OUT - 1.8, R_OUT - 1.0, 2.5
# ---- core (the capsule) -------------------------------------------------------------------------
CORE_R, CORE_WALL, CORE_SPH = 31.5, 1.6, 37.0     # cylinder R ∩ sphere R; 1.5 mm under the lip, 1.4 under the domes
CORE_R_IN, CORE_SPH_IN = CORE_R - CORE_WALL, CORE_SPH - CORE_WALL
CORE_Z_TOP, CAP_Z0, CAP_T = 22.6, 12.0, 1.6       # cap: CAP_Z0..CORE_Z_TOP, 1.6 thick plate at the top
CORE_HOLE_R = 14.0                                # bottom opening around dome A's hub
RAIL_X, RAIL_T, RAIL_CLR = 26.0, 1.2, 0.3   # cup rails: two ribs per side, the frame plate (X ±27.5) slides in from the top
CAP_BOSSES = [(27.5 * math.cos(math.radians(a)), 27.5 * math.sin(math.radians(a))) for a in (40, 340)] + [(-27.5, 3.0)]   # clear of the tray posts and the rails
USB_WIN = (13.0, 7.0)          # the receptacle face sits 1.7 mm behind the cap face, so the window takes the plug overmould (max 12.35 x 6.5)
USB_PT = (40.0, -13.3, 1.6 + 3.33 / 2)   # KiCad x, y, z of the receptacle's centre at the board edge
BTN_PT = (40.0, -21.6, 1.6 + 1.7)        # SW1 plunger axis: 1.7 mm above the board top (from the STEP)
PINHOLE_D = 1.3
# ---- motor (N20 micro metal gearmotor, 6 V 100 rpm suggested) ------------------------------------
M_W, M_H = 12.0, 10.0
M_MOTOR_L, M_GEAR_L = 15.0, 9.0
M_BOSS_D, M_BOSS_L = 4.0, 1.5
M_SHAFT_D, M_SHAFT_FLAT, M_SHAFT_L = 3.0, 2.5, 10.0
M_HOLE_PITCH, M_HOLE_D = 9.0, 1.8
SHAFT_TIP_Z = -33.0
Z_BOSS = SHAFT_TIP_Z + M_SHAFT_L            # -23: gearbox face
Z_GEAR0 = Z_BOSS + M_BOSS_L
Z_MOTOR1 = Z_BOSS + M_GEAR_L + M_MOTOR_L    # +1
HUB_SOCKET_DEPTH = 5.0
# ---- chassis ------------------------------------------------------------------------------------
# Stack (Y): motor on the axis (-5..5) | plate -4..-2 with a window for the motor, cheek ledges carry it at -5 |
# board back face at -7.5 (J3-J5 THT pins stick 1.8 mm out of it, 0.7 clear of the motor can), components down to -15.5 |
# cell -16.5..-24.5 | tray floor | ballast plate to -28.6 (core inner R 29.9).
PLATE_Y0, PLATE_Y1 = -4.0, -2.0
CHEEK_Y0 = -5.5                             # cheeks start 0.5 above the board's back; 1.15 mm ledges carry the motor
PLATE_X, PLATE_Z = (-27.5, 27.5), (-26.5, CAP_Z0 + 8.5)   # ends 0.5 under the cap's inner face
FACE_T = 1.5
PIN_D, PIN_L, POST_Z = 4.0, 7.5, (17.0, 28.0)
PIN_Z1 = POST_Z[1] + PIN_L                  # 35.5
HUB_B_BORE, HUB_B_DEPTH = 4.3, 6.0
BOARD_X_REF, BOARD_Y_BACK, BOARD_Z0 = -15.0, -7.5, -19.1   # KiCad y=0 edge (antenna) at X=-15 so the H2 standoff (X 6.5..10.9) clears the motor;
                                                          # back face at Y=-6; KiCad x=0 edge at Z=-19 so the USB-C face sits in the cap plate
BOARD_HOLES = [(37.4, -2.6), (2.6, -23.7)]
BOSS_D, BOSS_HOLE, STANDOFF = 4.4, 1.7, PLATE_Y0 - BOARD_Y_BACK   # 3.5
BOARD_COMP_H = 6.44                         # JST housings
# battery tray
TRAY_WALL, TRAY_FLOOR, TRAY_CLR = 1.2, 1.2, 0.4
BAT_X, BAT_Z = 0.0, 0.0
BAT_Y1 = BOARD_Y_BACK - 1.6 - BOARD_COMP_H - 1.0       # -15.04: 1 mm under the JST housings
TRAY_POSTS = [(CELL[1] / 2 + TRAY_CLR + TRAY_WALL + 2.0, -10.0), (CELL[1] / 2 + TRAY_CLR + TRAY_WALL + 2.0, 10.0)]   # +X side
BALLAST = (14.0, 2.5, 30.0, 8.2)            # steel plate X, Y, Z, grams (optional, glued under the tray)


def box(x0, y0, z0, x1, y1, z1):
    return Part.makeBox(abs(x1 - x0), abs(y1 - y0), abs(z1 - z0), V(min(x0, x1), min(y0, y1), min(z0, z1)))


def cyl_z(r, z0, z1, x=0.0, y=0.0):
    return Part.makeCylinder(r, z1 - z0, V(x, y, z0), V(0, 0, 1))


def cyl_y(r, y0, y1, x=0.0, z=0.0):
    return Part.makeCylinder(r, y1 - y0, V(x, y0, z), V(0, 1, 0))


def sphere(r):
    return Part.makeSphere(r)


def core_envelope(r_cyl, r_sph):
    return cyl_z(r_cyl, -50, 50).common(sphere(r_sph))


def thread(r_core, z0, external):
    h = TURNS * PITCH
    helix = Part.makeHelix(PITCH, h, r_core)
    helix.translate(V(0, 0, z0))
    s = 1 if external else -1
    r0, r1 = r_core - s * 0.2, r_core + s * TH_DEPTH
    pts = [V(r0, 0, z0 - TH_BASE / 2), V(r1, 0, z0 - TH_TOP / 2), V(r1, 0, z0 + TH_TOP / 2),
           V(r0, 0, z0 + TH_BASE / 2), V(r0, 0, z0 - TH_BASE / 2)]
    return Part.Wire(helix).makePipeShell([Part.Wire(Part.makePolygon(pts))], True, True)


def fabric_features(z_face, direction):
    """Tuck slot in the groove-side face at z_face (direction = which way the dome extends) and
    radial stitch holes from the outside into the slot."""
    z0, z1 = sorted((z_face, z_face + direction * TUCK_DEPTH))
    cut = cyl_z(TUCK_R1, z0 - 0.01, z1 + 0.01).cut(cyl_z(TUCK_R0, z0 - 1, z1 + 1))
    zh = z_face + direction * (TUCK_DEPTH - 1.0)
    for i in range(STITCH_N):
        a = 2 * math.pi * i / STITCH_N
        d = V(math.cos(a), math.sin(a), 0)
        cut = cut.fuse(Part.makeCylinder(STITCH_D / 2, 4.0, V(d.x * (TUCK_R0 + 0.3), d.y * (TUCK_R0 + 0.3), zh), d))
    return cut


def dome_a():
    body = sphere(R_OUT).cut(sphere(R_IN)).common(box(-50, -50, -50, 50, 50, Z_A_RIM))
    body = body.cut(box(-50, -50, -60, 50, 50, -POLE_FLAT))
    r_minor = R_LIP + TH_DEPTH + TH_CLR                                   # 36.9
    rim = cyl_z(R_OUT, RIM_THICK_Z, Z_A_RIM).cut(cyl_z(r_minor, RIM_THICK_Z - 1, Z_A_RIM + 1)).common(sphere(R_OUT))
    body = body.fuse(rim).cut(cyl_z(r_minor, LIP_Z0 - 0.5, Z_A_RIM + 1))
    body = body.fuse(thread(r_minor, LIP_Z0 + 1.0 + TH_BASE / 2 + PITCH / 2, external=False)
                     .common(cyl_z(r_minor + 0.01, LIP_Z0 - 1, Z_A_RIM - 0.2)))
    # hub: boss R5 up to the socket face, 45-degree cone at its base
    z_face = SHAFT_TIP_Z + HUB_SOCKET_DEPTH                               # -28
    z_base = -(R_IN + 1.0)
    cone_h = 7.0
    hub = Part.makeCone(5.0 + cone_h, 5.0, cone_h, V(0, 0, z_base), V(0, 0, 1))
    hub = hub.fuse(cyl_z(5.0, z_base, z_face)).common(sphere(R_IN + 0.5))
    body = body.fuse(hub)
    sock = cyl_z(M_SHAFT_D / 2 + 0.1, SHAFT_TIP_Z - 0.5, z_face + 1)
    sock = sock.cut(box(-5, M_SHAFT_FLAT - M_SHAFT_D / 2 + 0.1, SHAFT_TIP_Z - 1, 5, 5, z_face + 2))
    body = body.cut(sock)
    return body.cut(fabric_features(Z_A_RIM, -1))


def dome_b():
    body = sphere(R_OUT).cut(sphere(R_IN)).common(box(-50, -50, Z_B_BODY, 50, 50, 50))
    body = body.cut(box(-50, -50, POLE_FLAT, 50, 50, 60))
    lip = cyl_z(R_LIP, LIP_Z0, Z_B_BODY + 0.5).cut(cyl_z(R_LIP - LIP_WALL, LIP_Z0 - 1, Z_B_BODY + 2))
    body = body.fuse(lip)
    body = body.fuse(thread(R_LIP, LIP_Z0 + 1.0 + TH_BASE / 2, external=True)
                     .common(cyl_z(R_LIP + TH_DEPTH + 0.1, LIP_Z0, Z_A_RIM - 0.5)))
    sh = cyl_z(R_OUT, Z_B_BODY, Z_B_BODY + SHOULDER_H).common(sphere(R_OUT)).cut(cyl_z(R_LIP - LIP_WALL, 0, 20))
    body = body.fuse(sh)
    z_face = PIN_Z1 - HUB_B_DEPTH + 0.5
    z_base = R_IN + 1.0
    cone = Part.makeCone(5.0 + (z_base - z_face), 5.0, z_base - z_face, V(0, 0, z_base), V(0, 0, -1)).common(sphere(R_IN + 0.5))
    body = body.fuse(cone).cut(cyl_z(HUB_B_BORE / 2, z_face - 1, PIN_Z1 + 0.5))
    return body.cut(fabric_features(Z_B_BODY, +1))


def ring():
    r_in = R_LIP + RING_CLR
    z0, z1 = Z_A_RIM + RING_CLR, Z_A_RIM + RING_CLR + RING_W
    r = cyl_z(RING_R_OUT, z0, z1).cut(cyl_z(r_in, z0 - 1, z1 + 1))
    lug = box(-3.0, r_in + 0.5, z0, 3.0, RING_R_OUT + 1.0, z1).common(cyl_z(R_OUT - 1.0, z0, z1))
    r = r.fuse(lug)
    return r.cut(Part.makeCylinder(1.5, 10, V(-5, RING_R_OUT - 0.3, (z0 + z1) / 2), V(1, 0, 0)))


def board_placement():
    # KiCad STEP frame -> ball: x->+Z, Y->-X, z->-Y (det +1). Components face -Y (down).
    return App.Placement(App.Matrix(0, -1, 0, BOARD_X_REF,
                                    0, 0, -1, BOARD_Y_BACK,
                                    1, 0, 0, BOARD_Z0,
                                    0, 0, 0, 1))


def board_outline():
    b = box(0, -26, 0, 40, 0, 1.6)
    b.transformShape(board_placement().toMatrix())
    return b


def board_pt(bx, bY, bz=0.0):
    return board_placement().multVec(V(bx, bY, bz))


def frame():
    (x0, x1), (z0, z1) = PLATE_X, PLATE_Z
    body = box(x0, PLATE_Y0, z0, x1, PLATE_Y1, z1)
    # motor window, pad under the motor, cheeks, face plate (boss hole + M1.6 holes)
    body = body.cut(box(-M_W / 2 - 0.15, PLATE_Y0 - 1, Z_GEAR0 - 0.5, M_W / 2 + 0.15, PLATE_Y1 + 1, Z_MOTOR1 + 0.5))
    for sx in (-1, 1):
        body = body.fuse(box(sx * (M_W / 2 + 0.15), CHEEK_Y0, Z_GEAR0 - 0.5, sx * (M_W / 2 + 1.5), -M_H / 2 + 4.0, Z_MOTOR1 - 2.0))
        body = body.fuse(box(sx * (M_W / 2 - 1.0), CHEEK_Y0, Z_GEAR0 - 0.5, sx * (M_W / 2 + 1.5), -M_H / 2, Z_MOTOR1 - 2.0))   # ledge
    face = box(-M_W / 2 - 1.5, CHEEK_Y0, Z_BOSS - FACE_T, M_W / 2 + 1.5, M_H / 2 + 1.5, Z_BOSS)
    face = face.cut(cyl_z(M_BOSS_D / 2 + 0.15, Z_BOSS - 5, Z_BOSS + 5))
    for sx in (-1, 1):
        face = face.cut(cyl_z(M_HOLE_D / 2, Z_BOSS - 5, Z_BOSS + 5, x=sx * M_HOLE_PITCH / 2))
    body = body.fuse(face)
    # pin post: through the cap, pin pressed into its bore
    body = body.fuse(box(-3.0, PLATE_Y1 - 0.01, POST_Z[0], 3.0, 3.0, POST_Z[1]))
    body = body.cut(cyl_z(PIN_D / 2 - 0.05, POST_Z[1] - 6.0, POST_Z[1] + 1))
    # board standoffs under the plate (M2 from below)
    for hx, hY in BOARD_HOLES:
        c = board_pt(hx, hY)
        body = body.fuse(cyl_y(BOSS_D / 2, BOARD_Y_BACK, PLATE_Y0 + 0.01, x=c.x, z=c.z))
        body = body.cut(cyl_y(BOSS_HOLE / 2, BOARD_Y_BACK - 1, PLATE_Y0 + 3.0, x=c.x, z=c.z))
    # clearance holes for the tray screws
    for px, pz in TRAY_POSTS:
        body = body.cut(cyl_y(1.1, PLATE_Y0 - 1, PLATE_Y1 + 1, x=px, z=pz))
    return body.common(core_envelope(CORE_R_IN - 0.3, CORE_SPH_IN - 0.3))


def tray_and_battery():
    t, w, l, _ = CELL
    bat = box(BAT_X - w / 2, BAT_Y1 - t, BAT_Z - l / 2, BAT_X + w / 2, BAT_Y1, BAT_Z + l / 2)
    bb = bat.BoundBox
    yf = bb.YMin - TRAY_CLR - TRAY_FLOOR                                  # floor bottom
    x0, x1 = bb.XMin - TRAY_CLR - TRAY_WALL, bb.XMax + TRAY_CLR + TRAY_WALL
    z0, z1 = bb.ZMin - TRAY_CLR - TRAY_WALL, bb.ZMax + TRAY_CLR + TRAY_WALL
    tray = box(x0, yf, z0, x1, bb.YMax + 0.5, bb.ZMax + TRAY_CLR)
    tray = tray.cut(box(bb.XMin - TRAY_CLR, bb.YMin - TRAY_CLR, bb.ZMin - TRAY_CLR, bb.XMax + TRAY_CLR, bb.YMax + 2, bb.ZMax + TRAY_CLR + 1))
    tray = tray.cut(box(x0 - 1, bb.YMin + 2.5, bb.ZMin + 6, x1 + 1, bb.YMax + 2, bb.ZMax - 6))   # long-side openings
    for px, pz in TRAY_POSTS:
        tray = tray.fuse(cyl_y(2.4, yf, PLATE_Y0, x=px, z=pz))
        tray = tray.fuse(box(x1 - 1.0, yf, pz - 2.4, px, PLATE_Y0, pz + 2.4))                      # tie to the +X wall
        tray = tray.cut(cyl_y(0.85, PLATE_Y0 - 9, PLATE_Y0 + 1, x=px, z=pz))
    # ballast pocket under the floor: 1.2 mm ribs round a steel plate, open downward
    bx, by, bz, _ = BALLAST
    tray = tray.fuse(box(-bx / 2 - 1.2, yf - by, -bz / 2 - 1.2, bx / 2 + 1.2, yf + 0.01, bz / 2 + 1.2))
    tray = tray.cut(box(-bx / 2 - 0.15, yf - by - 1, -bz / 2 - 0.15, bx / 2 + 0.15, yf, bz / 2 + 0.15))
    ballast = box(-bx / 2, yf - by, -bz / 2, bx / 2, yf, bz / 2)
    return tray.common(core_envelope(CORE_R_IN - 0.3, CORE_SPH_IN - 0.3)), bat, ballast


def motor():
    m = box(-M_W / 2, -M_H / 2, Z_GEAR0 - 0.01, M_W / 2, M_H / 2, Z_MOTOR1)
    m = m.fuse(cyl_z(M_BOSS_D / 2, Z_BOSS, Z_GEAR0))
    sh = cyl_z(M_SHAFT_D / 2, SHAFT_TIP_Z, Z_BOSS)
    return m.fuse(sh.cut(box(-5, M_SHAFT_FLAT - M_SHAFT_D / 2, SHAFT_TIP_Z - 1, 5, 5, Z_BOSS + 1)))


def core_cup():
    outer = core_envelope(CORE_R, CORE_SPH)
    inner = core_envelope(CORE_R_IN, CORE_SPH_IN)
    body = outer.cut(inner).common(box(-50, -50, -50, 50, 50, CAP_Z0))
    body = body.cut(cyl_z(CORE_HOLE_R, -50, -20))                        # opening round dome A's hub
    # rails for the frame plate: two ribs per side from the floor to the rim; the chassis slides in from the top
    for sx in (-1, 1):
        for y0, y1 in ((PLATE_Y0 - RAIL_CLR - RAIL_T, PLATE_Y0 - RAIL_CLR), (PLATE_Y1 + RAIL_CLR, PLATE_Y1 + RAIL_CLR + RAIL_T)):
            body = body.fuse(box(sx * RAIL_X, y0, -40, sx * (CORE_R + 1), y1, CAP_Z0).common(outer))
    # bosses for the cap screws, inside the rim
    for x, y in CAP_BOSSES:
        body = body.fuse(cyl_z(2.6, CAP_Z0 - 8.0, CAP_Z0, x=x, y=y).common(core_envelope(CORE_R_IN + 0.5, CORE_SPH_IN + 0.5)))
        body = body.cut(cyl_z(BOSS_HOLE / 2, CAP_Z0 - 7.0, CAP_Z0 + 1, x=x, y=y))
    return body


def core_cap():
    outer = core_envelope(CORE_R, CORE_SPH)
    inner = core_envelope(CORE_R_IN, CORE_SPH_IN)
    body = outer.cut(inner).common(box(-50, -50, CAP_Z0, 50, 50, CORE_Z_TOP))                # skirt
    body = body.fuse(outer.common(box(-50, -50, CORE_Z_TOP - CAP_T, 50, 50, CORE_Z_TOP)))    # top plate
    for x, y in CAP_BOSSES:                                                                   # screw holes
        body = body.cut(cyl_z(1.1, CAP_Z0 - 1, CORE_Z_TOP + 1, x=x, y=y))
    # USB-C window: the receptacle face is at Z = +20 (board edge), centred on the connector
    u = board_pt(*USB_PT)
    w, h = USB_WIN
    win = box(u.x - w / 2 + h / 2, u.y - h / 2, CORE_Z_TOP - CAP_T - 1, u.x + w / 2 - h / 2, u.y + h / 2, CORE_Z_TOP + 1)
    win = win.fuse(cyl_z(h / 2, CORE_Z_TOP - CAP_T - 1, CORE_Z_TOP + 1, x=u.x - w / 2 + h / 2, y=u.y))
    win = win.fuse(cyl_z(h / 2, CORE_Z_TOP - CAP_T - 1, CORE_Z_TOP + 1, x=u.x + w / 2 - h / 2, y=u.y))
    body = body.cut(win)
    # pinhole over the button plunger (tip at KiCad x = 40.04, i.e. 0.06 behind the cap's inner face)
    p = board_pt(*BTN_PT)
    body = body.cut(cyl_z(PINHOLE_D / 2, CORE_Z_TOP - CAP_T - 1, CORE_Z_TOP + 1, x=p.x, y=p.y))
    # pin post passes through
    body = body.cut(box(-3.4, PLATE_Y1 - 0.4, CORE_Z_TOP - CAP_T - 1, 3.4, 3.4, CORE_Z_TOP + 1))
    return body


def com(shape):
    sols = shape.Solids or [shape]
    vt = sum(x.Volume for x in sols)
    c = V(0, 0, 0)
    for x in sols:
        c += x.CenterOfMass * (x.Volume / vt)
    return c


def export(doc, name, shape, outdir):
    o = doc.addObject('Part::Feature', name)
    o.Shape = shape
    Part.export([o], os.path.join(outdir, f'{name}.step'))
    MeshPart.meshFromShape(Shape=shape, LinearDeflection=0.04, AngularDeflection=0.25, Relative=False) \
        .write(os.path.join(outdir, f'{name}.stl'))
    bb = shape.BoundBox
    print(f'{name:9} {bb.XLength:6.2f} x {bb.YLength:6.2f} x {bb.ZLength:6.2f}  '
          f'[{bb.XMin:.1f}..{bb.XMax:.1f}, {bb.YMin:.1f}..{bb.YMax:.1f}, {bb.ZMin:.1f}..{bb.ZMax:.1f}]  vol {shape.Volume/1000:.1f} cm3')


def main():
    outdir = os.path.join(HERE, 'build', f'ball_{CELL_NAME}')
    os.makedirs(outdir, exist_ok=True)
    doc = App.newDocument('ball')
    P = {}
    P['dome_a'], P['dome_b'], P['ring'] = dome_a(), dome_b(), ring()
    P['core_cup'], P['core_cap'] = core_cup(), core_cap()
    P['frame'] = frame()
    P['tray'], P['battery'], P['ballast'] = tray_and_battery()
    P['motor'], P['board'] = motor(), board_outline()
    for n in ('dome_a', 'dome_b', 'ring', 'core_cup', 'core_cap', 'frame', 'tray', 'battery', 'motor', 'board', 'ballast'):
        export(doc, n, P[n], outdir)
    rho = 1.27e-3
    printed = [(P[n].Volume * rho, com(P[n])) for n in ('frame', 'tray', 'core_cup', 'core_cap')]
    fixed = [(CELL[3], com(P['battery'])), (10.0, com(P['motor'])), (9.0, com(P['board']))]
    for label, extra in (('without ballast', []), ('with ballast', [(BALLAST[3], com(P['ballast']))])):
        ms = printed + fixed + extra
        mt = sum(m for m, _ in ms)
        cy = sum(m * c.y for m, c in ms) / mt
        cx = sum(m * c.x for m, c in ms) / mt
        off = math.hypot(cx, cy)
        print(f'pendulum {label}: {mt:.0f} g, CG {off:.1f} mm below the axis, restoring torque {mt * off / 10:.0f} g*cm')
    print('shell masses (g): ' + ', '.join(f'{n} {P[n].Volume * rho:.0f}' for n in ('dome_a', 'dome_b', 'ring', 'core_cup', 'core_cap', 'frame', 'tray')))
    u, p = board_pt(*USB_PT), board_pt(*BTN_PT)
    print(f'cap: USB window centre X {u.x:.1f} Y {u.y:.1f}, receptacle face {CORE_Z_TOP - (BOARD_Z0 + 40.0):.1f} mm behind the cap face (cap plate is {CAP_T} thick); pinhole at X {p.x:.1f} Y {p.y:.1f}')
    if CHECK:
        problems = []
        inner_core = core_envelope(CORE_R_IN - 0.25, CORE_SPH_IN - 0.25)
        for n in ('frame', 'tray', 'battery', 'motor', 'board', 'ballast'):
            v = P[n].cut(inner_core).Volume
            if v > 0.01:
                problems.append(f'{n}: {v:.2f} mm3 outside the core cavity')
        dome_cavity = sphere(R_IN - 0.5).cut(cyl_z(50, LIP_Z0 - 0.5, Z_B_BODY + SHOULDER_H + 0.5).cut(cyl_z(R_LIP - LIP_WALL - 0.5, -50, 50)))
        for n in ('core_cup', 'core_cap'):
            v = P[n].cut(dome_cavity).Volume
            if v > 0.01:
                problems.append(f'{n}: {v:.2f} mm3 outside the dome cavity / inside the lip zone')
        pairs = [('frame', 'tray'), ('frame', 'battery'), ('frame', 'board'), ('frame', 'motor'), ('tray', 'battery'), ('tray', 'board'),
                 ('battery', 'board'), ('motor', 'battery'), ('motor', 'board'), ('motor', 'tray'), ('ballast', 'battery'),
                 ('core_cup', 'frame'), ('core_cup', 'tray'), ('core_cup', 'board'), ('core_cup', 'battery'), ('core_cup', 'motor'),
                 ('core_cap', 'frame'), ('core_cap', 'board'), ('core_cap', 'tray'), ('core_cup', 'core_cap'),
                 ('ring', 'dome_a'), ('ring', 'dome_b'), ('dome_a', 'dome_b'), ('motor', 'dome_a'), ('frame', 'dome_b'),
                 ('core_cup', 'dome_a'), ('core_cup', 'dome_b'), ('core_cap', 'dome_b'), ('core_cup', 'ring')]
        for a, b in pairs:
            c = P[a].common(P[b])
            if c.Volume > 0.5:
                cb = c.BoundBox
                problems.append(f'{a} x {b}: {c.Volume:.2f} mm3 overlap at x {cb.XMin:.1f}..{cb.XMax:.1f} y {cb.YMin:.1f}..{cb.YMax:.1f} z {cb.ZMin:.1f}..{cb.ZMax:.1f}')
        step = os.path.join(HERE, '..', 'zigbee_toy', 'build', 'zigbee_toy.step')
        if os.path.exists(step):
            brd = Part.read(step)
            brd.transformShape(board_placement().toMatrix())
            others = P['frame'].fuse(P['tray']).fuse(P['motor']).fuse(P['battery']).fuse(P['core_cup']).fuse(P['core_cap'])
            hits = 0.0
            for sol in brd.Solids:
                bb = sol.BoundBox
                if min(bb.XLength, bb.YLength, bb.ZLength) < 0.1:
                    continue
                if bb.intersect(others.BoundBox):
                    for on in ('frame', 'tray', 'motor', 'battery', 'core_cup', 'core_cap'):
                        c = sol.common(P[on])
                        if c.Volume > 0.05:
                            cb = c.BoundBox
                            hits += c.Volume
                            problems.append(f'board solid ({bb.XLength:.1f}x{bb.YLength:.1f}x{bb.ZLength:.1f}) x {on}: {c.Volume:.2f} mm3 at x {cb.XMin:.1f}..{cb.XMax:.1f} y {cb.YMin:.1f}..{cb.YMax:.1f} z {cb.ZMin:.1f}..{cb.ZMax:.1f}')
                v = sol.cut(inner_core).Volume
                if v > 0.01:
                    problems.append(f'board solid at ({bb.Center.x:.1f},{bb.Center.y:.1f},{bb.Center.z:.1f}) outside the core by {v:.2f} mm3')
            if hits > 0.5:
                problems.append(f'board STEP overlaps chassis/motor/battery/core by {hits:.2f} mm3')
            MeshPart.meshFromShape(Shape=brd, LinearDeflection=0.1, AngularDeflection=0.5, Relative=False).write(os.path.join(outdir, 'board_full.stl'))
        print('checks:', 'CLEAN' if not problems else 'PROBLEMS')
        for p in problems:
            print('   ' + p)
    App.closeDocument(doc.Name)


try:
    main()
except Exception:
    traceback.print_exc()
    raise
