"""Footprint placement, rev A floorplan v6 (board mm, origin top-left, Y down).

Rotation in degrees, KiCad convention (counter-clockwise on screen); a 4th field 'B' puts
the footprint on the back.
  * Top edge: module antenna hangs 2.8 mm past the edge (no FR4 under it; pads stay 0.7 mm in).
  * Left edge = internal wiring: three side-entry JST-PH stacked from the top corner
    (motor, speaker, battery), housings overhanging ~2.4 mm, each circuit beside it.
  * Right edge = external access through the shell: LEDs (above the USB-C, so the button
    doesn't shade them), USB-C, side-push button.
  * Under the module: crystal + decoupling row; VBAT trunk east-west beneath it.
  * Bottom band, tucked under the trunk: accelerometer (left), charger (centre-right).
  * 2 x M2 (ISO 7380 button head) holes in diagonal corners.
  * Back: 20 labelled test pads - power/charger block at the bottom, signal grid under the module.
"""

BOARD_W = 40.0
BOARD_H = 26.0
CORNER_R = 2.0

PLACE = {
    'U1': (24.0, 6.2, 180),

    # ---- left edge connectors (side entry, opening to the left)
    'J4': (4.4, 2.8, 270),       # MOTOR
    'J5': (4.4, 10.0, 270),      # SPEAKER
    'J3': (4.4, 17.2, 270),      # BATTERY

    # ---- motor driver (VM caps above it, either side of the VBAT via)
    'U2': (11.0, 3.8, 0),
    'C2': (8.0, 1.5, 180),
    'C3': (11.3, 1.5, 0),
    'R6': (13.8, 4.5, 90),

    # ---- speaker
    'R11': (10.6, 6.8, 0),
    'D1': (10.6, 10.1, 270),
    'Q2': (14.0, 11.2, 270),
    'R7': (15.4, 7.6, 270),
    'R8': (13.6, 7.6, 90),

    # ---- battery protection + reverse FET
    'C15': (9.0, 18.2, 270),
    'U5': (11.6, 19.8, 270),
    'Q1': (11.8, 15.0, 180),
    'C1': (14.6, 18.6, 270),

    # ---- under the module: crystal + decoupling row
    'C12': (17.4, 19.0, 90),
    'Y1': (20.2, 19.0, 180),
    'C13': (23.0, 19.0, 270),
    'C9': (24.6, 19.0, 270),
    'C8': (26.1, 19.0, 270),
    'C7': (27.6, 19.0, 270),
    'C6': (29.3, 19.0, 90),

    # ---- bottom band: accelerometer (left), charger (centre)
    'U3': (7.9, 23.5, 0),
    'C10': (10.7, 24.3, 0),
    'U4': (26.4, 23.9, 0),
    'R3': (30.0, 23.0, 0),
    'C4': (30.0, 24.8, 0),

    # ---- right edge: SWD, USB-C, CC resistors, button
    'J2': (33.4, 1.2, 0),
    'J1': (37.3, 13.3, 90),      # receptacle face 1.0 mm past the edge: flush with the shell outside
    'R1': (31.3, 18.0, 270),
    'SW1': (38.0, 21.6, 90),
    # LEDs above the USB-C, clear of the button: CC2 resistor | status | charge
    'R2': (35.79, 6.3, 90),
    'D3': (37.42, 6.3, 270),
    'D2': (39.1, 6.3, 270),
    # their series resistors stay in the bottom-right pocket
    'R5': (34.4, 21.0, 90),
    'R4': (34.4, 24.4, 90),

    # ---- test points, back side ('B'); label sides in silk.py.
    # Power + charger block along the bottom edge, labels right of the pads.
    'TP1': (10.4, 23.5, 0, 'B'), 'TP2': (15.6, 23.5, 0, 'B'), 'TP3': (20.8, 23.5, 0, 'B'),    # VBAT VDD VBUS
    'TP4': (10.4, 25.15, 0, 'B'), 'TP5': (15.6, 25.15, 0, 'B'), 'TP6': (20.8, 25.15, 0, 'B'),    # GND nCHG PROG
    # Signal grid under the module (the title moved out of its way), labels under the pads:
    #   row 1  PWM  DIR  ISNS GAIN TX      (motor driver | UART, next to J2's GND)
    #   row 2  SPK  GND  SDA  SCL  RX
    #   row 3  INT1 INT2 BTN  LED
    **{f'TP{n}': (x, y, 0, 'B') for n, x, y in [
        (7, 13.8, 3.6), (8, 17.2, 3.6), (9, 20.6, 3.6), (10, 24.0, 3.6), (19, 27.4, 3.6),
        (11, 13.8, 6.6), (12, 17.2, 6.6), (15, 20.6, 6.6), (16, 24.0, 6.6), (20, 27.4, 6.6),
        (13, 13.8, 9.6), (14, 17.2, 9.6), (17, 20.6, 9.6), (18, 24.0, 9.6)]},

    # ---- M2 mounting holes, NPTH
    'H1': (37.4, 2.6, 0),
    'H2': (2.6, 23.7, 0),
}
