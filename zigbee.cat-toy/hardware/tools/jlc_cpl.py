#!/usr/bin/env python3
"""Write the JLCPCB placement file (CPL) with each part expressed in JLC's own footprint frame.

JLC places a part by putting the origin of ITS library footprint (the EasyEDA/LCSC footprint for the
LCSC number) at Mid X/Mid Y and rotating that footprint by Rotation. KiCad's footprints have different
origins and 0-degree orientations for many packages (SOT-23 and SOT-23-5 are 180/270 degrees off, the
HRO USB-C origin is 1.57 mm away, the E73 origin is 2 mm away, EasyEDA's LED pin 1 is the anode...).
This script fits each KiCad footprint's pads onto the EasyEDA footprint's pads (rotation in 90-degree
steps plus translation) and writes the CPL in JLC's frame.

Usage (KiCad's python): jlc_cpl.py BOARD.kicad_pcb jlc_bom.csv EASYEDA_DIR out.csv
EASYEDA_DIR holds <LCSC>.json from https://easyeda.com/api/products/<LCSC>/components?version=6.4.19.5
"""
import csv, json, math, os, sys
import pcbnew

U = 0.254                      # mm per EasyEDA unit (10 mil)
# EasyEDA numbers the LED anode as pin 1; KiCad's LED symbol/footprint has the cathode as pin 1.
RENUMBER = {'C2286': {'1': '2', '2': '1'}, 'C2290': {'1': '2', '2': '1'}}


def easyeda_pads(path, code):
    d = json.load(open(path))['result']
    ds = d['packageDetail']['dataStr']
    hx, hy = ds['head']['x'], ds['head']['y']
    ren = RENUMBER.get(code, {})
    pads = []
    for s in ds['shape']:
        f = s.split('~')
        if f[0] == 'PAD':
            if f[1] == 'POLYGON' and f[10].strip():
                pts = list(map(float, f[10].split()))
                x, y = (min(pts[0::2]) + max(pts[0::2])) / 2, (min(pts[1::2]) + max(pts[1::2])) / 2
            else:
                x, y = float(f[2]), float(f[3])
            pads.append((ren.get(f[8], f[8]), (x - hx) * U, (y - hy) * U))
        elif f[0] == 'HOLE':
            pads.append(('', (float(f[1]) - hx) * U, (float(f[2]) - hy) * U))
    return pads, d.get('title')


def kicad_pads(fp):
    out = []
    for p in fp.Pads():
        if p.GetAttribute() == pcbnew.PAD_ATTRIB_SMD and not p.IsOnCopperLayer():   # paste-only apertures: not in EasyEDA data
            continue
        pos = p.GetPosition()
        out.append((p.GetNumber(), pos.x / 1e6, pos.y / 1e6))
    return out


def rot(x, y, phi):            # visual CCW rotation by phi in KiCad's y-down frame
    c, s = math.cos(math.radians(phi)), math.sin(math.radians(phi))
    return x * c + y * s, -x * s + y * c


def unique_numbers(pads):
    seen = {}
    for n, x, y in pads:
        if n:
            seen.setdefault(n, []).append((x, y))
    return {n: v[0] for n, v in seen.items() if len(v) == 1}


def fit(k, e):
    """Return (residual, phi, tx, ty, number_conflict) for the best 90-degree rotation + translation."""
    ku, eu = unique_numbers(k), unique_numbers(e)
    common = [n for n in ku if n in eu]
    best = None
    for phi in (0, 90, 180, 270):
        re = [(n, *rot(x, y, phi)) for n, x, y in e]
        cands = [(sum(x for _, x, _ in k) / len(k) - sum(x for _, x, _ in re) / len(re),
                  sum(y for _, _, y in k) / len(k) - sum(y for _, _, y in re) / len(re))]
        if len(common) >= 2:
            ren = {n: rot(*eu[n], phi) for n in common}
            cands.append((sum(ku[n][0] - ren[n][0] for n in common) / len(common),
                          sum(ku[n][1] - ren[n][1] for n in common) / len(common)))
        for tx, ty in cands:
            moved = [(n, tx + x, ty + y) for n, x, y in re]
            r1 = max(min(math.hypot(x - mx, y - my) for _, mx, my in moved) for _, x, y in k)
            r2 = max(min(math.hypot(x - mx, y - my) for _, x, y in k) for _, mx, my in moved)
            res = max(r1, r2)
            conflict = any(math.hypot(ku[n][0] - (tx + rot(*eu[n], phi)[0]), ku[n][1] - (ty + rot(*eu[n], phi)[1])) > 0.35
                           for n in common)
            score = (res + (1.0 if conflict else 0.0))
            if best is None or score < best[0] - 1e-6:
                best = (score, res, phi, tx, ty, conflict)
    return best[1:]


def main():
    board_path, bom_path, ee_dir, out_path = sys.argv[1:5]
    b = pcbnew.LoadBoard(board_path)
    lcsc = {}
    for row in csv.DictReader(open(bom_path)):
        for r in row['Designator'].split(','):
            lcsc[r.strip()] = row['LCSC Part #']
    rows, bad = [], []
    print(f"{'ref':5}{'lcsc':11}{'kicad':>7}{'jlc':>6}{'drot':>6}{'dx':>8}{'dy':>8}{'resid':>7}  easyeda footprint")
    for fp in sorted(b.GetFootprints(), key=lambda f: f.GetReference()):
        ref = fp.GetReference()
        if ref not in lcsc:
            continue
        path = os.path.join(ee_dir, lcsc[ref] + '.json')
        if not os.path.exists(path):
            bad.append(f'{ref}: no EasyEDA data {path}')
            continue
        e, title = easyeda_pads(path, lcsc[ref])
        k = kicad_pads(fp)
        res, phi, tx, ty, conflict = fit(k, e)
        c = fp.GetPosition()
        krot = fp.GetOrientationDegrees()
        note = ''
        if res > 0.35:
            note = '  <-- POOR FIT'
            bad.append(f'{ref}: residual {res:.2f} mm')
        if conflict:
            note += '  <-- PIN NUMBER CONFLICT'
            bad.append(f'{ref}: pad numbers disagree with geometry')
        print(f"{ref:5}{lcsc[ref]:11}{krot:7.0f}{phi:6.0f}{(phi - krot) % 360:6.0f}{tx - c.x / 1e6:8.3f}{ty - c.y / 1e6:8.3f}{res:7.3f}  {title}{note}")
        rows.append((ref, tx, ty, phi))
    with open(out_path, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['Designator', 'Mid X', 'Mid Y', 'Layer', 'Rotation'])
        for ref, tx, ty, phi in rows:
            w.writerow([ref, f'{tx:.4f}mm', f'{-ty:.4f}mm', 'Top', f'{phi:.1f}'])
    missing = sorted(set(lcsc) - {r[0] for r in rows})
    print(f'cpl: {len(rows)} placements' + (f'; BOM refs without placement: {missing}' if missing else ''))
    if bad:
        print('PROBLEMS:\n  ' + '\n  '.join(bad))
        sys.exit(1)


if __name__ == '__main__':
    main()
