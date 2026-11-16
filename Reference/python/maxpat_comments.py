#!/usr/bin/env python3
"""Extract all comment boxes (and message boxes starting with ';' or '//') from .maxpat files, with their subpatcher path."""
import json, sys, os

def iter_boxes(patcher, path=()):
    for b in patcher.get('boxes', []):
        box = b.get('box', b)
        yield path, box
        sub = box.get('patcher')
        if sub:
            name = box.get('text', '') or box.get('maxclass', '')
            yield from iter_boxes(sub, path + (name,))

for path in sys.argv[1:]:
    with open(path, 'r', encoding='utf-8', errors='replace') as f:
        d = json.load(f)
    print(f"\n##### {os.path.basename(path)}")
    for p, box in iter_boxes(d['patcher']):
        mc = box.get('maxclass')
        if mc == 'comment':
            txt = box.get('text', '').replace('\n', ' / ')
            print(f"  [{'/'.join(p) or 'root'}] {txt}")
