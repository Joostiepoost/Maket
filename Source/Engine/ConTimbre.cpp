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

} // namespace maket::contimbre
