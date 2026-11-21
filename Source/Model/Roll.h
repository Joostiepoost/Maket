// Roll.h — a roll: voices of chords of notes in proportional time, with
// pitch breakpoints, slots and markers. It holds what a bach.roll file
// holds, level for level (proposal, section 5).
//
// Time is in milliseconds and pitch in cents, both as real numbers that are
// never rounded. What Maket does not interpret yet (the content of a slot,
// a header entry it does not use) is kept as the list it came as.
//
// No interface code.

#pragma once

#include <JuceHeader.h>
#include <vector>

#include "Llll.h"

namespace maket
{

/** The flag at the end of a note, a chord or a voice: a sum of these. */
enum RollFlag { flagLocked = 1, flagMuted = 2, flagSolo = 4 };

/** A point along a note's duration line. */
struct Breakpoint
{
    double position = 0.0;      // 0 at the note head, 1 at its tail
    double cents = 0.0;         // pitch offset from the note's pitch
    double slope = 0.0;         // -1 to 1: the curve of the segment that leads to this point
    bool hasVelocity = false;
    int velocity = 0;
};

/** The content of one slot of one note. */
struct SlotValue
{
    int number = 0;             // bach's slot number, 1 to 30
    Llll content;               // the items after the number, as a list
};

struct Note
{
    double cents = 6000.0;
    double duration = 0.0;      // ms
    int velocity = 100;
    int flags = 0;
    std::vector<Breakpoint> breakpoints;    // empty when the note does not glide
    std::vector<SlotValue> slots;
    std::vector<Llll> others;               // name, graphic and whatever else the file gave; kept

    bool isMuted() const        { return (flags & flagMuted) != 0; }
    const SlotValue* slot (int number) const;
};

struct Chord
{
    double onset = 0.0;         // ms
    int flags = 0;
    std::vector<Note> notes;
    std::vector<Llll> others;   // name; kept

    bool isMuted() const        { return (flags & flagMuted) != 0; }
};

struct Voice
{
    int flags = 0;
    std::vector<Chord> chords;

    // From the header, when the file gives them:
    juce::String name;
    juce::String clef;
    int midiChannel = 1;

    bool isMuted() const        { return (flags & flagMuted) != 0; }
    int numNotes() const;
};

struct Marker
{
    double position = 0.0;      // ms
    Llll names;                 // one name or several; an empty list when the marker has none
    juce::String role;          // none, barline, timesig, tempo, division, subdivision ...
    Llll content;               // what belongs to the role, such as [ 3 4 ] for a time signature

    juce::String nameAsText() const;
};

/** The definition of one slot, from the file's slotinfo. */
struct SlotInfo
{
    int number = 0;
    juce::String name;
    juce::String type;          // function, text, int, float, llll, filelist, dynamics ...
    juce::String key;
    juce::String temporalMode;  // none, relative, milliseconds
    Llll all;                   // the whole definition as the file gave it
};

/** One entry of the file's header, in the order of the file. */
struct HeaderEntry
{
    juce::String name;          // slotinfo, clefs, markers ...
    Llll content;               // the whole entry, its name included
};

struct Roll
{
    std::vector<HeaderEntry> header;
    std::vector<SlotInfo> slotInfo;
    std::vector<Marker> markers;
    std::vector<Voice> voices;

    const HeaderEntry* headerEntry (const char* name) const;
    const SlotInfo* slot (int number) const;

    // Counts over the whole roll.
    int numChords() const;
    int numNotes() const;
    int numMutedNotes() const;                  // muted itself, or in a muted chord or voice
    int numNotesWithBreakpoints() const;
    int numBreakpoints() const;                 // stored points, the first and last of a gliding note included
    int numFilledSlots() const;
    int numFilledSlots (int slotNumber) const;
    double length() const;                      // the end of the last note, in ms
};

} // namespace maket
