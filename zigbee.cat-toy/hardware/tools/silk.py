"""Silkscreen spec, rev A (board mm, origin top-left, Y down).

Front: reference designators for the parts you'd look for while debugging (ICs, FETs,
diodes, LEDs, crystal, connectors, button), auto-placed clear of pads, bodies and each other.
Passive refs move to F.Fab (assembly drawing only).  Front legends are auto-placed near
their anchor and dropped with a warning if nothing fits.
Back: the wiring legend (connector names, polarity, SWD pinout, LEDs/button) and the title,
at fixed positions - the back is nearly empty, so it can carry the readable text.
"""

# J3-J5 get their function name instead (MOT/SPK/BAT below): that's what you look for when plugging in
FRONT_REFS = ['U1', 'U2', 'U3', 'U4', 'U5', 'Q1', 'Q2', 'D1', 'D2', 'D3', 'Y1',
              'J1', 'J2', 'SW1']

# fixed spots for refs the auto-placer gets wrong: ref -> (x, y, rot)
REF_AT = {
    'SW1': (35.8, 18.7, 0),      # between the USB-C body and the button's pads
}

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
    # title, behind the crystal / charger (the space under the module holds the test pads)
    ('zigbee toy', 24.0, 18.1, 0, 0.9),
    ('rev A  2026-10', 24.0, 19.65, 0, 0.8),
    # connectors: names along the clean strip right of the pins, polarity left of them
    ('MOTOR', 7.6, 4.1, 270, 0.9),
    ('SPEAKER', 7.6, 11.0, 270, 0.9),
    ('BATTERY', 7.6, 18.2, 270, 0.9),
    ('+', 2.6, 10.0, 0, 1.0), ('-', 2.6, 12.0, 0, 1.0),
    ('+', 2.6, 17.2, 0, 1.0), ('-', 2.6, 19.2, 0, 1.0),
    # SWD header J2 (pin 1 at the top)
    ('VDD', 31.4, 1.2, 0, 0.8), ('GND', 31.4, 2.47, 0, 0.8), ('RST', 31.4, 3.74, 0, 0.8),
    ('CLK', 31.4, 5.01, 0, 0.8), ('DIO', 31.4, 6.28, 0, 0.8),
    # right edge, behind the parts that face the shell
    ('USB', 37.4, 13.3, 270, 0.8),
    ('STAT', 37.5, 6.6, 270, 0.8),
    ('CHG', 39.1, 6.6, 270, 0.8),
    ('BTN', 38.0, 21.6, 270, 0.8),
]

# test pads: label = the TP's value, on this side of the pad ('right' or 'below')
TP_LABEL_SIZE = 0.8
TP_LABEL_SIDE = {f'TP{n}': 'right' for n in range(1, 7)}        # bottom-edge block
TP_LABEL_SIDE.update({f'TP{n}': 'below' for n in range(7, 21)})  # grid under the module
CHAR_W = 0.94          # stroke-font advance per character, x text size (for keepout estimates)


def tp_label_box(cx, cy, text, side, size=TP_LABEL_SIZE):
    """Where a test pad's label goes (x1, y1, x2, y2), next to its 1.52 mm silk ring (ink incl. stroke)."""
    w, h = len(text) * CHAR_W * size + 0.15, size + 0.15
    if side == 'right':
        x1 = cx + 0.76 + 0.25
        return (x1, cy - h / 2, x1 + w, cy + h / 2)
    y1 = cy + 0.76 + 0.25
    return (cx - w / 2, y1, cx + w / 2, y1 + h)


def text_box(x, y, rot, text, size):
    w, h = len(text) * CHAR_W * size + 0.15, size + 0.15
    if rot in (90, 270):
        w, h = h, w
    return (x - w / 2, y - h / 2, x + w / 2, y + h / 2)

# small paw print on the back, right of the title, clear of the USB shell legs: (cx, cy, size)
PAW = (31.9, 15.4, 2.4)
