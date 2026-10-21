"""Silkscreen spec, rev B (board mm, origin top-left, Y down).

Front: reference designators for the parts you'd look for while debugging (ICs, FETs,
diodes, LEDs, crystal, connectors, button), auto-placed clear of pads, bodies and each other.
Passive refs move to F.Fab (assembly drawing only).  Front legends are auto-placed near
their anchor and dropped with a warning if nothing fits.
Back: the wiring legend (connector names, polarity, LED/button/USB) and the title,
at fixed positions - the back is nearly empty, so it can carry the readable text.
"""

# J3-J5 get their function name instead (MOT/SPK/BAT below): that's what you look for when plugging in
FRONT_REFS = ['U1', 'U2', 'U3', 'U4', 'U5', 'U6', 'Q1', 'Q2', 'D1', 'D2', 'Y1',
              'J1', 'SW1']

# fixed spots for refs the auto-placer gets wrong: ref -> (x, y, rot)
REF_AT = {}

# front legends: (text, anchor 'REF.PAD' to auto-place near, or (x, y, rot) fixed, size)
FRONT_LABELS = [
    ('MOT', (7.6, 3.8), 0.8),
    ('SPK', (7.6, 11.0), 0.8),
    ('BAT', (7.6, 18.2), 0.8),
    ('+', 'J3.1', 0.8),
]

# back legends: (text, x, y, rot, size); x/y are the text centre in board coordinates,
# the text is mirrored so it reads correctly from the back.
BACK_TEXT = [
    # title, behind the module
    ('zigbee toy', 24.6, 6.55, 0, 0.9),
    ('rev B  2026-10', 24.6, 8.1, 0, 0.8),
    # connectors: names along the clean strip right of the pins, polarity left of them
    ('MOTOR', 6.6, 3.8, 270, 0.9),
    ('SPEAKER', 6.6, 11.0, 270, 0.9),
    ('BATTERY', 6.6, 18.2, 270, 0.9),
    ('+', 2.6, 10.0, 0, 1.0), ('-', 2.6, 12.0, 0, 1.0),
    ('+', 2.6, 17.2, 0, 1.0), ('-', 2.6, 19.2, 0, 1.0),
    # right edge, behind the parts that face the shell
    ('USB', 37.4, 12.9, 270, 0.8),
    ('STAT', 36.3, 6.3, 0, 0.8),
    ('BTN', 38.0, 21.2, 270, 0.8),
]

# test pads: label = the TP's value, on this side of the pad ('right' or 'below')
TP_LABEL_SIZE = 0.8
TP_LABEL_SIDE = {f'TP{n}': 'right' for n in (1, 2, 3, 4, 5, 6, 13, 15, 16, 18, 20)}
TP_LABEL_SIDE.update({f'TP{n}': 'above' for n in (7, 8, 9)})       # motor block, top row
TP_LABEL_SIDE.update({f'TP{n}': 'below' for n in (11, 12, 21)})
TP_LABEL_SIDE['TP10'] = 'right'
TP_LABEL_SIDE['TP17'] = 'left'                                     # BOOT
TP_LABEL_SIDE['TP22'] = 'below'                                    # UART GND
TP_LABEL_SIDE['TP16'] = 'left'                                     # SCL: right is the nCHG pad
TP_LABEL_SIDE['TP19'] = 'below'                                    # TX: right is the mounting hole, left the RX pad
TP_LABEL_SIDE['TP20'] = 'left'                                     # RX: right is the TX pad
CHAR_W = 0.94          # stroke-font advance per character, x text size (for keepout estimates)


def tp_label_box(cx, cy, text, side, size=TP_LABEL_SIZE):
    """Where a test pad's label goes (x1, y1, x2, y2), next to its 1.52 mm silk ring (ink incl. stroke)."""
    w, h = len(text) * CHAR_W * size + 0.15, size + 0.15
    g = 0.76 + 0.25
    if side == 'right':
        return (cx + g, cy - h / 2, cx + g + w, cy + h / 2)
    if side == 'left':
        return (cx - g - w, cy - h / 2, cx - g, cy + h / 2)
    if side == 'above':
        return (cx - w / 2, cy - g - h, cx + w / 2, cy - g)
    return (cx - w / 2, cy + g, cx + w / 2, cy + g + h)


def text_box(x, y, rot, text, size):
    w, h = len(text) * CHAR_W * size + 0.15, size + 0.15
    if rot in (90, 270):
        w, h = h, w
    return (x - w / 2, y - h / 2, x + w / 2, y + h / 2)

# small paw print on the back, behind the battery protection: (cx, cy, size)
PAW = (11.0, 17.0, 2.4)
