# Maket: test report

Maket 0.0.1, stage 1a. Machine: Apple M4, Mac OSX 27.2.1. Date: 21 Nov 2026 2:52:19pm.

## 1. The test program

This program builds Maket's sources that hold no interface code (model, file formats, engines, notation) without the window, runs checks on them and writes this report. This first section shows that the program builds, runs, can work with files and reports a result. Every stage after it adds its own section.

| Check | Passed |
| --- | --- |
| The version reads as three numbers (`0.0.1`) | yes |
| The build stage reads as a step number and a letter (`1a`) | yes |
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

On this machine the app keeps its settings in `/Users/jvkmuz/Library/Application Support/Maket/settings.json` (the file exists).

## 3. Reading bach rolls

Stage 1a: the list type bach's data comes in, the reader of bach's text, and the reader that makes a roll of it. The rules of the text are bach's own, taken from its source. Joost's two rolls are then checked against what bach itself says about them: its counts, and its dump of every note it would play.

### The text reader

Single words, read by bach's order of trial (whole number, fraction, real number, pitch name, symbol):

| Word | Read as | Value | Note |
| --- | --- | --- | --- |
| `117` | whole number | 117 |  |
| `-5` | whole number | -5 |  |
| `7100.0` | real number | 7100 | 117 and 117.0 are kept apart |
| `-0.2875` | real number | -0.2875 |  |
| `0.` | real number | 0 |  |
| `.5` | real number | 0.5 |  |
| `1e-3` | real number | 0.001 |  |
| `1/4` | fraction | 0.25 |  |
| `-3/2` | fraction | -1.5 |  |
| `C5` | pitch | 6000 | middle C |
| `Eb4` | pitch | 5100 |  |
| `c#5` | pitch | 6100 |  |
| `Dq5` | pitch | 6250 | quarter sharp |
| `Dd5` | pitch | 6150 | quarter flat |
| `C^5` | pitch | 6025 | eighth sharp |
| `Cv5` | pitch | 5975 | eighth flat |
| `bb4` | pitch | 5800 | B flat |
| `C5+1/4t` | pitch | 6050 | a quarter of a tone up |
| `G8vb` | pitch | 10175 | so bach writes this clef name with a backtick |
| `pizz` | symbol |  |  |
| `sul.pont` | symbol |  |  |
| `-eind` | symbol |  |  |
| `uitgegaan...` | symbol |  |  |
| `s3z1+2b.wav` | symbol |  |  |
| `12abc` | symbol |  |  |
| `1.2.3` | symbol |  |  |
| `L1a` | symbol |  |  |
| `Fl` | symbol |  |  |

| Check | Passed |
| --- | --- |
| All 28 words above are read as bach reads them | yes |
| A backtick makes a symbol of anything (`` `1 `` is the text 1, `` `gain\ (dB) `` is "gain (dB)"); quotes and a backslash keep spaces | yes |
| A quoted text keeps its apostrophe and the space at its end; a quote inside it is kept by a backslash | yes |
| Round and square brackets mean the same (bach's older files use round ones) | yes |
| `nil` is an empty list and `null` is nothing | yes |
| A bracket that is never closed, one that closes nothing, and a quote that never ends are refused, with the line | yes |
| Every atom keeps the way it was written, so `117.0` is written back as `117.0` | yes |

### doodsbed final v1c

`doodsbed final v1c.txt`: 57474 characters, 5342 numbers and words, lists 6 deep. Length of the music: 179.2 s. Header entries: slotinfo, commands, clefs, keys, voicenames, groups, markers, stafflines, midichannels, articulationinfo, noteheadinfo, numparts, loop.

| | Maket's reader | bach | The same |
| --- | --- | --- | --- |
| Every character of the file that is not a space comes back, in order, when the list is written out | yes |  | yes |
| Voices | 8 | 8 | yes |
| Chords per voice | 11 1 46 44 30 58 49 13 | 11 1 46 44 30 58 49 13 | yes |
| Notes | 307 | 307 | yes |
| Notes in each chord of each voice | compared one by one |  | yes |
| Markers, each with its time, name, role and content | 49 | 49 | yes |
| Notes bach sends when it plays everything (a muted note is not sent) | 306 = 307 - 1 muted | 306 | yes |
| Sent notes found in Maket's roll, in the same voice at the same onset and pitch | 306 | 306 | yes |
| Of those, with the same duration, velocity, breakpoints and slot contents | 306 | 306 | yes |
| The channel bach sends with each note is the voice's MIDI channel in the header | 306 | 306 | yes |
| Markers bach sends, each with its time, name and role | 49 | 49 | yes |
| The roll as bach saved it again reads to the same voices, chords, notes and markers | yes | bach added to the header: voicespacing, hidevoices, notationstyles | yes |

Read from it: 57 notes glide, with 126 breakpoints between them; 341 slots are filled: slot 1 Timbre (text) in 110 notes; slot 2 Modulation (function) in 89 notes; slot 3 gain (dB) (function) in 124 notes; slot 8 slot filelist (filelist) in 11 notes; slot 22 articulations (articulations) in 1 note; slot 23 notehead (notehead) in 6 notes.

### mixed_roll_example_01h

`mixed_roll_example_01h.txt`: 17614 characters, 1849 numbers and words, lists 6 deep. Length of the music: 7.6 s. Header entries: slotinfo, commands, clefs, keys, voicenames, voicespacing, hidevoices, groups, markers, stafflines, midichannels, notationstyles, articulationinfo, noteheadinfo, numparts, loop.

| | Maket's reader | bach | The same |
| --- | --- | --- | --- |
| Every character of the file that is not a space comes back, in order, when the list is written out | yes |  | yes |
| Voices | 18 | 18 | yes |
| Chords per voice | 2 1 2 2 3 2 1 1 1 2 1 2 2 1 0 2 0 0 | 2 1 2 2 3 2 1 1 1 2 1 2 2 1 0 2 0 0 | yes |
| Notes | 25 | 25 | yes |
| Notes in each chord of each voice | compared one by one |  | yes |
| Markers, each with its time, name, role and content | 0 | 0 | yes |
| Notes bach sends when it plays everything (a muted note is not sent) | 25 = 25 - 0 muted | 25 | yes |
| Sent notes found in Maket's roll, in the same voice at the same onset and pitch | 25 | 25 | yes |
| Of those, with the same duration, velocity, breakpoints and slot contents | 25 | 25 | yes |
| The channel bach sends with each note is the voice's MIDI channel in the header | 25 | 25 | yes |
| Markers bach sends, each with its time, name and role | 0 | 0 | yes |
| The roll as bach saved it again reads to the same voices, chords, notes and markers | yes | saved byte for byte the same | yes |

Read from it: 8 notes glide, with 27 breakpoints between them; 49 slots are filled: slot 1 cpe_timbre (text) in 8 notes; slot 2 modulation (function) in 16 notes; slot 3 cpe_gain-presence dB (function) in 3 notes; slot 6 waveshape (function) in 6 notes; slot 10 vibrato (function) in 2 notes; slot 20 dynamics (dynamics) in 3 notes; slot 22 articulations (articulations) in 2 notes; slot 29 fixed cc (llll) in 1 note; slot 30 iac port & ch (llll) in 8 notes.

## 4. Test material

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

All 47 checks passed.
