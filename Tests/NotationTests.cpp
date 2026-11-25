// NotationTests.cpp — section 8 of the report: where notes sit on the staves
// (stage 2a). Which note a pitch is drawn as, the staves of each clef,
// ledger lines, how the voices of a roll are stacked; the nine rolls of
// `Temp/Roll view 2`, read and written back; and the accidentals (stage 2b):
// the sign of each step, their places left of a chord, and a roll written
// for the comparison with bach.roll; glides and marker names (stage 2c), with
// a second such roll.

#include <JuceHeader.h>
#include <cmath>

#include "TestReport.h"
#include "../Source/Formats/BachRoll.h"
#include "../Source/Formats/LlllText.h"
#include "../Source/Notation/Chords.h"
#include "../Source/Notation/Glides.h"
#include "../Source/Notation/Spelling.h"
#include "../Source/Notation/Staves.h"

using namespace maket::notation;
using namespace testreport;

namespace
{
    // The sizes of the signs in staff spaces, about as in the app's font: the checks on places need some, not these exactly.
    struct SignSize { double above, below, width; };
    SignSize sizeOf (Accidental a)
    {
        switch (a)
        {
            case Accidental::sharp:             return { 1.40, 1.40, 1.00 };
            case Accidental::flat:              return { 1.75, 0.70, 0.90 };
            case Accidental::quarterSharp:      return { 1.30, 1.30, 0.65 };
            case Accidental::quarterFlat:       return { 1.75, 0.70, 0.90 };
            case Accidental::quarterSharpUp:    return { 2.20, 1.30, 0.75 };
            case Accidental::quarterFlatDown:   return { 1.75, 1.60, 0.90 };
            case Accidental::naturalUp:         return { 2.20, 1.35, 0.75 };
            case Accidental::naturalDown:       return { 1.35, 2.20, 0.75 };
            case Accidental::none:              break;
        }
        return { 0.0, 0.0, 0.0 };
    }

    const double headWidth = 1.18;

    /** The notes of a chord as the roll view hands them to placeAccidentals. */
    std::vector<ChordNote> chordNotes (const std::vector<double>& cents, int direction, int division = 8)
    {
        std::vector<int> steps;
        std::vector<Spelling> spellings;
        for (double c : cents) { spellings.push_back (spell (c, division)); steps.push_back (spellings.back().steps); }
        const auto heads = placeHeads (steps, direction, headWidth);

        std::vector<ChordNote> notes;
        for (size_t i = 0; i < cents.size(); ++i)
        {
            ChordNote n;
            n.step = steps[i];
            n.headLeft = heads[i].left;
            const auto sign = accidentalOf (spellings[i].alteration);
            const auto size = sizeOf (sign);
            n.hasAccidental = sign != Accidental::none;
            n.above = size.above; n.below = size.below; n.width = size.width;
            notes.push_back (n);
        }
        return notes;
    }

    bool near (double a, double b)      { return std::abs (a - b) < 1.0e-9; }

    /** Whether any two signs of the chord, or a sign and a head, lie over each other. */
    bool chordIsClear (const std::vector<ChordNote>& notes, const std::vector<double>& right)
    {
        struct Box { double left, right, top, bottom; };
        auto meet = [] (const Box& a, const Box& b) { return a.left < b.right - 1.0e-9 && b.left < a.right - 1.0e-9 && a.top < b.bottom - 1.0e-9 && b.top < a.bottom - 1.0e-9; };
        std::vector<Box> signs, heads;
        for (size_t i = 0; i < notes.size(); ++i)
        {
            const double y = -0.5 * notes[i].step;
            heads.push_back ({ notes[i].headLeft, notes[i].headLeft + headWidth, y - 0.5, y + 0.5 });
            if (notes[i].hasAccidental)
                signs.push_back ({ right[i] - notes[i].width, right[i], y - notes[i].above, y + notes[i].below });
        }
        for (size_t a = 0; a < signs.size(); ++a)
        {
            for (size_t b = a + 1; b < signs.size(); ++b) if (meet (signs[a], signs[b])) return false;
            for (const auto& h : heads) if (meet (signs[a], h)) return false;
        }
        // No head over another: heads a second apart count as over each other when they sit on the same side.
        for (size_t a = 0; a < heads.size(); ++a)
            for (size_t b = a + 1; b < heads.size(); ++b)
            {
                Box one = heads[a], other = heads[b];
                one.top += 0.2; one.bottom -= 0.2; other.top += 0.2; other.bottom -= 0.2;
                if (meet (one, other)) return false;
            }
        return true;
    }

    maket::HeaderEntry headerOf (const char* name, const juce::StringArray& words)
    {
        maket::HeaderEntry entry;
        entry.name = name;
        entry.content.items.push_back (maket::Llll::makeSymbol (name));
        for (const auto& w : words)
        {
            if (w.containsChar (' '))
            {
                // A name of several words: a list of them, as bach keeps it (one symbol with spaces shows in bach with quotes).
                maket::Llll several;
                juce::StringArray parts;
                parts.addTokens (w, " ", "");
                for (const auto& part : parts) several.items.push_back (maket::Llll::makeSymbol (part));
                entry.content.items.push_back (std::move (several));
            }
            else entry.content.items.push_back (w.containsOnly ("0123456789") ? maket::Llll::makeInteger (w.getLargeIntValue()) : maket::Llll::makeSymbol (w));
        }
        return entry;
    }

    void addChord (maket::Voice& voice, double onset, const std::vector<double>& cents, double duration)
    {
        maket::Chord chord;
        chord.onset = onset;
        for (double c : cents)
        {
            maket::Note note;
            note.cents = c;
            note.duration = duration;
            note.velocity = 127;
            chord.notes.push_back (note);
        }
        voice.chords.push_back (chord);
    }

    /** The roll for the comparison with bach.roll: what each voice holds is told in the report and the README. */
    maket::Roll accidentalsRoll()
    {
        maket::Roll roll;
        roll.header.push_back (headerOf ("clefs", { "G", "G", "F", "FG", "G" }));
        roll.header.push_back (headerOf ("voicenames", { "every step", "chords", "off the grid", "two staves", "named" }));
        roll.header.push_back (headerOf ("midichannels", { "1", "2", "3", "4", "5" }));
        roll.voices.resize (5);

        // 1: every eighth-tone step from middle C to the C above.
        for (int i = 0; i <= 48; ++i)
            addChord (roll.voices[0], 600.0 * i, { 6000.0 + 25.0 * i }, 450.0);

        // 2: chords whose accidentals are in each other's way.
        const std::vector<std::vector<double>> chords =
        {
            { 6100, 6300 },                                         // C sharp, E flat
            { 6100, 6300, 6600, 6800, 7000 },                       // the five black keys
            { 6050, 6150 },                                         // a second: C quarter sharp, D quarter flat
            { 6025, 6225, 6425 },                                   // three notes a second apart, each an eighth tone up
            { 6075, 6325, 6575, 6825 },                             // three eighths of a tone up and down: the quarter-tone signs with an arrow
            { 7300, 7500, 7800, 8000 },                             // high: the stem goes down
            { 6000, 6100 },                                         // C and C sharp: two heads at one height
            { 6100, 6175, 6250, 6325, 6400, 6475, 6550, 6625 }      // eight notes three eighths of a tone apart
        };
        for (size_t i = 0; i < chords.size(); ++i)
            addChord (roll.voices[1], 3000.0 * (double) i, chords[i], 2000.0);

        // 3: pitches between the steps of the grid, around the C an octave under middle C.
        const double offGrid[] = { 4800, 4810, 4812, 4813, 4820, 4837, 4838, 4850, 4862, 4863, 4875, 4887, 4888, 4900 };
        for (size_t i = 0; i < std::size (offGrid); ++i)
            addChord (roll.voices[2], 1800.0 * (double) i, { offGrid[i] }, 1400.0);

        // 4: chords over two staves.
        const std::vector<std::vector<double>> wide =
        {
            { 4100, 4600, 5100, 5600, 6100, 6600, 7000 },           // seven sharps and flats, a fourth or so apart
            { 5900, 6000, 6100 },                                   // B, C, C sharp around middle C
            { 5925, 5975, 6025, 6075 },                             // eighth tones around middle C
            { 4350, 5050, 6350, 7050 }                              // quarter tones, far apart
        };
        for (size_t i = 0; i < wide.size(); ++i)
            addChord (roll.voices[3], 6000.0 * (double) i, wide[i], 4500.0);

        // 5: the same pitch under two names, each written in the file by its name.
        struct Named { const char* name; double cents; };
        const Named named[] = { { "C#5", 6100 }, { "Db5", 6100 }, { "D#5", 6300 }, { "Eb5", 6300 }, { "E#5", 6500 }, { "F5", 6500 }, { "F#5", 6600 }, { "Gb5", 6600 },
                                { "G#5", 6800 }, { "Ab5", 6800 }, { "A#5", 7000 }, { "Bb5", 7000 }, { "B5", 7100 }, { "Cb6", 7100 } };
        for (size_t i = 0; i < std::size (named); ++i)
        {
            addChord (roll.voices[4], 2000.0 * (double) i, { named[i].cents }, 1500.0);
            roll.voices[4].chords.back().notes[0].pitchName = named[i].name;
        }

        return roll;
    }
}

/** Stage 2b: the accidentals. */
static void testAccidentals (const juce::File& temp)
{
    line ("### Accidentals (stage 2b)");
    line();
    line ("Which sign a note gets and where the signs of a chord stand, by bach's rules; the signs themselves are judged by eye, "
          "with the roll this test writes, opened in bach.roll beside Maket.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    // --- The sign of each step ---
    {
        const Accidental expected[9] = { Accidental::flat, Accidental::quarterFlatDown, Accidental::quarterFlat, Accidental::naturalDown, Accidental::none,
                                         Accidental::naturalUp, Accidental::quarterSharp, Accidental::quarterSharpUp, Accidental::sharp };
        bool ok = true;
        for (int a = -4; a <= 4; ++a) ok = ok && accidentalOf (a) == expected[a + 4];
        checkRow (ok, "Each alteration has bach's sign: a natural with an arrow for an eighth tone, the half sharp or the reversed flat for a quarter tone, "
                      "those two with an arrow further away for three eighths, sharp or flat for a semitone; none for a natural note");

        int signs8 = 0, signs2 = 0, other2 = 0;
        for (int i = 0; i < 48; ++i)
        {
            if (accidentalOf (spell (6000.0 + 25.0 * i, 8).alteration) != Accidental::none) ++signs8;
            const auto a = accidentalOf (spell (6000.0 + 25.0 * i, 2).alteration);
            if (a != Accidental::none) ++signs2;
            if (a != Accidental::none && a != Accidental::sharp && a != Accidental::flat) ++other2;
        }
        // A pitch given by name keeps its letter.
        struct Named { const char* name; double cents; int steps; int alteration; };
        const Named named[] =
        {
            { "G#6", 8000, 11, 4 },  { "A#6", 8200, 12, 4 },  { "Ab6", 8000, 12, -4 }, { "C#7", 8500, 14, 4 },  { "Db5", 6100, 1, -4 },
            { "E#5", 6500, 2, 4 },   { "Fb5", 6400, 3, -4 },  { "B#4", 6000, -1, 4 },  { "Cb5", 5900, 0, -4 },  { "G6", 7900, 11, 0 },
            { "C-1", -1200, -42, 0 }
        };
        bool namesOk = true;
        for (const auto& n : named) namesOk = namesOk && spellAs (n.name, n.cents, 8) == Spelling { n.steps, n.alteration }
                                                       && spellAs (n.name, n.cents, 2) == Spelling { n.steps, n.alteration };
        namesOk = namesOk && spellAs ("", 8000, 8) == spell (8000, 8) && spell (8000, 8) == Spelling { 12, -4 }      // without a name: A flat
                          && spellAs ("G#6", 8025, 8) == spell (8025, 8)                                             // beyond a sharp from G: the pitch decides
                          && spellAs ("G6", 7925, 8) == Spelling { 11, 1 }                                           // a named G an eighth tone up stays a G
                          && spellAs ("nonsense", 6100, 8) == spell (6100, 8);
        checkRow (namesOk, "A note the file gives by name keeps that letter, as in bach: G#6 is drawn as G sharp (the pitch alone gives A flat), A#6 as A sharp, "
                           "Db5, E#5, Fb5, B#4 and Cb5 as named; without a name, or when the name would need more than a sharp or a flat, the pitch decides");

        checkRow (signs8 == 41 && other2 == 0,
                  "Of the 48 eighth-tone steps of an octave, 41 carry a sign on the eighth-tone grid (all but the seven white keys); "
                  "on the semitone grid every one of them is drawn plain, sharp or flat (" + juce::String (signs2) + " of the 48 with a sign)");
    }

    // --- Where the heads sit (stage 2b.1) ---
    {
        const double w = headWidth;
        auto lefts = [w] (std::vector<int> steps, int direction, std::vector<double> expected, std::vector<bool> auxiliary)
        {
            const auto heads = placeHeads (steps, direction, w);
            bool ok = heads.size() == expected.size();
            for (size_t i = 0; ok && i < heads.size(); ++i)
                ok = near (heads[i].left, expected[i] * w) && heads[i].auxiliaryStem == auxiliary[i];
            return ok;
        };

        checkRow (lefts ({ 0 }, 1, { -1 }, { false }) && lefts ({ 0 }, -1, { 0 }, { false })
                  && lefts ({ 0, 2, 4 }, 1, { -1, -1, -1 }, { false, false, false }),
                  "Heads: left of a stem that goes up, right of one that goes down; notes a third apart sit above each other");
        checkRow (lefts ({ 0, 1 }, 1, { -1, 0 }, { false, false }) && lefts ({ 0, 1 }, -1, { -1, 0 }, { false, false })
                  && lefts ({ 0, 1, 2 }, 1, { -1, 0, -1 }, { false, false, false }),
                  "A second: the lower head left of the stem, the upper right of it, with the stem up or down; three notes a second apart take turns");
        checkRow (lefts ({ 0, 0 }, 1, { -1, 0 }, { false, false }) && lefts ({ 0, 0 }, -1, { -1, 0 }, { false, false }),
                  "Two notes at one height (C and C sharp) sit beside each other, one on each side of the stem");
        checkRow (lefts ({ -1, 0, 0 }, 1, { -1, 0, -2 }, { false, false, true }) && lefts ({ 0, 0, 0 }, 1, { -1, 0, -2 }, { false, false, true }),
                  "B, C and C sharp, and three notes at one height: the third head goes further out and gets a branch from the stem, as in bach.roll");

        const auto cluster = chordNotes ({ 6100, 6175, 6250, 6325, 6400, 6475, 6550, 6625 }, 1);
        const auto clusterHeads = placeHeads ({ 0, 1, 1, 2, 2, 3, 3, 4 }, 1, w);
        int branches = 0;
        for (const auto& h : clusterHeads) if (h.auxiliaryStem) ++branches;
        checkRow (chordIsClear (cluster, placeAccidentals (cluster)) && branches == 3,
                  "Eight notes three eighths of a tone apart (the last chord of the voice \"chords\"): no head over another, three of them on a branch");
    }

    // --- Where the signs stand ---
    {
        const double atHeads = -headWidth - accidentalFromNote;
        const double sharpStep = sizeOf (Accidental::sharp).width + accidentalFromAccidental;

        auto up = placeAccidentals (chordNotes ({ 6100 }, 1));
        auto down = placeAccidentals (chordNotes ({ 6100 }, -1));
        checkRow (near (up[0], atHeads) && near (down[0], -accidentalFromNote),
                  "One note: its sign ends half a staff space left of its head, on either side of the stem");

        auto octave = placeAccidentals (chordNotes ({ 6100, 7300 }, 1));
        auto none = placeAccidentals (chordNotes ({ 6000, 6400, 6700 }, 1));
        checkRow (near (octave[0], atHeads) && near (octave[1], atHeads) && near (none[0], 0.0) && near (none[1], 0.0) && near (none[2], 0.0),
                  "Two sharps an octave apart stand above each other; a chord of natural notes gets no sign");

        auto third = placeAccidentals (chordNotes ({ 6100, 6300 }, 1));
        checkRow (near (third[0], atHeads) && near (third[1], atHeads - sharpStep),
                  "C sharp and E flat in one chord: the lower sign at the heads, the upper one left of it, with bach's gap between them");

        auto fan = placeAccidentals (chordNotes ({ 6100, 6300, 6600 }, 1));
        checkRow (near (fan[0], atHeads) && near (fan[2], atHeads - sharpStep) && near (fan[1], atHeads - 2.0 * sharpStep),
                  "Three signs in each other's way (C sharp, E flat, F sharp): the lowest at the heads, then the highest, the middle one furthest left, as bach orders them");

        auto apart = placeAccidentals (chordNotes ({ 6100, 6600, 6800 }, 1));
        checkRow (near (apart[0], atHeads) && near (apart[2], atHeads) && near (apart[1], atHeads - sharpStep),
                  "C sharp, F sharp, A flat: the outer two are clear of each other and both stand at the heads, the middle one left of them");

        auto second = placeAccidentals (chordNotes ({ 6050, 6150 }, 1));
        const auto secondNotes = chordNotes ({ 6050, 6150 }, 1);
        checkRow (near (secondNotes[0].headLeft, -headWidth) && near (secondNotes[1].headLeft, 0.0) && second[0] <= atHeads + 1.0e-9 && second[1] <= atHeads + 1.0e-9
                  && chordIsClear (secondNotes, second),
                  "A second (C quarter sharp, D quarter flat): the heads on both sides of the stem, both signs left of the left head");
    }

    // --- No sign over another, in the test roll and in the nine rolls ---
    const auto testRoll = accidentalsRoll();
    {
        auto count = [] (const maket::Roll& roll, int& chords, int& crowded, int& unclear)
        {
            for (const auto& voice : roll.voices)
                for (const auto& chord : voice.chords)
                {
                    std::vector<double> cents;
                    for (const auto& n : chord.notes) cents.push_back (n.cents);
                    for (int direction : { 1, -1 })
                    {
                        const auto notes = chordNotes (cents, direction);
                        const auto right = placeAccidentals (notes);
                        int signs = 0;
                        for (const auto& n : notes) if (n.hasAccidental) ++signs;
                        if (direction == 1) { ++chords; if (signs >= 2) ++crowded; }
                        if (! chordIsClear (notes, right)) ++unclear;
                    }
                }
        };

        int chords = 0, crowded = 0, unclear = 0;
        count (testRoll, chords, crowded, unclear);
        checkRow (unclear == 0, "In the roll written below (" + juce::String (chords) + " chords, " + juce::String (crowded)
                                + " with two signs or more) no sign lies over another sign or over a head, and no head over another, with the stem up or down");

        const auto folder = temp.getChildFile ("Roll view 2").getChildFile ("roll txt");
        auto files = folder.findChildFiles (juce::File::findFiles, false, "*.txt");
        if (! files.isEmpty())
        {
            int allChords = 0, allCrowded = 0, allUnclear = 0;
            for (const auto& file : files)
            {
                maket::Roll roll;
                juce::StringArray remarks;
                if (maket::readBachRollFile (file, roll, remarks).wasOk()) count (roll, allChords, allCrowded, allUnclear);
            }
            checkRow (allUnclear == 0, "The same in the " + juce::String (files.size()) + " rolls of `Temp/Roll view 2` (" + juce::String (allChords) + " chords, "
                                       + juce::String (allCrowded) + " with two signs or more)");
        }
    }

    // --- The roll for bach.roll ---
    {
        const auto folder = temp.getChildFile ("written by Maket");
        const auto file = folder.getChildFile ("Accidentals by Maket.txt");
        const bool written = temp.isDirectory() && folder.createDirectory().wasOk() && maket::writeBachRollFile (file, testRoll).wasOk();

        maket::Roll back;
        juce::StringArray remarks;
        bool same = written && maket::readBachRollFile (file, back, remarks).wasOk() && back.voices.size() == testRoll.voices.size()
                    && back.numNotes() == testRoll.numNotes() && maket::writeBachRollText (back) == maket::writeBachRollText (testRoll);
        for (size_t v = 0; same && v < back.voices.size(); ++v)
            for (size_t c = 0; same && c < back.voices[v].chords.size(); ++c)
                for (size_t n = 0; same && n < back.voices[v].chords[c].notes.size(); ++n)
                    same = near (back.voices[v].chords[c].notes[n].cents, testRoll.voices[v].chords[c].notes[n].cents);
        checkRow (same, "`Temp/written by Maket/Accidentals by Maket.txt` is written (5 voices, " + juce::String (testRoll.numNotes())
                        + " notes) and, read back, holds the same notes");
    }
    line();
    line ("The roll `Accidentals by Maket.txt`, for bach.roll beside Maket (the README tells what to look at):");
    line();
    line ("| Voice | Clef | What it holds |");
    line ("| --- | --- | --- |");
    line ("| every step | G | the 49 eighth-tone steps from middle C to the C above, one every 0.6 s |");
    line ("| chords | G | 8 chords, one every 3 s, whose accidentals are in each other's way: C sharp with E flat; the five black keys; a second; "
          "three notes a second apart; four notes three eighths of a tone off a white key; a high chord (stem down); C with C sharp; eight notes three eighths of a tone apart |");
    line ("| off the grid | F | 14 pitches between the steps, one every 1.8 s: 4800, 4810, 4812, 4813, 4820, 4837, 4838, 4850, 4862, 4863, 4875, 4887, 4888 and 4900 cents |");
    line ("| two staves | FG | 4 chords, one every 6 s: seven sharps and flats over both staves; B, C and C sharp; four eighth tones around middle C; four quarter tones far apart |");
    line ("| named | G | 14 notes, one every 2 s, written in the file by name: seven pitches each under two names (C#5 and Db5, D#5 and Eb5, E#5 and F5, F#5 and Gb5, G#5 and Ab5, A#5 and Bb5, B5 and Cb6) |");
    line();
}

namespace
{
    void addGlide (maket::Voice& voice, double onset, double cents, double duration, std::vector<maket::Breakpoint> points)
    {
        addChord (voice, onset, { cents }, duration);
        maket::Breakpoint head;
        auto& note = voice.chords.back().notes[0];
        note.velocity = 100;
        note.breakpoints.push_back (head);
        for (const auto& p : points) note.breakpoints.push_back (p);
    }

    maket::Breakpoint point (double position, double cents, double slope)
    {
        maket::Breakpoint b;
        b.position = position; b.cents = cents; b.slope = slope;
        return b;
    }

    maket::Marker markerAt (double ms, const juce::String& name, const juce::String& role = "none", const juce::String& content = {})
    {
        maket::Marker m;
        m.position = ms;
        juce::StringArray words;
        words.addTokens (name, " ", "");
        for (const auto& w : words) m.names.items.push_back (maket::Llll::makeSymbol (w));
        m.role = role;
        if (content.isNotEmpty()) { m.hasContent = maket::readLlllText (content, m.content).wasOk(); }
        return m;
    }

    /** The roll for the comparison with bach.roll at stage 2c. */
    maket::Roll glidesRoll()
    {
        maket::Roll roll;
        roll.header.push_back (headerOf ("clefs", { "G", "G", "F", "F", "G" }));
        roll.header.push_back (headerOf ("voicenames", { "glides", "flags", "muted voice", "locked voice", "velocities" }));
        roll.header.push_back (headerOf ("midichannels", { "1", "2", "3", "4", "5" }));
        roll.voices.resize (5);

        // 1: glides.
        auto& glides = roll.voices[0];
        addGlide (glides, 0.0,     6000.0, 2500.0, { point (1.0, 700.0, 0.0) });                                  // straight up a fifth
        addGlide (glides, 3000.0,  6700.0, 2500.0, { point (1.0, -700.0, 0.5) });                                 // down, late
        addGlide (glides, 6000.0,  6700.0, 2500.0, { point (1.0, -700.0, -0.5) });                                // down, early
        addGlide (glides, 9000.0,  6400.0, 2500.0, { point (0.25, 300.0, 0.0), point (0.5, -200.0, -0.5),
                                                     point (0.75, 100.0, 0.5), point (1.0, 0.0, 0.0) });         // three points on the way
        addGlide (glides, 12000.0, 6100.0, 2500.0, { point (1.0, 200.0, -0.6) });                                 // from C sharp up a tone
        addGlide (glides, 15000.0, 6900.0, 2500.0, { point (0.5, 0.0, 0.0), point (1.0, 400.0, 0.0) });           // held, then up
        addChord (glides, 18000.0, { 6000.0, 6400.0, 6700.0 }, 2500.0);                                           // a chord, only its top note glides
        glides.chords.back().notes[2].breakpoints = { maket::Breakpoint(), point (1.0, 500.0, 0.0) };

        // 2: single notes and a chord that are locked, muted or solo.
        auto& flags = roll.voices[1];
        const int noteFlags[] = { 0, maket::flagLocked, maket::flagMuted, maket::flagSolo, maket::flagLocked | maket::flagMuted,
                                  maket::flagMuted | maket::flagSolo, maket::flagLocked | maket::flagMuted | maket::flagSolo };
        for (size_t i = 0; i < std::size (noteFlags); ++i)
        {
            addChord (flags, 2000.0 * (double) i, { 6700.0 }, 1500.0);
            flags.chords.back().notes[0].flags = noteFlags[i];
        }
        addChord (flags, 14000.0, { 6400.0, 6700.0, 7100.0 }, 1500.0);          // a muted chord
        flags.chords.back().flags = maket::flagMuted;
        addChord (flags, 16000.0, { 6400.0, 6700.0, 7100.0 }, 1500.0);          // a chord with one locked note
        flags.chords.back().notes[1].flags = maket::flagLocked;
        addChord (flags, 18000.0, { 6400.0, 6700.0, 7100.0 }, 1500.0);          // a solo chord
        flags.chords.back().flags = maket::flagSolo;

        // 3 and 4: a muted voice, a locked voice.
        for (int v : { 2, 3 })
        {
            for (int i = 0; i < 7; ++i)
                addChord (roll.voices[(size_t) v], 3000.0 * i, { 4800.0 + 200.0 * (i % 4) }, 2400.0);
            roll.voices[(size_t) v].flags = v == 2 ? maket::flagMuted : maket::flagLocked;
        }

        // 5: velocities from 1 to 127.
        const int velocities[] = { 1, 18, 36, 54, 72, 90, 108, 127 };
        for (size_t i = 0; i < std::size (velocities); ++i)
        {
            addChord (roll.voices[4], 2500.0 * (double) i, { 6000.0 + 100.0 * (double) i, 6700.0 + 100.0 * (double) i }, 2000.0);
            for (auto& n : roll.voices[4].chords.back().notes) n.velocity = velocities[i];
        }

        roll.markers.push_back (markerAt (1000.0, "first"));
        roll.markers.push_back (markerAt (1400.0, "second close by"));
        roll.markers.push_back (markerAt (1800.0, "third"));
        roll.markers.push_back (markerAt (4500.0, "fourth"));
        roll.markers.push_back (markerAt (9000.0, "section B"));
        roll.markers.push_back (markerAt (12000.0, "bar", "barline"));
        roll.markers.push_back (markerAt (15000.0, "metre", "timesig", "3 4"));
        roll.markers.push_back (markerAt (18000.0, "end"));
        return roll;
    }
}

/** Stage 2c: glides, markers. */
static void testGlidesAndMarkers (const juce::File& temp)
{
    line ("### Glides and markers (stage 2c)");
    line();
    line ("The line of a note whose pitch moves, and the lines marker names are written on, by bach's rules. The look of it, "
          "and of what is locked, muted or solo, is judged by eye with the roll this test writes.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    checkRow (near (slopeCurve (0.5, 0.0), 0.5) && near (slopeCurve (0.5, 0.5), 0.125) && near (slopeCurve (0.5, -0.5), 0.875)
              && near (slopeCurve (0.0, 0.5), 0.0) && near (slopeCurve (1.0, 0.5), 1.0) && near (slopeCurve (0.0, -0.5), 0.0) && near (slopeCurve (1.0, -0.5), 1.0)
              && near (slopeCurve (0.3, 1.0), 0.0) && near (slopeCurve (0.3, -1.0), 1.0),
              "bach's curve: slope 0 is a straight line; with slope 0.5 the line is an eighth of the way at half time, with -0.5 seven eighths; both begin and end where they should");

    {
        struct Height { double cents, steps; };
        const Height heights[] = { { 6000, 0 }, { 6100, 0.5 }, { 6200, 1 }, { 6400, 2 }, { 6450, 2.5 }, { 6500, 3 }, { 6600, 3.5 }, { 7100, 6 },
                                   { 7150, 6.5 }, { 7200, 7 }, { 5900, -1 }, { 4800, -7 }, { 6025, 0.125 } };
        bool ok = true;
        for (const auto& h : heights) ok = ok && near (continuousSteps (h.cents), h.steps);
        checkRow (ok, "The height of a pitch between the notes: C sharp half a step above C, a quarter tone above E half way to F, across octaves");
    }

    {
        const auto points = glidePoints ({ 0, 4 }, { maket::Breakpoint(), point (0.5, 200.0, 0.0), point (1.0, -100.0, 0.5) });      // from C sharp
        const bool shape = points.size() == 3 && near (points[0].steps, 0.0) && near (points[1].steps, 1.0) && near (points[2].steps, -1.0)
                           && near (points[2].position, 1.0);
        const bool between = near (glideStepsAt (points, 0.25), 0.5) && near (glideStepsAt (points, 0.75), 0.75) && near (glideStepsAt (points, 1.0), -1.0);
        checkRow (shape && between && glidePoints ({ 0, 0 }, {}).empty() && glidePoints ({ 0, 0 }, { maket::Breakpoint() }).empty(),
                  "A glide begins at the head and goes through its points: from a C sharp, +200 cents lies on D and -100 on B, as in bach; "
                  "half way along a curved piece the height follows the curve; a note without points has no glide");
    }

    {
        const auto lines = markerNameLines ({ 0, 50, 100, 300 }, { 80, 80, 80, 50 }, 6);
        const auto apart = markerNameLines ({ 0, 100, 200 }, { 80, 80, 80 }, 6);
        const auto stairs = markerNameLines ({ 0, 10, 20 }, { 80, 80, 80 }, 6);
        checkRow (lines == std::vector<int> ({ 0, 1, 0, 0 }) && apart == std::vector<int> ({ 0, 0, 0 }) && stairs == std::vector<int> ({ 0, 1, 2 }),
                  "Marker names: a name that would run into the one before goes a line lower, and back to the first line when there is room again");
    }

    // The rolls of Temp/Roll view 2: every glide is whole.
    {
        const auto folder = temp.getChildFile ("Roll view 2").getChildFile ("roll txt");
        auto files = folder.findChildFiles (juce::File::findFiles, false, "*.txt");
        if (! files.isEmpty())
        {
            int glides = 0, markers = 0, flagged = 0, broken = 0;
            for (const auto& file : files)
            {
                maket::Roll roll;
                juce::StringArray remarks;
                if (! maket::readBachRollFile (file, roll, remarks).wasOk()) { ++broken; continue; }
                markers += (int) roll.markers.size();
                for (const auto& voice : roll.voices)
                    for (const auto& chord : voice.chords)
                        for (const auto& note : chord.notes)
                        {
                            if ((note.flags | chord.flags | voice.flags) != 0) ++flagged;
                            const auto points = glidePoints (spellAs (note.pitchName, note.cents, 8), note.breakpoints);
                            if (points.empty()) continue;
                            ++glides;
                            bool ok = near (points.front().position, 0.0) && near (points.back().position, 1.0);
                            for (size_t i = 1; ok && i < points.size(); ++i)
                                ok = points[i].position >= points[i - 1].position && std::isfinite (points[i].steps);
                            if (! ok) ++broken;
                        }
            }
            checkRow (broken == 0, "In the " + juce::String (files.size()) + " rolls of `Temp/Roll view 2`: " + juce::String (glides) + " notes glide, each line whole from head to tail; "
                                   + juce::String (markers) + " markers; " + juce::String (flagged) + " notes locked, muted or solo (themselves, their chord or their voice)");
        }
    }

    // The roll for bach.roll.
    const auto testRoll = glidesRoll();
    {
        const auto folder = temp.getChildFile ("written by Maket");
        const auto file = folder.getChildFile ("Glides and markers by Maket.txt");
        const bool written = temp.isDirectory() && folder.createDirectory().wasOk() && maket::writeBachRollFile (file, testRoll).wasOk();

        maket::Roll back;
        juce::StringArray remarks;
        bool same = written && maket::readBachRollFile (file, back, remarks).wasOk() && back.voices.size() == testRoll.voices.size()
                    && back.numNotes() == testRoll.numNotes() && back.markers.size() == testRoll.markers.size()
                    && maket::writeBachRollText (back) == maket::writeBachRollText (testRoll);
        int glides = 0, flagged = 0;
        if (same)
            for (size_t v = 0; v < back.voices.size(); ++v)
                for (const auto& chord : back.voices[v].chords)
                    for (const auto& note : chord.notes)
                    {
                        if (! note.breakpoints.empty()) ++glides;
                        if ((note.flags | chord.flags | back.voices[v].flags) != 0) ++flagged;
                    }
        checkRow (same && glides == 7 && flagged == 27 && back.markers[6].role == "timesig" && back.markers[6].hasContent
                  && back.voices[2].name == "muted voice" && back.voices[0].name == "glides",
                  "`Temp/written by Maket/Glides and markers by Maket.txt` is written (5 voices, " + juce::String (testRoll.numNotes()) + " notes, 7 of them gliding, "
                  + juce::String (flagged) + " locked, muted or solo; " + juce::String ((int) testRoll.markers.size()) + " markers) and, read back, holds the same; "
                  "a voice name of two words is written as two words, as bach keeps it, and read back as one name");
    }
    line();
    line ("The roll `Glides and markers by Maket.txt`, for bach.roll beside Maket (the README tells what to look at):");
    line();
    line ("| Voice | Clef | What it holds |");
    line ("| --- | --- | --- |");
    line ("| glides | G | 7 notes, one every 3 s: up a fifth in a straight line; down a fifth with slope 0.5, then with -0.5; three points on the way; "
          "from C sharp up a tone (slope -0.6); held, then up; a chord whose top note alone glides |");
    line ("| flags | G | one G every 2 s: plain, locked, muted, solo, locked and muted, muted and solo, all three; then a muted chord, a chord with one locked note, a solo chord |");
    line ("| muted voice | F | 7 notes in a voice that is muted |");
    line ("| locked voice | F | the same notes in a voice that is locked |");
    line ("| velocities | G | 8 chords of two notes with velocities 1, 18, 36, 54, 72, 90, 108 and 127 |");
    line ("| markers | | first (1 s), second close by (1.4 s), third (1.8 s), fourth (4.5 s), section B (9 s), a bar line (12 s), a time signature 3 4 (15 s), end (18 s) |");
    line();
}

void testNotation (const juce::File& temp)
{
    line ("## 8. Notation: where notes sit on the staves (stages 2a to 2c)");
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

    testAccidentals (temp);
    testGlidesAndMarkers (temp);
}
