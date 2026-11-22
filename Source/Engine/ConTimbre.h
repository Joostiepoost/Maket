// ConTimbre.h — the commands of the conTimbre ePlayer as OSC messages.
//
// The ePlayer runs in Max. It takes its commands as Max messages, and the
// same over OSC through a udpreceive object. Every command names the voice
// of the orchestra it is for. A pitch is a MIDI note number with decimals
// (60 = middle C, 60.5 = a quarter tone higher), a dynamic runs from 0 to
// 127, times are in milliseconds. (ePlayer manual V2.1; the Max patch
// cpe_poly of ScAst sends the same.)
//
// This file holds only the commands the tests of stage 1c need. The full
// mapping of a note (dynamic curves, gain, glissando, options) is stage 1e.
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

} // namespace maket::contimbre
