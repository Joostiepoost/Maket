// Playing.cpp — see Playing.h.

#include "Playing.h"
#include "../Notation/Glides.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

namespace maket::playing
{

namespace
{
    struct Point { double x = 0.0, y = 0.0, slope = 0.0; };

    std::vector<Point> pointsOf (const Llll& content)
    {
        std::vector<Point> points;
        for (const auto& item : content.items)
            if (item.isList() && item.size() >= 2 && item[0].isNumber() && item[1].isNumber())
                points.push_back ({ item[0].value, item[1].value, item.size() > 2 && item[2].isNumber() ? item[2].value : 0.0 });
        return points;
    }

    Llll asContent (const std::vector<Point>& points)
    {
        Llll content;
        for (const auto& p : points)
        {
            Llll item;
            item.items.push_back (Llll::makeReal (p.x));
            item.items.push_back (Llll::makeReal (p.y));
            item.items.push_back (Llll::makeReal (p.slope));
            content.items.push_back (std::move (item));
        }
        return content;
    }

    /** The value of a function slot at `x`, along bach's curve between its points. */
    double valueAt (const std::vector<Point>& points, double x)
    {
        if (points.empty()) return 0.0;
        if (x <= points.front().x) return points.front().y;
        for (size_t i = 1; i < points.size(); ++i)
            if (x <= points[i].x)
            {
                const auto& a = points[i - 1];
                const auto& b = points[i];
                const double t = b.x > a.x ? (x - a.x) / (b.x - a.x) : 1.0;
                return a.y + (b.y - a.y) * maket::notation::slopeCurve (t, b.slope);
            }
        return points.back().y;
    }

    SlotValue* slotOf (Note& note, int number)
    {
        for (auto& s : note.slots)
            if (s.number == number) return &s;
        return nullptr;
    }

    /** The note from `start` to `end`, both in ms after its onset. */
    void cutNote (Note& note, double start, double end)
    {
        const double length = note.duration;
        if (length <= 0.0) return;
        const bool cutStart = start > 0.0, cutEnd = end < length;
        if (! cutStart && ! cutEnd) return;
        const double newLength = end - start;

        // The glide: the pitch at the new head becomes the note's pitch; the points go on from there.
        if (note.breakpoints.size() >= 2)
        {
            const double p0 = start / length, p1 = end / length;
            const double head = glideCentsAt (note, p0);
            std::vector<Breakpoint> points;
            points.push_back (Breakpoint());
            for (const auto& b : note.breakpoints)
            {
                if (b.position <= p0 || b.position >= p1) continue;
                auto moved = b;
                moved.position = (b.position - p0) / (p1 - p0);
                moved.cents = b.cents - head;
                points.push_back (moved);
            }
            Breakpoint tail;
            tail.position = 1.0;
            tail.cents = glideCentsAt (note, p1) - head;
            for (const auto& b : note.breakpoints)                  // the curve of the piece that reaches the new tail
                if (b.position >= p1) { tail.slope = b.slope; break; }
            points.push_back (tail);

            if (cutStart)
            {
                note.cents += head;
                note.pitchName.clear();
            }
            note.breakpoints = std::move (points);
        }

        // Slot 2, the dynamics: from the new head on. The note is struck at its velocity as always, and reaches
        // the value the curve has at that moment in catchUpTime.
        if (auto* slot = slotOf (note, 2))
        {
            const auto points = pointsOf (slot->content);
            if (points.size() >= 2)
            {
                std::vector<Point> cut;
                if (cutStart && points.front().x < start)
                {
                    const double now = valueAt (points, start);
                    cut.push_back ({ 0.0, now, 0.0 });
                    double next = std::numeric_limits<double>::max();
                    for (const auto& p : points) if (p.x > start) { next = p.x - start; break; }
                    if (next > catchUpTime + 1.0) cut.push_back ({ catchUpTime, now, 0.0 });
                }
                for (const auto& p : points)
                {
                    if (cutStart && p.x <= start) continue;
                    cut.push_back ({ p.x - start, p.y, p.slope });
                    if (p.x >= end) break;                     // the ramp that is under way at the end keeps its target and length
                }
                slot->content = asContent (cut);
            }
        }

        // Slot 3, the gain: steps. The last step before the new head holds at it.
        if (auto* slot = slotOf (note, 3))
        {
            const auto points = pointsOf (slot->content);
            if (! points.empty())
            {
                std::vector<Point> cut;
                const Point* last = nullptr;
                for (const auto& p : points)
                {
                    if (p.x <= start) { last = &p; continue; }
                    if (p.x >= end) break;
                    cut.push_back ({ p.x - start, p.y, p.slope });
                }
                if (last != nullptr) cut.insert (cut.begin(), { 0.0, last->y, last->slope });
                slot->content = asContent (cut);
            }
        }

        note.duration = newLength;
    }
}

namespace
{
    /** Slot 1 names a conTimbre technique when it holds a word: "pizz", "bass flute.ordinario". Numbers alone
        (as on the MIDI voices of Joost's mixed example) are not a technique. */
    bool namesTechnique (const Llll& content)
    {
        if (content.isSymbol()) return content.text.containsAnyOf ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
        for (const auto& item : content.items)
            if (namesTechnique (item)) return true;
        return false;
    }
}

std::vector<VoiceEngine> defaultEngines (const Roll& roll)
{
    std::vector<VoiceEngine> engines;
    for (const auto& voice : roll.voices)
    {
        bool technique = false;
        for (const auto& chord : voice.chords)
        {
            for (const auto& note : chord.notes)
                if (const auto* slot = note.slot (1))
                    if (namesTechnique (slot->content)) { technique = true; break; }
            if (technique) break;
        }
        const bool pitched = voice.clef != "Percussion" && voice.clef != "None";
        // A voice without a name is never sent to the ePlayer (as in the Max patch), so it cannot be a conTimbre voice.
        engines.push_back (technique && voice.name.isNotEmpty() ? VoiceEngine::conTimbre : pitched ? VoiceEngine::synth : VoiceEngine::silent);
    }
    return engines;
}

Roll forEngine (const Roll& roll, const std::vector<VoiceEngine>& engines, VoiceEngine which)
{
    Roll result;
    result.header = roll.header;
    result.slotInfo = roll.slotInfo;
    result.markers = roll.markers;
    result.voices.reserve (roll.voices.size());
    for (size_t v = 0; v < roll.voices.size(); ++v)
        result.voices.push_back ((v < engines.size() ? engines[v] : VoiceEngine::conTimbre) == which ? roll.voices[v] : roll.voices[v].withoutChords());
    return result;
}

bool anySolo (const Roll& roll)
{
    for (const auto& voice : roll.voices)
    {
        if (voice.flags & flagSolo) return true;
        for (const auto& chord : voice.chords)
        {
            if (chord.flags & flagSolo) return true;
            for (const auto& note : chord.notes)
                if (note.flags & flagSolo) return true;
        }
    }
    return false;
}

Roll audible (const Roll& roll)
{
    const bool solos = anySolo (roll);
    Roll result = roll;
    for (auto& voice : result.voices)
    {
        std::vector<Chord> chords;
        for (auto& chord : voice.chords)
        {
            std::vector<Note> notes;
            for (auto& note : chord.notes)
            {
                const int all = note.flags | chord.flags | voice.flags;
                if ((all & flagMuted) != 0) continue;
                if (solos && (all & flagSolo) == 0) continue;
                notes.push_back (std::move (note));
            }
            if (notes.empty()) continue;
            chord.notes = std::move (notes);
            chords.push_back (std::move (chord));
        }
        voice.chords = std::move (chords);
    }
    return result;
}

double glideCentsAt (const Note& note, double position)
{
    if (note.breakpoints.size() < 2) return 0.0;
    const Breakpoint* before = &note.breakpoints.front();
    if (position <= before->position) return before->cents;
    for (size_t i = 1; i < note.breakpoints.size(); ++i)
    {
        const auto& b = note.breakpoints[i];
        if (position <= b.position)
        {
            const double length = b.position - before->position;
            const double t = length > 0.0 ? (position - before->position) / length : 1.0;
            return before->cents + (b.cents - before->cents) * maket::notation::slopeCurve (t, b.slope);
        }
        before = &b;
    }
    return note.breakpoints.back().cents;
}

Roll excerpt (const Roll& roll, double from, double to)
{
    // Only what lies in the part is copied: a loop of ten seconds in a roll of an hour costs ten seconds' worth.
    Roll result;
    result.header = roll.header;
    result.slotInfo = roll.slotInfo;
    result.voices.reserve (roll.voices.size());

    for (const auto& voice : roll.voices)
    {
        auto out = voice.withoutChords();
        for (const auto& chord : voice.chords)
        {
            if (chord.onset >= to) continue;
            Chord cut;
            bool any = false;
            for (const auto& note : chord.notes)
            {
                const double end = chord.onset + note.duration;
                if (end <= from) continue;
                if (! any) { cut = chord.withoutNotes(); any = true; }
                cut.notes.push_back (note);
                cutNote (cut.notes.back(), std::max (0.0, from - chord.onset), std::min (note.duration, to - chord.onset));
            }
            if (! any) continue;

            // Notes of one chord cut at `from` all begin there; a chord that begins later keeps its onset.
            cut.onset = std::max (chord.onset, from);
            out.chords.push_back (std::move (cut));
        }
        std::stable_sort (out.chords.begin(), out.chords.end(), [] (const Chord& a, const Chord& b) { return a.onset < b.onset; });
        result.voices.push_back (std::move (out));
    }

    for (const auto& m : roll.markers)
        if (m.position >= from && m.position < to) result.markers.push_back (m);
    return result;
}

namespace
{
    contimbre::MappingOptions passOptions (int firstNumber)
    {
        contimbre::MappingOptions options;
        options.byNumber = true;
        options.firstNoteNumber = firstNumber;
        return options;
    }

    bool wholeRoll (double from, double to)     { return from <= 0.0 && to >= std::numeric_limits<double>::max(); }

    std::vector<contimbre::TimedMessage> programsAndGainsOf (const Roll& audibleRoll)
    {
        std::vector<contimbre::TimedMessage> kept;
        for (auto& m : contimbre::messagesForRoll (audibleRoll, {}, passOptions (1)))
            if (m.message.address == "program" || m.message.address == "gain") kept.push_back (std::move (m));
        return kept;
    }

    std::vector<contimbre::TimedMessage> passOf (const Roll& audibleRoll, const std::vector<contimbre::TimedMessage>* programsAndGains,
                                                 double from, double to, int firstNumber, int& numbersUsed)
    {
        const auto options = passOptions (firstNumber);
        auto messages = wholeRoll (from, to) ? contimbre::messagesForRoll (audibleRoll, {}, options)
                                             : contimbre::messagesForRoll (excerpt (audibleRoll, from, to), {}, options);
        numbersUsed = 0;
        for (const auto& m : messages)
            if (m.message.address == "ctnote") ++numbersUsed;

        if (from > 0.0)
        {
            // What every voice was last told before `from`, in a run from the start.
            auto before = contimbre::stateBefore (programsAndGains != nullptr ? *programsAndGains : programsAndGainsOf (audibleRoll), from);
            before.insert (before.end(), std::make_move_iterator (messages.begin()), std::make_move_iterator (messages.end()));
            messages = std::move (before);
        }
        return messages;
    }

    std::vector<SynthNote> synthNotesOf (const Roll& part)
    {
        std::vector<SynthNote> notes;
        for (const auto& voice : part.voices)
            for (const auto& chord : voice.chords)
                for (const auto& note : chord.notes)
                {
                    if (note.duration <= 0.0) continue;
                    SynthNote n;
                    n.start = chord.onset;
                    n.end = chord.onset + note.duration;
                    n.cents = note.cents;
                    n.velocity = note.velocity;
                    if (note.breakpoints.size() >= 2)
                        for (const auto& b : note.breakpoints) n.glide.push_back ({ b.position, b.cents, b.slope });
                    if (const auto* slot = note.slot (2))
                        for (const auto& p : pointsOf (slot->content)) n.levels.push_back ({ p.x, p.y, p.slope });
                    notes.push_back (std::move (n));
                }
        std::stable_sort (notes.begin(), notes.end(), [] (const SynthNote& a, const SynthNote& b) { return a.start < b.start; });
        return notes;
    }
}

std::vector<contimbre::TimedMessage> messagesForPass (const Roll& roll, double from, double to, int firstNumber, int& numbersUsed)
{
    return passOf (audible (roll), nullptr, from, to, firstNumber, numbersUsed);
}

std::vector<contimbre::TimedMessage> messagesForPass (const Roll& roll, const std::vector<VoiceEngine>& engines,
                                                     double from, double to, int firstNumber, int& numbersUsed)
{
    // Muted and solo are judged over the whole roll, whatever plays a voice.
    return passOf (forEngine (audible (roll), engines, VoiceEngine::conTimbre), nullptr, from, to, firstNumber, numbersUsed);
}

std::vector<SynthNote> synthNotesForPass (const Roll& roll, const std::vector<VoiceEngine>& engines, double from, double to)
{
    const auto mine = forEngine (audible (roll), engines, VoiceEngine::synth);
    return wholeRoll (from, to) ? synthNotesOf (mine) : synthNotesOf (excerpt (mine, from, to));
}

Prepared prepare (const Roll& roll, const std::vector<VoiceEngine>& engines)
{
    Prepared prepared;
    auto playing = audible (roll);
    prepared.conTimbre = forEngine (playing, engines, VoiceEngine::conTimbre);
    for (size_t v = 0; v < playing.voices.size(); ++v)                      // what is left becomes the synth's roll, without another copy
        if ((v < engines.size() ? engines[v] : VoiceEngine::conTimbre) != VoiceEngine::synth)
            playing.voices[v].chords.clear();
    prepared.synth = std::move (playing);
    prepared.programsAndGains = programsAndGainsOf (prepared.conTimbre);
    return prepared;
}

std::vector<contimbre::TimedMessage> messagesForPass (const Prepared& prepared, double from, double to, int firstNumber, int& numbersUsed)
{
    return passOf (prepared.conTimbre, &prepared.programsAndGains, from, to, firstNumber, numbersUsed);
}

std::vector<SynthNote> synthNotesForPass (const Prepared& prepared, double from, double to)
{
    return wholeRoll (from, to) ? synthNotesOf (prepared.synth) : synthNotesOf (excerpt (prepared.synth, from, to));
}

std::vector<int> soundingAt (const std::vector<contimbre::TimedMessage>& pass, double time)
{
    std::map<int, bool> on;
    for (const auto& m : pass)
    {
        if (m.time > time) break;
        if (m.message.address == "ctnote" && m.message.arguments.size() > 1)        on[m.message.arguments[1].integer] = true;
        else if (m.message.address == "ctnoteoff" && ! m.message.arguments.empty()) on.erase (m.message.arguments[0].integer);
    }
    std::vector<int> numbers;
    for (const auto& [number, sounding] : on)
        if (sounding) numbers.push_back (number);
    return numbers;
}

} // namespace maket::playing
