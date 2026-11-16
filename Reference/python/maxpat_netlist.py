#!/usr/bin/env python3
"""Dump a .maxpat (or a named subpatcher path inside it) as a readable netlist:
each box with short id, class/text, and its outgoing connections (outlet -> dest:inlet).
Usage: maxpat_netlist.py file.maxpat [--sub "p NAME/p SUB"] [--depth N] [--all]
"""
import json, sys, re, argparse

def load(path):
    with open(path, 'r', encoding='utf-8', errors='replace') as f:
        return json.load(f)

def find_sub(patcher, parts):
    if not parts:
        return patcher
    head, rest = parts[0], parts[1:]
    for b in patcher.get('boxes', []):
        box = b.get('box', b)
        if box.get('patcher') is not None:
            name = box.get('text', '') or box.get('maxclass', '')
            if name == head:
                return find_sub(box['patcher'], rest)
    raise SystemExit(f"subpatcher not found: {head}")

def label(box):
    mc = box.get('maxclass', '?')
    if mc == 'newobj':
        return box.get('text', '')
    if mc == 'message':
        return f"[msg] {box.get('text','')}"
    if mc == 'comment':
        return f"// {box.get('text','')}"
    if mc == 'bpatcher':
        return f"[bpatcher {box.get('name','')} {box.get('args','')}]"
    extra = ''
    for k in ('text', 'varname', 'parameter_longname'):
        if k in box: extra += f" {k}={box[k]}"
    sa = box.get('saved_attribute_attributes')
    return f"<{mc}>{extra}"

def dump(patcher, indent=0, depth=99, title='ROOT', show_comments=True):
    pad = '  ' * indent
    boxes = [b.get('box', b) for b in patcher.get('boxes', [])]
    ids = {}
    # short ids: obj-12 -> 12
    for b in boxes:
        bid = b.get('id', '?')
        ids[bid] = bid.replace('obj-', '')
    print(f"{pad}=== {title} ({len(boxes)} boxes) ===")
    # connections: group by source
    out = {}
    for l in patcher.get('lines', []):
        pl = l.get('patchline', l)
        s, si = pl['source']; d, di = pl['destination']
        out.setdefault(s, []).append((si, d, di))
    for b in boxes:
        bid = b.get('id', '?')
        mc = b.get('maxclass', '?')
        if mc == 'comment' and not show_comments:
            continue
        lab = label(b)
        if mc == 'comment':
            txt = lab.replace('\n', ' ')
            print(f"{pad}  #{ids[bid]:>4}  {txt[:160]}")
            continue
        conns = out.get(bid, [])
        conns_s = ', '.join(f"{si}->#{ids.get(d,d)}:{di}" for si, d, di in sorted(conns))
        varname = f"  @varname {b['varname']}" if 'varname' in b else ''
        # key attributes for bach objects
        attrs = ''
        if mc.startswith('bach.') or mc in ('umenu', 'tab', 'textedit', 'number', 'flonum', 'toggle'):
            keep = {k: v for k, v in b.items() if k not in ('id', 'maxclass', 'numinlets', 'numoutlets', 'outlettype', 'patching_rect', 'presentation_rect', 'presentation', 'fontsize', 'fontname', 'saved_attribute_attributes', 'whole_roll_data_0000000000', 'whole_score_data_0000000000', 'whole_slot_data_0000000000', 'items', 'text', 'varname', 'style', 'parameter_enable', 'background', 'hidden', 'ignoreclick', 'linecount', 'textjustification', 'bgcolor', 'textcolor', 'fontface', 'triscale', 'htricolor', 'hidden')}
            # only short attrs
            keep = {k: v for k, v in keep.items() if len(str(v)) < 80}
            if keep: attrs = '  ' + json.dumps(keep)
        lab1 = lab.replace('\n', ' ')
        print(f"{pad}  #{ids[bid]:>4}  {lab1[:200]}{varname}{attrs}")
        if conns_s:
            print(f"{pad}        -> {conns_s}")
    if depth > 0:
        for b in boxes:
            if b.get('patcher') is not None:
                name = b.get('text', '') or b.get('maxclass', '')
                dump(b['patcher'], indent + 1, depth - 1, title=f"{name} [#{ids[b['id']]}]", show_comments=show_comments)

if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('file')
    ap.add_argument('--sub', default='')
    ap.add_argument('--depth', type=int, default=0)
    ap.add_argument('--nocomments', action='store_true')
    a = ap.parse_args()
    d = load(a.file)
    root = d['patcher']
    parts = [p for p in a.sub.split('/') if p]
    sub = find_sub(root, parts)
    dump(sub, 0, a.depth, title=a.sub or 'ROOT', show_comments=not a.nocomments)
