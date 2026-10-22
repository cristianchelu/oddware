"""Hand-routed copper (rev B, 36 x 22). Locked before autorouting.

R(net, layer, width, *points): polyline; points are (x, y) or 'REF.PAD' (pad centre).
V(net, x, y[, size, drill]): through via.  Off-angle segments get 45-degree elbows.
"""

ROUTES = []
VIAS = []


def R(net, layer, width, *pts):
    ROUTES.append((net, layer, width, pts))


def V(net, x, y, size=0.6, drill=0.3):
    VIAS.append((net, x, y, size, drill))


# ------------------------------------------------------------------ VBAT trunk: battery FET -> back layer up the left column -> driver
R('VBAT', 'F', 0.6, 'Q1.2', (10.0, 13.0), 'C1.1')
V('VBAT', 10.0, 13.0, 0.8, 0.4)
R('VBAT', 'B', 0.8, (10.0, 13.0), (6.6, 13.0), (5.95, 12.35), (5.95, 1.85), (6.3, 1.5))
V('VBAT', 6.3, 1.5, 0.8, 0.4)
R('VBAT', 'F', 0.5, (6.3, 1.5), 'C3.1')
V('VBAT', 5.95, 6.0, 0.8, 0.4)                                        # speaker link
R('VBAT', 'F', 0.5, (5.95, 6.0), 'R11.1')

# ------------------------------------------------------------------ motor driver: VM bypass loop, outputs
R('VBAT', 'F', 0.3, (7.35, 2.5), 'U2.1')                              # (7.35, 2.5) is inside C3's VBAT pad
R('VBAT', 'F', 0.4, 'C3.1', (7.42, 1.5), (9.9, 1.5), 'C2.1')         # bulk cap, over the top of C3
R('GND', 'F', 0.35, 'C3.2', (8.38, 3.7))                              # C3's ground pad onto the thermal pad, between pins 1 and 8
V('GND', 8.3, 3.85, 0.5, 0.3)                                         # thermal pad vias, TI's DSG land pattern spots
V('GND', 8.3, 4.95, 0.5, 0.3)
R('GND', 'F', 0.25, 'U2.4', (8.0, 5.15))                              # into the thermal pad
R('GND', 'F', 0.4, 'C2.2', (12.0, 2.8))                               # C2 + C14 ground via, in the B gap between GAINSEL and IN2
V('GND', 12.0, 2.8, 0.5, 0.3)
# C14 (100n) at module pin 3: 3V3 runs down the gap between C9 and the module pads, GND to C2's pad.
R('3V3', 'F', 0.25, 'C14.1', (13.25, 4.22), (13.25, 5.3), (13.5, 5.55), 'U1.3')
R('GND', 'F', 0.3, 'C14.2', (12.0, 2.8))
# R5 (IO9 pull-up): 3V3 from C14, BTN up to a via and along B, under the antenna keepout edge, to TP17.
R('3V3', 'F', 0.25, 'R5.1', 'C14.1')
R('BTN', 'F', 0.2, 'R5.2', (12.8, 1.3))
V('BTN', 12.8, 1.3, 0.5, 0.3)
R('BTN', 'B', 0.2, (12.8, 1.3), (12.8, 1.9), (13.3, 2.4), (17.4, 2.4), 'TP17.1')
R('MOT_A', 'F', 0.5, 'J4.1', (6.4, 3.95))
R('MOT_A', 'F', 0.25, (6.4, 3.95), (6.6, 4.15), 'U2.2')
R('MOT_B', 'F', 0.5, 'J4.2', (6.4, 4.95))
R('MOT_B', 'F', 0.25, (6.4, 4.95), (6.7, 4.65), 'U2.3')

# ------------------------------------------------------------------ speaker: flyback across the jack, FET drain under the diode
R('SPK_P', 'F', 0.5, 'J5.1', (6.6, 9.15), 'D1.1')
R('SPK_P', 'F', 0.4, 'R11.2', 'D1.1')
R('SPK_N', 'F', 0.5, 'J5.2', (9.21, 10.15), 'Q2.3')                   # under D1's body, into Q2's drain
R('SPK_N', 'F', 0.5, (7.6, 10.15), 'D1.2')
R('GND', 'F', 0.4, 'Q2.2', (11.4, 7.95))
V('GND', 11.4, 7.95)

# ------------------------------------------------------------------ battery: + to the FET (top), - through the protection IC (below)
R('CELL_P', 'F', 0.6, 'J3.1', 'Q1.3')
R('CELL_P', 'F', 0.5, 'J3.1', (5.6, 15.55), (10.4, 15.55), (10.65, 15.8), (10.65, 18.65), (10.26, 19.04), 'U5.3')   # round the IC's ground pad
R('CELL_P', 'F', 0.5, (10.26, 19.04), (10.1, 19.2), (10.1, 20.6), (9.8, 20.9), 'C15.1')
R('CELL_N', 'F', 0.5, 'J3.2', (5.6, 17.94), (5.6, 20.4), (8.15, 20.4), (8.4, 20.15), (8.4, 19.3), 'U5.2')   # enters pad 2 at its west half, clear of C15's + pad
R('CELL_N', 'F', 0.5, (7.75, 20.4), (8.02, 20.67), 'C15.2')
R('GND', 'F', 0.5, 'U5.5', 'U5.4')                                    # protection IC VM pins = board GND
R('GND', 'F', 0.5, 'U5.5', (5.95, 16.95))
V('GND', 5.95, 16.95)
R('GND', 'F', 0.4, 'Q1.1', (10.05, 14.55))                            # FET gate
V('GND', 10.05, 14.55)
R('GND', 'F', 0.4, 'C1.2', (12.2, 16.6))                               # bulk cap ground: its own via
V('GND', 12.2, 16.6)

# ------------------------------------------------------------------ VBAT along the bottom: back-layer channel between the two test pad rows
R('VBAT', 'B', 0.6, (10.0, 13.0), (12.7, 13.0), (13.4, 13.7), (13.4, 18.2), 'TP1.1')
R('VBAT', 'B', 0.5, 'TP1.1', (15.5, 20.3), (27.9, 20.3))
R('VBAT', 'B', 0.4, (27.9, 20.3), (28.55, 20.95), (31.3, 20.95), (31.75, 20.5))   # dips under the charger, below its ground vias
R('VBAT', 'B', 0.5, (15.7, 20.3), (15.7, 21.9))                        # regulator input
V('VBAT', 15.7, 21.9)
R('VBAT', 'F', 0.5, 'C6.1', (15.7, 21.9), 'U6.1')
R('VBAT', 'B', 0.5, (18.95, 20.3), (18.95, 21.9))                      # EN = VIN, its own via (keeps the ground pin between them open)
V('VBAT', 18.95, 21.9)
R('VBAT', 'F', 0.4, (18.95, 21.9), 'U6.3')
R('GND', 'F', 0.3, 'U6.2', (17.35, 21.0))                              # regulator ground: via under its body
V('GND', 17.35, 21.0, 0.5, 0.3)
R('VBAT', 'B', 0.4, (21.75, 20.3), (21.75, 22.1))                      # battery divider, from below
V('VBAT', 21.75, 22.1)
R('VBAT', 'F', 0.3, (21.75, 22.1), 'R12.1')
V('VBAT', 31.75, 20.5)                                                # charger BAT
R('VBAT', 'F', 0.5, (31.75, 20.5), 'U4.3')
R('GND', 'F', 0.5, 'U4.2', (30.0, 20.3), (30.0, 19.5))                  # charger ground (its heat path): two vias under the body
V('GND', 30.0, 20.3, 0.5, 0.3)
V('GND', 30.0, 19.5, 0.5, 0.3)
R('VBAT_SENSE', 'F', 0.25, 'R12.2', 'R13.1', 'C16.1')

# ------------------------------------------------------------------ 3V3: regulator -> 22 uF over the top of the regulator; TP2 on the via
R('3V3', 'F', 0.4, 'U6.5', (16.4, 18.4), (20.15, 18.4), 'C8.1')
V('3V3', 19.0, 18.4)
R('3V3', 'B', 0.3, (19.0, 18.4), (16.0, 18.4), (14.025, 16.42), (14.025, 11.28), (12.54, 9.8), (11.66, 9.8))   # to the module's pin 3 side

# ------------------------------------------------------------------ VBUS: both pad pairs escape right inside the receptacle, back layer down to the charger
R('VBUS', 'F', 0.3, 'J1.A9', (30.4, 8.05), (32.3, 8.05))
R('VBUS', 'F', 0.3, 'J1.A4', (30.4, 12.15), (32.3, 12.15))
V('VBUS', 32.3, 8.05)
V('VBUS', 32.3, 12.15)
R('VBUS', 'B', 0.4, (32.3, 8.05), (32.3, 16.2))                         # right of the D- lane and the shell tab
V('VBUS', 32.3, 16.2)
R('VBUS', 'F', 0.4, (32.3, 16.2), 'C4.1', 'U4.4')

# ------------------------------------------------------------------ USB-C: each pad pair drops to B.Cu inside the receptacle and is joined there
for pad, net, x in (('B6', 'USB_DP', 30.45), ('A6', 'USB_DP', 30.45), ('A7', 'USB_DN', 31.15), ('B7', 'USB_DN', 31.15)):
    y = {'B6': 9.35, 'A6': 10.35, 'A7': 9.85, 'B7': 10.85}[pad]
    R(net, 'F', 0.2, 'J1.' + pad, (x, y))
    V(net, x, y, 0.5, 0.3)
R('USB_DP', 'B', 0.2, (30.45, 9.35), (30.45, 10.35))
R('USB_DN', 'B', 0.2, (31.15, 9.85), (31.15, 10.85))
# receptacle -> USBLC6 (right pins): D+ left of the shell tab, D- right of it
R('USB_DP', 'B', 0.2, (30.45, 10.35), (30.45, 11.7), (28.65, 13.5), (28.65, 14.3))
V('USB_DP', 28.65, 14.3, 0.5, 0.3)
R('USB_DP', 'F', 0.2, (28.65, 14.3), 'U7.3')
R('USB_DN', 'B', 0.2, (31.15, 10.85), (31.6, 11.3), (31.6, 16.0), (30.6, 17.0), (29.15, 17.0))
V('USB_DN', 29.15, 17.0, 0.5, 0.3)
R('USB_DN', 'F', 0.2, (29.15, 17.0), 'U7.1')
# USBLC6 (left pins) -> back layer right under the module's bottom pad row (the row of parts below keeps the back
# layer for its own vias) -> vias inside the pad ring at pins 17/18
R('USB_DP', 'F', 0.2, 'U7.4', (24.9, 14.65))
V('USB_DP', 24.9, 14.65, 0.5, 0.3)
R('USB_DP', 'B', 0.2, (24.9, 14.65), (24.9, 12.6), (20.6, 12.6), (20.0, 12.0))
R('USB_DN', 'F', 0.2, 'U7.6', (24.9, 16.8))
V('USB_DN', 24.9, 16.8, 0.5, 0.3)
R('USB_DN', 'B', 0.2, (24.9, 16.8), (24.3, 16.2), (24.3, 13.5), (24.0, 13.2), (19.8, 13.2), (19.2, 12.6), (19.2, 12.0))
# the USBLC6 joins pins 3/4 and 1/6 inside; KiCad needs copper for it: straight under the body
R('USB_DP', 'F', 0.2, 'U7.4', 'U7.3')
R('USB_DN', 'F', 0.2, 'U7.6', 'U7.1')
# its pin 5 on 3V3: on VBUS, the C6's D+ pull-up would back-feed VBUS through the steering diode while unplugged
R('3V3', 'F', 0.25, 'U7.5', (24.2, 15.85), (23.72, 15.37), 'R17.1')
V('USB_DP', 20.0, 12.0, 0.5, 0.3)
V('USB_DN', 19.2, 12.0, 0.5, 0.3)
R('USB_DP', 'F', 0.2, (20.0, 12.0), 'U1.18')
R('USB_DN', 'F', 0.2, (19.2, 12.0), 'U1.17')

# ------------------------------------------------------------------ accelerometer interrupt: via inside the pad ring at pin 16
V('ACC_INT1', 18.1, 12.1, 0.5, 0.3)
R('ACC_INT1', 'F', 0.2, (18.1, 12.1), 'U1.16')

# ------------------------------------------------------------------ accelerometer block (under the module's bottom-left pins)
R('ACC_INT1', 'B', 0.2, (18.1, 12.1), (18.1, 15.75), (19.35, 15.75), (19.35, 16.45))   # back layer, under the sensor; TP13 on the vertical
V('ACC_INT1', 19.35, 16.45, 0.5, 0.3)
R('ACC_INT1', 'F', 0.2, (19.35, 16.45), 'U3.5')
R('I2C_SDA', 'F', 0.2, 'U3.2', (18.6, 14.8), (18.4, 14.6), (18.4, 14.0), (18.7, 13.7), (22.8, 13.7), (23.2, 13.3), 'U1.22')
R('I2C_SDA', 'F', 0.2, 'R16.2', (22.2, 13.7))
R('I2C_SDA', 'F', 0.2, 'U1.22', (23.22, 11.85))
V('I2C_SDA', 23.22, 11.85, 0.5, 0.3)                                    # TP15 sits on it (back)
V('I2C_SCL', 25.434, 13.75, 0.5, 0.3)                                  # on the back-layer SCL run, next to pin 24
R('I2C_SCL', 'F', 0.2, (25.434, 13.75), 'U1.24')
R('I2C_SCL', 'F', 0.2, 'R17.2', (23.55, 13.93), (24.45, 13.93), 'U1.24')   # pull-up, straight to pin 24
R('3V3', 'F', 0.25, 'U3.3', (18.04, 15.3), (18.04, 16.9), (18.39, 17.25), 'C11.1')   # VDDIO, around the west side
R('GND', 'F', 0.2, 'U3.4', (18.85, 16.2), (18.57, 16.48), (18.57, 16.65))   # pad 4 to ground (datasheet connection table)
V('GND', 18.57, 16.65, 0.5, 0.3)
R('3V3', 'F', 0.3, 'C11.1', (19.12, 18.4))
R('GND', 'F', 0.25, 'C11.2', (20.15, 16.65))
V('GND', 20.15, 16.65, 0.5, 0.3)
R('3V3', 'F', 0.2, 'U3.7', (20.75, 16.19), (20.75, 17.08), 'C10.1')     # VDD: C10 below the sensor, its ground pad facing it
R('3V3', 'F', 0.3, 'C10.1', (21.3, 18.4), (19.9, 18.4))
R('3V3', 'F', 0.3, 'R16.1', 'R17.1')
R('3V3', 'F', 0.2, 'R16.1', (22.2, 16.8), 'C10.1')
R('GND', 'F', 0.25, 'U3.9', 'U3.8', (21.35, 15.4), 'C10.2')            # sensor ground + C10: one via between them
V('GND', 21.35, 15.4, 0.5, 0.3)

# battery sense: via inside the ring at pin 15, back layer down past the crystal's edge (a DC line), up to the divider
V('VBAT_SENSE', 17.3, 11.8, 0.5, 0.3)
R('VBAT_SENSE', 'F', 0.2, (17.3, 11.8), 'U1.15')
R('VBAT_SENSE', 'B', 0.2, (17.3, 11.8), (17.45, 11.95), (17.45, 17.0), (18.05, 17.6), (21.25, 17.6), (21.6, 17.95), (21.6, 18.95))
V('VBAT_SENSE', 21.6, 18.95, 0.5, 0.3)
R('VBAT_SENSE', 'F', 0.2, (21.6, 18.95), (21.6, 19.6), 'R12.2')

# ------------------------------------------------------------------ USB-C side strip: CC1, VBUS divider (sense to pin 25), charge status (pin 26)
R('CC1', 'F', 0.2, 'J1.A5', 'R1.1')                                   # CC pull-downs: vertical, beside their pads
R('CC2', 'F', 0.2, 'J1.B5', (28.4, 8.35), (28.14, 8.09), 'R2.1')
R('GND', 'F', 0.2, 'R1.2', (27.9, 9.75), 'R2.2')                      # their ground pads face each other; via to the west
R('GND', 'F', 0.2, (27.9, 9.75), (27.05, 9.75))
V('GND', 27.05, 9.75, 0.5, 0.3)
# VBUS divider beside H1, above the receptacle: VBUS from pad A9 up a lane left of the ground pad
R('VBUS', 'F', 0.2, 'J1.A9', (28.25, 7.4), (28.25, 6.2), (28.05, 6.0), 'R14.1')
R('VBUS_SENSE', 'F', 0.2, 'R14.2', 'R15.1')
R('GND', 'F', 0.2, 'R15.2', (27.2, 6.05), (26.8, 6.45))
V('GND', 26.8, 6.45, 0.5, 0.3)
# VBUS_SENSE to pin 25 on the back, under the module's right pad column
R('VBUS_SENSE', 'F', 0.2, 'R15.1', (27.2, 4.28), (26.98, 4.06))
V('VBUS_SENSE', 26.98, 4.06, 0.5, 0.3)
R('VBUS_SENSE', 'B', 0.2, (26.98, 4.06), (26.3, 4.74), (26.25, 4.79), (26.25, 12.9), (26.75, 13.4))
V('VBUS_SENSE', 26.75, 13.4, 0.5, 0.3)
R('VBUS_SENSE', 'F', 0.2, (26.75, 13.4), (26.75, 12.1), (26.6, 11.95), 'U1.25')
R('CHG_STAT', 'F', 0.2, 'U1.26', (26.75, 11.15), (26.95, 10.95))
V('CHG_STAT', 26.95, 10.95, 0.5, 0.3)
R('CHG_STAT', 'F', 0.2, 'R10.2', (23.0077, 17.9574))                  # pull-up: the done state is only a weak pull-down
R('3V3', 'F', 0.2, 'R10.1', (23.9, 16.15), (24.2, 15.85))
R('VBUS', 'B', 0.3, (32.3, 10.1), 'TP3.1')                              # test pad on the VBUS run, behind the receptacle

# ------------------------------------------------------------------ BTN: from the BOOT test pad's via, back layer along the top (clear of the
# antenna keepout), round H1 and down the right edge behind the USB-C, up to the button's upper pad
R('BTN', 'B', 0.2, (19.3286, 3.995), (19.3286, 2.75), (19.48, 2.6), (28.5, 2.6), (28.5, 4.15), (35.4, 4.15), (35.4, 17.8))
V('BTN', 35.4, 17.8, 0.5, 0.3)
R('BTN', 'F', 0.2, (35.4, 17.8), (34.2, 17.8), (33.9, 17.5), (33.5, 17.4))
R('GND', 'F', 0.3, 'SW1.2', (34.4, 18.6))                              # button ground: out the east side (its two pin-1 pads count as joined)
V('GND', 34.4, 18.6, 0.5, 0.3)
R('GND', 'F', 0.3, 'C5.2', (12.75, 11.55))                             # EN cap ground, onto TP12 on the back
V('GND', 12.75, 11.55, 0.5, 0.3)

# ------------------------------------------------------------------ module centre ground pads: vias into the back plane (ground return and heat)
for x, y in ((18.025, 5.325), (21.975, 5.325), (20.0, 7.3), (18.025, 9.275), (21.975, 9.275)):
    V('GND', x, y)
