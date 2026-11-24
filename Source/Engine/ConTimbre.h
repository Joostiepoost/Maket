// ConTimbre.h — the commands of the conTimbre ePlayer as OSC messages.
//
// The ePlayer runs in Max. It takes its commands as Max messages, and the
// same over OSC through a udpreceive object. Every command names the voice
// of the orchestra it is for. A pitch is a MIDI note number with decimals
// (60 = middle C, 60.5 = a quarter tone higher), a dynamic runs from 0 to
// 127, times are in milliseconds. (ePlayer manual V2.1; the Max patch
// cpe_poly of ScAst sends the same.)
//
// This file holds the commands one by one. How a note of a roll becomes a
// series of them is in ConTimbreMapping.
//
// No interface code.

#pragma once

#include <JuceHeader.h>
#include <vector>
#include "Osc.h"

namespace maket::contimbre
{

// The command words are sent as they stand, without a slash in front, as
// Max's own udpsend object sends them: the ePlayer's udpreceive hands the
// word on as the first word of the message. (Tested with the ePlayer on
// 22 Nov 2026; `/noteon` is not needed.)

OscMessage noteOn (const juce::String& voice, double pitch, int dynamic);
OscMessage noteOff (const juce::String& voice, double pitch);
OscMessage program (const juce::String& voice, const juce::String& programName);
OscMessage allNotesOff();

/** A change of loudness with its change of timbre, for the note of that
    pitch: to `target` (0 to 127) over `duration` ms. */
OscMessage dynamic (const juce::String& voice, double pitch, double target, double duration, bool wholeTarget = false);

/** The gain of a whole voice, in dB, at once. */
OscMessage gain (const juce::String& voice, double decibels);

// --- A note addressed by a number of its own (the ePlayer's `ct` commands) --------------
//
// The voice is then given by its number: the ePlayer plays on the voice named
// "voice" + that number (`voice1`). The number of the note is the sender's to
// choose; it tells two sounding notes of the same pitch apart. First tried
// against the player at stage 1f.2.

/** `ctnote <voice number> <note number> <pitch> <dynamic> <duration>`; the
    manual marks the duration (ms) as unused. */
OscMessage ctNote (int voiceNumber, int noteNumber, double pitch, int dynamic, double duration);
OscMessage ctNoteOff (int noteNumber);
/** The ePlayer takes the target of `ctdynamic` from 0. to 1. (found by
    Joost, 24 Nov 2026), not from 0 to 127 as that of `dynamic`; the caller
    gives it on that scale. `wholeTarget`: the target rounded and sent as a
    whole number, for the study's fourth and fifth runs, which tried other
    ideas. */
OscMessage ctDynamic (int noteNumber, double target, double duration, bool wholeTarget = false);
OscMessage ctGlissando (int noteNumber, double interval, double duration);

/** `chordon <voice> <pitch> <pitch> ...`: for a program in the player's
    "interpret chords" mode (multiphonics, play and sing, harmonics). It is
    switched off by a note-off with one of its pitches. Only by voice name;
    the manual has no form by number. First tried at stage 1f.4. */
OscMessage chordOn (const juce::String& voice, const std::vector<double>& pitches);

/** `dynamic_correction 1|0`: on is the player's default (its dynamics then
    depend on the MIDI volume); its manual advises off for commands sent directly. */
OscMessage dynamicCorrection (bool on);

/** `kammerton <Hz>`: the ePlayer's standard pitch, the frequency of the A
    above middle C (its own default: 442). Sent as a real number; `whole`
    sends a whole number instead, for the tuning check of stage 1f.8, which
    finds out which form the player takes. */
OscMessage kammerton (double hz, bool whole = false);

/** A glide of the note of that pitch: to `interval` semitones from the
    pitch it began on, over `duration` ms. */
OscMessage glissando (const juce::String& voice, double pitch, double interval, double duration);

} // namespace maket::contimbre
