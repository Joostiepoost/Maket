# Reference tools (Python)

Small scripts written while the Maquette proposal was prepared (November 2026). They are not part of the application. They record how each file format was read and checked, and they are the model for the C++ code of the build steps named below.

None of these files contains conTimbre data. The conTimbre scripts read the `search_data` folder of a conTimbre installation on the user's own disk; that folder is never copied here.

| Script | What it does | Ported at |
| --- | --- | --- |
| `ctdb.py` | Reads conTimbre's search database (`database.lisp`, `database eplayer.lisp` and the `.dat` files in `search_data`): instruments, techniques, key maps, values per sound. | step 14 |
| `ct_orc.py` | Reads and writes ePlayer orchestra files (`.cePlayerOrc`) in the player's exact text layout, and builds programs from the database: ordinary programs with 128 keys, and chord-mode programs (multiphonics, play and sing). | step 14 |
| `ct_catalogue.py` | Builds the catalogue of all techniques (instrument, technique, range, number of sounds) from the database. | step 14 |
| `ct_verify.py` | Checks: reading and rewriting an orchestra file gives the identical bytes; rebuilding its programs from the database does too. | step 14 |
| `omi_read.py` | Reads OpenMusic instance files (`.omi`) holding a chord-seq or a multi-seq. It parses the Lisp text as nested lists and evaluates nothing. | step 15 |
| `roll_survey.py` | Counts what a bach.roll text file holds: voices, notes, pitch grid, breakpoints, slots, markers. | step 1 |
| `maxpat_netlist.py`, `maxpat_comments.py`, `find_boxes.py` | Read a Max patch (`.maxpat`) as text: objects and connections, comments, objects matching a pattern. Used to study the ScAst patch. | not ported |

The scripts were run with `python3 -I`. Some hold paths of the workspace they were written in (`/home/claude/...`); pass your own paths where a script takes arguments, or change the constants at its top.

What the checks showed (November 2026): two of Joost's orchestra files are rebuilt from the database byte for byte, his orchestra with a multiphonic and a play-and-sing program likewise, and both of his OpenMusic examples (a chord-seq and a multi-seq) are read.
