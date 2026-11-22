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
OscMessage dynamic (const juce::String& voice, double pitch, double target, double duration);

/** The gain of a whole voice, in dB, at once. */
OscMessage gain (const juce::String& voice, double decibels);

/** A glide of the note of that pitch: to `interval` semitones from the
    pitch it began on, over `duration` ms. */
OscMessage glissando (const juce::String& voice, double pitch, double interval, double duration);

} // namespace maket::contimbre
