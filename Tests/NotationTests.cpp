// NotationTests.cpp — section 8 of the report: where notes sit on the staves
// (stage 2a). Which note a pitch is drawn as, the staves of each clef,
// ledger lines, how the voices of a roll are stacked; and the nine rolls of
// `Temp/Roll view 2`, read and written back.

#include <JuceHeader.h>
#include <cmath>

#include "TestReport.h"
#include "../Source/Formats/BachRoll.h"
#include "../Source/Notation/Spelling.h"
#include "../Source/Notation/Staves.h"

using namespace maket::notation;
using namespace testreport;

void testNotation (const juce::File& temp)
{
    line ("## 8. Notation: where notes sit on the staves (stage 2a)");
    line();
    line ("The roll view draws a note with the note name bach gives it, on the staves bach gives the voice. These checks are on "
          "that reckoning, which has no interface code; what the view draws from it is judged by eye against bach.roll.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    // --- Spelling ---
    {
        struct Case { double cents; int division; int steps; int alteration; };
        const Case cases[] =
        {
            { 6000, 8, 0, 0 },   { 6025, 8, 0, 1 },   { 6050, 8, 0, 2 },   { 6075, 8, 0, 3 },   { 6100, 8, 0, 4 },      // C up to C sharp
            { 6125, 8, 1, -3 },  { 6150, 8, 1, -2 },  { 6175, 8, 1, -1 },  { 6200, 8, 1, 0 },                           // then towards D
            { 6300, 8, 2, -4 },  { 6400, 8, 2, 0 },   { 6450, 8, 2, 2 },   { 6475, 8, 3, -1 },  { 6500, 8, 3, 0 },      // E flat, E, F
            { 6600, 8, 3, 4 },   { 6800, 8, 5, -4 },  { 7000, 8, 6, -4 },  { 7100, 8, 6, 0 },                           // F sharp, A flat, B flat, B
            { 7150, 8, 6, 2 },   { 7175, 8, 7, -1 },  { 7200, 8, 7, 0 },                                                // B quarter sharp, then the next C
            { 5975, 8, 0, -1 },  { 5900, 8, -1, 0 },  { 4800, 8, -7, 0 },  { 3625, 8, -14, 1 },                         // under middle C
            { 6012, 8, 0, 0 },   { 6013, 8, 0, 1 },   { 7190, 8, 7, 0 },                                                // off the grid: to the nearest step
            { 6025, 4, 0, 2 },   { 6074, 4, 0, 2 },   { 6075, 4, 0, 4 },                                                // quarter-tone grid
            { 6050, 2, 0, 4 },   { 6049, 2, 0, 0 },   { 6150, 2, 1, 0 }                                                 // semitone grid
        };
        bool ok = true;
        juce::String first;
        for (const auto& c : cases)
        {
            const auto s = spell (c.cents, c.division);
            if (! (s == Spelling { c.steps, c.alteration }))
            {
                ok = false;
                if (first.isEmpty()) first = juce::String (c.cents) + " on grid " + juce::String (c.division) + " gave " + juce::String (s.steps) + ", " + juce::String (s.alteration);
            }
        }
        checkRow (ok, "34 pitches get bach's note and accidental: C sharp, E flat, F sharp, A flat, B flat for the black keys; a quarter and "
                      "three eighths above a white key, three eighths to one eighth under the next; on the grids of 8, 4 and 2 steps to the tone"
                      + (first.isEmpty() ? juce::String() : " (first that differs: " + first + ")"));

        // Every eighth-tone step over four octaves: the spelling stands for that very pitch, and never by more than a sharp or a flat.
        bool back = true, small = true;
        for (int i = -96; i <= 96; ++i)
        {
            const double cents = 6000.0 + 25.0 * i;
            const auto s = spell (cents, 8);
            back = back && std::abs (centsOf (s) - cents) < 1.0e-9;
            small = small && std::abs (s.alteration) <= 4;
        }
        checkRow (back && small, "Over four octaves of eighth tones every spelling stands for its own pitch, with an accidental of a semitone at most");
    }

    // --- Clefs ---
    {
        struct Case { const char* name; int staves; int topStep; int bottomStep; double middleC; };
        const Case cases[] =
        {
            { "G", 1, 10, 2, 70.0 },        { "F", 1, -2, -10, 34.0 },      { "FG", 2, 10, -10, 70.0 },     { "FFGG", 4, 24, -24, 112.0 },
            { "FFG", 3, 10, -24, 70.0 },    { "FGG", 3, 24, -10, 112.0 },   { "FF", 2, -2, -24, 34.0 },     { "GG", 2, 24, 2, 112.0 },
            { "G8vb", 1, 3, -5, 49.0 },     { "G8va", 1, 17, 9, 91.0 },     { "G15ma", 1, 24, 16, 112.0 },  { "F15mb", 1, -16, -24, -8.0 },
            { "Alto", 1, 4, -4, 52.0 },     { "Tenor", 1, 2, -6, 46.0 },    { "Soprano", 1, 8, 0, 64.0 },   { "Percussion", 1, 10, 2, 70.0 },
            { "None", 1, 10, 2, 70.0 }
        };
        bool ok = true;
        juce::String first;
        for (const auto& c : cases)
        {
            const auto l = clefLayout (c.name);
            const bool same = l.known && l.numStaves() == c.staves && l.staves.front().topLineStep == c.topStep
                              && l.staves.back().bottomLineStep() == c.bottomStep && std::abs (l.unitsOf (0) - c.middleC) < 1.0e-9
                              && std::abs (l.topUnits() - 40.0) < 1.0e-9;
            if (! same && first.isEmpty()) first = c.name;
            ok = ok && same;
        }
        checkRow (ok && ! clefLayout ("Whatever").known,
                  "17 of bach's clefs: the number of staves, the top and bottom line, and middle C at the height bach reckons "
                  "(112 units under the voice's offset for FFGG, 70 for G and FG, 34 for F, 52 for Alto ...)"
                  + (first.isEmpty() ? juce::String() : " (first that differs: " + first + ")"));

        const auto fg = clefLayout ("FG");
        const auto ffgg = clefLayout ("FFGG");
        const auto g = clefLayout ("G");
        auto same = [] (const std::vector<int>& a, std::initializer_list<int> b) { return a == std::vector<int> (b); };
        checkRow (same (ledgerSteps (fg, 0), { 0 }) && same (ledgerSteps (fg, 5), {}) && same (ledgerSteps (fg, 1), {}) && same (ledgerSteps (fg, -1), {})
                  && same (ledgerSteps (g, 0), { 0 }) && same (ledgerSteps (g, -3), { 0, -2 }) && same (ledgerSteps (g, 13), { 12 })
                  && same (ledgerSteps (g, 16), { 12, 14, 16 }) && same (ledgerSteps (g, 11), {})
                  && same (ledgerSteps (ffgg, 12), { 12 }) && same (ledgerSteps (ffgg, 14), { 14 }) && same (ledgerSteps (ffgg, 13), { 14 })
                  && same (ledgerSteps (ffgg, -13), { -12 }) && same (ledgerSteps (ffgg, 27), { 26 }),
                  "Ledger lines: middle C between the staves of a piano system has one; a note between two staves takes the lines of the nearer staff; "
                  "none for a note on a staff or in the space next to it");
    }

    // --- The direction of a stem ---
    {
        auto place = [] (const char* clef, int part = 0, int parts = 1)
        {
            VoicePlace p;
            p.clef = clefLayout (clef);
            p.partIndex = part;
            p.numParts = parts;
            return p;
        };
        struct Middle { const char* clef; int step; };
        const Middle middles[] = { { "G", 6 }, { "F", -6 }, { "FG", 0 }, { "FFGG", 0 }, { "FFG", -8 }, { "FGG", 8 }, { "FF", -13 }, { "GG", 13 },
                                   { "Alto", 0 }, { "Tenor", -2 }, { "G8vb", -1 }, { "G8va", 13 }, { "F15mb", -20 }, { "None", 6 }, { "Percussion", 6 } };
        bool ok = true;
        for (const auto& m : middles) ok = ok && clefLayout (m.clef).middleStep == m.step;
        checkRow (ok && stemDirection (place ("FG"), 0.0) == -1 && stemDirection (place ("FG"), -1.0) == 1          // middle C on a piano system: down
                  && stemDirection (place ("FFGG"), 0.0) == -1 && stemDirection (place ("FFGG"), -0.5) == 1
                  && stemDirection (place ("G"), 6.0) == -1 && stemDirection (place ("G"), 5.0) == 1                 // the B on the middle line: down
                  && stemDirection (place ("G", 0, 2), 12.0) == 1 && stemDirection (place ("G", 1, 2), -3.0) == -1,  // an ensemble: the parts take turns
                  "A stem points down when its chord lies on or above the middle bach takes for the clef (the middle line of a single staff; "
                  "middle C for FG and FFGG, so middle C there has its stem down), up when under it; the parts of a voice ensemble take turns");
    }

    // --- The voices of doodsbed, stacked ---
    {
        auto offsetsOf = [] (const juce::File& file, const double* expected, double expectedHeight, RollLayout& layout)
        {
            maket::Roll roll;
            juce::StringArray remarks;
            if (maket::readBachRollFile (file, roll, remarks).failed()) return false;
            layout = layoutVoices (roll);
            bool ok = layout.voices.size() == 8 && std::abs (layout.height - expectedHeight) < 1.0e-6;
            for (size_t v = 0; ok && v < 8; ++v)
                ok = std::abs (layout.voices[v].offset - expected[v]) < 1.0e-6 && ! layout.voices[v].hidden && layout.voices[v].ownsStaves;
            return ok;
        };

        // The file of 2019 has no voicespacing in its header: 17 units everywhere.
        RollLayout plain, spaced;
        const double withoutSpacing[8] = { 17.0, 76.0, 135.0, 236.0, 295.0, 354.0, 413.0, 598.0 };
        const bool first = offsetsOf (material (temp, "doodsbed final v1c.txt"), withoutSpacing, 823.0, plain);
        checkRow (first && plain.voices[3].clef.staves[0].octaves == -1 && plain.voices[5].clef.staves[0].sign == ClefSign::c
                  && plain.voices[6].clef.numStaves() == 4,
                  "The eight voices of `doodsbed final v1c` are stacked by its clefs (None, None, FG, G8vb, F, Alto, FFGG, FFGG), 17 units between "
                  "them as its header names no spacing: offsets 17, 76, 135, 236, 295, 354, 413 and 598 units");

        // The same roll saved by today's bach names its spacing.
        const auto resaved = temp.getChildFile ("Roll view 2").getChildFile ("roll txt").getChildFile ("Doodsbed.txt");
        if (resaved.existsAsFile())
        {
            const double withSpacing[8] = { 17.0, 76.0, 138.0, 239.0, 310.3, 369.3, 428.3, 613.3 };
            checkRow (offsetsOf (resaved, withSpacing, 838.3, spaced),
                      "The same roll as saved by today's bach (`Roll view 2/roll txt/Doodsbed.txt`) is stacked by the voice spacing in its header "
                      "(17, 17, 20, 17, 29.3, 17 ...): offsets 17, 76, 138, 239, 310.3, 369.3, 428.3 and 613.3 units");
        }
    }

    // --- The rolls of Temp/Roll view 2 ---
    line();
    line ("The rolls Joost gave for the roll view (`Temp/Roll view 2/roll txt`), read and written back:");
    line();
    line ("| Roll | Voices | Staves | Chords | Notes | Markers | Clefs | Lowest and highest note | Length | Written back |");
    line ("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |");

    const auto folder = temp.getChildFile ("Roll view 2").getChildFile ("roll txt");
    auto files = folder.findChildFiles (juce::File::findFiles, false, "*.txt");
    files.sort();
    int numRead = 0, numSame = 0, numKnownClefs = 0, numSpelled = 0;
    for (const auto& file : files)
    {
        maket::Roll roll;
        juce::StringArray remarks;
        if (maket::readBachRollFile (file, roll, remarks).failed())
        {
            line ("| `" + file.getFileName() + "` | could not be read | | | | | | | | |");
            continue;
        }
        ++numRead;

        const auto original = file.loadFileAsString();
        const bool same = maket::writeBachRollText (roll) == original;
        if (same) ++numSame;

        const auto layout = layoutVoices (roll);
        juce::StringArray clefs;
        bool known = true, spelled = true;
        int staves = 0;
        double lowest = 1.0e9, highest = -1.0e9;
        for (const auto& place : layout.voices)
        {
            clefs.addIfNotAlreadyThere (place.clef.name);
            known = known && place.clef.known;
            if (place.ownsStaves && ! place.hidden) staves += place.clef.numStaves();
            for (const auto& chord : roll.voices[(size_t) place.voice].chords)
                for (const auto& note : chord.notes)
                {
                    lowest = juce::jmin (lowest, note.cents);
                    highest = juce::jmax (highest, note.cents);
                    spelled = spelled && std::abs (centsOf (spell (note.cents, 8)) - note.cents) <= 12.5 + 1.0e-9;
                }
        }
        if (known) ++numKnownClefs;
        if (spelled) ++numSpelled;

        line ("| `" + file.getFileName() + "` | " + juce::String ((int) roll.voices.size()) + " | " + juce::String (staves) + " | " + juce::String (roll.numChords())
              + " | " + juce::String (roll.numNotes()) + " | " + juce::String ((int) roll.markers.size()) + " | " + clefs.joinIntoString (", ")
              + " | " + juce::String (lowest, 0) + " to " + juce::String (highest, 0) + " | " + juce::String (roll.length() / 1000.0, 1) + " s | "
              + (same ? "byte for byte" : "**differs**") + " |");
    }
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");
    if (files.isEmpty())
    {
        line ("| (The folder `Temp/Roll view 2/roll txt` is not there: these checks are left out.) | |");
    }
    else
    {
        checkRow (numRead == files.size(), juce::String (files.size()) + " rolls are read");
        checkRow (numSame == files.size(), "Each of them, written back, is byte for byte the file (the test of stage 1b on new files)");
        checkRow (numKnownClefs == files.size() && numSpelled == files.size(),
                  "Every clef in them is one Maket knows, and every note gets a spelling within half a grid step of its pitch");
    }
    line();
}
