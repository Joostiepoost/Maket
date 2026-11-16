#!/usr/bin/env python3
"""conTimbre ePlayer orchestra files (.cePlayerOrc): reader, writer in the player's own text layout,
and a generator that builds programs from the conTimbre search database (see ctdb.py).

Only text metadata is read or written: instrument, technique and sample NAMES. No audio is touched."""
import re, sys, importlib.util, collections

def _load_ctdb():
    spec = importlib.util.spec_from_file_location('ctdb', '/home/claude/tools/ctdb.py')
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m); return m

# ------------------------------------------------------------------ reader
def read_orc(path):
    t = open(path, encoding='utf-8', errors='replace').read()
    head, voices_txt = t.split('\nvoices:', 1)
    g = {}
    for k in ('master_gain', 'reverb_is_on', 'reverb_gain', 'reverb_time', 'reverb_roomsize', 'reverb_damping'):
        m = re.search(r'%s:?\s+(\S+)' % k, head); g[k] = m.group(1) if m else None
    rev = re.findall(r'\n\s+out:\s+(-?\d+)', head)
    models = [dict(group=a, family=b, instrument=c) for a, b, c in re.findall(r'group:\s+"([^"]*)"\s+family:\s+"([^"]*)"\s+instrument:\s+"([^"]*)"', head)]
    voices = []
    for vm in re.finditer(r'\n    name: "([^"]*)"\n(.*?)(?=\n  \{\n    name: "|\Z)', voices_txt, flags=re.S):
        body = vm.group(2)
        v = {'name': vm.group(1)}
        vhead = body.split('programs: {', 1)[0]
        for k in ('color_index', 'num_programs', 'current_midiprogram', 'midichannel', 'gain', 'output', 'pan', 'solo', 'mute', 'gain_to_reverb'):
            m = re.search(r'\n?\s*%s:\s+(\S+)' % k, vhead); v[k] = m.group(1) if m else None
        v['programs'] = []
        for pm in re.finditer(r'number: (-?\d+)\n\s+name:\s+"([^"]*)"\n\s+chorus: (-?\d+)\n\s+chord_mode:\s+(\w+)\n\s+number_of_keys: (\d+)\n\s+keys: \{(.*?)\n        \}', body, flags=re.S):
            p = {'number': int(pm.group(1)), 'name': pm.group(2), 'chorus': int(pm.group(3)), 'chord_mode': pm.group(4), 'keys': []}
            for km in re.finditer(r'number: (\d+)\s+model_index: (-?\d+)\s+modes_index: (-?\d+)\s+note_menu_index: (-?\d+)\s+playing_mode: (-?\d+)\s+detune: ([-\d.]+)\s+gain: ([-\d.]+)\s+let_vibrate: ([-\d.]+)\s+filename: "([^"]*)"\s+pitch:\s+([-\d.]+)', pm.group(6)):
                p['keys'].append({'model': models[int(km.group(2))]['instrument'], 'modes_index': int(km.group(3)), 'note_menu_index': int(km.group(4)),
                                  'playing_mode': int(km.group(5)), 'detune': float(km.group(6)), 'gain': float(km.group(7)),
                                  'let_vibrate': km.group(8), 'filename': km.group(9), 'pitch': float(km.group(10))})
            if not p['keys']:
                # chord-mode programs ("interpret chords": multiphonics, play and sing) hold one short entry:
                # number, model_index, modes_index, let_vibrate; number_of_keys still says 128
                cm = re.search(r'number: (\d+)\s+model_index: (-?\d+)\s+modes_index: (-?\d+)\s+let_vibrate: ([-\d.]+)', pm.group(6))
                if cm:
                    p['compact'] = True; p['number_of_keys'] = int(pm.group(5))
                    p['keys'].append({'model': models[int(cm.group(2))]['instrument'], 'modes_index': int(cm.group(3)), 'let_vibrate': cm.group(4)})
            v['programs'].append(p)
        voices.append(v)
    return {'general': g, 'reverb_outputs': [int(x) for x in rev], 'models': models, 'voices': voices}

# ------------------------------------------------------------------ writer (player layout)
def write_orc(orc, gfi):
    """gfi: instrument -> (group, family). Models are listed in order of first use."""
    models = []
    for v in orc['voices']:
        for p in v['programs']:
            for k in p['keys']:
                if k['model'] not in models: models.append(k['model'])
    g = orc['general']; o = []
    o.append('conTimbre_eplayer_orchestra V2.0')
    o.append('general: {')
    o.append('master_gain %s' % g['master_gain'])
    o.append('  num_models:  %d' % len(models))
    o.append('  num_voices:  %d' % len(orc['voices']))
    o.append('  reverb_is_on:  %s' % g['reverb_is_on'])
    o.append('  reverb_gain:  %s' % g['reverb_gain'])
    o.append('  reverb_time:  %s' % g['reverb_time'])
    o.append('  reverb_roomsize:  %s' % g['reverb_roomsize'])
    o.append('  reverb_damping:  %s' % g['reverb_damping'])
    o.append('}')
    o.append('reverb_outputs: {')
    for r in orc['reverb_outputs']: o.append('  out:  %d' % r)
    o.append('}')
    o.append('models: {')
    for mname in models:
        grp, fam = gfi[mname]
        o += ['  {', '    group: "%s"' % grp, '    family: "%s"' % fam, '    instrument: "%s"' % mname, '  }']
    o.append('}')
    o.append('voices: {')
    for v in orc['voices']:
        o += ['  {', '    name: "%s"' % v['name'], '    color_index: %s' % v['color_index'], '    num_programs: %d' % len(v['programs']),
              '    current_midiprogram: %s' % v['current_midiprogram'], '    midichannel: %s' % v['midichannel'], '    gain: %s' % v['gain'],
              '    output: %s' % v['output'], '    pan: %s' % v['pan'], '    solo: %s' % v['solo'], '    mute: %s' % v['mute'],
              '    gain_to_reverb: %s' % v['gain_to_reverb'], '    programs: {']
        for p in v['programs']:
            o += ['      {', '        number: %d' % p['number'], '        name: "%s"' % p['name'], '        chorus: %d' % p['chorus'],
                  '        chord_mode:  %s' % p['chord_mode'],
                  '        number_of_keys: %d' % (p.get('number_of_keys', 128) if p.get('compact') else len(p['keys'])), '        keys: {']
            if p.get('compact'):
                k = p['keys'][0]   # the player's own layout: the two index lines are indented by 8, the others by 12
                o += ['          {', '            number: 0', '        model_index: %d' % models.index(k['model']),
                      '        modes_index: %d' % k['modes_index'], '            let_vibrate: %s' % k['let_vibrate'], '          }']
            for i, k in enumerate([] if p.get('compact') else p['keys']):
                o += ['          {', '            number: %d' % i, '            model_index: %d' % models.index(k['model']),
                      '            modes_index: %d' % k['modes_index'], '            note_menu_index: %d' % k['note_menu_index'],
                      '            playing_mode: %d' % k['playing_mode'], '            detune: %.6f' % k['detune'], '            gain: %.6f' % k['gain'],
                      '            let_vibrate: %s' % k['let_vibrate'], '            filename: "%s"' % k['filename'], '            pitch: %.6f' % k['pitch'], '          }']
            o += ['        }', '      }']
        o += ['    }', '  }']
    o.append('}')
    return '\n'.join(o) + '\n'

# ------------------------------------------------------------------ generator
def program_from_db(db, instrument, modes_index, name, chorus=1):
    ii = db.inst_names.index(instrument)
    keys = []
    for kg in db.keyboard(ii, modes_index)[:128]:
        s = kg['sounds'][0]
        perc = db.value(s, 'percussive')
        unpitched = db.value(s, 'pitch_english') < 0
        keys.append({'model': instrument, 'modes_index': kg['modes_index'], 'note_menu_index': kg['note_menu_index'],
                     'playing_mode': 0 if perc == 'percussive' else 1, 'detune': 0.0, 'gain': 0.0, 'let_vibrate': '0.1',
                     'filename': db.short_filename(s), 'pitch': -1000.0 if unpitched else kg['pitch']})
    return {'number': -1, 'name': name, 'chorus': chorus, 'chord_mode': 'false', 'keys': keys}

CHORD_TECHNIQUES = ('multiphonic', 'play and sing')

def is_chord_technique(db, instrument, modes_index):
    """True for techniques the player plays in 'interpret chords' mode (first word of the technique name)."""
    m = db.mode_name(db.inst_names.index(instrument), modes_index)   # a list of name parts, e.g. ['play and sing', 'man']
    first = m[0] if isinstance(m, (list, tuple)) and m else str(m).split('.')[0]
    return first in CHORD_TECHNIQUES

def chord_program_from_db(db, instrument, modes_index, name, chorus=1):
    """A chord-mode program: no key table, only the instrument and the technique's position in its list."""
    db.inst_names.index(instrument)
    return {'number': -1, 'name': name, 'chorus': chorus, 'chord_mode': 'true', 'compact': True, 'number_of_keys': 128,
            'keys': [{'model': instrument, 'modes_index': modes_index, 'let_vibrate': '0.1'}]}

def default_voice(name, programs, pan='0.500000', output='C1&2'):
    return {'name': name, 'color_index': '0', 'current_midiprogram': '0', 'midichannel': '0', 'gain': '0.000000', 'output': output,
            'pan': pan, 'solo': 'false', 'mute': 'false', 'gain_to_reverb': '0.000000', 'programs': programs}

DEFAULT_GENERAL = {'master_gain': '0.000000', 'reverb_is_on': 'true', 'reverb_gain': '0.000000', 'reverb_time': '1.500000',
                   'reverb_roomsize': '80.000000', 'reverb_damping': '0.900000'}

if __name__ == '__main__':
    ctdb = _load_ctdb()
    db = ctdb.CTDB(sys.argv[1], cache='/home/claude/ctdb_cache.pkl')
    gfi = {r[2]: (r[0], r[1]) for r in db.group_family_instruments}
    for f in sys.argv[2:]:
        orig = open(f, encoding='utf-8', errors='replace').read()
        orc = read_orc(f)
        nk = sum(len(p['keys']) for v in orc['voices'] for p in v['programs'])
        # test A: writer round trip
        a = write_orc(orc, gfi)
        # test B: rebuild every program from the database (only instrument + technique number + names + voice settings kept)
        reb = {'general': orc['general'], 'reverb_outputs': orc['reverb_outputs'], 'voices': []}
        notes = []
        for v in orc['voices']:
            nv = dict(v); nv['programs'] = []
            for p in v['programs']:
                inst = collections.Counter(k['model'] for k in p['keys']).most_common(1)[0][0]
                mi = collections.Counter(k['modes_index'] for k in p['keys']).most_common(1)[0][0]
                np_ = program_from_db(db, inst, mi, p['name'], p['chorus']); np_['number'] = p['number']; np_['chord_mode'] = p['chord_mode']
                d = sum(1 for x, y in zip(np_['keys'], p['keys']) if x != y)
                if d:
                    kinds = collections.Counter(fld for x, y in zip(np_['keys'], p['keys']) for fld in x if x[fld] != y[fld])
                    notes.append(f"{v['name']}/{p['name']}: {d} keys differ in {dict(kinds)}")
                nv['programs'].append(np_)
            reb['voices'].append(nv)
        b = write_orc(reb, gfi)
        print(f"{f.split('/')[-1][:44]:44} voices={len(orc['voices']):2} programs={sum(len(v['programs']) for v in orc['voices']):2} keys={nk:5} | "
              f"read+write: {'BYTE-IDENTICAL' if a == orig else 'differs'} | rebuilt from database: {'BYTE-IDENTICAL' if b == orig else 'differs'}")
        if a != orig:
            la, lo = a.split('\n'), orig.split('\n')
            for i, (x, y) in enumerate(zip(la, lo)):
                if x != y: print(f"     first writer difference at line {i+1}: mine={x!r} file={y!r}"); break
            if len(la) != len(lo): print(f"     line counts: mine {len(la)} file {len(lo)}")
        for n in notes: print("     " + n)
