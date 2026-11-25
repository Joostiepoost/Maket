// Spelling.h — which note and which accidental a pitch is drawn as.
//
// The rule is bach's own (`t_pitch::fromMC` in bach's source, `bach_pitch.cpp`,
// with the table that `fill_full_repr_talea_for_major_scale` gives for C major
// and the preference "auto"): the pitch is brought to the nearest step of the
// roll's grid, and every step of the grid has one fixed spelling. In an
// octave of eighth tones, from C:
//
//     C  +1/8  +1/4  +3/8  C#   then towards D:  -3/8  -1/4  -1/8  D ...
//     D ... up to +3/8, then Eb, and towards E: -3/8 -1/4 -1/8
//     E +1/8 +1/4, then F -1/8
//     F ... F#, G ... Ab, A ... Bb, B +1/8 +1/4, then C -1/8
//
// So the black keys are C#, Eb, F#, Ab, Bb, as bach writes them.
//
// Other keys than C major and a preference for sharps or flats are not
// here yet; none of Joost's rolls uses them.
//
// No interface code.

#pragma once

#include <JuceHeader.h>

namespace maket::notation
{

struct Spelling
{
    int steps = 0;          // diatonic steps above middle C (6000 cents): D above it is 1, the B below it -1
    int alteration = 0;     // in eighths of a tone: 4 a sharp, -4 a flat, 2 a quarter tone up, 1 an eighth tone up

    bool operator== (const Spelling& o) const   { return steps == o.steps && alteration == o.alteration; }
};

/** `toneDivision`: steps per whole tone, as bach's attribute: 2 semitones,
    4 quarter tones, 8 eighth tones. Anything else is taken as 8. */
Spelling spell (double cents, int toneDivision = 8);

/** The cents of a spelled note: the way back, for tests. */
double centsOf (const Spelling&);

/** The same for a note whose pitch the file gives as a name (G#6, Bb4): bach keeps the letter that was named
    (`note_compute_approximation` in its source) and draws the accidental that leads from there to the pitch
    on the grid, so G#6 stays a G sharp where the pitch alone would be drawn as A flat. Without a name, with
    one that cannot be read, or when the note would need more than a sharp or a flat: as `spell`. */
Spelling spellAs (const juce::String& pitchName, double cents, int toneDivision = 8);

/** The sign drawn for an alteration, as bach.roll draws it with its own font: an eighth tone is a natural
    with an arrow, a quarter tone the half sharp or the reversed flat, three eighths the quarter-tone sign
    with an arrow that points further away (half sharp with an arrow up, reversed flat with an arrow down).
    A natural note has no sign: in a roll bach gives every altered note its own accidental and never draws
    a natural (its "Classically" is, in a roll, "altered notes, no naturals"). */
enum class Accidental { none, naturalUp, quarterSharp, quarterSharpUp, sharp, naturalDown, quarterFlat, quarterFlatDown, flat };

Accidental accidentalOf (int alteration);

/** The note in words, for a list: "C5", "C5 sharp", "D5 three eighths flat" ... (middle C is C5, as in bach). */
juce::String spellingName (const Spelling&);

} // namespace maket::notation
