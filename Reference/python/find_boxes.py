#!/usr/bin/env python3
"""Find boxes whose text/varname matches a regex in a .maxpat; print patcher path, box label, varname, and what feeds it / what it feeds."""
import json, sys, re, collections
pat = re.compile(sys.argv[2], re.I)
d = json.load(open(sys.argv[1], encoding='utf-8', errors='replace'))
def label(b):
    mc = b.get('maxclass')
    t = b.get('text', '')
    v = b.get('varname', '')
    base = t if mc == 'newobj' else (f"[msg] {t}" if mc == 'message' else f"<{mc}>")
    return (base + (f" @varname {v}" if v else '')).replace('\n', ' ')
def walk(p, path):
    boxes = {b.get('box', b)['id']: b.get('box', b) for b in p.get('boxes', [])}
    ins = collections.defaultdict(list); outs = collections.defaultdict(list)
    for l in p.get('lines', []):
        pl = l.get('patchline', l)
        outs[pl['source'][0]].append(pl['destination'][0]); ins[pl['destination'][0]].append(pl['source'][0])
    for bid, b in boxes.items():
        lab = label(b)
        if pat.search(lab):
            print(f"[{'/'.join(path) or 'root'}] {lab[:150]}")
            if ins[bid]: print("      <= " + ' | '.join(label(boxes[s])[:60] for s in ins[bid][:5]))
            if outs[bid]: print("      => " + ' | '.join(label(boxes[s])[:60] for s in outs[bid][:5]))
        if b.get('patcher'):
            walk(b['patcher'], path + [(b.get('text') or b.get('maxclass') or '')[:40]])
walk(d['patcher'], [])
