# Maket: test report

Maket 0.0.1, stage 0c. Machine: Intel(R) Xeon(R) Processor @ 2.10GHz, Linux. Date: 20 Nov 2026 7:59:45pm.

## 1. The test program

This program builds Maket's sources that hold no interface code (model, file formats, engines, notation) without the window, runs checks on them and writes this report. This first section shows that the program builds, runs, can work with files and reports a result. Every stage after it adds its own section.

| Check | Passed |
| --- | --- |
| The version reads as three numbers (`0.0.1`) | yes |
| The build stage reads as a step number and a letter (`0c`) | yes |
| A work folder can be made in the system's temporary folder and removed again | yes |
| A text file of 4114 bytes with accents and both kinds of line ending reads back byte for byte | yes |
| The report folder can be written | yes |

## 2. Settings

Whatever differs from one Mac to another is a setting with a default, kept in one JSON file. The checks below work on a settings file in the test's own work folder; the real settings file of this Mac is never touched by this program.

The settings and their defaults:

| Setting | Kind | Default | Name in the file |
| --- | --- | --- | --- |
| Folders: Projects | folder | `~/Documents/Maket` | `projectsFolder` |
| Folders: Imports | folder | `~/Documents/Maket/Imports` | `importsFolder` |
| Folders: Exports | folder | `~/Documents/Maket/Exports` | `exportsFolder` |
| Folders: conTimbre search data | folder | not set | `contimbreDataFolder` |
| conTimbre ePlayer (OSC): Host | host | `127.0.0.1` | `ePlayerHost` |
| conTimbre ePlayer (OSC): Port | port | `46001` | `ePlayerPort` |

| Check | Passed |
| --- | --- |
| Every setting has its own name, a label, a help text and a default that is valid | yes |
| Without a settings file every setting has its default, and no file is made | yes |
| Five changed settings are written to the file and read back the same by a fresh start | yes |
| A folder inside the home folder is kept as `~/...` and leads back to the same folder; one outside it keeps its whole path | yes |
| The file is JSON on several lines and holds every setting by name | yes |
| Thirteen values that make no sense (port 0 or 65536, a host with a space, a folder that is not a whole path ...) are refused and change nothing | yes |
| The edges are accepted: ports 1 and 65535, the host `localhost`, conTimbre's folder left unset | yes |
| A setting put back to its default has it again, and the others keep their values | yes |
| An entry this version does not know is left in the file when it is written again | yes |
| A value in the file that makes no sense is replaced by its default and reported; the rest is read | yes |
| A file that cannot be read is set aside under another name, reported, and the defaults are used | yes |

On this machine the app keeps its settings in `/root/.config/Maket/settings.json` (no file yet: nothing was changed in the app so far).

## 3. Test material

The files Joost puts in the repository's `Temp` folder are what the coming steps are tested on. This section lists what the program finds there; nothing is read yet. A missing file is not a failed check at this stage.

| File in `Temp` | Size |
| --- | --- |
| `01a doodsbed roll stats.txt` | 59.7 KB |
| `01b doodsbed roll resave after dump.txt` | 56.6 KB |
| `02a mixed_roll_example_01h roll stats` | 8.3 KB |
| `02b mixed_roll_example_01h resave after dump.txt` | 17.2 KB |
| `doodsbed final v1c.txt` | 56.1 KB |
| `mixed_roll_example_01h.txt` | 17.2 KB |

Needed for the coming stages:

| File | Needed for | Found |
| --- | --- | --- |
| doodsbed final v1c | 1a and 1b: a roll to read and to write back | yes, as `doodsbed final v1c.txt` |
| mixed_roll_example_01h | 1a and 1b: a roll to read and to write back | yes, as `mixed_roll_example_01h.txt` |
| 01a doodsbed roll stats | 1a: bach's own counts and its dump of every note | yes, as `01a doodsbed roll stats.txt` |
| 02a mixed_roll_example_01h roll stats | 1a: bach's own counts and its dump of every note | yes, as `02a mixed_roll_example_01h roll stats` |
| 01b doodsbed roll resave after dump | 1b: the roll as bach saves it again | yes, as `01b doodsbed roll resave after dump.txt` |
| 02b mixed_roll_example_01h resave after dump | 1b: the roll as bach saves it again | yes, as `02b mixed_roll_example_01h resave after dump.txt` |

All 16 checks passed.
