# Maket

A standalone macOS application for composing with snippets. A snippet is a passage of notes in proportional notation in which every note carries slots, a fragment of audio, or a curve. Snippets are placed and nested on a build surface, rendered into a roll that is edited by hand, and played through several engines at once: the conTimbre ePlayer over OSC, instrument plugins and MIDI ports, a simple synth, custom engines in Max, and audio tracks.

Maket grows out of the Max/bach patch ScAst beta 4. It takes code and working method from [Projet](https://github.com/Joostiepoost/Projet), [Trajet](https://github.com/Joostiepoost/Trajet), Effet and Goblet, and is the first of a new family of score-based applications with ties to OpenMusic and Max/MSP.

macOS, Apple Silicon only. Built with [JUCE](https://juce.com) 9 and CMake. The full design is in the Maket proposal doc.

**Where the build stands: stage 0a**, the first one. The window, its four tabs and the inspector exist; everything else is still to come.

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
- `./build.sh tests` — the console test program; it arrives with stage 0b

## Using stage 0a

Stage 0a adds one thing: the main window.

- **Four tabs** along the top, in this order: Library and edition, Build surface, Render, Utilities. The tab in front is in bold with an amber line under it.
- **Switching tabs**: click a tab, press Cmd-1 to Cmd-4, or choose it in the **View** menu, which ticks the tab in front.
- **Each tab is empty.** Its page names the tab, says what will come there and at which build step.
- **The inspector** is the panel on the right. One inspector serves all four tabs; for now it names the tab in front and says what it will show there.
- The right end of the tab row reads `Maket 0.0.1 · stage 0a`, so that it is clear which build is open. The transport comes there at step 2.

Nothing is saved or remembered yet: Maket always opens on the first tab. Settings arrive with stage 0c.

### Testing stage 0a

1. `./build.sh` finishes without an error and Maket opens.
2. The window's title is Maket, and the menu bar shows Maket next to the Apple menu.
3. The four tabs are there in the order above; Library and edition is in front.
4. A click on each tab brings it in front: the amber line moves, the page and the inspector change with it.
5. Cmd-1, Cmd-2, Cmd-3 and Cmd-4 do the same, and so do the four entries of the View menu.
6. The window can be resized, made small and made full screen; the inspector keeps its width on the right.
7. Cmd-Q quits.

## The repository

```
build.sh            builds and opens Maket
CMakeLists.txt      what is built, and with which JUCE
Source/             the application
  Main.cpp            the application object and its window
  MainComponent.*     everything inside the window
  UI/                 interface code: tab row, empty pages, inspector, colours
Reference/python/   scripts from the design phase (file formats, checks); not part of the app
Temp/               Joost's test saves, crash reports and screen captures
```

`Source/` gains `Model/`, `Formats/`, `Engine/` and `Notation/` as their steps come. Those four hold no interface code, so that the console test program of stage 0b can build them without the window.

## Licence

Maket is published under the GNU Affero General Public License, version 3 (see `LICENSE.md`), the licence under which JUCE is used here.

Nothing of conTimbre is in this repository or in the app. Each user needs their own conTimbre and points Maket at it.
