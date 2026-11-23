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
};

struct Options
{
    int voiceNumber = 1;                    // the figure is played on the voice "voice" + this number
    juce::String program = "sul tasto";
    double gain = 5.777778;                 // dB, as the cello has it in doodsbed
    int repetitions = 20;                   // of every way
    bool withNumber = true;                 // the ways that address the note by number
    bool withCorrectionOff = true;          // the ways with the dynamic correction off

    juce::String voiceName() const          { return "voice" + juce::String (voiceNumber); }
};

/** The ways the study compares, in their fixed order. The first is Maket's own way as it sends now. */
std::vector<Way> ways (const Options&);

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
};

/** The whole run. With `onlyWay` 0 or more: one figure of that way alone (to try the sound). */
Plan makePlan (const Options&, int onlyWay = -1);

/** The plan as a text file: what was asked, the ways, and when each repetition begins. */
juce::String planAsText (const Plan&);

} // namespace maket::study
