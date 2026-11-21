// BachRoll.h — reads a bach.roll file (the text that bach's `writetxt`
// saves) into a Roll.
//
// The shape of the file is bach's "gathered syntax":
//
//   roll  [ header entry ] [ header entry ] ...  [ voice ] [ voice ] ...
//   voice:  [ chord chord ... flag ]
//   chord:  [ onset  note note ...  [ name ... ]  flag ]
//   note:   [ cents duration velocity  [ breakpoints ... ] [ slots ... ] ...  flag ]
//
// A header entry is a list that begins with a word (slotinfo, clefs,
// markers ...); a voice is a list that does not. Entries Maket does not use
// are kept as they are.
//
// No interface code.

#pragma once

#include <JuceHeader.h>

#include "../Model/Llll.h"
#include "../Model/Roll.h"

namespace maket
{

/** Builds a Roll from the list read from a file. `remarks` receives what was
    unusual but could be read (one line each). The result fails when the list
    is not a roll at all, or when a voice, chord or note is not in bach's form. */
juce::Result readBachRoll (const Llll& fileContent, Roll& roll, juce::StringArray& remarks);

/** Reads a bach.roll text file: the text into a list, the list into a Roll. */
juce::Result readBachRollFile (const juce::File& file, Roll& roll, juce::StringArray& remarks);

} // namespace maket
