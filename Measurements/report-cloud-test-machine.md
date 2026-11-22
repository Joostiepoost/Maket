# Maket: test report

Maket 0.0.1, stage 1e. Machine: Intel(R) Xeon(R) Processor @ 2.10GHz, Linux. Date: 22 Nov 2026 11:14:08pm.

## 1. The test program

This program builds Maket's sources that hold no interface code (model, file formats, engines, notation) without the window, runs checks on them and writes this report. This first section shows that the program builds, runs, can work with files and reports a result. Every stage after it adds its own section.

| Check | Passed |
| --- | --- |
| The version reads as three numbers (`0.0.1`) | yes |
| The build stage reads as a step number and a letter (`1e`) | yes |
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
| Tests: Logging patch port | port | `46002` | `logPatchPort` |
| Tests: Test results | folder | `~/Documents/Maket/Exports/Timing` | `timingFolder` |

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

## 4. Writing bach rolls

Stage 1b: the writer of bach's text, and the writer that makes a bach.roll file of a roll. The layout and the rules for numbers and texts are bach's own, taken from its source. The test agreed for this stage: Joost's two rolls, read and written back, come out byte for byte as they were, each with the header it came with.

### The text writer

| Check | Passed |
| --- | --- |
| A real number is written with at most six decimals and always a point, a whole number without one (`7100.0`, `0.5`, `35407.0514`, `40452.095651`, `179244.0`, `-0.2875`, `0.333333`, `0.666667`, `0.0`, `117`) | yes |
| bach's layout: a tab per level, every list on its own line, a closing bracket on the last line of its list or alone after an inner list, no line end at the end; round brackets come out square | yes |
| What was written reads back to the same list, and is written the same again | yes |

Texts, and how each is written so that bach and Maket read it back as the same text:

| Text | Written as | Why |
| --- | --- | --- |
| `pizz` | `` pizz `` |  |
| `-eind` | `` -eind `` |  |
| `uitgegaan...` | `` uitgegaan... `` |  |
| `s3z1+2b.wav` | `` s3z1+2b.wav `` |  |
| `1` | `` `1 `` | would be read as a number |
| `7100.0` | `` `7100.0 `` | would be read as a number |
| `G8vb` | `` `G8vb `` | would be read as a pitch |
| `nil` | `` `nil `` | would be read as an empty list |
| `[x]` | `` `[x] `` | brackets |
| `gain (dB)` | `` `gain\ (dB) `` | brackets and a space |
| `a b` | `` "a b" `` | a space |
| `x,y` | `` "x,y" `` | a comma |
| `D'aarde is niet uit haar ` | `` "D'aarde is niet uit haar " `` | spaces, one at the end |
| `say "hi"` | `` "say \"hi\"" `` | a space and quotes |
| `Flöte 1` | `` "Flöte 1" `` | an accent is nothing special |
| `a\b` | `` "a\\b" `` | a backslash |
| `(empty)` | `` "" `` | an empty text |

| Check | Passed |
| --- | --- |
| All 17 texts above are written as expected and read back as themselves | yes |

### doodsbed final v1c

`doodsbed final v1c.txt`: 57474 bytes, 3327 lines.

| Check | Result | Passed |
| --- | --- | --- |
| Read into a list and written out: byte for byte the file | the same | yes |
| Read into a roll and written out from the roll: byte for byte the file, with the header it came with | the same | yes |
| Every number and word of the file, written from its value alone by bach's rules, comes out as the file spelled it | 5342 of 5342 | yes |
| With the velocity of the first note raised by 1 in the roll, the written text differs in exactly one line | 1 line | yes |
| Written as a file, `Temp/written by Maket/doodsbed final v1c.txt`: byte for byte the original | the same | yes |

Of the 5342 numbers and words written from the roll, 2824 come from the roll's values (onsets, pitches, durations, velocities, breakpoints, marker times, flags); the others are kept as the file spelled them (slot contents, header).

### mixed_roll_example_01h

`mixed_roll_example_01h.txt`: 17614 bytes, 946 lines.

| Check | Result | Passed |
| --- | --- | --- |
| Read into a list and written out: byte for byte the file | the same | yes |
| Read into a roll and written out from the roll: byte for byte the file, with the header it came with | the same | yes |
| Every number and word of the file, written from its value alone by bach's rules, comes out as the file spelled it | 1849 of 1849 | yes |
| With the velocity of the first note raised by 1 in the roll, the written text differs in exactly one line | 1 line | yes |
| Written as a file, `Temp/written by Maket/mixed_roll_example_01h.txt`: byte for byte the original | the same | yes |

Of the 1849 numbers and words written from the roll, 331 come from the roll's values (onsets, pitches, durations, velocities, breakpoints, marker times, flags); the others are kept as the file spelled them (slot contents, header).

### A roll changed by Maket

`Temp/written by Maket/mixed_roll_example_01h changed by Maket.txt` is `mixed_roll_example_01h.txt` with two changes made in the roll:

- the first note of voice 3 (`synth1a`, at 1.45 s) is 50 cents, a quarter tone, higher: 6975.0 cents became 7025.0;
- a marker named `Maket test (1b)` stands at 1.0 s. In the file its name is written `` `Maket\ test\ (1b) ``.

| Check | Passed |
| --- | --- |
| The file is written, and read back it holds the higher note and the marker | yes |
| With the two changes taken out again, it is byte for byte the original: nothing else was touched | yes |

The same for the two files as bach saved them again (for doodsbed that is the file with the longer header of the newer bach):

### doodsbed, re-saved by bach

`01b doodsbed roll resave after dump.txt`: 57987 bytes, 3350 lines.

| Check | Result | Passed |
| --- | --- | --- |
| Read into a list and written out: byte for byte the file | the same | yes |
| Read into a roll and written out from the roll: byte for byte the file, with the header it came with | the same | yes |
| Every number and word of the file, written from its value alone by bach's rules, comes out as the file spelled it | 5410 of 5410 | yes |
| With the velocity of the first note raised by 1 in the roll, the written text differs in exactly one line | 1 line | yes |

Of the 5410 numbers and words written from the roll, 2824 come from the roll's values (onsets, pitches, durations, velocities, breakpoints, marker times, flags); the others are kept as the file spelled them (slot contents, header).

### mixed example, re-saved by bach

`02b mixed_roll_example_01h resave after dump.txt`: 17614 bytes, 946 lines.

| Check | Result | Passed |
| --- | --- | --- |
| Read into a list and written out: byte for byte the file | the same | yes |
| Read into a roll and written out from the roll: byte for byte the file, with the header it came with | the same | yes |
| Every number and word of the file, written from its value alone by bach's rules, comes out as the file spelled it | 1849 of 1849 | yes |
| With the velocity of the first note raised by 1 in the roll, the written text differs in exactly one line | 1 line | yes |

Of the 1849 numbers and words written from the roll, 331 come from the roll's values (onsets, pitches, durations, velocities, breakpoints, marker times, flags); the others are kept as the file spelled them (slot contents, header).

## 5. OSC: messages and sending

Stage 1c: Maket's side of the connection to Max. An OSC message is made, turned into the bytes of a network packet, and sent at its time by a thread of its own.

### The bytes of a message

`noteon Fl 60. 100` as it travels, 28 bytes (in groups of four):

```
6e 6f 74 65  6f 6e 00 00  2c 73 66 69  00 00 00 00  46 6c 00 00  42 70 00 00  00 00 00 64
```

That is: the word `noteon` and two zero bytes; the kinds of the arguments `,sfi` (a text, a real number, a whole number) and four zero bytes; `Fl` and two zero bytes; 60.0; 100.

| Check | Passed |
| --- | --- |
| `noteon Fl 60. 100` gives the 28 bytes worked out by hand from the OSC description | yes |
| `all_notes_off`, a command without arguments, gives its 20 bytes | yes |
| The commands read `noteon`, `noteoff`, `program`, without a slash in front; a name with a space stays one argument | yes |
| Five messages (texts with spaces, with an accent, empty, of four letters; negative numbers) read back from their bytes as they were, each a multiple of four bytes long | yes |
| Bytes that are not such a message are refused | yes |

### Sending, on this computer

A listener inside the test program takes the place of Max. This shows that the messages leave whole and in order, and how punctual the send thread is. It says nothing about Max: that is measured with the logging patch.

| Check | Passed |
| --- | --- |
| Without a target nothing is sent; with one, 50 messages arrive at the listener one by one, each as it was sent | yes |
| 200 messages, one every 5 ms, all leave and all arrive, in order | yes |
| None leaves before its time (the earliest: -0.027 ms) | yes |
| 95 in 100 leave within 2 ms after their time (within 0.005 ms; the latest of all: 0.019 ms, a measurement and not a check, since a busy computer can hold any program up for a moment) | yes |
| Six messages with the same time (a chord) leave in the order they were given | yes |
| A note-off that is due in a minute is sent at once when asked; and what still waits can be dropped and `all_notes_off` sent in its place (as when Maket quits) | yes |

How punctual the send thread was in the run of 200 messages, in milliseconds after the time each was due (the thread has the system's realtime priority):

| | Half of them within | 95 in 100 within | The latest |
| --- | --- | --- | --- |
| Left Maket | -0.008 | 0.005 | 0.019 |
| Arrived at the listener in this program | 0.082 | 0.129 | 0.380 |

## 6. Timing tests: the measuring

Stage 1d measures on Joost's Mac how evenly Maket's messages arrive in Max. This section checks the measuring itself. A deviation is always: how far a message arrived from the time it was due, after the two clocks have been laid on each other at the middle message.

### The passages

The dense passage: 1403 notes in 23.0 seconds, so 2806 messages (a note-on and a note-off each):

- part 1: a line, 40 notes a second (240 notes)
- part 2: chords of eight notes, four a second (192 notes)
- part 3: irregular, about 95 notes a second (571 notes)
- part 4: a burst, 200 notes a second (400 notes)

The long passage: a note every 100 ms, 3000 notes in 5.0 minutes.

| Check | Passed |
| --- | --- |
| The passages are the same notes at every run, in order of time | yes |
| Written as a bach.roll file and read back, the dense passage holds the same notes, the chords as chords of eight | yes |

### Reading the logs, and the logs of 22 Nov

| Check | Passed |
| --- | --- |
| The patch's file of 22 Nov reads as 72 lines, each with its number, time and message | yes |
| Maket's send log of 22 Nov reads as 72 lines, each with its due time, lateness and message | yes |
| Both hold the same 72 messages in the same order | yes |
| The comparison of the two gives the numbers worked out separately: typical 0.2415 ms, half within 0.1420, 95 in 100 within 0.5257, the farthest 0.6643 | yes |

### The comparison, on made-up logs with known faults

| Check | Passed |
| --- | --- |
| A log without faults, on a clock that runs two minutes ahead: every message exactly on time, whatever the clocks say | yes |
| A log with a lost message, two that changed places, late ones and an early one: each fault is found and counted (199 of 200, 1 out of order, latest +0.50, earliest -2.00), per part of the passage too | yes |
| bach.roll's playout is read note by note and chord by chord, each line against the onset it carries | yes |
| An empty log gives no numbers, and says why | yes |

### A short run on this computer

The first three seconds of the dense passage go through the send thread to a listener inside the test program, which notes the arrival times on a clock of its own, as the patch does. This checks the whole chain of the measuring; the numbers say nothing about Max.

| Check | Passed |
| --- | --- |
| All 240 messages are found in the listener's log, none out of order | yes |
| 95 in 100 arrive within 2 ms of their time (within 0.09 ms; the farthest 2.91 ms) | yes |

```
Arrived: 240 of 240 messages; out of order: 0.
All messages: half within 0.02 ms of their time, 95 in 100 within 0.09 ms, 99 in 100 within 0.69 ms; the farthest 2.91 ms (earliest -0.05, latest +2.91).
The note-ons alone: half within 0.02 ms of their time, 95 in 100 within 0.07 ms, 99 in 100 within 1.75 ms; the farthest 2.91 ms (earliest -0.05, latest +2.91).
  Part 1 (a line, 40 notes a second): 95 in 100 within 0.09 ms, the farthest 2.91 ms.
Leaving Maket: 95 in 100 within 0.02 ms of their time, the latest 1.73 ms after it.
```

### The takes of 22 Nov on Joost's Mac

Every log in `Temp/Timing 1d`, compared here again: how far the messages arrived in Max from their time, in ms. The conditions are in the names Joost gave the takes.

| Take | Played by | Arrived | Half within | 95 in 100 within | 99 in 100 within | Farthest |
| --- | --- | --- | --- | --- | --- | --- |
| 20261122-192950 dense maket test1 - overdrive IO vector 512 Signal v | Maket | 2806 of 2806 | 0.05 | 0.42 | 0.54 | 2.71 |
| 20261122-193138 dense maket test1 - overdrive IO vector 512 Signal v | Maket | 2806 of 2806 | 0.08 | 0.33 | 0.53 | 8.20 |
| 20261122-193333 dense maket test1 - overdrive IO vector 1024 Signal  | Maket | 2806 of 2806 | 0.07 | 0.36 | 0.48 | 0.83 |
| 20261122-193530 dense maket test1 - overdrive IO vector 512 Signal v | Maket | 2806 of 2806 | 0.05 | 0.42 | 0.61 | 3.13 |
| 20261122-193624 dense maket test1 - overdrive IO vector 512 Signal v | Maket | 2806 of 2806 | 0.06 | 0.38 | 0.56 | 3.66 |
| 20261122-194644 dense maket test1 - overdrive IO vector 512 Signal v | Maket | 2806 of 2806 | 0.07 | 0.31 | 0.42 | 3.24 |
| 20261122-194818 dense maket test1 - overdrive IO vector 512 Signal v | Maket | 2806 of 2806 | 0.07 | 0.33 | 0.45 | 6.91 |
| 20261122-195010 dense maket test4 - overdrive off | Maket | 2806 of 2806 | 0.72 | 8.73 | 18.91 | 29.86 |
| 20261122-195207 dense maket test5 - overdrive on - scheduler in audi | Maket | 2806 of 2806 | 1.18 | 2.48 | 2.81 | 3.17 |
| 20261122-195814 dense maket test6b - overdrive on - io vector 512 -  | Maket | 2806 of 2806 | 0.05 | 0.35 | 0.52 | 0.69 |
| 20261122-200039 long maket test7 - overdrive on - io vector 512 - s | Maket | 6000 of 6000 | 0.04 | 38.88 | 96.71 | 99.36 |
| 20261122-200711 long maket test7b - overdrive on - io vector 512 -  | Maket | 6000 of 6000 | 0.03 | 0.12 | 0.18 | 0.42 |
| 20261122-204355 long maket test7d - overdrive on - io vector 512 -  | Maket | 6000 of 6000 | 0.03 | 0.12 | 0.16 | 0.64 |
| 20261122-213411 long maket Test8 - overdrive on - scheduler in audi | Maket | 6000 of 6000 | 0.06 | 69.78 | 97.40 | 103.10 |
| 20261122-214401 long maket Test 8b - conditions same as 8a - Maket  | Maket | 6000 of 6000 | 0.05 | 0.14 | 0.18 | 1.12 |
| 20261122-222212 long maket Test9 - usual settings - bringing other  | Maket | 6000 of 6000 | 0.05 | 0.16 | 0.22 | 12.99 |
| test2 from max OSC log | Maket | 2806 of 2806 | 0.07 | 0.31 | 0.42 | 3.24 |
| test2 from max OSC log | bach.roll | 1403 of 1403 | 0.03 | 0.15 | 0.20 | 5.27 |
| test3 from max OSC log | Maket | 2806 of 2806 | 0.07 | 0.33 | 0.45 | 6.91 |
| test3 from max OSC log | bach.roll | 1403 of 1403 | 0.03 | 0.12 | 0.18 | 8.57 |
| test4 from max OSC log (overdrive off) | bach.roll | 1403 of 1403 | 0.08 | 6.66 | 38.37 | 40.62 |
| test5 from max OSC log (overdrive on - scheduler in audio interrupt on) | bach.roll | 1403 of 1403 | 4.83 | 13.74 | 14.04 | 14.09 |
| test6b from max OSC log (overdrive on - scheduler in audio interrupt on) | Maket | 2806 of 2806 | 0.05 | 0.35 | 0.52 | 0.69 |

| Check | Passed |
| --- | --- |
| Five of these rows (takes 2, 5, 7 and 7d) give the numbers worked out separately | yes |

## 7. The conTimbre mapping

Stage 1e: every note of a roll becomes the commands for the conTimbre ePlayer that Joost's Max patch `cpe_poly` makes of it. Whether they are the same commands is settled on his Mac, against a log of the patch playing the roll; this section checks the mapping on notes worked out by hand and the comparison itself.

### The mapping, on doodsbed

The first note of the cello (`Vlc`) in the file: 6690 cents, 4511 ms long, velocity 1, at 7200.802301 ms; slot 1 `sul tasto`; slot 2 two points (0 ms: 0, 4267 ms: 16.846475); slot 3 one point (0 ms: 5.777778 dB); two pitch breakpoints (start: 0 cents, end: +50 cents). The commands Maket makes of it, in the order of the Max patch:

```
  7200.802 ms   program Vlc "sul tasto"
  7200.802 ms   gain Vlc 5.777778
  7200.802 ms   noteon Vlc 66.9 1
  7200.802 ms   dynamic Vlc 66.9 16.846475 4267.
  7200.802 ms   glissando Vlc 66.9 0.5 4511.
 11711.802 ms   noteoff Vlc 66.9
```

| Check | Passed |
| --- | --- |
| That note gives program, gain, note-on, dynamic and glissando at its onset, in that order, and its note-off at its end | yes |
| A dynamic curve of five points gives four commands, each to the value of the next point; the moment it is sent is rounded to a whole millisecond, its duration is not | yes |
| Three pitch breakpoints give two glissandi: to 0 semitones over the first stretch, then to 0.25 over the rest | yes |
| (The whole roll: 1188 commands: 306 note-ons and as many note-offs, 109 programs, 218 dynamics, 180 gains, 69 glissandi.) | |
| Every note that is not muted gives a note-on and a note-off, every filled slot and every breakpoint its commands, in order of time; the muted note and the voice without a name give none | yes |
| Playing from 60 seconds in: only the notes that begin from there on, and before them each voice gets the last program and the last gain it was given earlier (the cello: `program Vlc ordinario`) | yes |
| Asked for one voice, Maket makes the commands of that voice only | yes |

### A second piece: Bult

| Check | Passed |
| --- | --- |
| Bult, eleven voices of which seven are played (three have no name, one is muted): 2478 commands, 991 note-ons, 325 programs, 92 gains, 79 dynamics; pitches written as names (`G6`, `C#7`) become numbers | yes |

### Note-on options, a command in a marker, muted notes

| Check | Passed |
| --- | --- |
| The items of slot 4 follow the note-on; a marker named `cpe all_notes_off` sends that command; a muted note, a muted chord, a muted voice and a voice without a name send nothing | yes |

### The comparison with a log from Max, on made-up logs

| Check | Passed |
| --- | --- |
| A log of the same commands on another clock: all 1166 are paired, nothing is left over, and the commands for a voice that was not chosen (Mezzo) are named apart | yes |
| The same commands as they read after travelling as OSC (numbers of 32 bits, such as 66.900002 for 66.9) are still paired | yes |
| A log with one command lost and one velocity changed: both are found, and the changed note-on is shown beside Maket's | yes |
| A log in which a dynamic goes to `0` where Maket sends `0.`: still paired, and the difference in the kind of number is told (34 times) | yes |
| A log in which the cello's gain comes after its note-on instead of before: paired all the same, and the other order is told (12 moments) | yes |
| A log of another piece gives no result, and says why | yes |

## 8. Test material

The files Joost puts in the repository's `Temp` folder are what the coming steps are tested on. This section lists what the program finds there; nothing is read yet. A missing file is not a failed check at this stage.

| File in `Temp` | Size |
| --- | --- |
| `01a doodsbed roll stats.txt` | 59.7 KB |
| `01b doodsbed roll resave after dump.txt` | 56.6 KB |
| `02a mixed_roll_example_01h roll stats` | 8.3 KB |
| `02b mixed_roll_example_01h resave after dump.txt` | 17.2 KB |
| `1c maket log.txt` | 3.3 KB |
| `1c max log.txt` | 2.8 KB |
| `Bult - maquette final.txt` | 94.2 KB |
| `Timing 1d/20261122-192950 dense maket test1 - overdrive IO vector 512 Signal v max log.txt` | 97.8 KB |
| `Timing 1d/20261122-192950 dense maket test1 - overdrive IO vector 512 Signal v send log.txt` | 188.5 KB |
| `Timing 1d/20261122-193138 dense maket test1 - overdrive IO vector 512 Signal v max log.txt` | 98.6 KB |
| `Timing 1d/20261122-193138 dense maket test1 - overdrive IO vector 512 Signal v send log.txt` | 194.2 KB |
| `Timing 1d/20261122-193333 dense maket test1 - overdrive IO vector 1024 Signal  max log.txt` | 98.6 KB |
| `Timing 1d/20261122-193333 dense maket test1 - overdrive IO vector 1024 Signal  send log.txt` | 194.2 KB |
| `Timing 1d/20261122-193530 dense maket test1 - overdrive IO vector 512 Signal v max log.txt` | 97.9 KB |
| `Timing 1d/20261122-193530 dense maket test1 - overdrive IO vector 512 Signal v send log.txt` | 188.5 KB |
| `Timing 1d/20261122-193624 dense maket test1 - overdrive IO vector 512 Signal v max log.txt` | 97.7 KB |
| `Timing 1d/20261122-193624 dense maket test1 - overdrive IO vector 512 Signal v send log.txt` | 194.0 KB |
| `Timing 1d/20261122-194644 dense maket test1 - overdrive IO vector 512 Signal v max log.txt` | 158.8 KB |
| `Timing 1d/20261122-194644 dense maket test1 - overdrive IO vector 512 Signal v send log.txt` | 193.3 KB |
| `Timing 1d/20261122-194818 dense maket test1 - overdrive IO vector 512 Signal v max log.txt` | 150.0 KB |
| `Timing 1d/20261122-194818 dense maket test1 - overdrive IO vector 512 Signal v send log.txt` | 193.4 KB |
| `Timing 1d/20261122-195010 dense maket test4 - overdrive off max log.txt` | 97.8 KB |
| `Timing 1d/20261122-195010 dense maket test4 - overdrive off send log.txt` | 193.9 KB |
| `Timing 1d/20261122-195207 dense maket test5 - overdrive on - scheduler in audi max log.txt` | 97.8 KB |
| `Timing 1d/20261122-195207 dense maket test5 - overdrive on - scheduler in audi send log.txt` | 194.0 KB |
| `Timing 1d/20261122-195814 dense maket test6b - overdrive on - io vector 512 -  max log.txt` | 98.6 KB |
| `Timing 1d/20261122-195814 dense maket test6b - overdrive on - io vector 512 -  send log.txt` | 194.0 KB |
| `Timing 1d/20261122-200039 long maket test7 - overdrive on - io vector 512 - s max log.txt` | 216.7 KB |
| `Timing 1d/20261122-200039 long maket test7 - overdrive on - io vector 512 - s send log.txt` | 418.8 KB |
| `Timing 1d/20261122-200711 long maket test7b - overdrive on - io vector 512 -  max log.txt` | 216.7 KB |
| `Timing 1d/20261122-200711 long maket test7b - overdrive on - io vector 512 -  send log.txt` | 417.8 KB |
| `Timing 1d/20261122-204355 long maket test7d - overdrive on - io vector 512 -  max log.txt` | 216.7 KB |
| `Timing 1d/20261122-204355 long maket test7d - overdrive on - io vector 512 -  send log.txt` | 418.1 KB |
| `Timing 1d/20261122-213411 long maket Test8 - overdrive on - scheduler in audi max log.txt` | 218.4 KB |
| `Timing 1d/20261122-213411 long maket Test8 - overdrive on - scheduler in audi send log.txt` | 419.3 KB |
| `Timing 1d/20261122-214401 long maket Test 8b - conditions same as 8a - Maket  max log.txt` | 218.4 KB |
| `Timing 1d/20261122-214401 long maket Test 8b - conditions same as 8a - Maket  send log.txt` | 418.2 KB |
| `Timing 1d/20261122-222212 long maket Test9 - usual settings - bringing other  max log.txt` | 216.6 KB |
| `Timing 1d/20261122-222212 long maket Test9 - usual settings - bringing other  send log.txt` | 418.4 KB |
| `Timing 1d/test2 from max OSC log` | 181.8 KB |
| `Timing 1d/test3 from max OSC log` | 180.6 KB |
| `Timing 1d/test4 from max OSC log (overdrive off)` | 82.2 KB |
| `Timing 1d/test5 from max OSC log (overdrive on - scheduler in audio interrupt on)` | 81.6 KB |
| `Timing 1d/test6b from max OSC log (overdrive on - scheduler in audio interrupt on)` | 98.6 KB |
| `Timing 1d/timing details.txt` | 14.7 KB |
| `Timing 1d/timing passage dense.txt` | 54.1 KB |
| `Timing 1d/timing passage long.txt` | 124.9 KB |
| `Timing 1d/timing results.md` | 3.8 KB |
| `cpe%.maxpat` | 9.0 KB |
| `cpe_poly.maxpat` | 44.5 KB |
| `doodsbed final v1c.txt` | 56.1 KB |
| `mixed_roll_example_01h.txt` | 17.2 KB |
| `roll tests s1b.maxpat` | 100.8 KB |
| `written by Maket/doodsbed final v1c.txt` | 56.1 KB |
| `written by Maket/mixed_roll_example_01h changed by Maket.txt` | 17.2 KB |
| `written by Maket/mixed_roll_example_01h.txt` | 17.2 KB |

Needed for the coming stages:

| File | Needed for | Found |
| --- | --- | --- |
| doodsbed final v1c | 1a and 1b: a roll to read and to write back | yes, as `doodsbed final v1c.txt` |
| mixed_roll_example_01h | 1a and 1b: a roll to read and to write back | yes, as `mixed_roll_example_01h.txt` |
| 01a doodsbed roll stats | 1a: bach's own counts and its dump of every note | yes, as `01a doodsbed roll stats.txt` |
| 02a mixed_roll_example_01h roll stats | 1a: bach's own counts and its dump of every note | yes, as `02a mixed_roll_example_01h roll stats` |
| 01b doodsbed roll resave after dump | 1b: the roll as bach saves it again | yes, as `01b doodsbed roll resave after dump.txt` |
| 02b mixed_roll_example_01h resave after dump | 1b: the roll as bach saves it again | yes, as `02b mixed_roll_example_01h resave after dump.txt` |
| 1c maket log | 1d: Maket's send log of 22 Nov, to check the measuring | yes, as `1c maket log.txt` |
| 1c max log | 1d: the Max patch's log of 22 Nov, to check the measuring | yes, as `1c max log.txt` |

All 109 checks passed.
