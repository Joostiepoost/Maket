// BachRoll.cpp

#include "BachRoll.h"
#include "LlllText.h"

#include <cmath>

namespace maket
{

namespace
{
    juce::String where (int voice, int chord = 0, int note = 0)
    {
        juce::String s ("Voice " + juce::String (voice));
        if (chord > 0) s << ", chord " << chord;
        if (note > 0)  s << ", note " << note;
        return s;
    }

    /** The last item of a voice, chord or note is its flag when it is a number. */
    int trailingFlag (const Llll& list)
    {
        return (! list.items.empty() && list.items.back().kind == Llll::Kind::integer)
                   ? (int) list.items.back().num : 0;
    }

    juce::Result readBreakpoints (const Llll& spec, Note& note, const juce::String& place)
    {
        for (int i = 1; i < spec.size(); ++i)
        {
            const auto& p = spec[i];
            if (! p.isList() || p.size() < 2 || ! p[0].isNumber() || ! p[1].isNumber())
                return juce::Result::fail (place + ": a pitch breakpoint is not in the form [ position cents slope ].");

            Breakpoint b;
            b.position = p[0].value;
            b.cents = p[1].value;
            if (p.size() > 2 && p[2].isNumber()) b.slope = p[2].value;
            if (p.size() > 3 && p[3].isNumber()) { b.hasVelocity = true; b.velocity = (int) p[3].value; }
            note.breakpoints.push_back (b);
        }
        return juce::Result::ok();
    }

    juce::Result readSlots (const Llll& spec, Note& note, const juce::String& place)
    {
        for (int i = 1; i < spec.size(); ++i)
        {
            const auto& s = spec[i];
            if (! s.isList() || s.size() < 1 || s[0].kind != Llll::Kind::integer)
                return juce::Result::fail (place + ": a slot does not begin with its number.");

            SlotValue v;
            v.number = (int) s[0].num;
            v.content.items.assign (s.items.begin() + 1, s.items.end());
            note.slots.push_back (std::move (v));
        }
        return juce::Result::ok();
    }

    juce::Result readNote (const Llll& n, Note& note, const juce::String& place)
    {
        if (n.size() < 3 || ! n[0].isNumber() || ! n[1].isNumber() || ! n[2].isNumber())
            return juce::Result::fail (place + ": a note needs pitch, duration and velocity.");

        note.cents = n[0].value;
        if (n[0].kind == Llll::Kind::pitch) note.pitchName = n[0].spelling;
        note.duration = n[1].value;
        note.velocity = (int) n[2].value;

        for (int i = 3; i < n.size(); ++i)
        {
            const auto& item = n[i];
            if (! item.isList())
            {
                if (i == n.size() - 1 && item.kind == Llll::Kind::integer) note.flags = (int) item.num;
                else return juce::Result::fail (place + ": unexpected `" + item.toLine() + "` in a note.");
                continue;
            }

            const auto name = item.head();
            juce::Result r = juce::Result::ok();
            if (name == "breakpoints")  r = readBreakpoints (item, note, place);
            else if (name == "slots")   r = readSlots (item, note, place);
            else                        note.others.push_back (item);       // name, graphic, articulation ...
            if (r.failed()) return r;
        }
        return juce::Result::ok();
    }

    juce::Result readChord (const Llll& c, Chord& chord, int voiceNumber, int chordNumber)
    {
        if (c.size() < 1 || ! c[0].isNumber())
            return juce::Result::fail (where (voiceNumber, chordNumber) + ": a chord must begin with its onset.");

        chord.onset = c[0].value;

        for (int i = 1; i < c.size(); ++i)
        {
            const auto& item = c[i];
            if (! item.isList())
            {
                if (i == c.size() - 1 && item.kind == Llll::Kind::integer) chord.flags = (int) item.num;
                else return juce::Result::fail (where (voiceNumber, chordNumber) + ": unexpected `" + item.toLine() + "` in a chord.");
                continue;
            }

            if (item.head().isNotEmpty())           // [ name ... ] and the like
            {
                chord.others.push_back (item);
                continue;
            }

            Note note;
            const auto r = readNote (item, note, where (voiceNumber, chordNumber, (int) chord.notes.size() + 1));
            if (r.failed()) return r;
            chord.notes.push_back (std::move (note));
        }
        return juce::Result::ok();
    }

    juce::Result readVoice (const Llll& v, Voice& voice, int voiceNumber)
    {
        voice.flags = trailingFlag (v);

        for (int i = 0; i < v.size(); ++i)
        {
            const auto& item = v[i];
            if (! item.isList())
            {
                if (i == v.size() - 1 && item.kind == Llll::Kind::integer) continue;        // the flag
                return juce::Result::fail (where (voiceNumber) + ": unexpected `" + item.toLine() + "` between chords.");
            }

            Chord chord;
            const auto r = readChord (item, chord, voiceNumber, (int) voice.chords.size() + 1);
            if (r.failed()) return r;
            voice.chords.push_back (std::move (chord));
        }
        return juce::Result::ok();
    }

    void readMarkers (const Llll& entry, Roll& roll, juce::StringArray& remarks)
    {
        for (int i = 1; i < entry.size(); ++i)
        {
            const auto& m = entry[i];
            if (! m.isList() || m.size() < 1 || ! m[0].isNumber())
            {
                remarks.add ("Marker " + juce::String (i) + " does not begin with a time and was left out of the marker list.");
                continue;
            }

            Marker marker;
            marker.position = m[0].value;
            if (m.size() > 1) marker.names = m[1];
            marker.role = (m.size() > 2 && m[2].isSymbol()) ? m[2].text : juce::String ("none");
            if (m.size() > 3) { marker.content = m[3]; marker.hasContent = true; }
            roll.markers.push_back (std::move (marker));
        }
    }

    void readSlotInfo (const Llll& entry, Roll& roll)
    {
        auto valueOf = [] (const Llll& def, const char* key)
        {
            const auto* pair = def.find (key);
            return (pair != nullptr && pair->size() > 1) ? ((*pair)[1].isSymbol() ? (*pair)[1].text : (*pair)[1].toLine())
                                                         : juce::String();
        };

        for (int i = 1; i < entry.size(); ++i)
        {
            const auto& def = entry[i];
            if (! def.isList() || def.size() < 1 || def[0].kind != Llll::Kind::integer) continue;

            SlotInfo info;
            info.number = (int) def[0].num;
            info.name = valueOf (def, "name");
            info.type = valueOf (def, "type");
            info.key = valueOf (def, "key");
            info.temporalMode = valueOf (def, "temporalmode");
            info.all = def;
            roll.slotInfo.push_back (std::move (info));
        }
    }

    /** What the header says per voice: names, clefs, MIDI channels. */
    void applyHeaderToVoices (Roll& roll)
    {
        auto item = [&roll] (const char* name, size_t voice) -> const Llll*
        {
            const auto* entry = roll.headerEntry (name);
            return (entry != nullptr && voice + 1 < entry->content.items.size()) ? &entry->content.items[voice + 1] : nullptr;
        };

        for (size_t v = 0; v < roll.voices.size(); ++v)
        {
            if (const auto* n = item ("voicenames", v))
            {
                if (n->isSymbol())                          roll.voices[v].name = n->text;
                else if (n->isList() && n->size() > 0)      roll.voices[v].name = (*n)[0].isSymbol() ? (*n)[0].text : (*n)[0].toLine();
                else if (! n->isList())                     roll.voices[v].name = n->toLine();
            }
            if (const auto* c = item ("clefs", v))          roll.voices[v].clef = c->isSymbol() ? c->text : c->toLine();
            if (const auto* m = item ("midichannels", v))   if (m->isNumber()) roll.voices[v].midiChannel = (int) m->value;
        }
    }
}

juce::Result readBachRoll (const Llll& fileContent, Roll& roll, juce::StringArray& remarks)
{
    roll = Roll();

    int first = 0;
    if (fileContent.size() > 0 && fileContent[0].isSymbol())
    {
        const auto kind = fileContent[0].text;
        if (kind == "score")
            return juce::Result::fail ("This is a bach.score file. Maket reads bach.roll files.");
        if (kind != "roll")
            return juce::Result::fail ("This does not begin with `roll`, so it is not a bach.roll file (it begins with `" + kind + "`).");
        first = 1;
    }
    else
    {
        remarks.add ("The file does not begin with the word `roll`; it was read as a roll all the same.");
    }

    for (int i = first; i < fileContent.size(); ++i)
    {
        const auto& item = fileContent[i];

        if (! item.isList())
        {
            remarks.add ("`" + item.toLine() + "` stands between the header and the voices and was skipped.");
            continue;
        }

        if (item.head().isNotEmpty())               // a header entry
        {
            roll.header.push_back ({ item.head(), item });
            if (item.head() == "markers")       readMarkers (item, roll, remarks);
            else if (item.head() == "slotinfo") readSlotInfo (item, roll);
            continue;
        }

        Voice voice;
        const auto r = readVoice (item, voice, (int) roll.voices.size() + 1);
        if (r.failed()) return r;
        roll.voices.push_back (std::move (voice));
    }

    applyHeaderToVoices (roll);
    return juce::Result::ok();
}

juce::Result readBachRollFile (const juce::File& file, Roll& roll, juce::StringArray& remarks)
{
    if (! file.existsAsFile())
        return juce::Result::fail ("There is no file " + file.getFullPathName() + ".");

    Llll content;
    auto r = readLlllText (file.loadFileAsString(), content);
    if (r.failed()) return r;

    return readBachRoll (content, roll, remarks);
}

// --- Writing ----------------------------------------------------------------------
//
// The list is built the way bach builds it when it saves (the functions
// get_rollvoice_values_as_llll, get_rollchord_values_as_llll,
// get_rollnote_values_as_llll and get_markers_as_llll in bach's source):
// the same items in the same order, onsets, pitches, durations and
// breakpoints as real numbers, velocities and flags as whole numbers.

namespace
{
    Llll symbolItem (const char* name)      { return Llll::makeSymbol (name); }

    Llll pitchItem (const Note& note)
    {
        if (note.pitchName.isNotEmpty())
        {
            const auto asNamed = readLlllWord (note.pitchName);
            if (asNamed.kind == Llll::Kind::pitch && std::abs (asNamed.value - note.cents) < 1.0e-9)
                return asNamed;
        }
        return Llll::makeReal (note.cents);
    }

    Llll noteAsList (const Note& note)
    {
        Llll n;
        n.items.push_back (pitchItem (note));
        n.items.push_back (Llll::makeReal (note.duration));
        n.items.push_back (Llll::makeInteger (note.velocity));

        for (const auto& other : note.others)
            if (other.head() == "graphic") n.items.push_back (other);

        if (! note.breakpoints.empty())
        {
            Llll points;
            points.items.push_back (symbolItem ("breakpoints"));
            for (const auto& b : note.breakpoints)
            {
                Llll point;
                point.items.push_back (Llll::makeReal (b.position));
                point.items.push_back (Llll::makeReal (b.cents));
                point.items.push_back (Llll::makeReal (b.slope));
                if (b.hasVelocity) point.items.push_back (Llll::makeInteger (b.velocity));
                points.items.push_back (std::move (point));
            }
            n.items.push_back (std::move (points));
        }

        if (! note.slots.empty())
        {
            Llll slots;
            slots.items.push_back (symbolItem ("slots"));
            for (const auto& slot : note.slots)
            {
                Llll one;
                one.items.push_back (Llll::makeInteger (slot.number));
                one.items.insert (one.items.end(), slot.content.items.begin(), slot.content.items.end());
                slots.items.push_back (std::move (one));
            }
            n.items.push_back (std::move (slots));
        }

        for (const auto& other : note.others)           // articulations, name
            if (other.head() != "graphic") n.items.push_back (other);

        n.items.push_back (Llll::makeInteger (note.flags));
        return n;
    }

    Llll chordAsList (const Chord& chord)
    {
        Llll c;
        c.items.push_back (Llll::makeReal (chord.onset));
        for (const auto& note : chord.notes)    c.items.push_back (noteAsList (note));
        for (const auto& other : chord.others)  c.items.push_back (other);
        c.items.push_back (Llll::makeInteger (chord.flags));
        return c;
    }

    Llll voiceAsList (const Voice& voice)
    {
        Llll v;
        for (const auto& chord : voice.chords) v.items.push_back (chordAsList (chord));
        v.items.push_back (Llll::makeInteger (voice.flags));
        return v;
    }

    Llll markersAsList (const Roll& roll)
    {
        Llll entry;
        entry.items.push_back (symbolItem ("markers"));
        for (const auto& marker : roll.markers)
        {
            Llll m;
            m.items.push_back (Llll::makeReal (marker.position));
            m.items.push_back (marker.names);
            m.items.push_back (Llll::makeSymbol (marker.role.isEmpty() ? juce::String ("none") : marker.role));
            if (marker.hasContent) m.items.push_back (marker.content);
            entry.items.push_back (std::move (m));
        }
        return entry;
    }

    Llll slotInfoAsList (const Roll& roll)
    {
        Llll entry;
        entry.items.push_back (symbolItem ("slotinfo"));
        for (const auto& info : roll.slotInfo) entry.items.push_back (info.all);
        return entry;
    }
}

Llll bachRollAsList (const Roll& roll)
{
    Llll file;
    file.items.push_back (symbolItem ("roll"));

    // The header, in the order it came in. Markers and slot definitions are
    // written from the roll's own lists; the other entries as they were kept.
    bool markersWritten = false, slotInfoWritten = false;
    for (const auto& entry : roll.header)
    {
        if (entry.name == "markers")
        {
            if (! markersWritten) file.items.push_back (markersAsList (roll));
            markersWritten = true;
        }
        else if (entry.name == "slotinfo")
        {
            if (! slotInfoWritten) file.items.push_back (slotInfoAsList (roll));
            slotInfoWritten = true;
        }
        else
        {
            file.items.push_back (entry.content);
        }
    }
    if (! slotInfoWritten && ! roll.slotInfo.empty()) file.items.push_back (slotInfoAsList (roll));
    if (! markersWritten && ! roll.markers.empty())   file.items.push_back (markersAsList (roll));

    for (const auto& voice : roll.voices) file.items.push_back (voiceAsList (voice));
    return file;
}

juce::String writeBachRollText (const Roll& roll)
{
    return writeLlllText (bachRollAsList (roll));
}

juce::Result writeBachRollFile (const juce::File& file, const Roll& roll)
{
    const auto text = writeBachRollText (roll);

    if (! file.getParentDirectory().createDirectory())
        return juce::Result::fail ("The folder " + file.getParentDirectory().getFullPathName() + " could not be made.");

    // Through a temporary file beside it, so that a failed write leaves an
    // existing file as it was. Plain UTF-8, Unix line ends, as bach writes.
    if (! file.replaceWithData (text.toRawUTF8(), text.getNumBytesAsUTF8()))
        return juce::Result::fail ("The file " + file.getFullPathName() + " could not be written.");

    return juce::Result::ok();
}

} // namespace maket
