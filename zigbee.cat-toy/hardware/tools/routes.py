"""Hand-routed copper (rev B). Locked before autorouting.

R(net, layer, width, *points): polyline; points are (x, y) or 'REF.PAD' (pad centre).
V(net, x, y[, size, drill]): through via.  Off-angle segments get 45-degree elbows.
"""

ROUTES = []
VIAS = []


def R(net, layer, width, *pts):
    ROUTES.append((net, layer, width, pts))


def V(net, x, y, size=0.6, drill=0.3):
    VIAS.append((net, x, y, size, drill))




# ------------------------------------------------------------------ battery + protection (left edge, lower)
R('CELL_P', 'F', 0.5, 'J3.1', (6.2, 17.2), (8.2, 15.2), 'Q1.3')       # main current to the FET drain
R('CELL_P', 'F', 0.4, (6.2, 17.2), 'C15.1')
R('CELL_P', 'F', 0.3, 'C15.1', (9.85, 17.62), 'U5.3')                 # XB5353A VDD sense
R('CELL_N', 'F', 0.5, 'J3.2', 'C15.2')
R('CELL_N', 'F', 0.4, 'C15.2', (9.2, 19.8), (11.2, 19.8), 'U5.2')     # between U5's pad rows
R('GND', 'F', 0.6, 'U5.4', (10.25, 22.0), (12.15, 22.0), 'U5.5')      # VM = board GND
V('GND', 10.25, 22.2, 0.8, 0.4)
V('GND', 12.15, 22.2, 0.8, 0.4)
R('GND', 'F', 0.4, 'Q1.1', (13.6, 16.15))                             # FET gate
V('GND', 13.6, 16.15)

# ------------------------------------------------------------------ VBAT
# bottom trunk: FET -> down the left column -> along the bottom edge -> charger BAT, with taps
R('VBAT', 'F', 0.6, 'Q1.2', (15.4, 14.25), (15.4, 24.9), (30.96, 24.9), 'U4.3')
R('VBAT', 'F', 0.6, 'C1.1', (15.4, 18.05))                            # bulk 22u
R('VBAT', 'F', 0.5, 'C6.1', (16.4, 24.9))                             # regulator input cap
R('VBAT', 'F', 0.5, 'U6.1', (18.0, 24.9))                             # VIN
R('VBAT', 'F', 0.3, 'U6.3', (19.9, 24.9))                             # EN = VIN
R('VBAT', 'F', 0.4, 'R12.1', (6.3, 21.3), (6.3, 24.9), (15.4, 24.9))  # battery divider
# B.Cu around the left column (open towards the module): FET -> motor driver, speaker link
V('VBAT', 12.34, 13.3, 0.8, 0.4)
R('VBAT', 'F', 0.6, 'Q1.2', (12.34, 13.3))
R('VBAT', 'B', 0.8, (12.34, 13.3), (7.0, 13.3), (6.35, 12.65), (6.35, 2.65), (7.0, 2.0), (11.0, 2.0), (11.77, 2.77), (11.77, 3.2))
V('VBAT', 11.77, 3.2, 0.8, 0.4)
R('VBAT', 'F', 0.6, (11.77, 3.2), 'C2.1')
R('VBAT', 'F', 0.5, (11.77, 3.2), 'C3.1')
R('VBAT', 'F', 0.3, 'C3.1', (9.65, 3.98), 'U2.1')
V('VBAT', 6.35, 7.1, 0.8, 0.4)
R('VBAT', 'F', 0.5, (6.35, 7.1), 'R11.1')

# ------------------------------------------------------------------ motor driver
R('MOT_A', 'F', 0.25, 'U2.2', (9.15, 4.8), (8.85, 4.5))
R('MOT_A', 'F', 0.5, (8.85, 4.5), (7.85, 3.5), (7.0, 3.5), 'J4.1')
R('MOT_B', 'F', 0.25, 'U2.3', (8.7, 5.3))
R('MOT_B', 'F', 0.5, (8.7, 5.3), (6.0, 5.3), 'J4.2')
R('GND', 'F', 0.25, 'U2.4', (10.4, 5.8))                              # into the thermal pad
V('GND', 10.6, 5.05)                                                  # thermal pad via

# ------------------------------------------------------------------ speaker
R('SPK_P', 'F', 0.5, 'R11.2', (9.02, 8.0), 'D1.1')
R('SPK_P', 'F', 0.5, 'D1.1', (6.35, 8.95), (5.4, 9.9), 'J5.1')
R('SPK_N', 'F', 0.5, 'J5.2', (6.0, 12.0), (6.25, 12.25), 'D1.2')
R('SPK_N', 'F', 0.5, 'D1.2', 'Q2.3')
R('GND', 'F', 0.4, 'Q2.2', (10.65, 8.55))
V('GND', 10.65, 8.55)

# ------------------------------------------------------------------ crystal (module bottom-left pins)
R('XTAL_N', 'F', 0.2, 'U1.13', (20.8, 15.0), (20.49, 15.31), 'Y1.2')
R('XTAL_N', 'F', 0.2, 'Y1.2', (21.3, 17.82), 'C13.1')
R('XTAL_P', 'F', 0.2, 'U1.12', (20.0, 14.6), (18.4, 14.6), (17.99, 15.01), 'Y1.1')
R('XTAL_P', 'F', 0.2, 'C12.1', (17.4, 17.82), 'Y1.1')
R('GND', 'F', 0.25, 'U1.14', (21.6, 15.7), 'C13.2')
R('GND', 'F', 0.3, 'C12.2', (16.4, 15.35))
V('GND', 16.4, 15.35)

# ------------------------------------------------------------------ 3V3: regulator -> trunk under row A -> module pin 3 (B.Cu inside the pad ring)
R('3V3', 'F', 0.4, (18.0, 18.9), (31.02, 18.9))
R('3V3', 'F', 0.4, 'U6.5', (18.0, 18.9))
for ref in ('C8.1', 'C11.1', 'C10.1', 'R16.1', 'R17.1'):
    R('3V3', 'F', 0.4, ref, ({'C8.1': 21.74, 'C11.1': 23.66, 'C10.1': 27.86, 'R16.1': 29.44, 'R17.1': 31.02}[ref], 18.9))
R('3V3', 'F', 0.4, (17.2, 18.9), (18.0, 18.9))                        # via west of the crystal, B.Cu stays clear of it
V('3V3', 17.2, 18.9)
R('3V3', 'B', 0.4, (17.2, 18.9), (17.2, 13.0), (19.95, 10.25), (19.95, 5.55))
V('3V3', 19.95, 5.55)
R('3V3', 'F', 0.4, 'U1.3', (19.95, 5.55))
R('3V3', 'F', 0.4, 'U1.3', 'C9.1')
R('GND', 'F', 0.3, 'U6.2', (18.95, 21.5))
V('GND', 18.95, 21.5)

# ------------------------------------------------------------------ USB-C
# Each pad pair drops to B.Cu right of the pad row (D+ vias at x 34.45, D- at 35.15) and is joined
# there; both run left on B.Cu right under the module's bottom edge and come up through the pad
# ring just inside pins 17/18. The channel below the module stays free for the vias of the bottom
# and right pins' signals; the gap left of the USB-C pads stays free for CC1/CC2.
for pad, net, x in (('B6', 'USB_DP', 34.45), ('A6', 'USB_DP', 34.45), ('A7', 'USB_DN', 35.15), ('B7', 'USB_DN', 35.15)):
    y = {'B6': 12.15, 'A6': 13.15, 'A7': 12.65, 'B7': 13.65}[pad]
    R(net, 'F', 0.2, 'J1.' + pad, (x, y))
    V(net, x, y, 0.5, 0.3)
R('USB_DP', 'B', 0.2, (34.45, 12.15), (34.45, 13.15), (24.85, 13.15), (24.85, 11.9))
R('USB_DN', 'B', 0.2, (35.15, 12.65), (35.15, 13.65), (34.85, 13.95), (24.0, 13.95), (24.0, 11.9))
V('USB_DP', 24.85, 11.9, 0.5, 0.3)
V('USB_DN', 24.0, 11.9, 0.5, 0.3)
R('USB_DP', 'F', 0.2, (24.85, 11.9), 'U1.18')
R('USB_DN', 'F', 0.2, (24.0, 11.9), 'U1.17')
# CC1 leaves left and runs down the gap to R1
R('CC1', 'F', 0.2, 'J1.A5', (32.2, 14.15), (32.2, 18.6), 'R1.1')
# CC2 leaves left, up the gap to R2
R('CC2', 'F', 0.2, 'J1.B5', (32.2, 11.15), (32.2, 4.3), 'R2.1')
# VBUS: both pad pairs drop to B.Cu right of the pad row, down the right edge to the charger
# (between each pad pair's locating peg and the neighbouring CC pad)
R('VBUS', 'F', 0.25, 'J1.A9', (34.3, 10.72), (35.8, 10.72))
V('VBUS', 35.8, 10.72)
R('VBUS', 'F', 0.25, 'J1.A4', (34.3, 15.08), (35.8, 15.08))
V('VBUS', 35.8, 15.08)
R('VBUS', 'B', 0.4, (35.8, 10.72), (35.8, 23.0), (35.05, 23.75), (34.2, 23.75))
V('VBUS', 34.2, 23.75)
R('VBUS', 'F', 0.4, (34.2, 23.75), 'U4.4')
R('VBUS', 'F', 0.4, (34.2, 23.75), (35.05, 23.75), 'C4.1')

# ------------------------------------------------------------------ ground vias for pads the pour can't reach (found by search: main B.Cu pour below, stub clear on F.Cu)
for pad, x, y in (('C8.2', 20.64, 22.35), ('R3.2', 28.0, 21.27), ('R6.2', 13.3, 5.88), ('C16.2', 22.86, 20.12), ('R15.2', 26.42, 19.88)):
    R('GND', 'F', 0.25, pad, (x, y))
    V('GND', x, y, 0.5, 0.3)

# ------------------------------------------------------------------ status LED: the module's LED line arrives on B.Cu at this via
V('LED_STATUS', 34.95, 5.0)
R('LED_STATUS', 'F', 0.25, (34.95, 5.0), 'R4.1')
R('LED_A', 'F', 0.25, 'R4.2', 'D2.2')

# ------------------------------------------------------------------ VBUS_SENSE: straight down the gap between the module and the USB-C pads
R('VBUS_SENSE', 'F', 0.2, 'U1.26', (31.45, 11.15), (31.8, 11.5), (31.8, 19.8), (31.909, 19.909), (27.85, 19.909),
  (27.51, 20.249), (27.51, 20.975), (26.72, 21.765), 'R15.1')
R('VBUS_SENSE', 'F', 0.2, 'R15.1', 'R14.2')

# ------------------------------------------------------------------ BTN: over the top of the USB-C, then B.Cu under it (no F.Cu under the shell)
R('BTN', 'F', 0.2, 'U1.23', (28.8, 11.4), (29.64, 10.56))
V('BTN', 29.64, 10.56)
R('BTN', 'B', 0.2, (29.64, 10.56), (30.35, 9.85), 'TP17.1')
R('BTN', 'B', 0.2, 'TP17.1', (31.35, 9.1), (31.35, 7.79), (32.3, 6.84), (36.0, 6.84), (36.4, 7.24), (36.4, 18.6), (36.65, 18.85))
V('BTN', 36.65, 18.85)
R('BTN', 'F', 0.2, (36.65, 18.85), (36.65, 19.975), (37.1, 19.975))
R('BTN', 'F', 0.2, (37.1, 19.975), (37.35, 19.975), (38.27, 20.9), (38.27, 21.51), (37.36, 22.425), (37.1, 22.425))   # both pads are '1'

# ------------------------------------------------------------------ CHG_STAT: module corner pin, past the USB lanes on F.Cu, then B.Cu down to the charger
R('CHG_STAT', 'F', 0.2, 'U1.25', (31.4, 11.95), (31.4, 14.45), (31.1, 14.75))
V('CHG_STAT', 31.1, 14.75, 0.5, 0.3)
V('GND', 30.25, 14.75, 0.5, 0.3)        # the module's corner GND pad is boxed in by CHG_STAT and SCL on F.Cu

# ------------------------------------------------------------------ ACC_INT1: past the USB lanes, B.Cu to a via in the middle of the accelerometer
R('ACC_INT1', 'F', 0.2, 'U1.16', (23.2, 14.6))
V('ACC_INT1', 23.2, 14.6, 0.5, 0.3)
R('ACC_INT1', 'B', 0.2, (23.2, 14.6), (23.2, 15.5), (24.8, 17.1), 'TP13.1')
V('ACC_INT1', 25.6, 17.1, 0.5, 0.3)
R('ACC_INT1', 'F', 0.2, (25.6, 17.1), (25.51, 17.19), 'U3.5')

# ------------------------------------------------------------------ I2C_SCL to the accelerometer: B.Cu from the SCL test-pad via, so no track threads between row A's pads
R('I2C_SCL', 'B', 0.2, (30.0826, 17.0), (28.3826, 15.3), (26.0, 15.3))
V('I2C_SCL', 26.0, 15.3, 0.5, 0.3)
R('I2C_SCL', 'F', 0.2, (26.0, 15.3), (25.51, 15.79), 'U3.12')
