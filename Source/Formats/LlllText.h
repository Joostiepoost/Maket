// LlllText.h — reads bach's text form of a list (what `writetxt` saves and
// what bach prints) into an Llll.
//
// The rules are bach's own, taken from its lexer (strparser.l):
//  - ( and [ open a list, ) and ] close one; both pairs mean the same.
//  - "..." is one symbol, spaces included; a backslash keeps the next
//    character as it is.
//  - `word is the symbol word, whatever it looks like: `1 is the text "1",
//    not the number, and `gain\ (dB) is the text "gain (dB)".
//  - Any other word is a whole number (117), a real number (7100.0, 1e-3),
//    a fraction (1/4), a pitch name (C5, Eb4, c#5+1/4t) or, failing those,
//    a symbol. `nil` is an empty list and `null` is nothing.
//
// No interface code. Nothing is ever run: the text is only read.

#pragma once

#include <JuceHeader.h>
#include "../Model/Llll.h"

namespace maket
{

/** Reads `text` into `result`, a list of the top-level items. On a mistake
    in the text (a bracket that is never closed or never opened, a quote that
    never ends) the result says which and on which line. */
juce::Result readLlllText (const juce::String& text, Llll& result);

/** Reads one word the way bach does, without brackets, quotes or backtick:
    "117" gives a whole number, "C5" a pitch, "pizz" a symbol. */
Llll readLlllWord (const juce::String& word);

} // namespace maket
