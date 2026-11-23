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

OscMessage ctNote (int voiceNumber, int noteNumber, double pitch, int dynamic, double duration)
{
    return OscMessage ("ctnote").addInteger (voiceNumber).addInteger (noteNumber).addReal (pitch).addInteger (dynamic).addReal (duration);
}

OscMessage ctNoteOff (int noteNumber)
{
    return OscMessage ("ctnoteoff").addInteger (noteNumber);
}

OscMessage ctDynamic (int noteNumber, double target, double duration)
{
    return OscMessage ("ctdynamic").addInteger (noteNumber).addReal (target).addReal (duration);
}

OscMessage ctGlissando (int noteNumber, double interval, double duration)
{
    return OscMessage ("ctglissando").addInteger (noteNumber).addReal (interval).addReal (duration);
}

OscMessage dynamicCorrection (bool on)
{
    return OscMessage ("dynamic_correction").addInteger (on ? 1 : 0);
}

OscMessage glissando (const juce::String& voice, double pitch, double interval, double duration)
{
    return OscMessage ("glissando").addText (voice).addReal (pitch).addReal (interval).addReal (duration);
}

} // namespace maket::contimbre
