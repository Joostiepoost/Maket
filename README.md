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
| 1f | Addressing by note, chords, tuning | in parts |
| 1f.1 | The ways of the Max patch by switch (order, glissando to 0, spacing), to hear whether any of them matters; the comparison sets apart what is known of the patch | done; tested by Joost on 23 Nov: no difference, Maket keeps its own way |
| 1f.2 | A study of the ePlayer: one figure sent in ten ways (by pitch or by number, timing of the dynamics, dynamic correction), to find out what makes it follow its ramps | done; run by Joost on 23 Nov |
| 1f.3 | The second run of the study: is a note by number louder as such; which part of the timing matters by number; do slow starts come from the note before | done; run by Joost on 23 Nov |
| 1f.4 | A roll played by number (Maket's default from here on); the third run of the study: fast repeated notes, a glide by number, a chord-mode program | built, to test |
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

## The repository

```
build.sh            builds and opens Maket; with "tests", builds and runs the test program
CMakeLists.txt      what is built, and with which JUCE
Source/             the application
  Main.cpp            the application object and its window
  MainComponent.*     everything inside the window
  UI/                 interface code: tab row, panel buttons, pages, preferences, inspector, colours
  Platform/           what differs per operating system (on macOS: telling the system that Maket is busy)
  Model/              no interface code: the settings; the name, version and build stage; bach's list type; the roll
  Formats/            no interface code: reading and writing files (bach's text, bach.roll)
  Engine/             no interface code: OSC messages, the sender and its thread, the ePlayer's commands, the timing test
Max/                patches for Max that belong to Maket (the OSC logging patch, the bach.roll reference)
Tests/              the console test program
Measurements/       its reports
Reference/python/   scripts from the design phase (file formats, checks); not part of the app
Temp/               Joost's test material, test saves, crash reports and screen captures
```

`Source/` gains `Notation/` when its step comes. `Model/`, `Formats/`, `Engine/` and `Notation/` hold no interface code, so that the test program can build them without the window.

## Licence

Maket is published under the GNU Affero General Public License, version 3 (see `LICENSE.md`), the licence under which JUCE is used here.

Nothing of conTimbre is in this repository or in the app. Each user needs their own conTimbre and points Maket at it.
