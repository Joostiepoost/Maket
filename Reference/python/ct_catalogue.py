#!/usr/bin/env python3
"""Build the technique catalogue from the conTimbre search database, in the player's own order:
group, family, instrument, position (= modes_index), technique name, strings covered, m-numbers, sounding range."""
import sys, re, json, collections, importlib.util
spec = importlib.util.spec_from_file_location('ctdb', '/home/claude/tools/ctdb.py'); m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
db = m.CTDB(sys.argv[1], cache='/home/claude/ctdb_cache.pkl')
gfi = {r[2]: (r[0], r[1]) for r in db.group_family_instruments}
rows = []
for ii, inst in enumerate(db.inst_names):
    for mp in range(db.n_modes(ii)):
        kb = db.keyboard(ii, mp)
        firsts = [k['sounds'][0] for k in kb]
        uniq = list(dict.fromkeys(firsts))
        strings = sorted(set(db.value(s, 'string-number') for s in uniq))
        strings = [s for s in strings if s >= 0]
        modes_first = db.value(uniq[0], 'modes_english')
        nostring = db.value(uniq[0], 'no-string-modes')
        fns = [db.short_filename(s) for s in uniq]
        mnums = sorted(set(int(x) for f in fns for x in re.findall(r'\.m(\d+)\.', f)))
        pitches = [k['pitch'] for k in kb[:128]]
        sounding = [p for p in pitches if p > 0]
        nsounds = len(set(s for k in kb for s in k['sounds']))
        rows.append({'group': gfi[inst][0], 'family': gfi[inst][1], 'instrument': inst, 'modes_index': mp,
                     'modes': modes_first, 'no_string_modes': nostring, 'strings': strings, 'm': mnums,
                     'keys': len(kb), 'distinct_keys': len(uniq), 'sounds': nsounds,
                     'low': min(sounding) if sounding else None, 'high': max(sounding) if sounding else None})
json.dump(rows, open(sys.argv[2], 'w'))
print("catalogue rows:", len(rows))
multi = [r for r in rows if len(r['strings']) > 1]
single = [r for r in rows if len(r['strings']) == 1]
print(f"keyboards spanning several strings: {len(multi)}; on one string: {len(single)}; without string: {len(rows) - len(multi) - len(single)}")
if len(sys.argv) > 3:
    inst = sys.argv[3]
    L = [r for r in rows if r['instrument'] == inst]
    print(f"\n{inst}: {len(L)} keyboards")
    for r in L:
        nm = '.'.join(r['no_string_modes'] if len(r['strings']) > 1 else r['modes'])
        st = ('strings ' + ','.join(str(s) for s in r['strings'])) if r['strings'] else ''
        print(f"  #{r['modes_index']:<4} {nm[:62]:62} {st:16} m={','.join(map(str, r['m']))[:20]:20} range {r['low']}..{r['high']}  sounds={r['sounds']}")
