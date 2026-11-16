#!/usr/bin/env python3
"""Reader for OpenMusic instance files (.omi) holding a chord-seq or a multi-seq.

The file is Lisp text. This reader parses it as nested lists and looks only for
(make-instance (quote chord-seq) :lmidic ... :lonset ... ) forms; it never evaluates anything.
"""
import re, sys

def tokens(text):
    text = '\n'.join(l for l in text.split('\n') if not l.lstrip().startswith(';'))
    return re.findall(r'"(?:[^"\\]|\\.)*"|[()]|[^\s()]+', text)

def atom(t):
    if t.startswith('"'): return ('str', t[1:-1])
    try: return int(t)
    except ValueError:
        try: return float(t)
        except ValueError: return t.lower()

def parse(text):
    stack = [[]]
    for t in tokens(text):
        if t == '(': stack.append([])
        elif t == ')':
            x = stack.pop(); stack[-1].append(x)
        else: stack[-1].append(atom(t))
    if len(stack) != 1: raise ValueError('unbalanced parentheses')
    return stack[0]

def unquote(x):
    return x[1] if isinstance(x, list) and len(x) == 2 and x[0] == 'quote' else x

def instances(tree, found=None):
    """All (make-instance (quote <class>) ...) forms, in document order, as (class, {keyword: value})."""
    if found is None: found = []
    if isinstance(tree, list):
        if len(tree) >= 2 and tree[0] == 'make-instance':
            cls = unquote(tree[1]); args = {}
            rest = tree[2:]
            for i in range(0, len(rest) - 1, 2):
                if isinstance(rest[i], str) and rest[i].startswith(':'): args[rest[i][1:]] = unquote(rest[i + 1])
            found.append((cls, args))
        for x in tree: instances(x, found)
    return found

def read_omi(path):
    """-> {'class': 'chord-seq' | 'multi-seq', 'voices': [ {onsets, end, chords:[[(cents, dur, vel, offset, chan)...]], approx, legato} ]}"""
    inst = instances(parse(open(path, encoding='utf-8', errors='replace').read()))
    if not inst: raise ValueError('no instance found')
    voices = []
    for cls, a in inst:
        if cls != 'chord-seq': continue
        mc, on = a.get('lmidic', []), a.get('lonset', [])
        chords = []
        for i, pitches in enumerate(mc):
            def col(key, default):
                v = a.get(key); row = v[i] if isinstance(v, list) and i < len(v) else default
                return row if isinstance(row, list) else [row] * len(pitches)
            du, ve, of, ch = col('ldur', 1000), col('lvel', 100), col('loffset', 0), col('lchan', 1)
            chords.append([(p, du[j % len(du)], ve[j % len(ve)], of[j % len(of)], ch[j % len(ch)]) for j, p in enumerate(pitches)])
        voices.append({'onsets': on[:len(mc)], 'end': on[len(mc)] if len(on) > len(mc) else None, 'chords': chords,
                       'approx': a.get('approx'), 'legato': a.get('legato')})
    return {'class': inst[0][0], 'voices': voices}

if __name__ == '__main__':
    for p in sys.argv[1:]:
        r = read_omi(p)
        print(p.split('/')[-1], '->', r['class'], '|', len(r['voices']), 'voice(s)')
        for i, v in enumerate(r['voices']):
            n = sum(len(c) for c in v['chords'])
            print('   voice %d: %d chords, %d notes, onsets %s … end %s, approx %s, legato %s, first chord %s'
                  % (i + 1, len(v['chords']), n, v['onsets'][:3], v['end'], v['approx'], v['legato'], [x[0] for x in v['chords'][0]]))
