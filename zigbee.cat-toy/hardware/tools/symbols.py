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
    gpio = 'bidirectional'
    s['ESP32-C6-MINI-1U'] = build_symbol(
        'ESP32-C6-MINI-1U', 'U', 'ESP32-C6-MINI-1U-H4', 'zigbee_toy:ESP32-C6-MINI-1U',
        'Espressif ESP32-C6 module (Wi-Fi 6, BLE 5, 802.15.4), 4 MB flash, u.FL antenna, 13.2x12.5 mm',
        w=25.4,
        left=[(3, '3V3', 'power_in'), (8, 'EN', 'input'), None,
              (12, 'IO0/XTAL_32K_P', gpio), (13, 'IO1/XTAL_32K_N', gpio), None,
              (17, 'IO12/USB_D-', gpio), (18, 'IO13/USB_D+', gpio), None,
              (30, 'RXD0/IO17', gpio), (31, 'TXD0/IO16', gpio), None,
              (4, 'NC', 'no_connect'), (7, 'NC', 'no_connect', 'stack'), (21, 'NC', 'no_connect', 'stack'),
              (32, 'NC', 'no_connect', 'stack'), (33, 'NC', 'no_connect', 'stack'),
              (34, 'NC', 'no_connect', 'stack'), (35, 'NC', 'no_connect', 'stack')],
        right=[(5, 'IO2/ADC1_2', gpio), (6, 'IO3/ADC1_3', gpio), (9, 'IO4/ADC1_4', gpio),
               (10, 'IO5/ADC1_5', gpio), (15, 'IO6/ADC1_6', gpio), (16, 'IO7', gpio),
               (22, 'IO8', gpio), (23, 'IO9/BOOT', gpio), (19, 'IO14', gpio), (20, 'IO15', gpio),
               (24, 'IO18', gpio), (25, 'IO19', gpio), (26, 'IO20', gpio), (27, 'IO21', gpio),
               (28, 'IO22', gpio), (29, 'IO23', gpio)],
        bottom=[(1, 'GND', 'power_in')] + [(n, 'GND', 'passive', 'stack')
                                           for n in [2, 11, 14] + list(range(36, 54))],
        datasheet='https://www.espressif.com/sites/default/files/documentation/esp32-c6-mini-1_mini-1u_datasheet_en.pdf')

    s['RT9080'] = build_symbol(
        'RT9080', 'U', 'RT9080-33GJ5', 'Package_TO_SOT_SMD:TSOT-23-5',
        '600 mA LDO, 2 uA quiescent, fixed 3.3 V',
        w=12.7,
        left=[(1, 'VIN', 'power_in'), (3, 'EN', 'input')],
        right=[(5, 'VOUT', 'power_out'), (4, 'NC', 'no_connect')],
        bottom=[(2, 'GND', 'power_in')],
        datasheet='https://www.richtek.com/assets/product_file/RT9080/DS9080-04.pdf')

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

    s['LIS2DW12TR-MS'] = build_symbol(
        'LIS2DW12TR-MS', 'U', 'LIS2DW12TR-MS', 'Package_LGA:LGA-12_2x2mm_P0.5mm',
        'MSKSEMI 3-axis accelerometer, I2C/SPI. NOT pin- or register-compatible with ST LIS2DW12 '
        '(WHO_AM_I 0x11, LIS2DH-style registers)',
        w=17.78,
        left=[(7, 'VDD', 'power_in'), (3, 'VDDIO', 'power_in'), None,
              (10, 'CS', 'input'), (1, 'SDO/SA0', 'input'), (4, 'NC', 'passive'), (11, 'NC', 'passive')],
        right=[(2, 'SDA', 'bidirectional'), (12, 'SCL', 'input'), None,
               (5, 'INT1', 'output'), (6, 'INT2', 'output')],
        bottom=[(9, 'GND', 'power_in'), (8, 'GNDIO', 'passive', 'stack')],
        datasheet='https://jlcpcb.com/partdetail/C50385411')
    return s
