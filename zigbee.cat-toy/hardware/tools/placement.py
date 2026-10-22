"""Footprint placement, rev B floorplan (board mm, origin top-left, Y down).

Rotation in degrees, KiCad convention (counter-clockwise on screen); a 4th field 'B' puts
the footprint on the back.
  * JLC Economic PCBA: every assembled body stays >= 1.0 mm from the edges. JLC panelizes
    Economic boards itself and routes them apart (about 0.2 mm tolerance) and asks for 0.3 mm
    (PCB Assembly FAQs part 2, item 14); no edge rails. The 2.5 mm in the general assembly terms
    is for Standard PCBA, where the conveyor grips the board's own edge. Only the parts that must
    face the shell sit on the short edges: the JSTs (hand-soldered) on the left, USB-C, button on
    the right.
  * Module as high as the edge rule allows: a MINI-1's antenna then overhangs the top edge, and
    the room goes to a channel between the module's bottom pins and the row under it.
  * Module top-centre, u.FL in its top-left corner, the antenna lead leaves over the top edge.
  * Left edge = internal wiring: three side-entry JST-PH stacked from the top corner
    (motor, speaker, battery), each circuit beside it, wired to the module's left pins.
  * Under the module: crystal, regulator, battery divider, accelerometer, charger.
  * Right edge: status LED above the USB-C, side-push button below it.
  * Back: labelled test pads in three groups, clear of the module so its underside stays free for routing.
"""

BOARD_W = 36.0
BOARD_H = 23.0
CORNER_R = 2.0
EDGE_KEEPOUT = 1.0          # assembled bodies vs the board edge (JLC Economic: 0.3 mm + margin)
EDGE_PARTS = {'J1': 'right', 'SW1': 'right', 'J4': 'left', 'J5': 'left'}   # face the shell through that edge on purpose

PLACE = {
    'U1': (20.0, 7.3, 0),

    # ---- left edge: JST-SH motor + speaker (front flush with the edge), JST-PH battery
    'J4': (3.075, 4.45, 270),     # MOTOR
    'J5': (3.075, 9.65, 270),     # SPEAKER
    'J3': (4.4, 14.74, 270),     # BATTERY

    # ---- motor driver
    'U2': (8.3, 4.4, 0),
    'C3': (7.9, 2.5, 0),
    'C2': (11.3, 1.8, 0),
    'R6': (10.9, 4.0, 0),
    'C7': (10.9, 5.05, 0),
    'C9': (12.5, 6.2, 90),

    # ---- speaker
    'R11': (7.0, 6.5, 270),
    'D1': (7.6, 9.8, 270),
    'Q2': (10.5, 9.8, 180),
    'R7': (10.6, 12.0, 180),
    'R8': (12.55, 12.0, 0),
    'R9': (12.65, 8.6, 90),
    'C5': (12.65, 10.5, 270),

    # ---- battery
    'C15': (8.6, 21.4, 180),
    'U5': (7.8, 18.25, 90),
    'Q1': (7.9, 13.9, 180),
    'C1': (11.4, 14.6, 270),

    # ---- under the module
    'Y1': (16.0, 15.0, 180),
    'C12': (15.0, 16.8, 0),
    'C13': (17.0, 16.8, 0),
    'U3': (19.6, 15.05, 0),
    'C10': (21.35, 16.6, 90),
    'C11': (19.6, 17.3, 0),
    'R16': (22.2, 14.85, 90),
    'R17': (23.15, 14.85, 90),
    'C6': (14.85, 20.9, 90),
    'U6': (17.35, 20.9, 90),
    'C8': (20.15, 20.9, 270),
    'R12': (21.75, 20.6, 90),
    'R13': (22.75, 20.6, 270),
    'C16': (23.75, 20.6, 270),

    # ---- right edge: LED, USB-C, button; USB protection, charger
    'R4': (34.1, 3.7, 0),
    'D2': (34.1, 2.4, 180),
    'J1': (33.3, 10.1, 90),      # receptacle face 1.0 mm past the edge: flush with the shell outside
    'SW1': (33.85, 18.6, 90),
    'R2': (28.3, 5.6, 0),
    'R1': (27.5, 10.0, 180),
    'U7': (27.0, 15.85, 0),
    'U4': (30.0, 20.6, 90),
    'R3': (27.7, 20.1, 90),
    'C4': (30.4, 18.0, 180),
    'R14': (27.5, 12.55, 180),
    'R15': (27.45, 13.7, 0),

    # ---- test pads, back side ('B'): placeholder grid, placed properly after routing
    'TP1': (14.6, 19.4, 0, 'B'),
    'TP2': (19.0, 18.4, 0, 'B'),
    'TP3': (33.15, 10.1, 0, 'B'),
    'TP4': (14.6, 21.6, 0, 'B'),
    'TP5': (24.0, 21.6, 0, 'B'),
    'TP6': (24.0, 19.4, 0, 'B'),
    'TP7': (7.2, 6.0, 0, 'B'),
    'TP8': (7.2, 8.4, 0, 'B'),
    'TP9': (7.2, 10.8, 0, 'B'),
    'TP10': (12.4, 6.0, 0, 'B'),
    'TP11': (12.4, 8.4, 0, 'B'),
    'TP12': (12.4, 10.8, 0, 'B'),
    'TP13': (18.6, 16.85, 0, 'B'),
    'TP15': (23.22, 11.85, 0, 'B'),
    'TP16': (25.0, 10.6, 0, 'B'),
    'TP17': (18.4, 3.4, 0, 'B'),
    'TP18': (23.4, 3.4, 0, 'B'),
    'TP19': (31.6, 21.85, 0, 'B'),
    'TP20': (29.5, 19.4, 0, 'B'),
    'TP21': (14.8, 3.4, 0, 'B'),
    'TP22': (29.5, 21.8, 0, 'B'),

    # ---- M2 mounting holes, NPTH
    'H1': (30.2, 2.6, 0),
    'H2': (12.0, 20.6, 0),
}
