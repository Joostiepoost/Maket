// PlayerStudy.h — a study of how the conTimbre ePlayer takes its commands
// (stage 1f.2).
//
// The recordings of stages 1e and 1f.1 showed that the ePlayer makes about
// one dynamic ramp in five only in part, whoever sends it. This study plays
// one short figure many times and changes one thing at a time, to find out
// whether anything on the sender's side makes the player follow more surely:
//
//   - the note addressed by its pitch (`noteon`, `dynamic`) or by a number
//     of its own (`ctnote`, `ctdynamic`);
//   - how long after the note-on the first `dynamic` is sent;
//   - whether the command of the next ramp comes before, at, or after the
//     end of the ramp before it;
//   - the player's dynamic correction on or off.
//
// The figure: two notes of one voice, the second beginning while the first
// still sounds, as in the opening of the cello in doodsbed.
//
//   note 1   67.4,  velocity 15, 1.7 s long   ramp to 60 over 1.5 s
//   note 2   67.68, velocity 36, 1.9 s long   begins 1.5 s after note 1;
//                                             ramps to 70 over 0.7 s, to 100
//                                             over 0.6 s, to 127 over 0.4 s
//
// A repetition takes 4 seconds. The ways are played in turns (one figure of
// each, then again), so that a slow change in the player touches all alike.
// One recording of the whole run, with the plan this file writes, is enough
// to compare them.
//
// No interface code.

#pragma once

#include <JuceHeader.h>
#include <vector>

#include "ConTimbreMapping.h"

namespace maket::study
{

/** One way of sending the figure. */
struct Way
{
    juce::String name;
    bool byNumber = false;      // `ctnote` and `ctdynamic` instead of `noteon` and `dynamic`
    double firstDelay = 0.0;    // ms from a note-on to its first `dynamic`
    double lead = 0.3;          // the next ramp's command comes this many ms before the ramp before it ends (minus: after)
    bool correction = true;     // the player's dynamic correction

    // The second run (stage 1f.3):
    int singleVelocity = 0;     // above 0: not the figure but one note of 1.5 s at this velocity, without any dynamic
    double restBefore = 0.0;    // ms of silence added before it, on top of the 0.6 s between figures
    double share = 1.0;         // how many of it are played, as a part of Options::repetitions

    // The third run (stage 1f.4):
    int voiceNumber = 0;        // above 0: on this voice instead of the one of the options
    juce::String program;       // not empty: this program instead of the one of the options
    double gain = 0.0;          // dB of that voice (used with `program`)
    int repeatCount = 0;        // above 0: one pitch struck this many times, velocity 90
    double period = 0.0;        //   ms from one note-on to the next
    double overlap = 0.0;       //   ms a note goes on after the next has begun (minus: it ends before)
    double glide = 0.0;         // not 0: one note of 2 s, velocity 80, that glides this many semitones over 1.5 s
    int fixedNumber = 0;        // above 0: the note gets this number instead of the next one
    bool chord = false;         // `chordon` with the two pitches 60 and 79, ended by a note-off of 60
    double singlePitch = 67.4;  // the pitch of a single note

    // The fourth run (stage 1f.5):
    int rampFrom = 0;           // above 0: one note of 1.9 s struck at this velocity, with at most one ramp
    double rampTarget = -1.0;   //   0 or more: 0.2 s after the note-on a ramp to this target over 0.8 s
    bool wholeTarget = false;   //   the target sent as a whole number instead of a real one

    // The fifth run (stage 1f.6): with `rampFrom`, by number, `ctdynamic` with these numbers one after the other,
    // the first 0.2 s after the note-on.
    std::vector<double> steps;
    double stepLength = 800.0;  //   ms each ramp takes
    double stepEvery = 900.0;   //   ms from one command to the next
    bool realSteps = false;     //   the numbers sent as real numbers instead of whole ones

    // The tuning check (stage 1f.8):
    double kammerton = 0.0;         // above 0: `kammerton` with this frequency just before the note
    bool kammertonWhole = false;    //   sent as a whole number instead of a real one
    double kammertonDuring = 0.0;   // above 0: `kammerton` with this frequency 0.7 s into the note

    double soundLength() const;     // ms from its first note-on to its last note-off
    double itemLength() const;      // ms it takes in the run, silence after it included
};

struct Options
{
    int voiceNumber = 1;                    // the figure is played on the voice "voice" + this number
    juce::String program = "sul tasto";
    double gain = 5.777778;                 // dB, as the cello has it in doodsbed
    int repetitions = 20;                   // of every way
    bool withNumber = true;                 // the ways that address the note by number
    bool withCorrectionOff = true;          // the ways with the dynamic correction off
    double standardPitch = 442.0;           // run 7 ends by sending this as the kammerton


    juce::String voiceName() const          { return "voice" + juce::String (voiceNumber); }
};

/** The ways the study compares, in their fixed order. The first is Maket's own way as it sends now.

    Run 1 (stage 1f.2): the figure in ten ways.
    Run 2 (stage 1f.3), after what run 1 showed: single notes without any
    dynamic at five velocities, by pitch and by number (is a note by number
    louder as such?); the figure by number in four timings (which of the two
    distances matters?) beside the figure by pitch; and the figure by pitch
    after three seconds of silence (do the slow starts come from the note
    before?).
    Run 3 (stage 1f.4), on the orchestra "doodsbed by number" (cello on
    voice3, flute on voice4): one pitch struck eight times fast, by pitch
    and by number, the notes overlapping, meeting, or apart; a glide of two
    semitones by pitch and by number; a note with a very large number; and
    the flute's `harmonic` program, which is in chord mode, by `chordon` and
    by a plain note-on.
    Run 4 (stage 1f.5), on the same orchestra, after the recording of
    doodsbed by number (every ramp by number went to about the same loud
    level, whatever its target; only a ramp to 0 came down): one note
    struck at 40 or 100 with one ramp to 0, 10, 40, 80 or 127, by pitch and
    by number, the target by number as a whole number and as a real one
    (the form of a number is the suspect); and the repeated notes of run 3
    once more on a plucked program, where each attack can be counted.
    Run 5 (stage 1f.6), after Joost found by ear that `ctdynamic` moves a
    note from where it is (up for a number above 0, down for one below):
    one step of -60 to +60 from a note struck at 60, steps from 30 and
    from 100 that land on the same sums, the same step as a real number,
    a step over 0.2 and over 1.5 s, six sequences of three steps (do they
    add up, come back, stop at the top and at the bottom, and what does 0
    do), beside the ramps by pitch to the same levels and the struck notes
    alone; and the plucked repeated notes of run 4. It replaces run 4.
    Run 6 (stage 1f.7), after Joost found that `ctdynamic` takes its
    target from 0. to 1.: on the cello (`sul tasto`) and on the bass
    clarinet (`ordinario`, voice5), a note struck at 40 or 100 with one ramp
    by number to 0, 0.1, 0.25, 0.5, 0.75 or 1, beside the ramp by pitch to
    127 times that; the struck notes alone; two sequences by number (0.25,
    0.75, 0.25: does it come back; 0.5 three times: does it stay); and the
    plucked repeated notes. It replaces runs 4 and 5.
    Run 7 (stage 1f.8), the tuning check: the A above middle C (pitch 69)
    on the cello, each time after `kammerton` with another frequency, as a
    real number and as a whole one, and once with the command sent while
    the note sounds. The A must sound at the frequency sent. At the end
    the standard pitch of the settings is sent again. */
std::vector<Way> ways (const Options&, int run = 1);

constexpr double singleNoteLength = 1500.0;         // ms
constexpr double singleRepetitionLength = 2000.0;

constexpr double rampNoteLength = 1900.0;           // run 4: a note with one ramp
constexpr double rampRepetitionLength = 2400.0;
constexpr double rampDelay = 200.0, rampLength = 800.0;

constexpr double figureLength = 3400.0;     // ms, from the first note-on to the last note-off
constexpr double repetitionLength = 4000.0;

/** The commands of one figure that begins at `start` ms. `nextNumber` is
    the number the next note gets when it is addressed by number. */
std::vector<contimbre::TimedMessage> figure (const Options&, const Way&, double start, int& nextNumber);

struct Repetition
{
    int way = 0;            // which of the ways (from 0)
    double start = 0.0;     // ms from the beginning of the run
};

struct Plan
{
    Options options;
    std::vector<Way> ways;
    std::vector<Repetition> repetitions;
    std::vector<contimbre::TimedMessage> messages;      // in order of time; ends with the correction back on
    double length = 0.0;                                // ms
    int run = 1;
};

/** The whole run. With `onlyWay` 0 or more: one figure of that way alone (to try the sound). */
Plan makePlan (const Options&, int onlyWay = -1, int run = 1);

/** The plan as a text file: what was asked, the ways, and when each repetition begins. */
juce::String planAsText (const Plan&);

} // namespace maket::study
