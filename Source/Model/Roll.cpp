// Roll.cpp

#include "Roll.h"

#include <algorithm>

namespace maket
{

const SlotValue* Note::slot (int number) const
{
    for (const auto& s : slots)
        if (s.number == number) return &s;
    return nullptr;
}

int Voice::numNotes() const
{
    int n = 0;
    for (const auto& c : chords) n += (int) c.notes.size();
    return n;
}

juce::String Marker::nameAsText() const
{
    if (! names.isList()) return names.isSymbol() ? names.text : names.toLine();

    juce::StringArray words;
    for (const auto& item : names.items)
        words.add (item.isSymbol() ? item.text : item.toLine());
    return words.joinIntoString (" ");
}

const HeaderEntry* Roll::headerEntry (const char* name) const
{
    for (const auto& e : header)
        if (e.name == name) return &e;
    return nullptr;
}

const SlotInfo* Roll::slot (int number) const
{
    for (const auto& s : slotInfo)
        if (s.number == number) return &s;
    return nullptr;
}

int Roll::numChords() const
{
    int n = 0;
    for (const auto& v : voices) n += (int) v.chords.size();
    return n;
}

int Roll::numNotes() const
{
    int n = 0;
    for (const auto& v : voices) n += v.numNotes();
    return n;
}

int Roll::numMutedNotes() const
{
    int n = 0;
    for (const auto& v : voices)
        for (const auto& c : v.chords)
            for (const auto& note : c.notes)
                if (v.isMuted() || c.isMuted() || note.isMuted()) ++n;
    return n;
}

int Roll::numNotesWithBreakpoints() const
{
    int n = 0;
    for (const auto& v : voices)
        for (const auto& c : v.chords)
            for (const auto& note : c.notes)
                if (! note.breakpoints.empty()) ++n;
    return n;
}

int Roll::numBreakpoints() const
{
    int n = 0;
    for (const auto& v : voices)
        for (const auto& c : v.chords)
            for (const auto& note : c.notes)
                n += (int) note.breakpoints.size();
    return n;
}

int Roll::numFilledSlots() const
{
    int n = 0;
    for (const auto& v : voices)
        for (const auto& c : v.chords)
            for (const auto& note : c.notes)
                n += (int) note.slots.size();
    return n;
}

int Roll::numFilledSlots (int slotNumber) const
{
    int n = 0;
    for (const auto& v : voices)
        for (const auto& c : v.chords)
            for (const auto& note : c.notes)
                if (note.slot (slotNumber) != nullptr) ++n;
    return n;
}

double Roll::length() const
{
    double end = 0.0;
    for (const auto& v : voices)
        for (const auto& c : v.chords)
            for (const auto& note : c.notes)
                end = std::max (end, c.onset + note.duration);
    return end;
}

} // namespace maket
