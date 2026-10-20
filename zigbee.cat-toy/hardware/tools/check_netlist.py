#!/usr/bin/env python3
"""Compare a KiCad netlist export with netlist_intent.txt written by gen_sch.py."""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from sexpr import parse, find, findall  # noqa: E402

net_file, intent_file = sys.argv[1:3]
t = parse(open(net_file).read())
nets = {}
for n in findall(find(t, 'nets'), 'net'):
    name = find(n, 'name')[1].lstrip('/')
    nodes = [f"{find(x, 'ref')[1]}.{find(x, 'pin')[1]}" for x in findall(n, 'node')
             if not find(x, 'ref')[1].startswith('#')]
    if name.startswith('unconnected-'):
        name = 'NC'
    if nodes:
        nets.setdefault(name, []).extend(nodes)
intent = {}
for line in open(intent_file):
    k, v = line.split(':', 1)
    intent[k] = sorted(v.split())
bad = [k for k in sorted(set(nets) | set(intent)) if sorted(nets.get(k, [])) != intent.get(k, [])]
for k in bad:
    print(f'MISMATCH {k}\n  kicad : {sorted(nets.get(k, []))}\n  intent: {intent.get(k, [])}')
print(f'{len(intent)} nets checked, {len(bad)} mismatches')
sys.exit(1 if bad else 0)
