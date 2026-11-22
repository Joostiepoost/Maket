// TimingTest.h — the timing tests of stage 1d: how evenly do Maket's
// messages arrive in Max, and how does that compare with bach.roll playing
// the same passage inside Max?
//
// A passage is a fixed list of notes. Maket plays it by sending, for every
// note-on and note-off, a numbered tick to its logging patch in Max (and,
// when wanted, the real note to the ePlayer). The patch notes the time each
// tick arrives. Afterwards the arrival times are held against the times the
// messages were due.
//
// The two clocks (Maket's and Max's) have different zero points, so only
// the evenness can be measured, not the delay itself: the arrival times are
// shifted as a whole until the middle one of all messages sits exactly on
// time, and what is reported is how far the others lie from that.
//
// For the reference, the same passage is written as a bach.roll file and
// played by bach.roll in Max, with its playout sent to the same patch.
//
// No interface code.

#pragma once

#include <JuceHeader.h>
#include <vector>

#include "../Model/Roll.h"
#include "OscScheduler.h"

namespace maket::timing
{

struct PassageNote
{
    double onset = 0.0;         // ms from the start of the passage
    double duration = 0.0;      // ms
    double pitch = 60.0;        // MIDI note number
    int part = 0;               // which part of the passage it belongs to
};

struct Passage
{
    juce::String name;                  // "dense", "long"
    juce::StringArray parts;            // what each part is, in words
    std::vector<PassageNote> notes;     // in order of onset

    double length() const;              // the end of the last note, in ms
    int numMessages() const             { return 2 * (int) notes.size(); }     // a note-on and a note-off each
};

/** About 23 seconds, four parts: a fast line, chords of eight, an irregular
    passage, a burst. The same notes at every call. */
Passage densePassage();

/** A note every 100 ms for `minutes` minutes. */
Passage longPassage (double minutes);

/** The passage as a roll of one voice, to be written as a bach.roll file. */
Roll passageAsRoll (const Passage& passage, int velocity);

/** The tick that goes with a note: its number says which note and whether
    it is the note-on (even) or the note-off (odd). */
OscMessage tickMessage (int noteIndex, bool isNoteOff);

// --- Reading the logs ---------------------------------------------------------------

/** One line of the logging patch's file: the patch's number, the time on
    Max's clock in ms, the message. */
struct LogLine
{
    int number = 0;
    double time = 0.0;
    juce::String message;
};

/** Reads the text file the patch's coll writes (`12, 3410.52 noteon Cl 60. 100;`). */
std::vector<LogLine> readCollLog (const juce::String& text);

/** One line of the send log that Maket's OSC test page saves. */
struct SendLogLine
{
    double due = 0.0, late = 0.0;
    juce::String message;
};
std::vector<SendLogLine> readMaketSendLog (const juce::String& text);

// --- Comparing ----------------------------------------------------------------------

/** How a set of messages lay around their times. All in milliseconds. */
struct Spread
{
    int count = 0;
    double typical = 0.0;       // the standard deviation
    double half = 0.0;          // half of them lie within this of their time
    double most = 0.0;          // 95 in 100 within this
    double nearlyAll = 0.0;     // 99 in 100 within this
    double worst = 0.0;         // the farthest one, early or late
    double earliest = 0.0;      // the most early one (negative)
    double latest = 0.0;        // the most late one
};

Spread spreadOf (const std::vector<double>& deviations);

struct Result
{
    bool usable = false;
    juce::String problem;               // why not, when not usable

    int expected = 0;                   // messages that should be in the log
    int arrived = 0;
    int outOfOrder = 0;                 // arrived before a message that was due earlier
    Spread all;                         // every message
    Spread noteOns;                     // the note-ons alone (bach.roll sends nothing else)
    std::vector<Spread> perPart;        // all messages, per part of the passage
    Spread leftMaket;                   // how late the messages left Maket (empty for a bach.roll take)
    double span = 0.0;                  // from the first to the last message, in ms
    double driftPerMinute = 0.0;        // the two clocks drifting apart, ms per minute (meaningful for long runs)
    juce::String mode;                  // for a bach.roll take: "notewise" or "chordwise"
};

/** A run from Maket: the ticks the send thread sent, against the patch's log. */
Result compareMaketRun (const Passage& passage, const std::vector<OscScheduler::Sent>& sent,
                        const std::vector<LogLine>& log);

/** The same without Maket's record of what it sent: each tick against the
    time the passage gives it. For a log that was saved by hand. */
Result compareTicksToPassage (const Passage& passage, const std::vector<LogLine>& log);

/** The highest tick number in a log, or -1 when it holds no ticks. */
int highestTick (const std::vector<LogLine>& log);

/** A take from bach.roll: its playout in the patch's log, each line against
    the onset it carries. */
Result compareBachTake (const Passage& passage, const std::vector<LogLine>& log);

/** Any two logs of the same messages in the same order (the logs of stage 1c). */
Result compareLogs (const std::vector<SendLogLine>& sent, const std::vector<LogLine>& log);

/** True when the log holds lines of bach.roll's playout. */
bool hasBachLines (const std::vector<LogLine>& log);

/** The result in a few lines of plain text, for the page and the results file. */
juce::String describe (const Result& result, const Passage& passage);

/** The heading and one row of the table in the results file. */
juce::String tableHeading();
juce::String tableRow (const juce::String& when, const juce::String& passage, const juce::String& source,
                       const juce::String& condition, const Result& result);

} // namespace maket::timing
