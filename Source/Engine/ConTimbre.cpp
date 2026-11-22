// ConTimbre.cpp

#include "ConTimbre.h"

namespace maket::contimbre
{

namespace
{
    OscMessage command (const char* word, CommandForm form)
    {
        return OscMessage ((form == CommandForm::withSlash ? "/" : "") + juce::String (word));
    }
}

OscMessage noteOn (const juce::String& voice, double pitch, int dynamic, CommandForm form)
{
    return command ("noteon", form).addText (voice).addReal (pitch).addInteger (dynamic);
}

OscMessage noteOff (const juce::String& voice, double pitch, CommandForm form)
{
    return command ("noteoff", form).addText (voice).addReal (pitch);
}

OscMessage program (const juce::String& voice, const juce::String& programName, CommandForm form)
{
    return command ("program", form).addText (voice).addText (programName);
}

OscMessage allNotesOff (CommandForm form)
{
    return command ("all_notes_off", form);
}

} // namespace maket::contimbre
