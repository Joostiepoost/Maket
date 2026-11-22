// ConTimbre.cpp

#include "ConTimbre.h"

namespace maket::contimbre
{

OscMessage noteOn (const juce::String& voice, double pitch, int dynamic)
{
    return OscMessage ("noteon").addText (voice).addReal (pitch).addInteger (dynamic);
}

OscMessage noteOff (const juce::String& voice, double pitch)
{
    return OscMessage ("noteoff").addText (voice).addReal (pitch);
}

OscMessage program (const juce::String& voice, const juce::String& programName)
{
    return OscMessage ("program").addText (voice).addText (programName);
}

OscMessage allNotesOff()
{
    return OscMessage ("all_notes_off");
}

OscMessage dynamic (const juce::String& voice, double pitch, double target, double duration)
{
    return OscMessage ("dynamic").addText (voice).addReal (pitch).addReal (target).addReal (duration);
}

OscMessage gain (const juce::String& voice, double decibels)
{
    return OscMessage ("gain").addText (voice).addReal (decibels);
}

OscMessage glissando (const juce::String& voice, double pitch, double interval, double duration)
{
    return OscMessage ("glissando").addText (voice).addReal (pitch).addReal (interval).addReal (duration);
}

} // namespace maket::contimbre
