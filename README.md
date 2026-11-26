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
| 1d | Gate: timing tests of the OSC connection | done |
| 1e | Gate: the full conTimbre mapping (program, dynamic, gain, glissando, markers) | done; tested by Joost on 23 Nov. **The gate is passed** (Joost, 23 Nov) |
| 1f | Addressing by note, tuning (chords: abandoned on 24 Nov) | done, in the parts below |
| 1f.1 | The ways of the Max patch by switch (order, glissando to 0, spacing), to hear whether any of them matters; the comparison sets apart what is known of the patch | done; tested by Joost on 23 Nov: no difference, Maket keeps its own way |
| 1f.2 | A study of the ePlayer: one figure sent in ten ways (by pitch or by number, timing of the dynamics, dynamic correction), to find out what makes it follow its ramps | done; run by Joost on 23 Nov |
| 1f.3 | The second run of the study: is a note by number louder as such; which part of the timing matters by number; do slow starts come from the note before | done; run by Joost on 23 Nov |
| 1f.4 | A roll played by number (Maket's default from here on); the third run of the study: fast repeated notes, a glide by number, a chord-mode program | done; run by Joost on 24 Nov |
| 1f.5 | By number, the target of a dynamic ramp as a whole number; any command typed on the OSC test page; the fourth run of the study: one ramp per note in each form, plucked repeated notes | done; tried by Joost on 24 Nov: the form makes no difference, a ramp by number is a step from where the note is |
| 1f.6 | By number, a dynamic ramp sent as a step from the level before; the fifth run of the study: how such steps behave (it replaces the fourth run) | set aside on 24 Nov: the idea of steps did not hold (see 1f.7) |
| 1f.7 | By number, the target of a dynamic ramp on the ePlayer's scale from 0 to 1; the sixth run of the study: that scale beside the ramps by pitch, on the cello and the bass clarinet (it replaces runs four and five) | done; run by Joost on 24 Nov: by number and by pitch give the same dynamics |
| 1f.8 | Tuning: the standard pitch as a setting, sent to the ePlayer (`kammerton`) at the start of every play; a tuning check of 48 seconds | done; run by Joost on 24 Nov: the A lands on the frequency sent, within a third of a cent |
| 2 | Roll view and transport: staves, microtonal accidentals, duration lines, breakpoints, markers; play through conTimbre and the synth | in parts |
| 2a | A roll opens in *Library and edition*: staves and clefs per voice, notes with stems and duration lines in proportional time, a time ruler, zoom and scroll; on paper, as bach.roll. No accidentals yet | done; tested by Joost on 25 Nov. 2a.1: the direction of a stem by bach's own rule |
| 2b | Accidentals: bach's sign for every step of the eighth-tone grid, placed left of the chord by bach's method; the semitone grid as the other choice; a note the file gives by name keeps its letter | done; tested by Joost on 25 Nov. 2b.1: clusters as bach's small tree; velocity shown in grey, colour or alpha |
| 2c | Glides (pitch breakpoints) along the duration line, markers with their names and roles, the colours of what is locked, muted or solo | done; tested by Joost on 25 Nov |
| 2d | Transport on conTimbre: a playhead, play from any point, play prescheduled, stop, a loop region; voice names and the percussion clef as in ScAst | done; tested by Joost on 25 Nov |
| 2e | Sound from Maket itself: the audio output as a setting, a first plain synth (one sine per note), and per voice the choice of conTimbre, synth or silent | done; tested by Joost on 26 Nov. On his Mac the synth waits 91 ms for the ePlayer (*Synth timing*) |
| 2f | The closing of step 2: speed with a very large roll (measured, and made faster where it was slow), the looks gone through once more | built 26 Nov; to be tested by Joost |
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
- **One thing the test had to settle:** how the command word must be written. Maket writes `noteon`, as Max's own udpsend object does, and the ePlayer takes that (tested 22 Nov). The option to write them with a slash (`/noteon`) was therefore removed at stage 1d.

A message that left Maket has not necessarily arrived: this kind of network message gets no answer. Whether Max got it shows in Max, by the sound and in the logging patch.

### The logging patch

`Max/Maket OSC log.maxpat` listens on port 46002 and gives every message that arrives a number and the time on the computer's clock, in milliseconds since the last **reset**. The lines are kept in a `coll`: double-click it to see them, click **write** to save them as a text file. A line reads `12, 3410.52 noteon voice1 60. 100;`. It does not pass anything on to the ePlayer: Maket sends each message twice, once to the ePlayer and once to this patch.

The patch was written outside Max; its first version opened and worked in Max 9 on 22 Nov. Stage 1d added to it (see *Using stage 1d*). Its core, should it ever need rebuilding:

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
5. **Play note.** A middle C should sound for one second.
6. Type a program name of that voice in *Program* and play again: the voice should change to it.
7. Change the pitch to `60.5`: a quarter tone higher.
8. Click **reset** in the logging patch, then **Play 8 notes** in Maket: a rising run of eight short notes. Maket's log shows 16 lines (8 note-ons, 8 note-offs); the number in the patch runs to 16; double-clicking the `coll` shows 16 lines with their times.
9. Save both logs into the repository's `Temp` folder: **write** in the patch (as `1c max log.txt`), **Save log…** in Maket (as `1c maket log.txt`).
10. Set the duration to `10000`, play a note and quit Maket (Cmd-Q) while it sounds: the note should stop at once.
11. Push `Measurements/report.md` and the two logs.

## Using stage 1d

Stage 1d is the measurement the gate is about: do Maket's messages arrive in Max evenly enough to play music with, also when there are many of them, for a long time, and while Max is busy? And how does that compare with bach.roll playing the same notes inside Max, as ScAst does now?

- **The Timing test page** is a new entry in the list of the Utilities tab. One take is one click: Maket tells the logging patch to start a fresh log, plays a fixed passage, tells the patch where to write its log, reads that file, and files the result.
- **Two passages.** *Dense*: 1403 notes in 23 seconds, in four parts: a line of 40 notes a second, chords of eight notes four times a second, an irregular part of about 95 notes a second, and a burst of 200 notes a second. *Long*: a note every 100 ms for five minutes.
- **What is measured.** For every note-on and note-off Maket sends a numbered tick to the logging patch, and, when the box is ticked, the real note to the ePlayer, so that the ePlayer is at work as in a piece. The patch notes when each tick arrives. Maket's clock and Max's clock start at different moments, so the delay itself cannot be measured, only the evenness: the arrival times are shifted as a whole until the middle message sits exactly on its time, and the result says how far the others lie from theirs.
- **The reference.** *Write the passages as bach.roll files* writes both passages as bach.roll files. The new patch `Max/Maket bach reference.maxpat` holds a bach.roll whose playout goes to the same logging patch. *Start a bach.roll take* freshens the log; you press play in Max; *Finish the bach.roll take* has the log written and read. When bach.roll plays while Maket runs, both are measured in the one take.
- **The results** go to the folder set as *Test results* (at this stage still called *Timing results*) on the Preferences page: `timing results.md` (one line per take), `timing details.txt` (the same in words, with every part of the passage apart), and per take the patch's log and Maket's send log. They also show on the page.
- **The logging patch** now obeys two words from Maket, `reset` and `write <file>`, and logs what any patch sends to `s maket_log_in`. Its buttons still do both by hand.
- **Removed:** the option to write commands with a slash. When Maket quits while messages still wait, they are dropped and the ePlayer gets `all_notes_off`.

How to read a result: *half within 0.05 ms* means that half of the messages arrived no more than 0.05 ms from their time; *95 in 100 within* and *99 in 100 within* likewise; *the farthest* is the single worst one. For the ear, differences under a millisecond or two between notes that should sound together are not heard as such; the farthest ones and the chords (part 2) are what to look at.

### Testing stage 1d

Before the takes:

1. `git pull`, then `./build.sh`: `stage 1d`. `./build.sh tests` ends with `All 95 checks passed.`
2. Cmd-4, Preferences, *Timing results*: **Choose…** a new folder `timing 1d` inside the repository's `Temp` folder, so that the files can be pushed.
3. In Max: the ePlayer with an orchestra, audio on, as for ScAst. Open `Max/Maket OSC log.maxpat` (the new version) and `Max/Maket bach reference.maxpat`. No error should show in the Max window.
4. In Maket: Cmd-4, **Timing test**. Click **Write the passages as bach.roll files**. In the reference patch, click **read** and choose `timing passage dense.txt` in the timing folder. The passage should show in the bach.roll.

Then the takes. Before each, type in *Condition* what the state of Max is; the result is filed under those words. *Voice* is a voice of the loaded orchestra; the dense passage is not music, so keep the dynamic low, or untick the ePlayer box if it is too much.

| Take | Max | What to do in Maket |
| --- | --- | --- |
| 1 | your usual settings; say in the condition what they are: Overdrive on or off, Scheduler in Audio Interrupt on or off (Audio Status window), the signal vector size | **Run from Maket** |
| 2 | the same | **Start a bach.roll take**, press **play** in the reference patch, wait until it has played to the end, **Finish the bach.roll take** |
| 3 | the same, both at once | **Run from Maket** and press **play** in the reference patch right after it |
| 4 | Overdrive off (Options menu) | **Run from Maket**; then a bach.roll take |
| 5 | Overdrive on, Scheduler in Audio Interrupt the other way than usual | **Run from Maket**; then a bach.roll take |
| 6 | usual settings, Max kept busy: during the run, drag and resize a Max window, or scroll through a large roll | **Run from Maket** |
| 7 | usual settings, the Mac left alone | choose the *Long* passage, **Run from Maket** (five minutes) |

After the takes: push the `timing 1d` folder and `Measurements/report.md`.

### After the first takes (22 Nov): what they showed and what changed

The takes are in `Temp/Timing 1d`; the test report lists them all in its section 6, compared once more.

- **With Overdrive on** (and Scheduler in Audio Interrupt off) Maket's messages arrive as evenly as bach.roll's own notes: 99 in 100 within about half a millisecond for both. A rare single message, about one in 1600, arrives 1 to 8 ms late; in the takes where both played at once, bach.roll's late notes fall at the same moments on Max's clock, so those are moments at which Max itself was held up.
- **With Overdrive off** both are uneven (95 in 100 within 7 to 9 ms, single ones 30 to 40 ms). Overdrive must be on, as it must for ScAst.
- **With Scheduler in Audio Interrupt on** Maket's messages land within 3 ms; bach.roll's own notes within 14 ms.
- **Five minutes untouched:** 6000 messages, the farthest 0.4 and 0.6 ms from its time.
- **The one fault found** (take 7): when another app was used while Maket played, macOS slowed Maket down and messages left up to 99 ms late. Maket now tells macOS that it is busy for as long as messages wait (`Source/UI/KeepAwake.h`, `Source/Platform/BusyActivity`), and its `Info.plist` asks not to be napped at all. A first attempt, with a class of JUCE's, had no effect and crashed Maket five seconds after the last message (take 8): that class lets go of the system's token too early. Maket now holds the token itself.
- **Waiting for Max's log:** Max writes its log when its interface is free, so not while a window is being dragged. Maket now goes on waiting instead of giving up after 8 seconds; **Stop** ends the wait, and **Read the log again** files the take later.

Two things to do with this version:

1. `git pull`, `./build.sh`, and `./build.sh tests`, which ends with `All 95 checks passed.`
Take 9 (22 Nov, 22:22) confirmed it: five minutes with other apps in front, every message left Maket within 0.32 ms of its time, and no crash. In that take 12 of the 6000 messages arrived in Max between 1 and 13 ms late although they had left on time: the Mac was busy with the other apps.

The steps of that last test, for the record:

2. **Take 9:** your usual settings, the *Long* passage, **Run from Maket**. After about a minute, bring another app to the front and use it for a minute or two, as in takes 7 and 8a. The stream should stay even, and in the result *Leaving Maket … the latest* should stay under a millisecond. Maket should not crash at the end; wait ten seconds after the last note to be sure.
3. Push the `Timing 1d` folder and `Measurements/report.md`. Should Maket crash again: the report macOS writes is in `~/Library/Logs/DiagnosticReports` (a file whose name starts with `Maket`); copy it into `Temp`.

## Using stage 1e

Stage 1e is the full conTimbre mapping: every note of a roll becomes the series of commands for the ePlayer that the Max patch `cpe_poly` makes of it. Until now Maket could only send single test notes.

- **What a note becomes** (the voice's name comes first in every command; the pitch is the note's pitch in cents divided by 100):

  | In the roll | Command | When |
  | --- | --- | --- |
  | slot 1, the timbre | `program <voice> <name>` | at the onset, before the note |
  | slot 3, the gain curve | `gain <voice> <dB>`, one for every point | at the time of each point |
  | the note | `noteon <voice> <pitch> <velocity>`, followed by the first two items of slot 4 when it is filled | at the onset |
  | | `noteoff <voice> <pitch>` | at the end |
  | slot 2, the dynamic curve | `dynamic <voice> <pitch> <value of the next point> <ms to it>`, one for every point but the last | at the time of each point, rounded to a whole millisecond |
  | pitch breakpoints | `glissando <voice> <pitch> <semitones at the next breakpoint> <ms to it>`, one for every breakpoint but the last | at each breakpoint, rounded to a whole millisecond |
  | a marker named `cpe ...` | what follows `cpe`, as a command of its own | at the marker |

  Muted notes, chords and voices send nothing, and neither does a voice without a name. The value of the first point of the dynamic curve is not sent: the note starts at its velocity, as in the patch.
- **The conTimbre test page** is a new entry in the list of the Utilities tab. **Choose…** a roll file (or drop one on the page). The voices that have a name appear with a tick each; the table under *Results* says what each voice gives.
- **Play** sends the roll to the ePlayer; **Stop** ends it and sends `all_notes_off`. *from, in seconds* starts in the middle: the notes that begin from there on, and before them each voice gets the last timbre and the last gain it was given earlier, so that a passage sounds the same as in a run from the start.
- **The message-by-message check.** The logging patch has a new part, the *tap*: while it is on, every command that a patch in Max sends to the ePlayer (to `s contimbre_eplayer`, where `cpe_poly` ends) is logged too. **Start a ScAst take** empties the log and switches the tap on; you play the roll in ScAst; **Finish the ScAst take** switches the tap off, has the log written, and holds Maket's own commands for the roll against it. The result says, per kind of command, how many are the same on both sides; what only ScAst sends; what only Maket sends; for a command without a partner, Maket's nearest one of that kind beside it; whether commands due at the same moment come in the same order; and whether a number is whole on one side and real on the other.
- **Files**, in the folder set as *Test results* on the Preferences page (the setting that was called *Timing results*): per take the patch's log (`... scast max log.txt`) and Maket's own commands with their times (`... scast maket commands.txt`), and `mapping results.txt`, to which every comparison is added. **Compare with a log file…** does the comparison for a log saved by hand.
- `MaketTests --compare <roll file> <log file>` does the same comparison in Terminal.

What is not in this stage: chords as one command and the addressing of a note by a number of its own (1f), and anything about tuning.

### Testing stage 1e

Before the tests:

1. `git pull`, then `./build.sh`: `stage 1e`. `./build.sh tests` ends with `All 109 checks passed.` Should git refuse to pull because of `Max/Maket OSC log.maxpat` (Max saved it once more on your Mac): `git checkout -- "Max/Maket OSC log.maxpat"`, then pull again.
2. Cmd-4, Preferences, *Test results*: **Choose…** a new folder `Mapping 1e` inside the repository's `Temp` folder.
3. In Max: ScAst with doodsbed, and the ePlayer with the `doodsbed kwintet` orchestra, as you play it. Close the old logging patch and open the new `Max/Maket OSC log.maxpat`; no error should show in the Max window.

**Test 1, the check, command by command:**

4. In Maket: Cmd-4, **conTimbre test**, **Choose…** `Temp/support materials/doodsbed final v1c.txt`. It must be the roll that is open in ScAst; if that one has changed since, save it from ScAst and choose that file. The table should list eight voices, 306 notes in all.
5. **Start a ScAst take.** In the logging patch the switch of the tap goes on.
6. In ScAst: play the roll from its very start to its end, as you usually do.
7. In Maket: **Finish the ScAst take.** The result appears under *Results*, and the status line says in one sentence how it went: either *The same commands on both sides*, or how many differ.

**Test 2, by ear:**

8. **Play** in Maket, and compare with ScAst playing the same roll: timbres, dynamics, glissandi, balance. *from, in seconds* takes you to a spot.
9. **Stop** in the middle: silence at once.

**Test 3, if you like, the second piece:** the same two tests with the Bult roll and its orchestra.

After the tests: push the `Mapping 1e` folder and `Measurements/report.md`. Whatever the check shows, the files say exactly what differs, so there is nothing you need to write down beyond what you hear.

Please keep the orchestra files (`.cePlayerOrc`) out of the repository: they hold conTimbre's sample names and the repository is public. Git now ignores files with that ending.

### After the test of 23 Nov

Joost's four takes are in `Temp/Mapping 1e`. Every `program`, `gain`, `noteon` and `noteoff` of both pieces is the same in Maket and in ScAst. Three things differ, all on ScAst's side: ScAst sends a `glissando` to 0 for every note without pitch breakpoints; in a chord, ScAst sends every `dynamic` and `glissando` with the pitch of the chord's last note, so that the lower note of a double stop gets no curve (Maket gives each note its own); and what goes through a delay in the patch (`gain`, `dynamic`, `glissando`) comes after the note-on. The `set_window_titles` in the logs is the ePlayer's own.

The test material now lives in `Temp/support materials`; the test program looks there first.

## Using stage 1f.1

The check of stage 1e showed three ways in which the stream of the Max patch differs from Maket's, and the recordings of the cello's opening left one thing open: over its first four notes Maket's takes were a few dB louder than ScAst's. Stage 1f.1 makes each of the three ways a switch, so that Maket can send exactly as the patch does and the difference can be looked for by ear and by measurement.

- **Three switches on the conTimbre test page**, under the voices; all off is Maket's own way:
  - *note-on before gain, dynamic, glissando*: the patch's order. Of the notes of a voice that begin together, first every program and note-on, then every gain, dynamic and glissando. Maket's own order is, per note: program, gain, note-on, dynamic, glissando.
  - *glissando to 0 without breakpoints*: the patch sends `glissando <voice> <pitch> 0. <duration>` for every note that has no pitch breakpoints.
  - *0.1 ms between commands*: the commands of one voice that are due at the same moment leave 0.1 ms after each other, as they do from Max. Otherwise Maket sends them as fast as it can, about 0.03 ms apart.
- The switches hold for **Play** and for the comparison alike. What stays Maket's in every case: in a chord each note gets its own dynamic and glissando (the patch sends them with the pitch of the chord's last note).
- **Every run writes a send log** in the *Test results* folder (`... maket play send log.txt`): its first line says which way was played, and for every command when it was due and how long after that it left Maket.
- **The comparison keeps apart what is known of the Max patch**: its glissandi to 0, the chord commands with the last note's pitch, `all_notes_off` at a stop, and what the ePlayer's own Max side sends (`set_window_titles`, `end_dialog`). What is left over after that is new. Your takes of 23 Nov are now part of the test program: `./build.sh tests` compares both of them and expects nothing left over.

### Testing stage 1f.1

1. `git pull`, then `./build.sh`: `stage 1f.1`. `./build.sh tests` ends with `All 117 checks passed.`
2. Cmd-4, Preferences, *Test results*: **Choose…** a new folder `Order 1f1` inside the repository's `Temp` folder.
3. ScAst with doodsbed and the ePlayer with the `doodsbed kwintet` orchestra, as on 23 Nov. In Maket: **conTimbre test**, **Choose…** `Temp/support materials/doodsbed final v1c.txt`, and untick every voice but `Vlc`.
4. Record the cello's opening nine times, in this order, each time from the start until just after the note at 35 s has begun (about 40 s), stopping at the same place every time so that the ePlayer is in the same state when the next take begins:

   | Take | Played by | Switches |
   | --- | --- | --- |
   | S1, S2, S3 | ScAst | |
   | A1, A2, A3 | Maket | all three off |
   | B1, B2, B3 | Maket | all three on |

   in the order S1, A1, B1, S2, A2, B2, S3, A3, B3. Names with those letters and numbers in them are enough.
5. Send the nine recordings, and push the `Order 1f1` folder (Maket's send logs of the six runs) and `Measurements/report.md`.

What the recordings will say: if the B takes lie with ScAst's and the A takes apart, one of the three ways is heard, and the switches tell which; if A and B lie together and ScAst's apart, it is something outside the commands; if all nine lie within each other's spread, the difference of 23 Nov was chance.

### After the test of 1f.1 (23 Nov)

Nine recordings of the cello's opening (three from ScAst, three from Maket its own way, three from Maket in the patch's ways) cannot be told apart by group: two takes of one kind lie as far from each other as takes of different kinds, and in each kind about one dynamic ramp in five ends 5 dB or more short of the best take. Maket keeps its own way. The switches stay on the conTimbre test page for now.

## Using stage 1f.2

The ePlayer makes about one dynamic ramp in five only in part, whoever sends it. Stage 1f.2 is a study of that: Maket plays one short figure many times and changes one thing at a time, so that one recording shows whether anything on Maket's side makes the player follow more surely. It is also the first time Maket addresses a note by a number of its own.

- **The ePlayer study page**, a new entry in the list of the Utilities tab.
- **The figure**: two notes of one voice, the second beginning while the first still sounds, each with a crescendo (the second in three ramps). Four seconds per figure.
- **Ten ways of sending it**, played in turns:
  - by pitch as Maket sends now; timed as ScAst (the first `dynamic` 0.7 ms after the note-on, each next ramp 1.4 ms before the end of the one before);
  - the first `dynamic` 5 ms and 20 ms after the note-on;
  - each next ramp 0.6 ms after, and 5 ms before, the end of the one before;
  - the note addressed by number (`ctnote`, `ctdynamic`, `ctnoteoff`), as Maket sends now and timed as ScAst;
  - by pitch and by number with the player's dynamic correction off. Maket sends `dynamic_correction` before every figure and switches it back on at the end of the run and at Stop.
- **Addressing by number** needs a voice whose name is `voice` and a number. The study plays on `voice1` with the program `sul tasto`; both can be changed on the page.
- **Files** in the *Test results* folder: `... player study plan.txt` (the ways, and when each figure begins) and `... player study send log.txt` (when each command left).

### Testing stage 1f.2

1. `git pull`, then `./build.sh`: `stage 1f.2`. `./build.sh tests` ends with `All 126 checks passed.`
2. Cmd-4, Preferences, *Test results*: **Choose…** a new folder `Study 1f2` inside the repository's `Temp` folder.
3. In the ePlayer: load the orchestra `Maket test 1f2`, which Claude made from your own doodsbed orchestra and sent in the chat (not in this repository). It has two voices with the same cello programs, `Vlc` and `voice1`. Leave the dynamic correction at its default.
4. In Maket: Cmd-4, **ePlayer study**.
   - **Try one figure by pitch**: two notes, each growing louder.
   - **Try one figure by number**: the same should sound. If nothing sounds, or the ePlayer complains: untick *with the ways that address a note by number* and say so; the study then runs with the other ways.
5. Start a recording, then **Run the study**: 200 figures, 13 minutes 20. It needs no attention; the status line says where it is. Stop the recording when the status says the run is over.
6. Send the recording, and push the `Study 1f2` folder and `Measurements/report.md`. Afterwards load your own orchestra in the ePlayer again.

### After the run of 1f.2 (23 Nov)

- The ePlayer takes a note by number over OSC (`ctnote`, `ctdynamic`, `ctnoteoff`) as Maket sends it.
- The dynamic correction makes no measurable difference; it stays at its default.
- By pitch, when the dynamics are sent does not help: none of the timings makes the figures more regular.
- By number a note starts far more evenly (10 % of the figures low early in the first note, against 31 % by pitch), and with Maket's own tight timing it is the most regular way of all (2 of 20 figures low anywhere); timed as ScAst it is the least regular (13 of 20).
- By number sounds louder at soft dynamics (about 7 dB early in the first note, equal at the top of the crescendo).

## Using stage 1f.3

A second run on the same page, **Run the second study**, to settle what the first left open. Played in turns, 10 minutes 56:

- **Single notes without any dynamic**, 1.5 s long, at the velocities 15, 36, 60, 100 and 127, by pitch and by number: is a note by number louder as such, or does its crescendo set in sooner?
- **The figure by number in four timings**, beside the figure by pitch: as Maket sends now; timed as ScAst; only the first `dynamic` as ScAst (0.7 ms after the note-on); only the next ramps as ScAst (1.4 ms before the end of the one before). Which of the two distances makes it irregular?
- **The figure by pitch after three seconds of silence**: do the slow starts come from the note before?

Files: `... player study 2 plan.txt` and `... player study 2 send log.txt` in the *Test results* folder.

### Testing stage 1f.3

1. `git pull`, then `./build.sh`: `stage 1f.3`. `./build.sh tests` ends with `All 129 checks passed.`
2. *Test results* may stay on `Study 1f2`. In the ePlayer: the orchestra `Maket test 1f2`, as for 1f.2.
3. Start a recording, then Cmd-4, **ePlayer study**, **Run the second study**. Stop the recording when the status says the run is over.
4. Send the recording, and push the folder and `Measurements/report.md`.

### After the run of 1f.3 (23 Nov)

- Without a dynamic the ePlayer plays a note exactly the same every time (eight repeats within 0.1 dB), and by number exactly as by pitch. All unevenness lies in its dynamic ramps; nothing Maket sends differently changes it.
- A ramp does not end where a note struck at that value lies: a note struck at 15 and ramped to 60 ends about 12 dB up (by number about 17 dB), where a note struck at 60 is 5.4 dB up.
- **Decided by Joost on 23 Nov: by number is Maket's default way of addressing conTimbre**, by pitch the fallback. Maket is for new pieces; older ones stay with ScAst.

## Using stage 1f.4

- **A roll by number.** On the conTimbre test page, the switch *by number* sends every note with a number of its own (`ctnote`, `ctdynamic`, `ctglissando`, `ctnoteoff`). The voice at place k of the roll then plays on the ePlayer's voice named `voice` + k, whatever its name in the roll; `program` and `gain` go to that name. For doodsbed: the cello on `voice3`, flute `voice4`, clarinet `voice5`, viola `voice6`, the piano's hands `voice7` and `voice8`. Where instruments get their voice number in the finished Maket is a matter for step 7.
- **The third run of the study** (**Run the third study** on the ePlayer study page, 4 minutes 20), on the same orchestra:
  - one pitch struck eight times, every 120 ms and every 70 ms, the notes overlapping by 30 ms, meeting exactly, or 20 ms apart, by pitch and by number;
  - a glide of two semitones by pitch (`glissando`) and by number (`ctglissando`);
  - a note with the number 2000000;
  - the flute's program `harmonic`, which is in chord mode: `chordon voice4 60. 79.` beside a plain note-on.

### Testing stage 1f.4

1. `git pull`, then `./build.sh`: `stage 1f.4`. `./build.sh tests` ends with `All 136 checks passed.`
2. In the ePlayer: load the orchestra `doodsbed by number`, which Claude made from your doodsbed orchestra and sent in the chat (not in this repository): the same voices and programs, named `voice3` to `voice8`.
3. **By ear:** conTimbre test page, the doodsbed roll, untick `Mezzo`, tick *by number*, **Play**. It should be doodsbed as you know it, with stronger dynamic curves. Listen for the piano's repeated notes and anything that hangs or is cut short. **Stop** in the middle: silence at once?
4. **The run:** start a recording, ePlayer study page, **Run the third study**; stop the recording when the status says the run is over.
5. Send the recording and what you heard at step 3; push the *Test results* folder and `Measurements/report.md`.

### After the run of 1f.4 (24 Nov)

- **Dynamics by number went wrong, and the form of a number is the suspect.** In the recording of doodsbed by number, a note that has a dynamic curve goes to about the same loud level after every ramp, whatever the ramp's target; only a ramp to 0 comes down. The cello's low note at 62 s (struck at 70, ramps to 22, 14, 11, 2) sounds 20 to 30 dB louder than by pitch. Maket sent the target of `ctdynamic` as a real number (`22.4`), as it does for `dynamic` by pitch; the measurements fit a player that takes such a target for the top. The second run fits it too: there the ramps by number lay about 5 dB over those by pitch at the start of a crescendo and met them at its top. So "the ramps by number are stronger and start more evenly", reported after 1f.2 and 1f.3, was this fault and no gain. Stage 1f.5 sends the target as a whole number and tests it.
- Notes without a curve are right by number (piano, short notes), no note hung after the end, and all 295 notes got their note-off.
- A glide by number equals a glide by pitch (both two semitones, the same course in all 16). A note with the number 2000000 sounds and ends as any other.
- Repeated notes on a bowed sound: where the notes do not overlap, by pitch and by number are the same sound to 0.0 dB at one note every 120 ms; at one every 70 ms a few items of both have a hole (5 of 16 by pitch, 2 of 16 by number: too few to tell them apart). Where the notes overlap by 30 ms, by pitch is steady and by number varies from item to item (two notes of one sample sound at once and add up differently each time). A bowed sound has no attacks to count, so this goes once more on a plucked one in 1f.5.
- `chordon voice4 60. 79.` was refused by the ePlayer ("chord_on: bad parameters"), and a plain note-on on the chord-mode program gave silence. The right form is not known; the typed command line of 1f.5 is there to try forms by hand.

## Using stage 1f.5

- **The target of a ramp by number as a whole number.** On the conTimbre test page, under *by number*, a second switch (on by default): a ramp to 22.4 is sent as `ctdynamic <number> 22 <duration>`. Off: as a real number, as in 1f.4.
- **Any command, typed** (OSC test page): a line such as `ctdynamic 1 20 1000.` is sent as it stands when you press Return. `20` is a whole number, `20.` a real one, anything else text; a text with spaces goes in double quotes (`program voice3 "sul tasto"`). Several commands with `;` between them leave at the same moment, in the order typed. The list under it shows what was sent.
- **The fourth run of the study** (**Run the fourth study**, 6 minutes 14), on the orchestra `doodsbed by number`:
  - one note of 1.9 s on the cello (`sul tasto`), struck at 40 or at 100; 0.2 s later one ramp over 0.8 s to 0, 10, 40, 80 or 127: by pitch, by number with the target as a whole number, by number with the target as a real number, and the struck note without any ramp (4 of each);
  - one pitch plucked eight times (`pizz`), every 120 or 70 ms, overlapping by 30 ms or 20 ms apart, by pitch and by number (6 of each).

### Testing stage 1f.5

1. `git pull`, then `./build.sh`: `stage 1f.5`. `./build.sh tests` ends with `All 143 checks passed.`
2. In the ePlayer: the orchestra `doodsbed by number`, as for 1f.4.
3. **By ear, one minute** (OSC test page, the typed line; Return after each):
   - `program voice3 "sul tasto"`
   - `ctnote 3 1 60. 60 20000.` : a cello note that goes on sounding;
   - `ctdynamic 1 20 1000.` : it should get softer over a second;
   - `ctdynamic 1 20. 1000.` : the same target with a point. If the idea is right, it now gets loud instead;
   - `ctdynamic 1 20 1000.` once more, then `ctnoteoff 1`.
4. **By ear:** conTimbre test page, the doodsbed roll, untick `Mezzo`, tick *by number* (the switch under it stays on), **Play**. Are the dynamics now those you know from ScAst?
5. **The run:** start a recording, ePlayer study page, **Run the fourth study**; stop the recording when the status says the run is over.
6. **Chords, if you have five minutes** (the typed line again; `All notes off` between tries). Which of these gives the flute's harmonic, and what does the ePlayer say to each?
   - `program voice4 harmonic` first;
   - `noteon voice4 60. 80 ; noteon voice4 79. 80` (two plain note-ons at the same moment);
   - `chordon voice4 60 79`
   - `chordon voice4 60. 79. 80`
   - `chordon voice4 80 60. 79.`
7. Send the recording and what you heard at steps 3, 4 and 6; push the *Test results* folder and `Measurements/report.md`.

### After the first try of 1f.5 (24 Nov)

Joost, by ear with the typed line, on a note struck at 33: `ctdynamic 1 20 1000.` and `ctdynamic 1 20. 1000.` both made it louder, `ctdynamic 1 -40 1000.` made it softer, and so on through thirteen commands. So the form of the number is not the cause, and `ctdynamic` does not go to a target as the manual says: it moves the note from where it is, up for a number above 0 and down for one below. That explains the recording of doodsbed by number as well: all numbers of a dynamic curve are above 0, so every ramp went up until the top was reached.

## Using stage 1f.6

- **A ramp by number as a step.** On the conTimbre test page, under *by number*, a new switch (on by default): each ramp is sent as the way from the level before it. A note struck at 70 whose curve goes to 22, then 14, gets `ctdynamic <number> -48 ...`, then `-8`. The steps of a note, added to its velocity, end exactly on its last target. This assumes that a step counts in the same units as a velocity; the fifth run measures whether it does.
- **The fifth run of the study** (**Run the fifth study**, 7 minutes 24), on the orchestra `doodsbed by number`, the cello on `voice3` (`sul tasto`). It replaces the fourth run.
  - a note struck at 60 with one step of -60, -40, -20, -10, 0, +10, +20, +40, +60;
  - steps from a note struck at 30 and at 100 that land on the same sums;
  - a step as a real number; a step over 0.2 s and over 1.5 s;
  - three steps one after the other: +20 +20 +20; +30 -30 +30; -30 -30 +60; +40 0 0; +60 +60 -60; -60 -60 +60 (do they add up, come back, stop at the top and at the bottom, and what does 0 do);
  - beside them the ramps by pitch from 60 to 0, 20, 40, 50, 70, 80, 100, 120, and the notes struck at 30, 60, 100 alone;
  - one pitch plucked eight times (`pizz`), every 120 or 70 ms, overlapping or apart, by pitch and by number.
  Four of each, in turns.

### Testing stage 1f.6

1. `git pull`, then `./build.sh`: `stage 1f.6`. `./build.sh tests` ends with `All 146 checks passed.`
2. In the ePlayer: the orchestra `doodsbed by number`.
3. **By ear:** conTimbre test page, the doodsbed roll, untick `Mezzo`, tick *by number* (the two switches under it stay on), **Play**. Are the dynamics those you know from ScAst?
4. **The run:** start a recording, ePlayer study page, **Run the fifth study**; stop the recording when the status says the run is over.
5. The chord forms of 1f.5, step 6, if not yet tried.
6. Send the recording and what you heard; push the *Test results* folder and `Measurements/report.md`.

### After the tries of 24 Nov (Joost, by hand with the typed line)

- **`ctdynamic` goes to an absolute target, as the manual says, but on a scale from 0. to 1.**, not from 0 to 127 as `dynamic`. That accounts for everything seen that day: any target of 1 or more is the top (the doodsbed recording by number; "20 makes it louder"), anything under 0 is the bottom ("-40 makes it softer"; the bass clarinet that knew only its softest and its loudest), and 0 is silence. The ideas of 1f.5 (the form of the number) and 1f.6 (steps from the level before) are set aside; their study runs stay in the code but left the page.
- **Chord mode is abandoned.** `chordon` takes whole numbers only, so only tempered pitches, which does not fit multiphonics. Joost keeps such programs in note mode and picks the key by hand, as in ScAst. Maket will not send `chordon`.

## Using stage 1f.7

- **By number, a ramp's target from 0 to 1.** Maket divides the roll's target (0 to 127) by 127 when it sends `ctdynamic`; the roll itself is not changed. On the conTimbre test page this is the one switch under *by number*, on by default (off: the roll's target as it stands, as in 1f.4).
- **The sixth run of the study** (**Run the sixth study**, 6 minutes 37), on the orchestra `doodsbed by number`: the cello on `voice3` (`sul tasto`) and the bass clarinet on `voice5` (`ordinario`, pitch 50). On each:
  - a note struck at 40 with one ramp by number to 0, 0.1, 0.25, 0.5, 0.75 or 1, and beside each the ramp by pitch to 127 times that (0, 12.7, 31.75, 63.5, 95.25, 127); the same from a note struck at 100 to 0.1, 0.5 and 1;
  - the notes struck at 40 and at 100 alone;
  - by number, three ramps in a row to 0.25, 0.75, 0.25 (does it come back) and to 0.5, 0.5, 0.5 (does it stay).
  And the cello's pitch plucked eight times (`pizz`), every 120 or 70 ms, overlapping or apart, by pitch and by number. Three of each ramp, four of each plucked way, in turns.

### Testing stage 1f.7

1. `git pull`, then `./build.sh`: `stage 1f.7`. `./build.sh tests` ends with `All 147 checks passed.`
2. In the ePlayer: the orchestra `doodsbed by number`.
3. **By ear:** conTimbre test page, the doodsbed roll, untick `Mezzo`, tick *by number* (the switch under it stays on), **Play**. Are the dynamics those you know from ScAst?
4. **The run:** start a recording, ePlayer study page, **Run the sixth study**; stop the recording when the status says the run is over.
5. Send the recording and what you heard at step 3; push the *Test results* folder and `Measurements/report.md`.

### After the run of 1f.7 (24 Nov)

- **The scale holds.** A ramp by number to a target from 0 to 1 lands where the ramp by pitch to 127 times it lands: over the 18 pairs the difference is -0.2 dB in the middle, and 0.0 to 0.6 dB wherever the takes are steady. The bass clarinet is graded like the cello.
- **Doodsbed by number and by pitch are the same** within what two takes of one way differ: -0.2 dB in the middle, within 3 dB for 86 % of the time, one place of half a second further apart.
- **Ramps stray equally in both ways:** 9 of 54 by number and 11 of 54 by pitch end 3 dB or more from the middle of their kind. It is the ePlayer's own variation, known since 1e.
- **Where a ramp ends depends on its target only**, not on the velocity the note was struck at; and a ramp to 0 is not silence but the softest dynamic (on the cello about 2 dB under a note struck at 40, on the bass clarinet 9 dB).
- **A later ramp goes to its own target** from wherever the note is (0.25, 0.75, 0.25 comes back); a repeated target leaves the note where it is.
- **Plucked repeated notes:** all eight attacks sound in every item, every 120 and every 70 ms, by pitch and by number alike. With the bowed notes of 1f.4, no measurement shows by number to repeat notes better or worse than by pitch.

- The one place where the two doodsbed recordings lay further apart (43.3 s) was looked into by Joost: the version by number is the right one there; in the version by pitch a note took over a gain meant for the next note and became too loud again. Filed as an exception with a known cause.

## Using stage 1f.8

- **The standard pitch** (Preferences, under *Tuning*): the frequency of the A above middle C, 442 Hz by default, which is also the ePlayer's own default. From 380 to 480, with up to three decimals. For now it is one value for all of Maket; it moves into the project when projects come.
- **Maket sends it to the ePlayer** as `kammerton <Hz>` at the start of every play on the conTimbre test page, a quarter of a second before the first command of the roll. The page shows the value in its top line, and the send log has it as its first line.
- **The tuning check** (ePlayer study page, **Run the tuning check**, 48 seconds), on the orchestra `doodsbed by number`: the A above middle C on the cello, 24 times, each after `kammerton` with another frequency: 442, 440, 430, 415.3 (a semitone under 440) and 466.16 (a semitone over); 440 and 430 also as whole numbers; and three notes that begin at 442 and get `kammerton 430` while they sound. At its end, and at Stop, the standard pitch of the Preferences is sent again. It answers: does the player take the command, in which form of the number, is the A exactly at the frequency sent, and does a sounding note follow.

### Testing stage 1f.8

1. `git pull`, then `./build.sh`: `stage 1f.8`. `./build.sh tests` ends with `All 150 checks passed.`
2. Preferences: under *Tuning* stands `Standard pitch (Hz)`, 442.
3. In the ePlayer: the orchestra `doodsbed by number`. Start a recording, ePlayer study page, **Run the tuning check**; stop the recording when the status says the run is over. By ear you should hear the A step down and up, by a semitone at the widest.
4. If you like, by ear: set the standard pitch to 466.16 in the Preferences and play the start of doodsbed on the conTimbre test page: a semitone higher. Then press **Default** to go back to 442 and play again.
5. Send the recording; push the *Test results* folder and `Measurements/report.md`.

### After the run of 1f.8 (24 Nov)

- **The ePlayer takes `kammerton` over OSC, as a real number and as a whole number alike.** In all 21 notes the A lies within 0.3 cents of the frequency sent (442, 440, 430, 415.3, 466.16 Hz).
- **A note that already sounds keeps its pitch**; the next note has the new one. The three notes that got `kammerton 430` while sounding stayed at 442 Hz to their end. So sending the standard pitch once before a play, as Maket does, is the right moment.
- The change is in place within the 145 ms between the command and the note (no shorter time was tried).
- The start of doodsbed played at 466.16 Hz lies 92 cents over the same at 442 Hz (expected: 92.1).

Step 1 is complete with this.

## Using stage 2a

- **Open a roll:** the **Open…** button of the tab *Library and edition*, *File > Open* (⌘O), or a bach.roll text file dropped on the window. The file is only read.
- **What is drawn:** every voice on the staves of its clef, stacked as bach stacks them (the voice spacing of the file's header is followed, a hidden voice is left out, the parts of a voice ensemble share their staves); the voice names; each note as a head on its line or space, with ledger lines, a stem per chord and a duration line that ends in a small tick; soft notes lighter and loud notes blacker, as in ScAst's bach.roll. The roll is light with dark notation.
- **Where a note sits** follows bach's own spelling on the eighth-tone grid (C sharp, E flat, F sharp, A flat, B flat for the black keys). The accidentals themselves are not drawn yet, so a C sharp looks like a C for now.
- **Time:** a ruler at the top, a faint grid behind the staves. The stem of a chord stands at its onset.
- **Zoom:** the two pairs of − and + in the bar (time detail, staff size) and **Fit**; ⌘+ and ⌘− for time, ⌥⌘+ and ⌥⌘− for the staves, ⌘0 for the whole roll; pinching on the trackpad and ⌘ with the scroll wheel zoom the time around the pointer, ⌥ with the wheel the staves.
- **Scroll:** the trackpad or wheel in both directions (⇧ with a wheel goes sideways), and the two scroll bars. The clefs and names stay at the left, the ruler at the top.
- Not yet: accidentals (2b); glides and markers (2c); playing (2d); what is in the slots (step 5); selecting and editing (step 4).

### After the test of 2a (25 Nov)

Joost: all checks passed on his Mac; the rolls look as expected and as in ScAst. One difference seen: a middle C on a piano system (clef FG) had its stem up in Maket and down in ScAst. **2a.1** takes over bach's rule: a stem points down when its chord lies on or above the middle bach takes for the clef, and for a clef of several staves that is one place for all of them (middle C for FG and FFGG), not the middle line of the nearest staff; the parts of a voice ensemble take turns, the first up, the second down. `./build.sh tests` now ends with `All 160 checks passed.`

### Testing stage 2a

1. `git pull`, then `./build.sh`: `stage 2a`. `./build.sh tests` ends with `All 159 checks passed.`
2. *Library and edition*, **Open…**, a roll from `Temp/Roll view 2/roll txt`. Beside it the same roll in bach.roll.
3. For each roll you care to look at: the same voices with the same clefs, in the same order and at about the same distances? The notes at the same times and on the same lines and spaces (mind: no accidentals yet)? Chords: the same notes on the stem?
4. Zoom and scroll: is it smooth with Bult (1189 notes), also zoomed far out? Does anything jump or lag?
5. What looks wrong or unlike bach.roll in a way that bothers you: sizes, thickness of lines, the greys, the stems, the place of the names. A screenshot with a remark is the quickest.
6. Push `Measurements/report.md`.

## Using stage 2b

- **Accidentals are drawn.** Every altered note carries its own sign, a natural note none: that is what bach.roll does (its "Classically" is, in a roll, "altered notes, no naturals").
- **The signs** are bach's, one per step of the eighth-tone grid:

  | Above a white key | Sign | Under a white key | Sign |
  | --- | --- | --- | --- |
  | an eighth tone | natural with an arrow up | an eighth tone | natural with an arrow down |
  | a quarter tone | half sharp (one upright stroke) | a quarter tone | reversed flat |
  | three eighths | half sharp with an arrow up | three eighths | reversed flat with an arrow down |
  | a semitone | sharp | a semitone | flat |

  Which of the two a pitch gets is bach's choice, as since 2a: from C upwards plain, arrow up, half sharp, half sharp with arrow, sharp; then towards D reversed flat with arrow, reversed flat, arrow down, D. The black keys are C sharp, E flat, F sharp, A flat, B flat.
- **Where they stand:** left of the chord, by bach's own method: half a staff space from the head; when two signs of a chord are in each other's way, the lowest stands at the heads, then the highest, and the ones between go further left. The signs come from Maket's font (Bravura), so their shapes and widths differ a little from bach's own font; the arrowheads are heavier.
- **A note the file gives by name** (bach writes `G#6` and not `8000.` when the spelling was chosen by hand) keeps that letter, as in bach: in `Bult` the clarinet's run at 33.9 s has a G sharp and an A sharp that would otherwise be drawn as A flat and B flat.
- **The grid:** a menu in the bar, *Eighth tones* (as ScAst's roll) or *Semitones*. A pitch between two steps is drawn as the nearest one; on *Semitones* every note is plain, sharp or flat. The choice is not kept between runs yet and is not in the roll file (bach keeps it with the object too, not in the file).
- **Time zero** stands two staff spaces further from the clefs than in 2a, and further still when the chords that begin the roll need the room, so that their accidentals are not cut off.
- Not yet: bach's other ways of showing an alteration (as a fraction, in cents); double sharps and flats, and a named note more than a semitone from its letter (drawn by its pitch); keys other than C major (all your rolls name C major).

### Testing stage 2b

1. `git pull`, then `./build.sh`: `stage 2b`. `./build.sh tests` ends with `All 172 checks passed.` and writes `Temp/written by Maket/Accidentals by Maket.txt`.
2. Open that file in ScAst's bach.roll and, beside it, in Maket (*Library and edition*, **Open…**). It has five voices:
   - **every step**: the 49 eighth-tone steps from middle C up to the next C. Is it the same sign on the same line or space, note for note?
   - **chords**: eight chords whose signs are in each other's way. The same signs? About the same arrangement: which sign stands at the heads, which further left? (Exact distances will differ a little with the font.)
   - **off the grid**: pitches between the steps (4800, 4810, 4812, 4813, 4820, 4837, 4838, 4850, 4862, 4863, 4875, 4887, 4888, 4900 cents). Maket changes sign from 4812 to 4813, from 4837 to 4838, from 4862 to 4863 and from 4887 to 4888. Does bach change at the same notes?
   - **two staves**: four chords on a piano system.
   - **named**: seven pitches, each twice under two names (C sharp then D flat, D sharp then E flat, E sharp then F ...). Does bach draw them as named, as Maket does?
3. In Maket, switch the menu to *Semitones*: only sharps and flats remain. (In bach the same is `tonedivision 2` in the inspector, if you care to compare.)
4. Open a few of your own rolls in both. Anything that looks wrong or unlike bach.roll in a way that bothers you: size or weight of the signs, their distance to the heads, the heavier arrowheads. A screenshot with a remark is the quickest.
5. Push `Measurements/report.md`.

### After the test of 2b (25 Nov)

Joost: all checks passed on his Mac; the test roll looks the same in ScAst and in Maket but for the clusters (voice 2 at 21 s, voice 4 at 6 and 12 s), which bach.roll draws as a small tree. He asked for that, and for other ways to show velocity. **2b.1** does both:

- **Clusters.** The heads of a chord are now placed by bach's own method, read in its source. Notes at one height (C and C sharp) sit beside each other; a third and further note at that height goes further out and gets a small branch from the stem (bach calls these auxiliary stems). With the stem down, the lower of two neighbours is the left one, as in bach.
- **The onset is the middle of the head**, not the stem: bach.roll's own alignment ("Notehead Center" in your inspector). Notes stand half a head further left (stem up) or right (stem down) than in 2b.
- **Velocity** has a menu in the bar: *grey* (bach's colour scale, as ScAst: soft is light), *colour* (blue is soft, through green and yellow to red for loud) or *alpha* (bach's: soft is see-through). bach's own colour spectrum runs the other way round (red is soft, through green and blue to magenta) and is paler; say so if you would rather have that one. The choice is not kept between runs yet.
- The duration line now has bach's thickness (a third of a staff space) and starts at the right edge of the head; the tail is bach's short block.

From your inspector screenshots (`Temp/Roll view 2/inspector`): tone division 8, accidentals "Classically", stems "Main And Auxiliary", velocity "Colorscale", chords aligned at the notehead centre, duration line width 2, breakpoint size 3, markers with smart name placing, and bach's default colours for locked, muted, solo and markers. Maket follows these.

## Using stage 2c

- **Glides.** A note with pitch breakpoints draws its duration line through them: from point to point along bach's curve (the slope of each piece), a small diamond at each point between head and tail, the tail at the height where the note ends. Heights are bach's: the line starts at the head, and a point lies where its pitch lies between the white keys.
- **Markers.** A green line over the whole height and the name in bold at the top; names that would run into each other go a line lower (bach's smart placing). A marker with a role is dashed in the role's colour with the role written beside it in small capitals, one letter under the other (BARLINE, TIME SIG, TEMPO, DIVISION ...); a time signature or tempo shows what it holds (3 4) where a plain marker shows its name.
- **Locked, muted, solo.** Notes take bach's colours: red for locked, blue for muted, olive for solo, the mean of them when several hold. A note counts as locked (muted, solo) when it is so itself or its chord or its voice is. A voice that is locked, muted or solo also has its staff lines and clefs in that colour. Velocity still shows on top (a soft muted note is pale blue).
- Not yet: glides drawn with small note heads (bach's "breakpoints have noteheads"; yours is off), velocities of breakpoints, selecting anything (step 4), and all that comes from the slots: techniques, dynamics, the curves (step 5).

### Testing stage 2c

1. `git pull`, then `./build.sh`: `stage 2c`. `./build.sh tests` ends with `All 183 checks passed.` and writes two rolls into `Temp/written by Maket`.
2. Open **`Accidentals by Maket.txt`** again in Maket: the three clusters you marked (voice 2 at 21 s, voice 4 at 6 and 12 s) should now be small trees as in ScAst.
3. Open **`Glides and markers by Maket.txt`** in ScAst's bach.roll and in Maket, side by side:
   - **glides**: seven notes. The same lines? Straight up a fifth; down a fifth, once bending late and once early; a line through three diamonds; from C sharp up a tone; held, then up; a chord whose top note alone glides.
   - **flags**: a G every two seconds: plain, locked, muted, solo, locked and muted, muted and solo, all three; then a muted chord, a chord with one locked note, a solo chord. The same colours?
   - **muted voice**, **locked voice**: staff, clef and notes in blue and in red.
   - **velocities**: eight chords from velocity 1 to 127. Try the three displays in Maket's menu.
   - **markers**: `first`, `second close by` (a line lower), `third`, `fourth`, `section B`, a bar line, a time signature showing 3 4, `end`.
4. Open Doodsbed (57 notes with glides, 35 bar lines and time signatures) and one or two other rolls in both.
5. Push `Measurements/report.md`; screenshots with remarks for whatever differs.

### After the test of 2c (25 Nov)

Joost: all 183 checks passed on his Mac. The clusters are trees as in ScAst; glides, markers, and the colours of locked, muted and solo look the same; velocity answers to all three displays. His screenshots of three rolls in both programs are in `Temp/Roll view 2`. What still differs in them, none of it asked for yet:

- ScAst centres the voice names in their column and writes the name of a muted or locked voice in that colour; Maket writes them at the left, in black.
- A name that the file holds as one word with spaces in it shows in ScAst with quotes or backslashes (`"muted voice"`, `` `Maket\ test\ (1b) ``); Maket shows it plain. When Maket comes to write names itself (voices at step 7, markers at step 4) it should write the words separately, as bach does, so that ScAst shows them plain too.
- The percussion clef is Bravura's two bars, not bach's open box.
- Everything that comes from the slots (techniques, the curves, dynamics with hairpins, articulations, the shape of note heads) is step 5.

## Using stage 2d

The roll in *Library and edition* plays on the conTimbre ePlayer, through the mapping of step 1 as it stands: every note by a number of its own, dynamics from 0 to 1, the standard pitch of the settings sent first. As on the conTimbre test page, the voice at place 1 of the roll plays on the ePlayer's `voice1`, and so on, and the ePlayer has its orchestra loaded by hand. A voice without a name does not play.

- **The second row of the bar** holds the transport: **Play**, **Play ps**, **Stop**, **Loop**, and beside them where the playhead is.
- **The playhead** is the green line with a small flag in the ruler. A click in the ruler puts it there (drag to move it). Play begins at the playhead.
- **Play** (space): the playhead follows the sound, and when it reaches the right edge the page turns, as in ScAst.
- **Play ps** (⇧space), "play prescheduled": the same sound, but nothing on screen follows it. The playhead stays where play began. In Maket the sound never waits for the screen in either way of playing: every command is handed to the send thread ahead of time with its moment. What Play ps saves is the drawing, 30 times a second, of the playhead.
- **Stop** (space again): every sounding note is ended, and the playhead goes back to where play began. The view stays on the page it was on. **⌥space** plays on from where it stopped. **Return** puts the playhead at the start.
- **A note already sounding at the playhead** plays its remainder, as in ScAst: it enters at the pitch its glide has reached, with the rest of its glide. Its dynamics go, in 20 ms, to the value their curve has at that moment and on from there. (ScAst lets such a note start at its velocity and go from there to the next point.)
- **The loop.** Drag in the ruler with ⌥ held: the region shows in purple and **Loop** switches on (L switches it off and on; ⌥-click in the ruler removes the region). With the loop on, play goes from the playhead to the loop's end and then round the loop. At the loop's end every sounding note is ended; at its start the notes sounding there enter as described above. A play that begins beyond the loop's end runs to the end of the roll. A change to the loop while it plays takes effect at the next play.
- **Muted and solo**, bach's rule: what is muted does not play (a note, its chord or its voice); when anything in the roll is solo, only that plays.
- **Looks**, after your remarks: voice names stand in the middle of their column and take the colour of a muted, locked or solo voice; the percussion clef is the open box.
- Not yet: playing only what is selected (step 4); sound from Maket itself (2e).

### Testing stage 2d

With the ePlayer in Max. The roll view plays by number, so the ePlayer needs an orchestra whose voices are named `voice1`, `voice2` ... by their place in the roll:

- for Doodsbed (the roll is the one you always used, `doodsbed final v1c.txt` or `Temp/Roll view 2/roll txt/Doodsbed.txt`): the orchestra **`doodsbed by number`** of stage 1f.4. It has no `voice1`, so the Mezzo's notes are sent and not heard, as when Mezzo was unticked on the test page;
- for Maket's own test rolls (step 7): **`Maket test 2d`**, sent in the chat on 25 Nov: `voice1` to `voice5`, each a copy of the cello voice of `doodsbed kwintet`, gain 0. The test rolls name no technique, so each voice plays the program that is selected for it in the ePlayer.

Orchestra files stay out of the repository.

1. `git pull`, then `./build.sh`: `stage 2d`. `./build.sh tests` ends with `All 194 checks passed.`
2. Open Doodsbed in *Library and edition* and press space. Does it sound as it did from the test page (by number)? Is the playhead where the sound is? Does the page turn at the right edge?
3. Space again: does everything stop, with no note hanging? The playhead is back at the start. ⌥space: does it play on from where it stopped?
4. Click in the ruler somewhere in the middle of long notes (around 1'00") and press space: do the notes that are sounding there come in at once, at a sensible level? If you like, compare with ScAst from the same place.
5. Drag in the ruler with ⌥ held over a few bars (for instance 0'46" to 0'58") and press space. Does it go round without a hole and without a hanging note? L while stopped switches the loop off and on; ⌥-click in the ruler removes it.
6. ⇧space (Play ps): the same sound with the playhead standing still. If a heavy score ever stutters in normal play and not here, I would like to know.
7. Open `Temp/written by Maket/Glides and markers by Maket.txt` and play it: it holds solo items, so only the solo G at 6 s and the solo chord at 18 s should sound (on `voice2` of the orchestra `Maket test 2d`).
8. Looks: the voice names (centred; blue and red for the muted and the locked voice of that roll), and the percussion clef in the mixed example.
9. Push `Measurements/report.md`. A recording of anything that sounds wrong helps, as before.

### After the test of 2d (25 Nov)

All of it passed on Joost's Mac: looping without hiccups or hanging notes, play prescheduled, entering midway with the 20 ms ramp, the loop's seam, and the keys staying quiet while a text field is typed in. His recording of Doodsbed played from the roll view was compared with the one from the test page (by number): the two differ no more than two takes of the ePlayer differ from each other.

## Using stage 2e

Maket makes sound of its own for the first time, next to the ePlayer.

- **A tag under every voice name** says what plays the voice: `conTimbre` (the ePlayer, as before), `synth` (Maket's own) or `silent`. A click on the tag goes to the next of the three. Maket's first guess when a roll opens: a voice whose notes name a technique in words in slot 1 goes to conTimbre; a voice with a percussion clef or no clef is silent; every other voice goes to the synth. So Doodsbed plays as it did (its Mezzo, which names no technique and has no clef, is now silent instead of sent to a voice the orchestra does not have), and in the mixed example only `L1a` and `L2a` go to the ePlayer. The choice is per open roll and not kept; from step 7 it belongs to the instrument in the project.
- **The synth** is as plain as can be: one sine per note. Its pitch is exact, eighth tones and any pitch in between, glides with bach's curves, on the standard pitch of the preferences. Its loudness is the note's velocity times the modulation curve of slot 2 when the note has one (0 to 127, in ms), under the level of the preferences. A note fades in over 10 ms and out over 50 ms. Other timbres, and the waveshape slot, wait for the discussion on the synth.
- **In time with the ePlayer**: both are driven from the one clock. A synth note sounds at the very moment its command would leave for the ePlayer; the ePlayer then needs a moment of its own before its sample is heard, so the synth will be a little early. *Synth timing (ms)* in the preferences moves the synth: positive is later.
- **Preferences → Sound from Maket**: *Audio output* (Choose lists what this Mac has; not set is the Mac's default output; the line under it says what is open now), *Synth level (dB)* (how loud one note at full velocity is; -18 at first), *Synth timing (ms)*. The audio output is opened as soon as an open roll has a voice on the synth; a roll with only conTimbre voices opens none.
- Mute and solo, entering midway, the loop, Stop and play prescheduled work for the synth as for conTimbre.

Not yet: the choice of engine is not kept with the roll; a change of tag, level or timing during play takes effect at the next play.

### Testing stage 2e

With the ePlayer in Max and the orchestra **`Maket test 2d`** (its `voice1` is the cello).

1. `git pull`, then `./build.sh`: `stage 2e`. `./build.sh tests` ends with `All 208 checks passed.` (it also writes the roll of step 2).
2. Open `Temp/written by Maket/Synth and conTimbre by Maket.txt` and play it. It has three voices: `cello` (conTimbre), `synth` and `synth alone`.
   - 1 to 8 s: eight plucked notes on the cello, each with a short high tone of the synth. Do they fall together? If not: which comes first, and roughly by how much?
   - 10 to 16 s: the same A, bowed and on the synth. Is it one pitch, without beating?
   - 18 s: nine notes rising by eighth tones. 25 s: a glide up a fifth and back. 30 s: a note that swells and fades. 35 s: eight notes at once: clean, without distortion?
3. A recording of the first 16 seconds with both the ePlayer and Maket in it would let me measure how far apart they are, to the millisecond, and set *Synth timing* to the right number for your Mac. Or set it by ear: try 10, 20, 30.
4. *Utilities → Preferences → Sound from Maket*: what does the line under *Audio output* say (device, rate, block size, delay)? Choose another output and play again: does the synth move there?
5. Set *Synth level* to -12: louder? Back to -18, or to what suits you.
6. The mixed example: are the tags as described above? Play; click a few tags and play again; Stop in the middle of long notes: is everything silent at once?
7. Doodsbed with its orchestra `doodsbed by number`: as before. Then click the Mezzo's tag to `synth` and play: its line as a plain tone with the ensemble.
8. On any roll with synth voices: click in the ruler in the middle of a long or gliding note and play (it should come in at once, at the pitch the glide has reached); a loop over synth notes; ⇧space.
9. Push `Measurements/report.md`.

### After the test of 2e (26 Nov)

Joost: all 208 checks passed on his Mac; every section of `Synth and conTimbre by Maket` sounds as expected; changing the audio output and the level works; Doodsbed by number sounds as before, also with the Mezzo on the synth; Stop in the middle of the mixed example silences everything.

His recording of the test roll (`Contimbre & synth test2.ogg`, attached in the chat), measured:

- The synth is exactly on its clock: its eight tones begin 1000.00 ms apart, within 0.05 ms, and its attack is the 10 ms it should be.
- The cello's plucks begin **90 to 93 ms after the synth's tones** (mean 91.5 ms). They alternate between two moments 2.67 ms apart, which is one block of 128 samples at 48000: the ePlayer starts a note on a block of Max's audio.
- The held A is at 221.0 Hz, as it should be on 442.

Joost first found the two together by ear at 35 to 40 ms; listening again with the measured figure he confirmed it: at **91 ms** they fall exactly together. That is his setting of *Synth timing*. It belongs to his Mac, his audio output and Max's audio settings (it is the time the ePlayer needs from a command to its sound), so the setting's first value in the code stays 0.

## Using stage 2f

The closing of step 2. Nothing new to play with; what was asked of this stage is speed with large rolls and one more look at everything beside ScAst.

**Speed.** The test program now writes a roll far larger than any of yours, `Temp/written by Maket/Large roll by Maket.txt` (24 voices, 20 minutes, 74157 notes, half of the voices on conTimbre and half on the synth), and times what Maket does with it (section 11 of the report). Three things were slow with it and are changed:

- *The start of a play.* Working out a play of the whole roll took most of a second on the test machine, and the clock was read before that work: the first second of music came late and bunched together. The clock is now read after the work, so however large the roll, the opening is in time; a very large roll starts a moment after the press instead.
- *A loop.* Every round of a loop was worked out from the whole roll (0.3 s per round with this roll, during which the window stood still). The roll is now made ready once, and a round is worked out from its own stretch: 6 ms.
- *Handing over.* Commands go to the send thread a few thousand at a time and in order, so the first ones are on their way while the rest of a very large roll is still being handed over. The synth gets room for its notes without the sound having to wait.

With your own rolls none of this was noticeable; they are a hundred times smaller.

**Drawing times.** *View > Show drawing times* (⌥⌘T) writes in the lower left corner of the roll how long the last drawing of the paper took and how many notes were in view. On the test machine (no graphics card, so slower than your Mac): your rolls at full view about 10 ms (Draaikolk, 424 notes in view); the large roll about 14 ms at its opening zoom (480 notes in view) and 240 to 320 ms with 22000 to 28000 notes in view at once. Nearly all of that time goes to drawing note heads and accidentals one by one. If the figures on your Mac call for it, they can be drawn several times faster; I would rather decide that on your figures than on mine.

**Looks.** Six more of your rolls (Draaikolk, FMmod, Mexican Bird, Priem boventonen, both Capriccios) were set beside your ScAst screenshots. Staves, notes, accidentals, velocities, markers and the colour of muted voices agree. Two things were found and changed:

- Voices that share their staves (Draaikolk's `sample1` and `sample2`) now show both names, one under the other as in ScAst, each with its own tag; before, the second voice had no name and no tag to click.
- A roll whose voices have short names or none now keeps room for the tags beside the staves (they were cut off or lay over the clefs).

What still differs from ScAst is what the slots hold (techniques, curves, dynamics, articulations, note head shapes): step 5.

### Testing stage 2f

1. `git pull`, then `./build.sh`: `stage 2f`. `./build.sh tests` ends with `All 214 checks passed.`; it takes a few seconds longer than before, and writes the large roll.
2. Open `Temp/written by Maket/Large roll by Maket.txt` and switch on *View > Show drawing times*. What does the line in the corner say as it opens, and after Fit? Scroll and zoom: where does it feel slow, if anywhere?
3. Play it from the start (the ePlayer may stay closed for this roll: twelve of its voices play on the synth, and it is a wall of sound, not music). Does it begin at once and in time, does the playhead run evenly, do the pages turn? Stop: silent at once?
4. Make a loop of some ten seconds somewhere in the middle and play: does it go round without the window hesitating at each round?
5. The looks, once more with rolls of your choice beside ScAst; Draaikolk for the two names on one staff. Anything that differs, apart from what the slots hold, I would like to hear.
6. Push `Measurements/report.md`: its section 11 holds your Mac's times.

With that, step 2 is closed: real files look as in bach.roll, eighth tones are right, play, stop and loop work, on conTimbre and on Maket's own synth.

## The repository

```
build.sh            builds and opens Maket; with "tests", builds and runs the test program
CMakeLists.txt      what is built, and with which JUCE
Source/             the application
  Main.cpp            the application object and its window
  MainComponent.*     everything inside the window
  UI/                 interface code: tab row, panel buttons, pages, preferences, inspector, colours, the roll view, the music font
  Platform/           what differs per operating system (on macOS: telling the system that Maket is busy)
  Model/              no interface code: the settings; the name, version and build stage; bach's list type; the roll
  Formats/            no interface code: reading and writing files (bach's text, bach.roll)
  Engine/             no interface code: OSC messages, the sender and its thread, the ePlayer's commands, the timing test; what of a roll plays from where to where and on which engine (`Playing`), the transport (`Transport`), the synth (`Synth`) and the clock of the audio stream (`StreamClock`)
  Audio/              the audio output: the device, and the call in which the synth fills each block
  Notation/           no interface code: which note and accidental a pitch is drawn as, the staves of each clef, how voices are stacked, the heads and accidentals of a chord, glides, the lines of marker names
Resources/Fonts/    the Bravura music font (Steinberg, SIL Open Font License), built into the app
Max/                patches for Max that belong to Maket (the OSC logging patch, the bach.roll reference)
Tests/              the console test program
Measurements/       its reports
Reference/python/   scripts from the design phase (file formats, checks); not part of the app
Temp/               Joost's test material, test saves, crash reports and screen captures
```

`Model/`, `Formats/`, `Engine/` and `Notation/` hold no interface code, so that the test program can build them without the window.

## Licence

Maket is published under the GNU Affero General Public License, version 3 (see `LICENSE.md`), the licence under which JUCE is used here.

The music signs are drawn with the Bravura font by Steinberg, which is built into the app under the SIL Open Font License (`Resources/Fonts/Bravura OFL.txt`). How notes are spelled, staves laid out, heads and accidentals placed and glides drawn follows the source of bach (GPL, version 3), read as a reference.

Nothing of conTimbre is in this repository or in the app. Each user needs their own conTimbre and points Maket at it.
