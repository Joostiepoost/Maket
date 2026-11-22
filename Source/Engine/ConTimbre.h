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

/** How the command word is written: `noteon`, as Max's own udpsend sends
    it, or `/noteon`, as OSC addresses usually look. The ePlayer's udpreceive
    passes the word on as it comes, so only one of the two can be right;
    which one is part of the test of stage 1c. */
enum class CommandForm { plain, withSlash };

OscMessage noteOn (const juce::String& voice, double pitch, int dynamic, CommandForm form = CommandForm::plain);
OscMessage noteOff (const juce::String& voice, double pitch, CommandForm form = CommandForm::plain);
OscMessage program (const juce::String& voice, const juce::String& programName, CommandForm form = CommandForm::plain);
OscMessage allNotesOff (CommandForm form = CommandForm::plain);

} // namespace maket::contimbre
