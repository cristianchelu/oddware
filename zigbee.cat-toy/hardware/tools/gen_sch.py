#!/usr/bin/env python3
"""Generate the zigbee_toy controller schematic (KiCad 9/10 format).

Every pin gets a short wire stub ending in a net label (or a GND symbol),
so connectivity is defined purely by the NETS given to place().
Run:  python3 gen_sch.py   (writes ../zigbee_toy/zigbee_toy.kicad_sch + project lib)
"""
import math
import os
import sys
import uuid as _uuid

sys.path.insert(0, os.path.dirname(__file__))
from sexpr import Sym, parse, dump, find, findall  # noqa: E402
from symbols import custom_symbols  # noqa: E402

KLIB = os.environ.get(
    'KICAD_SYMBOL_DIR',
    '/var/lib/flatpak/runtime/org.kicad.kicad.Library/x86_64/stable/'
    '5a7d69b241b1417e15f791cb32499119ec931accda6a18544b2f9969d60da867/files/symbols')
HERE = os.path.dirname(os.path.abspath(__file__))
PROJ_DIR = os.path.join(HERE, '..', 'zigbee_toy')
PROJECT = 'zigbee_toy'
STUB = 2.54

_ns = _uuid.UUID('6f1c2a7e-0d6b-4d0c-9a51-2b7f3c1e9d00')


def uid(*parts):
    """Deterministic UUIDs keep diffs between regenerations small."""
    return str(_uuid.uuid5(_ns, '/'.join(str(p) for p in parts)))


ROOT_UUID = uid('root')


def E(*items):
    return list(items)


def S(x):
    return Sym(x)


def font(size=1.27, justify=None, hide=False):
    eff = [S('effects'), [S('font'), [S('size'), size, size]]]
    if justify:
        eff.append([S('justify'), *[S(j) for j in justify.split()]])
    return eff


def snap(v, g=1.27):
    return round(round(v / g) * g, 4)


# ---------------------------------------------------------------- libraries
_lib_cache = {}
CUSTOM = custom_symbols()


def _load_lib(lib):
    if lib not in _lib_cache:
        t = parse(open(os.path.join(KLIB, lib + '.kicad_sym')).read())
        _lib_cache[lib] = {s[1]: s for s in findall(t, 'symbol')}
    return _lib_cache[lib]


def lib_symbol(lib_id):
    """Return a flattened symbol named lib_id, suitable for lib_symbols."""
    lib, name = lib_id.split(':')
    if lib == PROJECT:
        src = CUSTOM[name]
        parent = None
    else:
        syms = _load_lib(lib)
        src = syms[name]
        ext = find(src, 'extends')
        parent = syms[ext[1]] if ext else None
    out = [S('symbol'), lib_id]
    base = parent if parent is not None else src
    # flags / properties: child overrides parent
    props = {}
    for p in findall(base, 'property'):
        props[p[1]] = p
    if parent is not None:
        for p in findall(src, 'property'):
            props[p[1]] = p
    for x in base[2:]:
        if isinstance(x, list) and x[0] in ('pin_numbers', 'pin_names', 'exclude_from_sim',
                                             'in_bom', 'on_board', 'power',
                                             'duplicate_pin_numbers_are_jumpers'):
            out.append(x)
    out += list(props.values())
    for sub in findall(base, 'symbol'):
        sub = list(sub)
        suffix = sub[1][len(base[1]):]
        sub[1] = name + suffix
        out.append(sub)
    out.append([S('embedded_fonts'), S('no')])
    return out


def lib_pins(symdef):
    pins = []
    for sub in findall(symdef, 'symbol'):
        unit = int(sub[1].rsplit('_', 2)[-2])
        for p in findall(sub, 'pin'):
            at = find(p, 'at')
            pins.append(dict(num=str(find(p, 'number')[1]), name=find(p, 'name')[1],
                             type=str(p[1]), x=float(at[1]), y=float(at[2]),
                             a=float(at[3]) if len(at) > 3 else 0.0, unit=unit))
    return pins


# ---------------------------------------------------------------- schematic
class Sheet:
    def __init__(self):
        self.items = []
        self.libs = {}
        self.pwr_n = 0
        self.flg_n = 0
        self.refs = {}
        self.netpins = {}

    # geometry helpers --------------------------------------------------
    @staticmethod
    def xform(x, y, X, Y, rot):
        """lib (Y up) -> sheet (Y down), rotation CCW on screen in degrees."""
        sx, sy = x, -y
        r = math.radians(rot)
        c, s = round(math.cos(r)), round(math.sin(r))
        return X + sx * c + sy * s, Y - sx * s + sy * c

    @staticmethod
    def outward(angle, rot):
        # pin angle points from connection point into the body (lib coords)
        a = math.radians(angle)
        dx, dy = -math.cos(a), -math.sin(a)          # outward, lib coords
        sx, sy = dx, -dy                              # sheet coords
        r = math.radians(rot)
        c, s = round(math.cos(r)), round(math.sin(r))
        return round(sx * c + sy * s), round(-sx * s + sy * c)

    def _use_lib(self, lib_id):
        if lib_id not in self.libs:
            self.libs[lib_id] = lib_symbol(lib_id)
        return self.libs[lib_id]

    # primitives ---------------------------------------------------------
    def wire(self, x1, y1, x2, y2):
        self.items.append(E(S('wire'), E(S('pts'), E(S('xy'), x1, y1), E(S('xy'), x2, y2)),
                            E(S('stroke'), E(S('width'), 0), E(S('type'), S('default'))),
                            E(S('uuid'), uid('w', x1, y1, x2, y2))))

    def label(self, net, x, y, d):
        ang, just = {(1, 0): (0, 'left bottom'), (-1, 0): (180, 'right bottom'),
                     (0, -1): (90, 'left bottom'), (0, 1): (270, 'right bottom')}[d]
        self.items.append(E(S('label'), net, E(S('at'), x, y, ang),
                            font(justify=just), E(S('uuid'), uid('l', net, x, y))))

    def no_connect(self, x, y):
        self.items.append(E(S('no_connect'), E(S('at'), x, y), E(S('uuid'), uid('nc', x, y))))

    def text(self, s, x, y, size=1.27, bold=False):
        eff = [S('effects'), [S('font'), [S('size'), size, size]] + ([[S('bold'), S('yes')]] if bold else []),
               [S('justify'), S('left'), S('top')]]
        self.items.append(E(S('text'), s, E(S('exclude_from_sim'), S('no')),
                            E(S('at'), x, y, 0), eff, E(S('uuid'), uid('t', s[:40], x, y))))

    def box(self, x1, y1, x2, y2, title):
        self.items.append(E(S('rectangle'), E(S('start'), x1, y1), E(S('end'), x2, y2),
                            E(S('stroke'), E(S('width'), 0), E(S('type'), S('dash'))),
                            E(S('fill'), E(S('type'), S('none'))),
                            E(S('uuid'), uid('r', x1, y1))))
        self.text(title, x1 + 1.27, y1 + 1.27, size=2.0, bold=True)

    # symbols ------------------------------------------------------------
    def _instance(self, lib_id, ref, value, X, Y, rot, props, pin_nums, dnp=False,
                  in_bom=True, prop_pos=None):
        symdef = self._use_lib(lib_id)
        u = uid('s', ref)
        out = E(S('symbol'), E(S('lib_id'), lib_id), E(S('at'), X, Y, rot), E(S('unit'), 1),
                E(S('exclude_from_sim'), S('no')), E(S('in_bom'), S('yes' if in_bom else 'no')),
                E(S('on_board'), S('yes')), E(S('dnp'), S('yes' if dnp else 'no')),
                E(S('uuid'), u))
        libprops = {p[1]: p for p in findall(symdef, 'property')}
        allprops = dict(Reference=ref, Value=value)
        allprops.update(props)
        for k in ('Footprint', 'Datasheet', 'Description'):
            if k not in allprops:
                lp = libprops.get(k)
                allprops[k] = lp[2] if lp else ''
        for k, v in allprops.items():
            lp = libprops.get(k)
            hide = k not in ('Reference', 'Value')
            if lp is not None and k in ('Reference', 'Value'):
                h = find(lp, 'hide')
                hide = bool(h and h[1] == 'yes')
                eff = find(lp, 'effects')
                if eff is not None and Sym('hide') in eff:
                    hide = True
            if prop_pos and k in prop_pos:
                px, py, just = prop_pos[k]
            elif lp is not None:
                at = find(lp, 'at')
                px, py = self.xform(float(at[1]), float(at[2]), X, Y, rot)
                j = find(find(lp, 'effects') or [], 'justify')
                just = ' '.join(str(t) for t in j[1:]) if j else None
            else:
                px, py, just = X, Y, None
            fang = 90 if rot in (90, 270) else 0      # field angle is relative to the symbol
            p = E(S('property'), k, v, E(S('at'), round(px, 4), round(py, 4), fang))
            if hide:
                p.append(E(S('hide'), S('yes')))
            p.append(font(justify=just))
            out.append(p)
        for n in pin_nums:
            out.append(E(S('pin'), n, E(S('uuid'), uid('p', ref, n))))
        out.append(E(S('instances'), E(S('project'), PROJECT,
                                       E(S('path'), '/' + ROOT_UUID, E(S('reference'), ref),
                                         E(S('unit'), 1)))))
        self.items.append(out)

    def place(self, lib_id, ref, value, X, Y, nets, rot=0, fp=None, lcsc=None, dnp=False,
              props=None, prop_pos=None, in_bom=True, stub=STUB):
        """nets: {pin_number: net_name | 'NC'}; every pin must be listed."""
        X, Y = snap(X), snap(Y)
        symdef = self._use_lib(lib_id)
        pins = lib_pins(symdef)
        p = dict(props or {})
        if fp is not None:
            p['Footprint'] = fp
        if lcsc:
            p['LCSC'] = lcsc
        if ref.startswith('#'):
            in_bom = False
        missing = {pp['num'] for pp in pins} - set(nets)
        extra = set(nets) - {pp['num'] for pp in pins}
        if missing or extra:
            raise SystemExit(f'{ref}: missing nets for pins {sorted(missing)}, unknown {sorted(extra)}')
        self.refs[ref] = dict(lib_id=lib_id, value=value, props=p, dnp=dnp, in_bom=in_bom)
        self._instance(lib_id, ref, value, X, Y, rot, p, [pp['num'] for pp in pins], dnp=dnp,
                       in_bom=in_bom, prop_pos=prop_pos)
        groups = {}
        for pp in pins:
            pt = self.xform(pp['x'], pp['y'], X, Y, rot)
            pt = (round(pt[0], 4), round(pt[1], 4))
            groups.setdefault(pt, []).append(pp)
        for pt, gp in groups.items():
            gnets = {nets[g['num']] for g in gp}
            if len(gnets) != 1:
                raise SystemExit(f'{ref}: stacked pins {[g["num"] for g in gp]} have different nets {gnets}')
            net = gnets.pop()
            for g in gp:
                if not ref.startswith('#'):
                    self.netpins.setdefault(net, []).append(f'{ref}.{g["num"]}')
            if net is None:
                continue                      # connected by caller (power symbol)
            if net == 'NC':
                self.no_connect(*pt)
                continue
            d = self.outward(gp[0]['a'], rot)
            ex, ey = round(pt[0] + d[0] * stub, 4), round(pt[1] + d[1] * stub, 4)
            self.wire(pt[0], pt[1], ex, ey)
            if net == 'GND':
                self.gnd(ex, ey, d)
            else:
                self.label(net, ex, ey, d)

    def gnd(self, x, y, d):
        self.pwr_n += 1
        rot = {(0, 1): 0, (1, 0): 90, (0, -1): 180, (-1, 0): 270}[d]
        self.place('power:GND', f'#PWR{self.pwr_n:02d}', 'GND', x, y, {'1': None}, rot=rot)

    def flag(self, net, x, y):
        self.flg_n += 1
        self.place('power:PWR_FLAG', f'#FLG{self.flg_n:02d}', 'PWR_FLAG', x, y, {'1': net})

    # two-pin helpers ------------------------------------------------------
    def passive(self, kind, ref, value, X, Y, n1, n2, fp, lcsc=None, dnp=False, horizontal=False):
        lib_id = {'R': 'Device:R', 'C': 'Device:C', 'L': 'Device:L'}[kind]
        X, Y = snap(X), snap(Y)
        if horizontal:
            pos = {'Reference': (X, Y - 2.54, None), 'Value': (X, Y + 2.54, None)}
            self.place(lib_id, ref, value, X, Y, {'1': n1, '2': n2}, rot=90, fp=fp, lcsc=lcsc,
                       dnp=dnp, prop_pos=pos)
        else:
            pos = {'Reference': (X + 2.54, Y - 1.27, 'left'), 'Value': (X + 2.54, Y + 1.27, 'left')}
            self.place(lib_id, ref, value, X, Y, {'1': n1, '2': n2}, fp=fp, lcsc=lcsc, dnp=dnp,
                       prop_pos=pos)

    def write(self, path, title, rev, date, comments):
        tb = E(S('title_block'), E(S('title'), title), E(S('date'), date), E(S('rev'), rev),
               E(S('company'), 'zigbee_toy'))
        for i, c in enumerate(comments, 1):
            tb.append(E(S('comment'), i, c))
        doc = E(S('kicad_sch'), E(S('version'), 20250114), E(S('generator'), 'eeschema'),
                E(S('generator_version'), '9.0'), E(S('uuid'), ROOT_UUID), E(S('paper'), 'A3'), tb,
                E(S('lib_symbols'), *self.libs.values()), *self.items,
                E(S('sheet_instances'), E(S('path'), '/', E(S('page'), '1'))),
                E(S('embedded_fonts'), S('no')))
        with open(path, 'w') as f:
            f.write(dump(doc) + '\n')


def write_project_lib(path):
    doc = E(S('kicad_symbol_lib'), E(S('version'), 20250114), E(S('generator'), 'kicad_symbol_editor'),
            E(S('generator_version'), '9.0'), *CUSTOM.values())
    with open(path, 'w') as f:
        f.write(dump(doc) + '\n')


if __name__ == '__main__':
    from design import build
    os.makedirs(PROJ_DIR, exist_ok=True)
    sh = Sheet()
    build(sh)
    sh.write(os.path.join(PROJ_DIR, f'{PROJECT}.kicad_sch'),
             'Zigbee cat toy controller', 'A', '2026-10-19',
             ['nRF52840 (E73) + DRV8213 + LIS2DH12 + TP4054 + XB5353A, 1S LiPo',
              'One board for bird / fish / ball - see docs/hardware-design.md'])
    write_project_lib(os.path.join(PROJ_DIR, f'{PROJECT}.kicad_sym'))
    with open(os.path.join(PROJ_DIR, 'netlist_intent.txt'), 'w') as f:
        for net in sorted(sh.netpins):
            f.write(f'{net}: {" ".join(sorted(sh.netpins[net]))}\n')
    print('ok:', len(sh.refs), 'symbols')
