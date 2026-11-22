// LlllText.h — reads bach's text form of a list (what `writetxt` saves and
// what bach prints) into an Llll, and writes an Llll as that text.
//
// Reading.
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
// Writing. The text comes out as bach.roll's `writetxt` lays it out: a tab
// per level, every list on a line of its own, square brackets. An atom read
// from a file is written exactly as it was spelled there. An atom made by
// Maket is written by bach's rules: a real number with at most six decimals
// and always a point (7100.0), a symbol with a backtick or in quotes when it
// needs one to be read back as a symbol.
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

/** Writes the items of `items` (a list of top-level items, as readLlllText
    gives it) as bach's text. No line end after the last line, as in bach. */
juce::String writeLlllText (const Llll& items);

/** One atom as it is written: its spelling when it has one, else by bach's rules. */
juce::String writeLlllAtom (const Llll& atom);

} // namespace maket
