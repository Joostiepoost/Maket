// BachRoll.h — reads a bach.roll file (the text that bach's `writetxt`
// saves) into a Roll, and writes a Roll as such a file.
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
// Writing gives what bach itself would save for the same roll: the same
// items in the same order and layout, numbers in bach's form. A roll that
// bach saved, read and written back without a change, is the same file byte
// for byte. A file from elsewhere (a whole number where bach writes a real
// one, another order inside a note) comes out in bach's form.
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

/** The roll as the list bach saves: `roll`, the header entries, the voices. */
Llll bachRollAsList (const Roll& roll);

/** The roll as the text of a bach.roll file. */
juce::String writeBachRollText (const Roll& roll);

/** Writes the roll as a bach.roll text file, replacing a file of that name. */
juce::Result writeBachRollFile (const juce::File& file, const Roll& roll);

} // namespace maket
