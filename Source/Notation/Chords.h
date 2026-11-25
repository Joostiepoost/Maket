// Chords.h — the notes of one chord beside each other: which heads go to the
// far side of the stem, and where the accidentals stand left of the chord.
//
// As bach.roll does it (`calculate_chord_parameters` in bach's source).
// Lengths in staff spaces, counted from the stem. No interface code.

#pragma once

#include <vector>

namespace maket::notation
{

/** Heads a second (or less) apart cannot sit above each other: every other one goes to the far side of the stem.
    `steps` are the notes' heights in diatonic steps, `direction` the stem's (1 up, -1 down); counted from the end
    of the stem that holds the heads. Returned for each note: whether its head sits on the far side
    (right of a stem that goes up, left of one that goes down). */
std::vector<bool> farSideHeads (const std::vector<int>& steps, int direction);

/** One note of a chord, for placing the accidentals left of it. Lengths in staff spaces. */
struct ChordNote
{
    int step = 0;                   // the note's height in diatonic steps
    double headLeft = 0.0;          // the left edge of its head, counted from the stem (negative: left of the stem)
    bool hasAccidental = false;
    double above = 0.0, below = 0.0;    // how far the sign reaches above and below the note's height
    double width = 0.0;
};

/** Where bach.roll puts the accidentals of a chord (`calculate_chord_parameters` in its source): for each note,
    the right edge of its sign counted from the stem (negative: left of it); 0 for a note without one.
    bach keeps, for every tenth of a step in height, how far left the chord reaches: at first the stem, then
    each head pushes it half a space left of itself, over one step above and below. Then, again and again, it
    takes the accidental that can sit furthest to the right (on a tie the lowest, then the highest, then the
    second lowest ...: the signs fan out from the outer notes) and puts it there; the heights that sign covers
    then reach to its left edge, less a small gap. */
std::vector<double> placeAccidentals (const std::vector<ChordNote>&);

constexpr double accidentalFromNote = 0.5;              // bach: 3 of its units; a staff space is 6
constexpr double accidentalFromAccidental = 0.8 / 6.0;  // bach: 0.8 of its units

} // namespace maket::notation
