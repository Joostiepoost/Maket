// Staves.h — the staves of a voice: what each of bach's clefs means, where
// a note sits on them, which ledger lines it needs, and how the voices of
// a roll are stacked.
//
// The numbers are bach's (`compute_middleC_position_for_voice`,
// `get_num_staves_voice`, `calculate_voice_offsets` and the list of clefs in
// its source), so that a roll is laid out as bach.roll lays it out.
//
// Heights are in bach's own unit, which the view multiplies by its
// vertical zoom: a step (from a line to the space next to it) is 3 units,
// so the lines of a staff are 6 units apart. A voice begins at its
// `offset`; the top line of its first staff lies 40 units under that;
// every staff the clef has adds 42 units to where the next voice begins,
// and the voice's own spacing comes on top of that.
//
// No interface code.

#pragma once

#include <JuceHeader.h>
#include <vector>

#include "../Model/Roll.h"

namespace maket::notation
{

constexpr double stepUnits = 3.0;           // from a line to the next space
constexpr double topLineUnits = 40.0;       // from a voice's offset to the top line of its first staff
constexpr double staffAdvanceUnits = 42.0;  // what each staff adds to the offset of the next voice
constexpr double defaultSpacingUnits = 17.0;

enum class ClefSign { g, f, c, percussion, none };

/** One staff of five lines. Steps are diatonic steps above middle C. */
struct Staff
{
    ClefSign sign = ClefSign::g;
    int octaves = 0;            // the little 8 or 15 at the clef: 1 and 2 above, -1 and -2 below
    int topLineStep = 10;       // the step of its top line; the other lines lie 2, 4, 6 and 8 steps under it
    int clefLineStep = 4;       // the line the clef sits on (the G, the F or the C line), as a step

    int bottomLineStep() const  { return topLineStep - 8; }
};

/** A clef as bach names it (G, F, FG, FFGG, G8vb, Alto, Percussion, None ...). */
struct ClefLayout
{
    juce::String name;
    std::vector<Staff> staves;  // from the top one down
    bool known = true;          // false: a name Maket does not know; laid out as G

    int numStaves() const       { return (int) staves.size(); }

    /** Units from the voice's offset down to a step. */
    double unitsOf (double step) const;

    /** Units from the voice's offset to the top line of the first staff and to the bottom line of the last. */
    double topUnits() const     { return topLineUnits; }
    double bottomUnits() const;
};

ClefLayout clefLayout (const juce::String& clefName);

/** The ledger lines a note head at `step` needs, as steps: the lines between
    the head and the nearest staff, the one through the head included. Empty
    when the head lies on a staff. */
std::vector<int> ledgerSteps (const ClefLayout&, int step);

/** Where one voice is drawn. */
struct VoicePlace
{
    int voice = 0;              // index in the roll
    ClefLayout clef;
    double offset = 0.0;        // units from the top of the roll
    bool hidden = false;        // the header's hidevoices
    bool ownsStaves = true;     // false: a further part of a voice ensemble, drawn on the staves of the part before it
};

struct RollLayout
{
    std::vector<VoicePlace> voices;
    double height = 0.0;        // units, the space under the last voice included
};

/** Stacks the voices as bach does, from the header's clefs, voicespacing,
    hidevoices and numparts. */
RollLayout layoutVoices (const Roll&);

} // namespace maket::notation
