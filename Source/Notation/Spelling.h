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

} // namespace maket::notation
