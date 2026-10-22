"""Silkscreen spec, rev B (board mm, origin top-left, Y down).

Front: reference designators for the parts you'd look for while debugging (ICs, FETs,
diodes, LEDs, crystal, connectors, button), auto-placed clear of pads, bodies and each other,
all the same size and all horizontal.
Passive refs move to F.Fab (assembly drawing only).  Front legends are auto-placed near
their anchor and dropped with a warning if nothing fits.
Back: the wiring legend (connector names, polarity, LED/button/USB) and the title,
at fixed positions - the back is nearly empty, so it can carry the readable text.
"""

# J3 gets its function name instead (BAT below); the motor and speaker JST-SH have no room on the front, their names are on the back
FRONT_REFS = ['U1', 'U2', 'U3', 'U4', 'U5', 'U6', 'U7', 'Q1', 'Q2', 'D1', 'D2', 'Y1',
              'J1', 'SW1']

# fixed spots for refs the auto-placer gets wrong: ref -> (x, y, rot)
REF_AT = {'U1': (13.59, 14.7, 90)}   # below the module's corner, standing: C1 to the left, the crystal to the right

# front legends: (text, anchor 'REF.PAD' to auto-place near, or (x, y, rot) fixed, size)
FRONT_LABELS = [
    ('BAT', 'J3.2', 0.8),
    ('+', 'J3.1', 0.8),
]

# back legends: (text, x, y, rot, size); x/y are the text centre in board coordinates,
# the text is mirrored so it reads correctly from the back.
BACK_TEXT = [
    # title + paw in the strip a MINI-1's antenna keepout leaves bare (no copper there, silk is fine)
    ('zigbee toy', 19.0, 1.15, 0, 0.9),
    # revision in the bottom-left corner, under the battery legend
    ('rev B', 2.9, 20.3, 0, 0.8),
    ('2026-10', 3.6, 21.6, 0, 0.8),
    # connectors: names along the left edge (the SH plugs are SMD, so the back is clear), battery polarity beside its pins
    ('MOTOR', 2.3, 4.45, 270, 0.8),
    ('SPEAKER', 2.3, 9.65, 270, 0.8),
    ('BATTERY', 2.1, 16.0, 270, 0.8),
    ('+', 6.6, 14.74, 0, 1.0), ('-', 6.6, 16.74, 0, 1.0),
    # right edge, behind the parts that face the shell
    ('USB', 35.15, 10.1, 270, 0.8),
    ('STAT', 33.9, 4.45, 0, 0.8),
    ('BTN', 35.15, 18.6, 270, 0.8),
]

# test pads: label = the TP's value, on this side of the pad ('right', 'left', 'above' or 'below')
TP_LABEL_SIZE = 0.8
TP_LABEL_SIDE = {f'TP{n}': 'right' for n in (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20, 21, 22)}
TP_LABEL_SIDE['TP15'] = 'below'                                    # SDA: TP16 to the right, a via to the left
TP_LABEL_SIDE.update({'TP1': 'above', 'TP19': 'above', 'TP20': 'above', 'TP3': 'left', 'TP5': 'left', 'TP22': 'left'})   # clear of the neighbouring pads and the edge
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

# small paw print on the back, beside the title (reads left of it from the back): (cx, cy, size)
PAW = (25.3, 1.2, 2.0)
