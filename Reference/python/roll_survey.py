#!/usr/bin/env python3
"""Survey bach roll/score llll text files (also shelf-less plain llll):
header fields, voices, chords, notes, pitch grid, breakpoints, markers, groups, slot usage with samples."""
import sys, re, collections, os, json

def tokenize(s):
    return re.findall(r'"[^"]*"|\[|\]|\(|\)|[^\s\[\]\(\)]+', s)

def parse(tokens):
    stack = [[]]
    for t in tokens:
        if t in ('[', '('):
            stack.append([])
        elif t in (']', ')'):
            if len(stack) > 1:
                l = stack.pop(); stack[-1].append(l)
        else:
            stack[-1].append(t.strip('"'))
    return stack[0]

def num(x):
    try:
        return float(x)
    except Exception:
        return None

def find(l, key):
    for x in l:
        if isinstance(x, list) and x and x[0] == key:
            return x
    return None

def flat_str(x, lim=110):
    def f(y):
        if isinstance(y, list):
            return '[' + ' '.join(f(z) for z in y) + ']'
        return str(y)
    s = f(x)
    return s if len(s) <= lim else s[:lim] + '…'

def survey(tree, label):
    kind = tree[0] if tree and isinstance(tree[0], str) else '?'
    print(f"\n### {label}  kind={kind}")
    hdr = {}
    voices = []
    for it in tree[1:] if isinstance(tree[0], str) else tree:
        if isinstance(it, list) and it and isinstance(it[0], str):
            hdr[it[0]] = it[1:]
        elif isinstance(it, list):
            voices.append(it)
    for k in ('clefs', 'voicenames', 'midichannels', 'groups', 'keys', 'stafflines', 'numparts'):
        if k in hdr:
            print(f"  {k}: {flat_str(hdr[k], 200)}")
    si = hdr.get('slotinfo')
    slotnames = {}
    if si:
        rows = []
        for s in si:
            if not isinstance(s, list): continue
            d = {x[0]: ' '.join(map(str, x[1:])) for x in s[1:] if isinstance(x, list) and x}
            slotnames[str(s[0])] = (d.get('name', ''), d.get('type', ''), d.get('temporalmode', ''))
            if d.get('type') not in (None, '', 'none'):
                rows.append(f"{s[0]}:{d.get('name','')}<{d.get('type','')}/{d.get('temporalmode','')}>")
        print("  slotinfo (typed): " + '  '.join(rows))
    mk = hdr.get('markers')
    if mk:
        print(f"  markers: {len(mk)}  e.g. " + ' | '.join(flat_str(m, 70) for m in mk[:6]))
    if kind == 'score':
        # count measures roughly
        print(f"  (score) voices: {len(voices)}")
    nchords = nnotes = nbp = 0
    grid = collections.Counter()
    slot_used = collections.Counter()
    slot_samples = collections.defaultdict(list)
    tmax = 0.0
    chordsizes = collections.Counter()
    vel = collections.Counter()
    def is_note(x):
        return isinstance(x, list) and len(x) >= 3 and num(x[0]) is not None and num(x[1]) is not None and num(x[2]) is not None
    for v in voices:
        for ch in v:
            if not (isinstance(ch, list) and ch and num(ch[0]) is not None):
                continue
            notes = [n for n in ch[1:] if is_note(n)]
            if not notes: continue
            nchords += 1; chordsizes[min(len(notes), 5)] += 1
            for n in notes:
                nnotes += 1
                c = num(n[0]); d = num(n[1])
                tmax = max(tmax, num(ch[0]) + d)
                r = c % 100
                if abs(r) < 0.01 or abs(r - 100) < 0.01: grid['semitone'] += 1
                elif abs(r - 50) < 0.01: grid['quarter'] += 1
                elif abs(r - 25) < 0.01 or abs(r - 75) < 0.01: grid['eighth'] += 1
                else: grid['off-grid'] += 1
                for x in n[3:]:
                    if isinstance(x, list) and x:
                        if x[0] == 'breakpoints':
                            nbp += 1
                            if len(slot_samples['bp']) < 2: slot_samples['bp'].append(flat_str(x[1:], 120))
                        elif x[0] == 'slots':
                            for s in x[1:]:
                                if isinstance(s, list) and s:
                                    k = str(s[0]); slot_used[k] += 1
                                    if len(slot_samples[k]) < 3:
                                        smp = flat_str(s[1:], 100)
                                        if smp not in slot_samples[k]: slot_samples[k].append(smp)
    print(f"  voices={len(voices)} chords={nchords} notes={nnotes} length={tmax/1000:.1f}s  chord sizes={dict(sorted(chordsizes.items()))}")
    print(f"  pitch grid: {dict(grid)}   notes with pitch breakpoints: {nbp}")
    for k in sorted(slot_used, key=lambda z: int(z) if z.isdigit() else 999):
        nm = slotnames.get(k, ('', '', ''))
        print(f"    slot {k:>2} {nm[0][:22]:22} <{nm[1]}/{nm[2]}> x{slot_used[k]:<4} e.g. " + '  ‖  '.join(slot_samples[k][:2]))
    if slot_samples.get('bp'):
        print("    breakpoints e.g. " + '  ‖  '.join(slot_samples['bp']))

def load_any(path):
    txt = open(path, encoding='utf-8', errors='replace').read()
    st = txt.lstrip()
    if st.startswith('{'):
        # shelf json
        d = json.loads(txt)
        keys = d.get('_x_x_x_x_keys_x_x_x_x_', [])
        out = []
        for k in keys:
            cnt = d.get(f'{k}_count', [1])[0]
            atoms = []
            for i in range(int(cnt)):
                atoms += d.get(f'{k}_{i:010d}', [])
            out.append((k, atoms))
        return 'shelf', out
    return 'llll', parse(tokenize(txt))

def atoms_to_tree(atoms):
    """bach native-in-JSON: list of atoms incl. '[' ']' symbols and float64 packed as marker+2 ints."""
    import struct
    toks = []
    i = 0
    while i < len(atoms):
        a = atoms[i]
        if a == '_x_x_x_x_bach_float64_x_x_x_x_' and i + 2 < len(atoms):
            lo, hi = atoms[i + 1], atoms[i + 2]
            try:
                v = struct.unpack('<d', struct.pack('<II', lo & 0xffffffff, hi & 0xffffffff))[0]
            except Exception:
                v = 0.0
            toks.append(repr(v)); i += 3; continue
        toks.append(str(a)); i += 1
    return parse(toks)

if __name__ == '__main__':
    for f in sys.argv[1:]:
        try:
            kind, data = load_any(f)
        except Exception as e:
            print(f"\n### {os.path.basename(f)}: cannot read ({e})"); continue
        if kind == 'llll':
            survey(data, os.path.basename(f))
        else:
            print(f"\n######## SHELF {os.path.basename(f)}: {len(data)} items: " + ', '.join(k for k, _ in data)[:400])
            for k, atoms in data[:40]:
                survey(atoms_to_tree(atoms), f"shelf item '{k}'")
