#!/usr/bin/env python3
"""Reader for the conTimbre 'algorithmic orchestration' search data (the files that ship with
conTimbre's own Lisp interface). Mirrors contimbre_library.lisp:
  - database.lisp          : instruments, group_family_instruments, contimbre_categories, contimbre_data_indices
  - database eplayer.lisp  : eplayer_keygroup_indices, eplayer_keyboard_indices
  - *.dat                  : per-category binary data (longs, floats, strings, lists)
  - eplayer_banks.dat      : keygroups (modes_index, note_menu_index, pitch*1000, numzones, sound indices)
Only reads metadata; no audio is touched.
"""
import os, re, struct, sys, pickle

class CTDB:
    def __init__(self, search_dir, cache=None):
        self.dir = search_dir
        if cache and os.path.exists(cache):
            self.__dict__.update(pickle.load(open(cache, 'rb')))
            self.dir = search_dir
        else:
            self._load_lisp()
            if cache:
                pickle.dump({k: v for k, v in self.__dict__.items() if k != 'buf'}, open(cache, 'wb'))
        self.buf = {}

    # ---------- s-expression reading (ints and strings only) ----------
    @staticmethod
    def _forms(path):
        """yield (name, python nested list) for each (setf COMMON-LISP-USER::name '( ... )) form"""
        txt = open(path, encoding='latin-1').read()
        tok = re.compile(r'"((?:[^"\\]|\\.)*)"|\(|\)|[^\s()"]+')
        pos = 0
        for m in re.finditer(r'\(setf COMMON-LISP-USER::([A-Za-z_]+)\s+\'', txt):
            name = m.group(1).lower()
            i = m.end()
            stack = []; cur = None; root = None
            for t in tok.finditer(txt, i):
                s = t.group(0)
                if s == '(':
                    new = []
                    if cur is not None: cur.append(new); stack.append(cur)
                    cur = new
                    if root is None: root = new
                elif s == ')':
                    if not stack:
                        break
                    cur = stack.pop()
                elif s.startswith('"'):
                    cur.append(t.group(1).replace('\\"', '"').replace('\\\\', '\\'))
                else:
                    try: cur.append(int(s))
                    except ValueError:
                        try: cur.append(float(s))
                        except ValueError: cur.append(s)
            yield name, root

    def _load_lisp(self):
        for name, val in self._forms(os.path.join(self.dir, 'database.lisp')):
            setattr(self, name, val)
        for name, val in self._forms(os.path.join(self.dir, 'database eplayer.lisp')):
            setattr(self, name, val)
        self.inst_names = [r[0] for r in self.instruments]
        self.categories = [c[0] for c in self.contimbre_categories]
        self.cat_types = [c[1] for c in self.contimbre_categories]

    # ---------- binary access ----------
    def b(self, name):
        if name not in self.buf:
            self.buf[name] = open(os.path.join(self.dir, name + '.dat'), 'rb').read()
        return self.buf[name]

    @staticmethod
    def long(buf, off): return struct.unpack_from('<i', buf, off)[0]
    @staticmethod
    def flt(buf, off): return struct.unpack_from('<f', buf, off)[0]
    @staticmethod
    def cstr(buf, off):
        end = buf.index(b'\0', off)
        return buf[off:end].decode('latin-1')

    def strlist(self, buf, off):
        n = self.long(buf, off)
        return [self.cstr(buf, off + self.long(buf, off + 4 + 4 * i)) for i in range(n)]

    def longlist(self, buf, off):
        n = self.long(buf, off)
        return [self.long(buf, off + 4 + 4 * i) for i in range(n)]

    def floatlist(self, buf, off):
        n = self.long(buf, off)
        return [self.flt(buf, off + 4 + 4 * i) for i in range(n)]

    def value(self, sound_index, category):
        ci = self.categories.index(category)
        off = self.contimbre_data_indices[sound_index][ci]
        buf = self.b(category); t = self.cat_types[ci].upper()
        if t == 'LONG': return self.long(buf, off)
        if t == 'FLOAT': return self.flt(buf, off)
        if t == 'STRING': return self.cstr(buf, off)
        if t == 'STRINGLIST': return self.strlist(buf, off)
        if t == 'LONGLIST': return self.longlist(buf, off)
        if t == 'FLOATLIST': return self.floatlist(buf, off)
        raise ValueError(t)

    def short_filename(self, sound_index):
        return self.value(sound_index, 'filename_english')[50:]

    # ---------- ePlayer keyboards ----------
    def keygroup(self, inst_index, bank_offset):
        bk = self.b('eplayer_banks')
        nz = self.long(bk, bank_offset + 12)
        return {
            'model': self.inst_names[inst_index],
            'modes_index': self.long(bk, bank_offset),
            'note_menu_index': self.long(bk, bank_offset + 4),
            'pitch': self.long(bk, bank_offset + 8) * 0.001,
            'sounds': [self.long(bk, bank_offset + 16 + 4 * i) for i in range(nz)],
        }

    def n_modes(self, inst_index):
        return len(self.eplayer_keyboard_indices[inst_index])

    def keyboard(self, inst_index, mode_pos):
        a, z = self.eplayer_keyboard_indices[inst_index][mode_pos]
        offs = self.eplayer_keygroup_indices[inst_index][a:z]
        return [self.keygroup(inst_index, o) for o in offs]

    def mode_name(self, inst_index, mode_pos):
        a, z = self.eplayer_keyboard_indices[inst_index][mode_pos]
        off = self.eplayer_keygroup_indices[inst_index][a]
        first_sound = self.long(self.b('eplayer_banks'), off + 16)
        return self.value(first_sound, 'modes_english')

if __name__ == '__main__':
    db = CTDB(sys.argv[1], cache=sys.argv[2] if len(sys.argv) > 2 else None)
    print("instruments:", len(db.instruments), "| group/family/instrument rows:", len(db.group_family_instruments))
    print("categories:", len(db.categories), db.categories)
    print("sounds:", len(db.contimbre_data_indices))
    print("keyboards (instrument x technique):", sum(db.n_modes(i) for i in range(len(db.inst_names))))
