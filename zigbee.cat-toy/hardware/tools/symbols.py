"""Custom symbols for the zigbee_toy project library.

Each symbol is described as a box with pins; build_symbol() turns the
description into a KiCad symbol S-expression (lib coordinates, Y up).
"""
from sexpr import Sym

PIN_LEN = 2.54


def _font(size=1.27):
    return [Sym('effects'), [Sym('font'), [Sym('size'), size, size]]]


def _prop(name, value, x, y, hide=False, justify=None):
    p = [Sym('property'), name, value, [Sym('at'), x, y, 0]]
    if hide:
        p.append([Sym('hide'), Sym('yes')])
    eff = _font()
    if justify:
        eff.append([Sym('justify'), Sym(justify)])
    p.append(eff)
    return p


def _pin(num, name, etype, x, y, angle, hidden=False):
    p = [Sym('pin'), Sym(etype), Sym('line'), [Sym('at'), x, y, angle],
         [Sym('length'), PIN_LEN]]
    if hidden:
        p.append([Sym('hide'), Sym('yes')])
    p += [[Sym('name'), name, _font()], [Sym('number'), str(num), _font()]]
    return p


def build_symbol(name, ref, value, footprint, description, w, left=(), right=(),
                 top=(), bottom=(), datasheet='', pitch=2.54):
    """left/right/top/bottom: lists of (num, name, type) or None for a gap.

    A tuple (num, name, type, 'stack') is placed at the same position as the
    previous pin (stacked pins, e.g. multiple GND pads).
    """
    def count(side):
        return sum(1 for p in side if p is None or len(p) < 4)

    rows = max(count(left), count(right))
    cols = max(count(top), count(bottom))
    h = max(rows + 1, 3) * pitch
    w = max(w, (cols + 1) * pitch)
    hw, hh = w / 2, h / 2
    hw = round(hw / 1.27) * 1.27
    hh = round(hh / 1.27) * 1.27

    pins = []

    def place(side, coords):
        i = 0
        last = None
        for p in side:
            if p is not None and len(p) == 4:
                x, y, a = last
                pins.append(_pin(p[0], p[1], p[2], x, y, a, hidden=True))
                continue
            pos = coords(i)
            i += 1
            if p is None:
                continue
            last = pos
            pins.append(_pin(p[0], p[1], p[2], *pos))

    y0 = hh - pitch
    place(left, lambda i: (-hw - PIN_LEN, y0 - i * pitch, 0))
    place(right, lambda i: (hw + PIN_LEN, y0 - i * pitch, 180))
    x0 = -((count(top) - 1) * pitch) / 2
    place(top, lambda i: (x0 + i * pitch, hh + PIN_LEN, 270))
    x0 = -((count(bottom) - 1) * pitch) / 2
    place(bottom, lambda i: (x0 + i * pitch, -hh - PIN_LEN, 90))

    top_y = hh + (PIN_LEN + 1.27 if top else 1.27)
    bot_y = -hh - (PIN_LEN + 1.27 if bottom else 1.27)
    sym = [Sym('symbol'), name,
           [Sym('pin_names'), [Sym('offset'), 1.016]],
           [Sym('exclude_from_sim'), Sym('no')],
           [Sym('in_bom'), Sym('yes')],
           [Sym('on_board'), Sym('yes')],
           _prop('Reference', ref, -hw, top_y, justify='left'),
           (_prop('Value', value, -2.54, bot_y, justify='right') if bottom else
            _prop('Value', value, -hw, bot_y, justify='left')),
           _prop('Footprint', footprint, 0, bot_y - 2.54, hide=True),
           _prop('Datasheet', datasheet, 0, 0, hide=True),
           _prop('Description', description, 0, 0, hide=True),
           [Sym('symbol'), f'{name}_0_1',
            [Sym('rectangle'), [Sym('start'), -hw, hh], [Sym('end'), hw, -hh],
             [Sym('stroke'), [Sym('width'), 0.254], [Sym('type'), Sym('default')]],
             [Sym('fill'), [Sym('type'), Sym('background')]]]],
           [Sym('symbol'), f'{name}_1_1', *pins],
           [Sym('embedded_fonts'), Sym('no')]]
    return sym


def custom_symbols():
    s = {}
    s['E73-2G4M08S1C'] = build_symbol(
        'E73-2G4M08S1C', 'U', 'E73-2G4M08S1C', 'zigbee_toy:Ebyte_E73-2G4M08S1C',
        'Ebyte nRF52840 module, ceramic antenna, 13x18 mm, 43 castellated pads',
        w=27.94,
        left=[(19, 'VCC(VDD)', 'power_in'), (23, 'VDDH', 'power_in'),
              (25, 'DCCH', 'passive'), (27, 'VBUS', 'power_in'), None,
              (31, 'D+', 'bidirectional'), (29, 'D-', 'bidirectional'), None,
              (11, 'XL1/P0.00', 'passive'), (13, 'XL2/P0.01', 'passive'), None,
              (26, 'P0.18/~{RESET}', 'bidirectional'), (37, 'SWDIO', 'bidirectional'),
              (39, 'SWDCLK', 'input'), None,
              (41, 'P0.09/NFC1', 'passive'), (43, 'P0.10/NFC2', 'passive')],
        right=[(7, 'P0.02/AIN0', 'bidirectional'), (3, 'P0.03/AIN1', 'bidirectional'),
               (18, 'P0.04/AIN2', 'bidirectional'), (15, 'P0.05/AIN3', 'bidirectional'),
               (4, 'P0.28/AIN4', 'bidirectional'), (8, 'P0.29/AIN5', 'bidirectional'),
               (10, 'P0.30/AIN6', 'bidirectional'), (9, 'P0.31/AIN7', 'bidirectional'),
               (14, 'P0.06', 'bidirectional'), (22, 'P0.07', 'bidirectional'),
               (16, 'P0.08', 'bidirectional'), (20, 'P0.12', 'bidirectional'),
               (33, 'P0.13', 'bidirectional'), (28, 'P0.15', 'bidirectional'),
               (30, 'P0.17', 'bidirectional'), (32, 'P0.20', 'bidirectional'),
               (34, 'P0.22', 'bidirectional'), (35, 'P0.24', 'bidirectional'),
               (12, 'P0.26', 'bidirectional'), (36, 'P1.00', 'bidirectional'),
               (38, 'P1.02', 'bidirectional'), (40, 'P1.04', 'bidirectional'),
               (42, 'P1.06', 'bidirectional'), (17, 'P1.09', 'bidirectional'),
               (2, 'P1.10', 'bidirectional'), (1, 'P1.11', 'bidirectional'),
               (6, 'P1.13', 'bidirectional')],
        bottom=[(5, 'GND', 'power_in'), (21, 'GND', 'passive', 'stack'),
                (24, 'GND', 'passive', 'stack')],
        datasheet='https://www.cdebyte.com/products/E73-2G4M08S1C')

    s['DRV8213DSG'] = build_symbol(
        'DRV8213DSG', 'U', 'DRV8213DSGR',
        'Package_SON:Texas_DSG0008A_WSON-8-1EP_2x2mm_P0.5mm_EP0.9x1.6mm',
        '1.65-11 V 4 A brushed DC H-bridge, integrated current sense (IPROPI) and regulation',
        w=20.32,
        left=[(1, 'VM', 'power_in'), None, (6, 'IN1', 'input'), (5, 'IN2', 'input'),
              (7, 'GAINSEL', 'input'), None],
        right=[(2, 'OUT1', 'output'), (3, 'OUT2', 'output'), None, None,
               (8, 'IPROPI', 'output'), None],
        bottom=[(4, 'GND', 'power_in'), (9, 'EP', 'passive', 'stack')],
        datasheet='https://www.ti.com/lit/gpn/drv8213')

    s['XB5353A'] = build_symbol(
        'XB5353A', 'U', 'XB5353A', 'Package_TO_SOT_SMD:SOT-23-5',
        '1-cell Li-ion/LiPo protection IC with integrated 54 mOhm FETs (low side)',
        w=15.24,
        left=[(3, 'VDD', 'power_in'), (1, 'VT', 'passive'), None],
        right=[(4, 'VM', 'passive'), (5, 'VM', 'passive', 'stack')],
        bottom=[(2, 'GND', 'power_in')],
        datasheet='https://jlcpcb.com/partdetail/C80628')

    s['TP4054'] = build_symbol(
        'TP4054', 'U', 'TP4054-42-SOT25R', 'Package_TO_SOT_SMD:SOT-23-5',
        '1-cell Li-ion linear charger, 4.2 V, I = 1000 V / R_PROG',
        w=17.78,
        left=[(4, 'VCC', 'power_in'), (5, 'PROG', 'passive'), None],
        right=[(3, 'BAT', 'passive'), (1, '~{CHRG}', 'open_collector'), None],
        bottom=[(2, 'GND', 'power_in')])

    s['LIS2DH12'] = build_symbol(
        'LIS2DH12', 'U', 'LIS2DH12TR', 'Package_LGA:LGA-12_2x2mm_P0.5mm',
        'ST 3-axis accelerometer, I2C/SPI, ultra low power',
        w=17.78,
        left=[(9, 'VDD', 'power_in'), (10, 'VDD_IO', 'power_in'), None,
              (2, 'CS', 'input'), (3, 'SDO/SA0', 'input'), (5, 'Res', 'passive'), None],
        right=[(4, 'SDA', 'bidirectional'), (1, 'SCL', 'input'), None,
               (12, 'INT1', 'output'), (11, 'INT2', 'output')],
        bottom=[(6, 'GND', 'power_in'), (7, 'GND', 'passive', 'stack'),
                (8, 'GND', 'passive', 'stack')],
        datasheet='https://www.st.com/resource/en/datasheet/lis2dh12.pdf')
    return s
