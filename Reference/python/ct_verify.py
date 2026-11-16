#!/usr/bin/env python3
"""Check the conTimbre search database against real .cePlayerOrc files:
for every program in the files, rebuild its 128 keys from the database and compare."""
import sys, re, glob, collections, importlib.util
spec = importlib.util.spec_from_file_location('ctdb', '/home/claude/tools/ctdb.py'); m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
db = m.CTDB(sys.argv[1], cache='/home/claude/ctdb_cache.pkl')

# 1. is a keygroup's modes_index the position of its keyboard within the instrument?
same = diff = 0; lens = collections.Counter()
for ii in range(len(db.inst_names)):
    for mp in range(db.n_modes(ii)):
        kb = db.keyboard(ii, mp); lens[len(kb)] += 1
        s = set(k['modes_index'] for k in kb)
        if s == {mp}: same += 1
        else: diff += 1
print(f"keyboards whose keys all carry modes_index == their position in the instrument's list: {same}; others: {diff}")
print("keys per keyboard:", dict(lens.most_common(6)))

def parse_orc(path):
    t = open(path, encoding='utf-8', errors='replace').read()
    head, voices = t.split('\nvoices:', 1)
    models = re.findall(r'instrument: "([^"]+)"', head)
    out = []
    for vm in re.finditer(r'\n    name: "([^"]*)"(.*?)(?=\n    name: "|\Z)', voices, flags=re.S):
        for pm in re.finditer(r'\n        name:\s+"([^"]*)"(.*?)(?=\n        name:\s+"|\Z)', vm.group(2), flags=re.S):
            keys = []
            for km in re.finditer(r'number: (\d+)\s+model_index: (-?\d+)\s+modes_index: (-?\d+)\s+note_menu_index: (-?\d+)(.*?)filename: "([^"]*)"\s+pitch:\s+([-\d.]+)', pm.group(2), flags=re.S):
                keys.append({'model': models[int(km.group(2))] if 0 <= int(km.group(2)) < len(models) else '?', 'modes_index': int(km.group(3)), 'note_menu_index': int(km.group(4)), 'filename': km.group(6), 'pitch': float(km.group(7))})
            out.append((vm.group(1), pm.group(1), keys))
    return out

tot = ok = 0
for f in sys.argv[2:]:
    for voice, prog, keys in parse_orc(f):
        if not keys: continue
        tot += 1
        inst = collections.Counter(k['model'] for k in keys).most_common(1)[0][0]
        mi = collections.Counter(k['modes_index'] for k in keys).most_common(1)[0][0]
        if inst not in db.inst_names:
            print(f"  ?? {f.split('/')[-1][:30]:30} {voice[:9]:9} {prog[:34]:34} instrument {inst!r} not in database"); continue
        ii = db.inst_names.index(inst)
        if not (0 <= mi < db.n_modes(ii)):
            print(f"  ?? {prog}: modes_index {mi} out of range ({db.n_modes(ii)})"); continue
        kb = db.keyboard(ii, mi)
        name = '.'.join(db.mode_name(ii, mi))
        # compare key by key
        nmatch = 0; first_bad = None
        for i, k in enumerate(keys):
            if i < len(kb):
                d = kb[i]
                fn = db.short_filename(d['sounds'][0])
                if fn == k['filename'] and abs(d['pitch'] - k['pitch']) < 1e-3 and d['note_menu_index'] == k['note_menu_index'] and d['modes_index'] == k['modes_index']:
                    nmatch += 1
                elif first_bad is None:
                    first_bad = (i, k['filename'], fn, k['pitch'], d['pitch'])
        status = 'IDENTICAL' if nmatch == len(keys) == len(kb) else f"{nmatch}/{len(keys)} keys (db has {len(kb)})"
        if nmatch == len(keys) == len(kb): ok += 1
        print(f"  {f.split('/')[-1][:26]:26} {voice[:9]:9} {prog[:32]:32} -> db: {inst[:18]:18} #{mi:<4} {name[:44]:44} {status}" + (f"  first diff {first_bad}" if first_bad and nmatch != len(keys) else ''))
print(f"\nprograms rebuilt identically from the database: {ok} of {tot}")
