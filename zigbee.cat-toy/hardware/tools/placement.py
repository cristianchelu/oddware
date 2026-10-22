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

BOARD_W = 40.0
BOARD_H = 26.0
CORNER_R = 2.0
EDGE_KEEPOUT = 1.0          # assembled bodies vs the board edge (JLC Economic: 0.3 mm + margin)
EDGE_PARTS = {'J1', 'SW1'}  # face the shell through the right edge on purpose

PLACE = {
    'U1': (24.8, 7.3, 0),

    # ---- left edge connectors (side entry, opening to the left)
    'J4': (4.4, 2.8, 270),       # MOTOR
    'J5': (4.4, 10.0, 270),      # SPEAKER
    'J3': (4.4, 17.2, 270),      # BATTERY

    # ---- motor driver, VM caps above it: C3's ground pad sits over the driver's thermal pad
    'U2': (10.6, 5.05, 0),
    'C3': (9.9, 2.95, 0),
    'C2': (13.85, 3.2, 0),
    'R6': (13.4, 5.9, 90),

    # ---- speaker
    'R11': (8.2, 7.1, 0),
    'D1': (8.4, 10.6, 270),
    'Q2': (11.6, 10.8, 270),
    'R7': (14.4, 12.4, 90),
    'R8': (14.4, 9.4, 90),

    # ---- battery protection + reverse FET, battery divider in the corner pocket
    'C15': (8.6, 18.4, 270),
    'U5': (11.2, 19.8, 270),
    'Q1': (11.4, 14.9, 180),
    'C1': (14.2, 19.0, 270),
    'R12': (7.9, 21.3, 0),
    'R13': (7.9, 22.9, 0),

    # ---- module: 3V3 decoupling and EN delay at its left pins
    'C9': (16.6, 4.85, 90),
    'R9': (16.6, 8.45, 270),
    'C5': (16.6, 11.55, 270),

    # ---- row under the module: crystal, accelerometer, I2C pull-ups
    'C12': (16.4, 17.0, 90),
    'Y1': (19.24, 17.0, 180),
    'C13': (22.08, 17.0, 90),
    'C11': (23.66, 17.0, 90),
    'U3': (25.76, 17.0, 0),
    'C10': (27.86, 17.0, 90),
    'R16': (29.44, 17.0, 90),
    'R17': (31.02, 17.0, 90),

    # ---- bottom row: regulator, ADC filter, USB detect, charger, CC1
    'C6': (16.4, 21.4, 90),
    'U6': (18.95, 21.5, 90),
    'C8': (21.74, 21.4, 270),
    'C16': (23.56, 21.4, 90),
    'R14': (25.14, 21.4, 270),
    'R15': (26.72, 21.4, 90),
    'R3': (28.3, 21.4, 90),
    'U4': (32.1, 21.95, 0),
    'C4': (35.05, 21.85, 90),
    'R1': (33.4, 19.35, 0),

    # ---- right edge: CC2 pull-down, status LED, USB-C, button
    'R2': (33.25, 3.3, 0),
    'R4': (34.0, 5.9, 270),
    'D2': (36.3, 6.3, 180),
    'J1': (37.3, 12.9, 90),      # receptacle face 1.0 mm past the edge: flush with the shell outside
    'SW1': (38.0, 21.2, 90),

    # ---- test pads, back side ('B'), grouped by where they are probed; label sides in silk.py.
    # Bottom edge (labels right): power, charger
    'TP1': (10.4, 23.5, 0, 'B'), 'TP4': (10.4, 25.15, 0, 'B'),     # VBAT  GND
    'TP2': (15.6, 23.5, 0, 'B'),                                   # 3V3
    'TP5': (32.14, 21.97, 0, 'B'),                                 # nCHG, on its via under the charger (label right)
    'TP3': (20.8, 23.5, 0, 'B'), 'TP6': (20.8, 25.15, 0, 'B'),     # VBUS  PROG
    # Behind the accelerometer, each centred on its signal's via (labels right): SDA INT1 SCL
    'TP15': (22.4, 15.4, 0, 'B'), 'TP13': (25.6, 17.1, 0, 'B'), 'TP16': (29.65, 22.04, 0, 'B'),
    # Behind the motor driver and speaker
    'TP7': (8.7, 6.0, 0, 'B'), 'TP8': (11.4, 6.0, 0, 'B'), 'TP9': (14.7, 6.0, 0, 'B'),     # IN2  IN1  ISNS (labels above)
    'TP10': (15.6, 7.5, 0, 'B'), 'TP11': (11.7, 9.6, 0, 'B'), 'TP12': (14.7, 11.0, 0, 'B'),  # GAIN (on its via, label right) SPK GND (below)
    # Behind the module's right pads and the USB-C: UART with its ground, LED data, BOOT, EN
    'TP19': (33.2, 3.0, 0, 'B'),                                                             # TX, clear of the MINI-1 antenna keepout
    'TP20': (30.6, 3.85, 0, 'B'), 'TP18': (30.6, 5.85, 0, 'B'),                                # RX LED (labels right)
    'TP22': (32.8, 10.2, 0, 'B'), 'TP17': (30.6, 9.85, 0, 'B'),                              # GND (label below) BOOT (label left)
    'TP21': (17.5, 9.55, 0, 'B'),                                                            # EN, on its via beside the EN resistor (label left)

    # ---- M2 mounting holes, NPTH
    'H1': (37.4, 2.6, 0),
    'H2': (2.6, 23.7, 0),
}
