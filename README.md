# Maket

A standalone macOS application for composing with snippets. A snippet is a passage of notes in proportional notation in which every note carries slots, a fragment of audio, or a curve. Snippets are placed and nested on a build surface, rendered into a roll that is edited by hand, and played through several engines at once: the conTimbre ePlayer over OSC, instrument plugins and MIDI ports, a simple synth, custom engines in Max, and audio tracks.

Maket grows out of the Max/bach patch ScAst beta 4. It takes code and working method from [Projet](https://github.com/Joostiepoost/Projet), [Trajet](https://github.com/Joostiepoost/Trajet), Effet and Goblet, and is the first of a new family of score-based applications with ties to OpenMusic and Max/MSP.

macOS, Apple Silicon only. Built with [JUCE](https://juce.com) 9 and CMake. The full design is in the Maket proposal doc.

**Where the build stands: stage 0c.** The window with its four tabs and the inspector exists, the console test program that checks every later step exists, and Maket keeps its settings. See *Status* below.

## Building

Requirements: Xcode (opened once, licence accepted), Homebrew, CMake (`brew install cmake`) — the same as for Projet, Trajet and Effet.

In Terminal, from this folder:

```
./build.sh
```

The first build downloads JUCE and takes a few minutes; later builds are much faster. The app opens when the build finishes, and lives at `build/Maket_artefacts/Release/Maket.app`.

If Terminal answers `permission denied`, run `chmod +x build.sh` once and try again.

To skip the JUCE download, point the build at the copy Projet (or Trajet) already downloaded:

```
cmake -B build -DMAKET_JUCE_PATH=../Projet/build/_deps/juce-src
./build.sh
```

(Adjust the path to where your Projet folder is. Only needed once; the setting is remembered.)

Other options:

- `./build.sh debug` — Debug build (slower, more checks)
- `./build.sh xcode` — generate and open an Xcode project, for debugging
- `./build.sh tests` — build and run the console test program (see *Using stage 0b* below)

## Status

Each step is cut into sub-stages that add one thing and are tested on Joost's Mac before the next begins. The sub-stages of a step are laid out when that step begins.

| Stage | Content | State |
| --- | --- | --- |
| 0a | Skeleton: repository, build script, the window with four empty tabs and the inspector | done |
| 0b | Skeleton: console test program and its report; this status table | done |
| 0c | Skeleton: preferences page and settings file (folders, OSC host and port) | done |
| 0d | Skeleton: buttons to hide and show the list and the inspector | done |
| 1a | Gate: list type and bach text reader | done |
| 1b | Gate: bach text writer | done |
| 1c | Gate: OSC sender, and a Max test patch that logs when each message arrives | done |
| 1d | Gate: timing tests of the OSC connection | |
| 1e | Gate: the full conTimbre mapping (program, dynamic, gain, glissando, markers) | |
| 1f | Gate: addressing by note, chords, tuning | |
| 2 | Roll view and transport: staves, microtonal accidentals, duration lines, breakpoints, markers; play through conTimbre and the synth | |
| 3 | Build surface, first part: tracks, audio snippets, the library with thumbnails, linked note blocks | |
| 4 | Roll editing: bach's gestures, linear entry, grids, copy and paste, undo, save | |
| 5 | Slot list and data: the project's slots, all built types, import mapping | |
| 6 | Slot editing: slot window, slot layer, function editor, dynamics slot | |
| 7 | Instruments: voice assignment, slot bindings, conTimbre settings, tuning, mixer | |
| 8 | Plugins and MIDI: hosting, MIDI ports, routing table, microtones | |
| 9 | Build surface, second part: containers and nesting, stretch, window, warp, transposition, voice map | |
| 10 | Render and freeze | |
| 11 | BPF snippets: curve editor, curves on the surface, baking into slots | |
| 12 | Custom engine and capture | |
| 13 | Quantize and musical time: quantizer, engraved preview, MusicXML | |
| 14 | Orchestra generator for conTimbre | |
| 15 | Interchange: bach shelf, OpenMusic instances, SPEAR, MIDI files | |
| 16 | Editing commands and scripting | |
| 17 | Sharing | |

## Using stage 0a

Stage 0a adds one thing: the main window.

- **Four tabs** along the top, in this order: Library and edition, Build surface, Render, Utilities. The tab in front is in bold with an amber line under it.
- **Switching tabs**: click a tab, press Cmd-1 to Cmd-4, or choose it in the **View** menu, which ticks the tab in front.
- **Each tab is empty.** Its page names the tab, says what will come there and at which build step.
- **The inspector** is the panel on the right. One inspector serves all four tabs; for now it names the tab in front and says what it will show there.
- The right end of the tab row reads `Maket 0.0.1` and the build stage, so that it is clear which build is open. The transport comes there at step 2.

Maket always opens on the first tab.

### Testing stage 0a

1. `./build.sh` finishes without an error and Maket opens.
2. The window's title is Maket, and the menu bar shows Maket next to the Apple menu.
3. The four tabs are there in the order above; Library and edition is in front.
4. A click on each tab brings it in front: the amber line moves, the page and the inspector change with it.
5. Cmd-1, Cmd-2, Cmd-3 and Cmd-4 do the same, and so do the four entries of the View menu.
6. The window can be resized, made small and made full screen; the inspector keeps its width on the right.
7. Cmd-Q quits.

## Using stage 0b

Stage 0b adds one thing: the console test program. It has no window. It builds the parts of Maket that hold no interface code, runs checks on them and writes a report. From step 1a on, every step adds its own section to that report: file round trips, counts against bach's, the timing of the OSC messages.

```
./build.sh tests
```

- The first run builds the program, which takes a minute or two. It uses the JUCE the app build already has, so nothing is downloaded again.
- Terminal shows the sections as they run and ends with a verdict, such as `All 5 checks passed.`, and the place of the report.
- **The report** is `Measurements/report.md`, a text file in Markdown. It is written again at every run. Open it in any text editor.
- At stage 0b the report has two sections. **The test program** has five checks: the version and the stage read correctly, a work folder can be made and removed, a file reads back byte for byte, the report folder can be written. **Test material**, always the last section, lists the files in `Temp` and says whether the files the coming stages need are there.
- `Measurements/report-cloud-test-machine.md` is the same report from the machine the code is written on, for comparison.

If you commit and push `Measurements/report.md` after a run, the results of your Mac can be read where the code is written. That matters from step 1c on, when the report holds timings that only your Mac with Max and the ePlayer can produce.

## Using stage 0c

Stage 0c adds one thing: Maket's settings, with a page to change them. Nothing in Maket points at a folder or a port that exists on one Mac only; whatever differs from one Mac to another is a setting with a default.

**Where.** The Utilities tab (Cmd-4). The list on the left chooses the page. Preferences is the only page so far; the others are listed in grey with the step that brings them.

**The settings.**

| Setting | Default | What it is for |
| --- | --- | --- |
| Projects | `~/Documents/Maket` | where new projects are made and where opening a project starts |
| Imports | `~/Documents/Maket/Imports` | where the file dialog starts when something is imported |
| Exports | `~/Documents/Maket/Exports` | where the file dialog starts when something is exported |
| conTimbre search data | not set | the `search_data` folder of conTimbre, for the orchestra generator (step 14) |
| Host | `127.0.0.1` | the computer on which Max runs with the ePlayer; 127.0.0.1 is this Mac |
| Port | `46001` | the port on which the Max patch listens |

Nothing uses these values yet. The host and the port come into use at stage 1c, the folders from step 2 on.

**Changing a setting.**

- Type in the field and press Return, or leave the field by Tab or a click elsewhere. Escape puts back what was there.
- For a folder, **Choose…** opens the usual folder dialog.
- **Default** puts a setting back to its default; the button is grey while the setting has its default.
- A change is saved at once; there is no Save button. A value still being typed when you quit is saved too.
- A value that makes no sense is refused: the field goes back to what it was, and a line under it says why. A port is a whole number from 1 to 65535; a host has no spaces; a folder is a whole path starting with `/` or `~`.
- The line under a folder shows where it is on this Mac and whether it exists. A folder need not exist yet.
- A folder inside your home folder is kept as `~/…`, so the settings do not carry your user name.

**The inspector** shows the setting whose field has the keyboard: what it is for, its default, and its name in the settings file.

**The settings file** is `~/Library/Application Support/Maket/settings.json`, plain text in JSON. It is made at the first change. **Show in Finder**, at the bottom of the page, leads to it. If the file is damaged, Maket sets it aside as `settings (unreadable).json`, starts with the defaults, and says so at the top of the preferences page.

**The test report** has a new section 2, Settings, with eleven checks on a settings file of its own; it never touches the real one. The report now has 16 checks in all.

### Testing stage 0c

1. `git pull`, then `./build.sh`. Maket opens, and the right end of the tab row reads `Maket 0.0.1 · stage 0c`.
2. Cmd-4 shows the Utilities tab: the list on the left with Preferences in front, and the settings with their defaults.
3. Click in the Port field. The inspector shows the setting. Type `7400` and press Return: the Default button of that row lights up.
4. Type `99999` in the Port field and press Return: the field goes back to `7400` and a red line says why.
5. Press **Choose…** next to Projects and pick any folder: the field shows it (with `~` if it is in your home folder), and the line under it says that the folder exists.
6. Type a host, for example `studio.local`, and quit with Cmd-Q **without** pressing Return.
7. Open Maket again (`./build.sh`) and go to the Utilities tab: port, folder and host are as you left them. This is the test of the stage.
8. **Show in Finder** shows `settings.json`; opened in a text editor, it holds the settings by name.
9. Press **Default** on the three rows you changed: they go back, and the buttons turn grey.
10. `./build.sh tests` ends with `All 16 checks passed.`

## Using stage 0d

Stage 0d adds one thing, asked for after 0c: the list on the left of a tab and the inspector on the right can be hidden, so that the page has the whole window.

- **Two buttons** at the right end of the tab row. Each draws a small window with a bar on the side of its panel: the left one is for the list, the right one for the inspector. The bar is filled while the panel shows and empty while it is hidden.
- **Shortcuts:** Cmd-Alt-L for the list, Cmd-Alt-I for the inspector (the same as the panel in Projet). Both are in the View menu too, which reads Hide or Show according to the state.
- **The list** is, for now, the list of pages in the Utilities tab. In a tab without a list the left button is grey. The choice holds for all tabs: when the library gets its list at step 3, the same button hides it.
- Maket opens with both panels showing. What was hidden is not remembered from one start to the next yet.

### Testing stage 0d

1. `git pull`, then `./build.sh`. The two buttons stand at the right end of the tab row.
2. Cmd-4, then click the left button: the list of pages goes away and the preferences take its place. Click again: it is back.
3. Click the right button: the inspector goes away and the page widens. Click again: it is back.
4. Cmd-Alt-L and Cmd-Alt-I do the same, and so do the two entries at the bottom of the View menu.
5. In another tab (Cmd-1) the left button is grey; the right one still works.

## Using stage 1a

Stage 1a is the first stage of step 1, the gate: the tests that must pass before anything is built on top. It adds what Maket needs to read a bach.roll file. Nothing changes in the window, apart from `stage 1a` at the right of the tab row; the work shows in the test report.

- **The list type.** bach keeps everything in nested lists of numbers and words. Maket now has the same: whole numbers, real numbers, fractions, pitch names, words and lists, to any depth. Every number and word keeps the way it was written in the file (`117.0` stays `117.0`), which stage 1b needs to write a file back byte for byte.
- **The reader of bach's text.** It follows bach's own rules, taken from bach's source: round and square brackets mean the same; a backtick makes a word of anything (bach writes `` `1 `` for the text 1, and `` `G8vb `` because `G8vb` alone would be read as a pitch); quotes and backslashes keep spaces; `nil` is an empty list and `null` is nothing. A file with a bracket too many or too few is refused, with the line.
- **The roll reader.** It turns the list of a saved bach.roll into Maket's roll: the header (slot definitions, clefs, voice names, MIDI channels, markers, and every entry Maket does not use yet, kept as it is), then voices, chords and notes, each note with its pitch, duration, velocity, breakpoints, slots and flags (locked, muted, solo).
- **The test** holds the reader against bach itself, with the files Joost made in Max and put in `Temp`: bach's counts of voices, chords, notes and markers, and bach's dump of every note it plays. Each dumped note is looked up in Maket's roll and compared in full: duration, velocity, breakpoints and the content of every slot.

Two things to know. bach's dump leaves out muted notes (one in doodsbed), so the test expects 306 of 307 there. And bach writes a dynamics slot differently when it plays than when it saves (`[ [ auto 0 0.5 ] mf > ]` against `[ auto mf > ]`); the test brings the first back to the second before comparing.

### Testing stage 1a

1. `git pull`, then `./build.sh`: Maket opens as before, with `stage 1a` at the right of the tab row.
2. `./build.sh tests` ends with `All 47 checks passed.`
3. Open `Measurements/report.md` and look at section 3, *Reading bach rolls*: for each of the two rolls a table with Maket's reader on the left, bach on the right, and `yes` in every row. Below each table, one line says what was read from the roll: how many notes glide, and which slots are filled in how many notes. Check that line against what you know of the two pieces.
4. Push `Measurements/report.md`, as before.

## Using stage 1b

Stage 1b adds the other direction: Maket can write a roll as a bach.roll file. Again nothing changes in the window, apart from `stage 1b`; the work shows in the test report and in three files the test writes.

- **The text writer** lays a list out as bach.roll's `writetxt` does: a tab per level, every list on a line of its own, square brackets, no line end after the last line. The layout and the rules below are bach's own, taken from its source.
- **Numbers and texts.** What was read from a file and not changed is written exactly as the file spelled it. What Maket makes itself is written by bach's rules: a real number with at most six decimals and always a point (`7100.0`), a whole number without one (`88`), a text with a backtick or in quotes when it needs one to be read back as a text (`` `gain\ (dB) ``, `"baan getreden"`).
- **The roll writer** builds the file the way bach builds it when it saves: the header entries in the order they came in, then the voices; in each note the pitch, duration, velocity, breakpoints, slots and flag. Onsets, pitches, durations, velocities, breakpoints, marker times and flags are written from the roll's values, not copied from the text they were read from. A header keeps the entries it came with; none are added.
- **The test** is the one agreed for this stage: each of the two rolls, read and written back, is byte for byte the file it was. It is done through the list alone and through the roll, and also for the two files bach saved again. As a further check, every number and word in the files is written from its value alone and compared with the file's spelling: 5342 of 5342 in doodsbed, 1849 of 1849 in the mixed example.

`./build.sh tests` writes three files into `Temp/written by Maket/` (a folder git leaves alone):

- `doodsbed final v1c.txt` and `mixed_roll_example_01h.txt`, the same byte for byte as your originals;
- `mixed_roll_example_01h changed by Maket.txt`, the mixed example with two changes made in the roll: a marker named `Maket test (1b)` at 1.0 s, and the first note of voice 3 (`synth1a`, at 1.45 s) 50 cents higher (6975 became 7025 cents). This is the first file bach reads that bach did not write itself.

### Testing stage 1b

1. `git pull`, then `./build.sh`: Maket opens as before, with `stage 1b` at the right of the tab row.
2. `./build.sh tests` ends with `All 71 checks passed.` Section 4 of `Measurements/report.md`, *Writing bach rolls*, has `yes` in every row.
3. In Max, open `Temp/written by Maket/doodsbed final v1c.txt` and `Temp/written by Maket/mixed_roll_example_01h.txt` in a bach.roll (`read`). They should look and play as your own files do; since they are the same bytes, bach cannot tell them apart.
4. In Max, open `Temp/written by Maket/mixed_roll_example_01h changed by Maket.txt`. It should open without a message in the Max window and show a marker `Maket test (1b)` at 1 second, and the first note of the third voice (`synth1a`) a quarter tone higher than in the original (6975 became 7025 cents). Everything else should be as in the original.
5. Push `Measurements/report.md`, as before.

## Using stage 1c

Stage 1c is Maket's first word to Max: it sends OSC messages to the conTimbre ePlayer. It adds a test page in Maket and a small logging patch for Max.

- **The OSC test page** is a new entry at the bottom of the list in the Utilities tab (the list is now clickable). It sends a single note, or a run of 8 rising notes (one every 250 ms, 200 ms long), to the voice you name. A note is `noteon <voice> <pitch> <dynamic>` now and `noteoff <voice> <pitch>` after its duration; with a program name filled in, `program <voice> <name>` goes first. The pitch is a MIDI note number with decimals (60 is middle C, 60.5 a quarter tone higher), as the ScAst patch sends it.
- **Where the messages go** is set on the Preferences page: the ePlayer's host and port (127.0.0.1 and 46001), and a new setting, the logging patch's port (46002).
- **The send log** on the page shows, for every message, when it was due, how many milliseconds after that it left Maket, and where it went (P the ePlayer, L the logging patch). **Save log…** writes it as a text file.
- **A thread of its own** sends each message at its time, so a busy window does not hold a message up. **All notes off** sends `all_notes_off` and drops what still waits. When Maket quits while a note sounds, its note-off is sent at once.
- **One thing the test had to settle:** how the command word must be written. Maket writes `noteon`, as Max's own udpsend object does, and the ePlayer takes that (tested 22 Nov). The option *Write the commands with a slash* (`/noteon`) is therefore not needed and goes at the next stage.

A message that left Maket has not necessarily arrived: this kind of network message gets no answer. Whether Max got it shows in Max, by the sound and in the logging patch.

### The logging patch

`Max/Maket OSC log.maxpat` listens on port 46002 and gives every message that arrives a number and the time on the computer's clock, in milliseconds since the last **reset**. The lines are kept in a `coll`: double-click it to see them, click **write** to save them as a text file. A line reads `12, 3410.52 noteon voice1 60. 100;`. It does not pass anything on to the ePlayer: Maket sends each message twice, once to the ePlayer and once to this patch.

The patch was written outside Max; it opened and worked in Max 9 on 22 Nov. Should it ever need rebuilding, this is what it holds:

- `udpreceive 46002` into `t l b b`.
- The right outlet of the `t` into `cpuclock`, that into the left inlet of `- 0.`, that into the right inlet of `pack 0 0.`.
- The middle outlet of the `t` into the left inlet of `int`. The `int` into `+ 1`, and `+ 1` back into the right inlet of the `int`. The `int` also into the left inlet of the `pack`.
- The `pack` into `prepend set`, that into `prepend 0`. The left outlet of the `t` into the same `prepend 0`. The `prepend 0` into `coll maket_osc_log`.
- For the reset: a message `reset` (and `loadbang`) into `t b b b`. Its right outlet into a second `cpuclock`, that into the right inlet of `- 0.`. Its middle outlet into a message `1`, that into the right inlet of the `int`. Its left outlet into a message `clear`, that into the `coll`.
- A message `write` into the `coll`.

### Testing stage 1c

1. `git pull`, then `./build.sh`: `stage 1c` at the right of the tab row. `./build.sh tests` ends with `All 82 checks passed.`
2. In Max, open the ePlayer with an orchestra, as you do for ScAst. Check that the port it listens on is the port on Maket's Preferences page (Cmd-4, Preferences).
3. In Max, open `Max/Maket OSC log.maxpat`. Look at the Max window: no error should show.
4. In Maket: Cmd-4, then **OSC test** at the bottom of the list. In *Voice*, type the name of a voice of the loaded orchestra, exactly as the ePlayer shows it.
5. **Play note.** A middle C should sound for one second. If nothing sounds, tick *Write the commands with a slash* and play again. Which of the two works is the first thing to report.
6. Type a program name of that voice in *Program* and play again: the voice should change to it.
7. Change the pitch to `60.5`: a quarter tone higher.
8. Click **reset** in the logging patch, then **Play 8 notes** in Maket: a rising run of eight short notes. Maket's log shows 16 lines (8 note-ons, 8 note-offs); the number in the patch runs to 16; double-clicking the `coll` shows 16 lines with their times.
9. Save both logs into the repository's `Temp` folder: **write** in the patch (as `1c max log.txt`), **Save log…** in Maket (as `1c maket log.txt`).
10. Set the duration to `10000`, play a note and quit Maket (Cmd-Q) while it sounds: the note should stop at once.
11. Push `Measurements/report.md` and the two logs.

## The repository

```
build.sh            builds and opens Maket; with "tests", builds and runs the test program
CMakeLists.txt      what is built, and with which JUCE
Source/             the application
  Main.cpp            the application object and its window
  MainComponent.*     everything inside the window
  UI/                 interface code: tab row, panel buttons, pages, preferences, inspector, colours
  Model/              no interface code: the settings; the name, version and build stage; bach's list type; the roll
  Formats/            no interface code: reading and writing files (bach's text, bach.roll)
  Engine/             no interface code: OSC messages, the sender and its thread, the ePlayer's commands
Max/                patches for Max that belong to Maket (the OSC logging patch)
Tests/              the console test program
Measurements/       its reports
Reference/python/   scripts from the design phase (file formats, checks); not part of the app
Temp/               Joost's test material, test saves, crash reports and screen captures
```

`Source/` gains `Notation/` when its step comes. `Model/`, `Formats/`, `Engine/` and `Notation/` hold no interface code, so that the test program can build them without the window.

## Licence

Maket is published under the GNU Affero General Public License, version 3 (see `LICENSE.md`), the licence under which JUCE is used here.

Nothing of conTimbre is in this repository or in the app. Each user needs their own conTimbre and points Maket at it.
