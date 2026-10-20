"""Hand-routed copper (rev A, floorplan v8: antenna overhang, everything below the module 2.8 mm up). Locked before autorouting.

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
R('CELL_P', 'F', 0.5, 'J3.1', (6.0, 17.2), (8.2, 15.0), 'Q1.3')       # main current to the FET drain
R('CELL_P', 'F', 0.4, (6.0, 17.2), 'C15.1')
R('CELL_P', 'F', 0.3, 'C15.1', (10.65, 17.43), 'U5.3')                # XB5353A VDD sense
R('CELL_N', 'F', 0.5, 'J3.2', 'C15.2')
R('CELL_N', 'F', 0.4, 'C15.2', (9.6, 19.8), (11.6, 19.8), 'U5.2')     # between U5's pad rows
R('GND', 'F', 0.6, 'U5.4', (10.65, 22.1), (12.55, 22.1), 'U5.5')      # VM = board GND
V('GND', 10.65, 22.3, 0.8, 0.4)
V('GND', 12.55, 22.3, 0.8, 0.4)
R('GND', 'F', 0.4, 'Q1.1', (13.9, 16.4))                              # FET gate
V('GND', 13.9, 16.4)

# ------------------------------------------------------------------ VBAT trunk
R('VBAT', 'F', 0.6, 'Q1.2', (15.85, 14.05), (15.85, 21.7), (28.45, 21.7))
R('VBAT', 'F', 0.6, 'C1.1', (15.85, 17.65))                           # bulk 22u
R('VBAT', 'F', 0.5, (24.0, 21.7), (24.0, 24.85), 'U4.3')              # charger BAT
R('VBAT', 'F', 0.4, (28.45, 21.7), (28.45, 16.95), (27.18, 16.95), 'U1.23')   # VDDH
R('VBAT', 'F', 0.4, (27.6, 16.95), 'C7.1')
R('VBAT', 'F', 0.4, 'C6.1', (28.45, 19.77))
# B.Cu hop up the left column to the motor driver, tapping the speaker supply on the way
V('VBAT', 12.74, 13.1, 0.8, 0.4)
R('VBAT', 'F', 0.6, 'Q1.2', (12.74, 13.1))
R('VBAT', 'B', 0.8, (12.74, 13.1), (8.6, 8.96), (8.6, 3.6), (9.7, 2.5))
V('VBAT', 9.7, 2.5, 0.8, 0.4)
R('VBAT', 'F', 0.6, (9.7, 2.5), 'C2.1')
R('VBAT', 'F', 0.5, (9.7, 2.5), 'C3.1')
R('VBAT', 'F', 0.3, (9.7, 2.5), 'U2.1')
V('VBAT', 8.6, 6.8)
R('VBAT', 'F', 0.5, (8.6, 6.8), 'R11.1')

# ------------------------------------------------------------------ motor driver
R('MOT_A', 'F', 0.3, 'U2.2', (9.3, 3.55), (9.0, 3.25))
R('MOT_A', 'F', 0.6, (9.0, 3.25), (8.55, 2.8), 'J4.1')
R('MOT_B', 'F', 0.3, 'U2.3', (9.3, 4.05), (9.0, 4.35))
R('MOT_B', 'F', 0.6, (9.0, 4.35), (8.55, 4.8), 'J4.2')
R('GND', 'F', 0.25, 'U2.4', (10.6, 4.55), 'U2.9')
V('GND', 11.0, 3.8)                                                   # thermal pad via
R('GND', 'F', 0.4, 'C3.2', (13.1, 1.5))
R('GND', 'F', 0.3, 'R6.2', (13.8, 2.2), (13.1, 1.5))
V('GND', 13.1, 1.5)
R('GND', 'F', 0.4, 'C2.2', (6.2, 1.4))
V('GND', 6.2, 1.4)

# ------------------------------------------------------------------ speaker
R('SPK_P', 'F', 0.5, 'J5.1', (6.0, 10.0), (7.55, 8.45), 'D1.1')
R('SPK_P', 'F', 0.5, 'D1.1', (11.51, 7.54), 'R11.2')
R('SPK_N', 'F', 0.5, 'J5.2', (6.0, 12.0), (6.25, 11.75), 'D1.2')
R('SPK_N', 'F', 0.5, 'D1.2', 'Q2.3')
R('GND', 'F', 0.4, 'Q2.2', (12.2, 9.6))
V('GND', 12.2, 9.6)
R('GND', 'F', 0.3, 'R8.2', (12.7, 6.78))
V('GND', 12.7, 6.78)
V('GND', 12.68, 7.9, 0.55, 0.3)           # R8's pour pocket is fenced on B.Cu by the GAINSEL/SPK_PWM loop: this drop is outside it

# ------------------------------------------------------------------ crystal
R('XL1', 'F', 0.2, 'U1.11', (19.55, 16.5), (18.95, 17.1), 'Y1.1')
R('XL1', 'F', 0.2, (18.95, 19.77), 'C12.1')
R('XL2', 'F', 0.2, 'U1.13', (20.82, 16.5), (21.45, 17.13), 'Y1.2')
R('XL2', 'F', 0.2, (21.45, 18.22), 'C13.1')
R('GND', 'F', 0.3, 'C12.2', (17.4, 17.1))
V('GND', 17.4, 17.1)
R('GND', 'F', 0.3, 'C13.2', (23.0, 20.75))
V('GND', 23.0, 20.75)

# ------------------------------------------------------------------ module bottom pads: PWM escape, VDD, GND
R('MOT_PWM', 'F', 0.25, 'U1.15', (22.09, 16.9))
V('MOT_PWM', 22.09, 16.9)
R('VDD', 'F', 0.3, 'U1.19', 'C9.1', 'C8.1')
R('GND', 'F', 0.3, 'U1.21', (25.91, 16.95))
V('GND', 25.91, 16.95)
for x in (24.6, 26.1, 27.6):                                          # cap returns
    R('GND', 'F', 0.3, (x, 19.77), (x, 20.75))
    V('GND', x, 20.75)
R('GND', 'F', 0.3, 'C6.2', (30.3, 17.6))
V('GND', 30.3, 17.6)

# ------------------------------------------------------------------ module inner bottom row: escape inward to vias
for pin, net in (('12', 'ACC_INT1'), ('14', 'I2C_SDA'), ('16', 'I2C_SCL'), ('18', 'ACC_INT2'),
                 ('20', 'LED_STATUS'), ('22', 'BTN'), ('24', 'GND')):
    x = {'12': 20.19, '14': 21.46, '16': 22.73, '18': 24.0, '20': 25.27, '22': 26.54, '24': 27.81}[pin]
    R(net, 'F', 0.25, 'U1.' + pin, (x, 12.0))
    V(net, x, 12.0)

# ------------------------------------------------------------------ right edge grounds, charger GND, accel GND
R('GND', 'F', 0.3, 'J1.A12', (34.17, 9.6))                            # USB GND pins -> shell
R('GND', 'F', 0.3, 'J1.A1', (34.17, 17.0))
R('GND', 'F', 0.3, 'R1.2', (31.3, 19.9))                              # CC1 pull-down
V('GND', 31.3, 19.9)
R('GND', 'F', 0.3, 'R2.2', (36.57, 4.55), 'D3.1')                    # CC2 pull-down + status LED cathode
V('GND', 36.57, 4.55)
R('GND', 'F', 0.3, 'U4.2', (26.4, 23.9))                              # TP4054 GND under its body
V('GND', 26.4, 23.9)
R('GND', 'F', 0.25, 'U3.6', (8.15, 25.3))                             # accelerometer GND pads
V('GND', 8.15, 25.3)

# ------------------------------------------------------------------ VBUS (hand-routed: the USB pad row is too dense for 45-degree autorouting)
# both USB VBUS pads -> vias just past the pad row -> B.Cu down the right side, west under the
# charger, up to the module's VBUS pin; taps for the charge LED, TP4054 VCC and its cap.
R('VBUS', 'F', 0.25, 'J1.A9', (34.3, 11.12), (35.8, 11.12))             # clear of the USB locating peg, and of CC2
V('VBUS', 35.8, 11.12)
R('VBUS', 'F', 0.4, 'J1.A4', (34.4, 15.3), (35.8, 15.3))
V('VBUS', 35.8, 15.3)
R('VBUS', 'B', 0.4, (35.8, 11.12), (35.8, 23.6), (29.15, 23.6), (29.15, 11.8))
R('VBUS', 'B', 0.4, (29.15, 23.6), (28.4, 24.35))
V('VBUS', 29.15, 11.8)
R('VBUS', 'F', 0.3, 'U1.27', (29.15, 11.8))
V('VBUS', 28.4, 24.35)
R('VBUS', 'F', 0.4, (28.4, 24.35), 'U4.4')
R('VBUS', 'F', 0.4, (28.4, 24.35), 'C4.1')
R('VBUS', 'B', 0.4, (35.8, 23.6), (33.2, 23.6))                       # charge-LED feed
V('VBUS', 33.2, 23.6)
R('VBUS', 'F', 0.3, (33.2, 23.6), 'R4.1')

# ------------------------------------------------------------------ CC2 (hand-routed: B5 is boxed in by D+/D- on the left, VBUS above)
# out of the pad row to the right, under the receptacle on B.Cu, round its shell leg, up to R2
R('CC2', 'F', 0.2, 'J1.B5', (34.35, 11.55), (34.65, 11.85))
V('CC2', 34.65, 11.85)
R('CC2', 'B', 0.2, (34.65, 11.85), (32.8, 10.0), (32.8, 8.05), (35.79, 8.05))
V('CC2', 35.79, 8.05)
R('CC2', 'F', 0.2, (35.79, 8.05), 'R2.1')

# ------------------------------------------------------------------ debug UART: module inner right row escapes inward to vias
R('DBG_TX', 'F', 0.25, 'U1.28', (27.3, 10.69))
V('DBG_TX', 27.3, 10.69)
R('DBG_RX', 'F', 0.25, 'U1.30', (27.3, 9.43))
V('DBG_RX', 27.3, 9.43)
