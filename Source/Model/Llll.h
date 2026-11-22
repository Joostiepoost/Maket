// Llll.h — the list type that bach's data comes in.
//
// bach keeps everything (a roll, a slot, a marker) as a "Lisp-like linked
// list of lists": a list whose items are numbers, symbols, or lists again.
// Maket uses the same shape to read and write bach's files, to keep the
// parts of a file it does not interpret, and later for list slots.
//
// An Llll is one item: an atom (whole number, real number, fraction, pitch
// name, symbol) or a list of items. Whole and real numbers are kept apart,
// as bach does: 117 and 117.0 are different atoms. Every atom also keeps the
// way it was written in the file it came from (its "spelling"), so that a
// file can be written back exactly as it was.

#pragma once

#include <JuceHeader.h>
#include <vector>

namespace maket
{

struct Llll
{
    enum class Kind
    {
        integer,    // 117
        real,       // 7100.0
        rational,   // 1/4
        pitch,      // a note name such as C5 or Eb4; its value is in cents
        symbol,     // a word or a text
        list        // [ ... ]
    };

    Kind kind = Kind::list;

    juce::int64 num = 0;        // integer: the number; rational: the numerator
    juce::int64 den = 1;        // rational: the denominator
    double value = 0.0;         // any number: its value; pitch: cents (C5 = 6000)
    juce::String text;          // symbol: its text, without quotes, backtick or escapes
    juce::String spelling;      // an atom as it was written in the file; empty when made by Maket.
                                // The writer uses it as it stands: to change an atom, put a new one in its place.
    std::vector<Llll> items;    // list: its items

    // --- making items ---
    static Llll makeList()                                  { return {}; }
    static Llll makeInteger (juce::int64 n);
    static Llll makeReal (double x);
    static Llll makeSymbol (const juce::String& s);

    // --- asking ---
    bool isList() const                 { return kind == Kind::list; }
    bool isSymbol() const               { return kind == Kind::symbol; }
    bool isSymbol (const char* s) const { return kind == Kind::symbol && text == s; }
    bool isNumber() const               { return kind == Kind::integer || kind == Kind::real
                                              || kind == Kind::rational || kind == Kind::pitch; }
    int size() const                    { return (int) items.size(); }
    const Llll& operator[] (int i) const { return items[(size_t) i]; }

    /** The first item of a list, when that item is a symbol; else an empty string. */
    juce::String head() const;

    /** In a list of lists: the first one whose first item is the symbol `name`. */
    const Llll* find (const char* name) const;

    /** How many atoms the item holds, at any depth. */
    int countAtoms() const;

    /** How deep the lists go: an atom is 0, a list of atoms 1. */
    int depth() const;

    /** The item as text on one line, atoms as they were spelled, lists in
        square brackets: for messages and reports. A file is written with
        writeLlllText (Formats/LlllText.h), in bach's own layout. */
    juce::String toLine() const;

    /** True when two items hold the same: same kinds of atoms and lists, the
        same symbols, and numbers that differ by no more than `tolerance`.
        Whole and real numbers of equal value count as the same here. */
    static bool sameContent (const Llll& a, const Llll& b, double tolerance = 0.0);
};

} // namespace maket
