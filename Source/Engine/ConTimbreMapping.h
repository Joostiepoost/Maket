// ConTimbreMapping.h — how the notes of a roll become commands for the
// conTimbre ePlayer, and the comparison of those commands with what
// Joost's Max patch (ScAst) sends for the same roll.
//
// The mapping follows the Max patch `cpe_poly`, which plays one voice of a
// bach.roll on the ePlayer. For every note that is not muted, in this order:
//
//   slot 1 (text)       program <voice> <name>                at the onset
//   slot 3 (function)   gain <voice> <dB>                     at each point's time
//   the note            noteon <voice> <pitch> <velocity>     at the onset,
//                       followed by the first two items of slot 4 when it is filled
//                       noteoff <voice> <pitch>               at the end
//   slot 2 (function)   dynamic <voice> <pitch> <value of the next point> <ms to it>
//                                                             at each point but the last
//   pitch breakpoints   glissando <voice> <pitch> <semitones at the next point> <ms to it>
//                                                             at each breakpoint but the last
//
// The pitch is the note's pitch in cents divided by 100. The times of slots
// 2 and 3 are milliseconds after the onset, as the roll keeps them. The
// value of the first point of slot 2 is not sent: the note starts at its
// velocity. The moment a dynamic or a glissando is sent is rounded to a
// whole millisecond, as the patch does; the durations inside them are not.
// Curves between points are not sent: the ePlayer shapes them itself.
//
// A marker whose name begins with `cpe` is sent as a command of its own.
//
// No interface code.

#pragma once

#include <JuceHeader.h>
#include <vector>

#include "../Model/Roll.h"
#include "Osc.h"
#include "TimingTest.h"

namespace maket::contimbre
{

struct TimedMessage
{
    double time = 0.0;      // ms from the start of the roll
    OscMessage message;
    int voice = -1;         // which voice of the roll it comes from (from 0); -1 for a marker
    double noteOnset = 0.0; // the onset of the note it belongs to; for a marker its own time
    double moment = 0.0;    // the time it is due by the roll; `time` lies after it when the commands of a moment are spaced
};

/** The three ways in which the stream of the Max patch differs from Maket's
    own, each to be switched on by itself (stage 1f.1, to find out whether
    any of them is heard). All off is Maket's way. */
struct MappingOptions
{
    /** The patch's order: of the notes of one voice that begin together
        first every program and note-on, then every gain, dynamic and
        glissando. Maket's own order is, per note: program, gain, note-on,
        dynamic, glissando. */
    bool noteOnFirst = false;

    /** The patch sends `glissando <voice> <pitch> 0. <duration>` for every
        note that has no pitch breakpoints. Maket sends nothing for it. */
    bool glissandoToZero = false;

    /** Milliseconds between the commands of one voice that are due at the
        same moment. In the patch they follow each other at about 0.1 ms;
        Maket sends them as fast as it can (about 0.03 ms apart). */
    double spacing = 0.0;

    /** Every note addressed by a number of its own (`ctnote`, `ctdynamic`,
        `ctglissando`, `ctnoteoff`) instead of by its pitch: Maket's default
        way from stage 1f.4 on (decided by Joost on 23 Nov 2026), because two
        sounding notes of one pitch can then be told apart. The voice at
        place k of the roll (from 1) is played on the ePlayer's voice named
        "voice" + k, whatever its name in the roll; program and gain go to
        that name. The notes are numbered from 1 in the order of the roll. */
    bool byNumber = false;

    /** By number: the target of a dynamic ramp on the ePlayer's own scale
        for `ctdynamic`, from 0. to 1.: the roll's target (0 to 127) divided
        by 127 (stage 1f.7). Found by Joost on 24 Nov 2026: `ctdynamic` goes
        to an absolute target, as the manual says, but takes 1 for the top,
        so every roll target of 1 or more came out as the top. Off: the
        roll's target as it stands, as sent in 1f.4. */
    bool normalizedTargets = true;
};

/** The commands for the voices of `roll` named in `voices` (numbers from 0;
    an empty list means every voice that has a name), in order of time. */
std::vector<TimedMessage> messagesForRoll (const Roll& roll, const std::vector<int>& voices = {}, const MappingOptions& options = {});

/** What to send when playing begins at `from` ms instead of at the start:
    the commands of the notes that begin at or after it, and before them,
    at `from` itself, the last program and the last gain each voice was
    given earlier, so that the first notes sound as they would in a run
    from the start. (The Max patch does not do the latter: there a voice
    keeps whatever was played last.) Notes already sounding at `from` are
    left out. */
std::vector<TimedMessage> messagesFrom (const std::vector<TimedMessage>& all, double from);

// --- Held against a log from Max -------------------------------------------------------

struct Comparison
{
    bool usable = false;
    juce::String problem;

    int inMaket = 0;                    // commands Maket makes for the roll
    int inLog = 0;                      // commands in the log
    int matched = 0;                    // the same command at the same time in both
    juce::StringArray commands;         // noteon, noteoff ... as far as they occur
    std::vector<int> maketPerCommand, logPerCommand, matchedPerCommand;

    juce::StringArray onlyInLog;        // the first of them, each on a line, with its time
    juce::StringArray onlyInMaket;
    juce::StringArray nearMisses;       // a command in the log beside Maket's nearest one of that kind
    int numOnlyInLog = 0, numOnlyInMaket = 0;
    juce::StringArray otherVoices;      // voices in the log that Maket was not asked to play

    // What is known of the Max patch and of the ePlayer, counted and kept out of the left-overs.
    int numZeroGlissandi = 0;           // the patch's `glissando ... 0.` for a note without pitch breakpoints
    int numChordFault = 0;              // commands of a chord that the patch sends with the pitch of the chord's last note
    int numStops = 0;                   // `all_notes_off` when playing was stopped in Max
    juce::StringArray playersOwn;       // what the ePlayer's own Max side sends: "171 set_window_titles"

    // Pairs in which a number is whole on one side and real on the other (60 and 60.): the
    // same value, but not the same kind of number on its way to the ePlayer.
    int numKindDifferences = 0;
    juce::StringArray kindDifferences;  // the first of them: Max's line, then Maket's

    // Commands of one voice that are due at the same moment (program, gain, note-on ... of a
    // note): the times they come in another order in the log than Maket sends them.
    int numOrderDifferences = 0;
    juce::StringArray orderDifferences; // "12 times   Max: program, noteon, gain   Maket: program, gain, noteon"

    double offset = 0.0;                // Max's clock minus the roll's time, at the middle command
    timing::Spread timing;              // how far the matched ones lie from each other in time
};

/** Holds `expected` against the commands in a log of the logging patch.
    Known differences are counted apart (see Comparison). Two
    commands are the same when their words are equal and their numbers
    differ by less than 0.002; they are paired when they also lie within a
    quarter of a second of each other once the two clocks are laid together. */
Comparison compareWithLog (const std::vector<TimedMessage>& expected, const std::vector<timing::LogLine>& log);

juce::String describe (const Comparison& comparison);

} // namespace maket::contimbre
