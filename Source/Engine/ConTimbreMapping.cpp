// ConTimbreMapping.cpp

#include "ConTimbreMapping.h"
#include "ConTimbre.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace maket::contimbre
{

namespace
{
    /** The points of a function slot or of the breakpoints: [ x y ... ] each. */
    struct Point { double x = 0.0, y = 0.0; };

    std::vector<Point> pointsOf (const Llll& content)
    {
        std::vector<Point> points;
        for (const auto& item : content.items)
            if (item.isList() && item.size() >= 2 && item[0].isNumber() && item[1].isNumber())
                points.push_back ({ item[0].value, item[1].value });
        return points;
    }

    void addAtom (OscMessage& message, const Llll& atom)
    {
        if (atom.kind == Llll::Kind::integer)   message.addInteger ((int) atom.num);
        else if (atom.isNumber())               message.addReal (atom.value);
        else if (atom.isSymbol())               message.addText (atom.text);
    }

    /** Which commands of a note to make: all of them in Maket's order, or one of
        the two halves of the Max patch's order. */
    enum class Part { all, programAndNote, curves };

    void addNote (std::vector<TimedMessage>& out, const juce::String& voice, int voiceIndex, double onset, const Note& note,
                  Part part, bool glissandoToZero, int number = 0, int voiceNumber = 0)
    {
        // number above 0: the note is addressed by that number, on the voice "voice" + voiceNumber.
        const bool byNumber = number > 0;
        const double pitch = note.cents / 100.0;
        auto add = [&] (double time, OscMessage message) { out.push_back ({ time, std::move (message), voiceIndex, onset, time }); };

        const bool start = part != Part::curves, curves = part != Part::programAndNote;

        // Slot 1: the program, before the note.
        if (start)
            if (const auto* slot = note.slot (1))
                if (slot->content.size() > 0 && ! slot->content[0].isList())
                    add (onset, program (voice, slot->content[0].isSymbol() ? slot->content[0].text : slot->content[0].toLine()));

        // Slot 3: the gain of the voice, a step at every point.
        auto addGains = [&]
        {
            if (const auto* slot = note.slot (3))
                for (const auto& p : pointsOf (slot->content))
                    add (onset + p.x, gain (voice, p.y));
        };
        if (part == Part::all) addGains();

        // The note itself, with the first two items of slot 4 when there are any.
        if (start)
        {
            auto on = byNumber ? ctNote (voiceNumber, number, pitch, note.velocity, note.duration) : noteOn (voice, pitch, note.velocity);
            if (const auto* slot = note.slot (4))
                for (int i = 0; i < std::min (2, slot->content.size()); ++i)
                    addAtom (on, slot->content[i]);
            add (onset, std::move (on));
            add (onset + note.duration, byNumber ? ctNoteOff (number) : noteOff (voice, pitch));
        }

        if (! curves) return;
        if (part == Part::curves) addGains();

        // Slot 2: from every point to the next one.
        if (const auto* slot = note.slot (2))
        {
            const auto points = pointsOf (slot->content);
            for (size_t i = 0; i + 1 < points.size(); ++i)
                add (onset + std::round (points[i].x), byNumber ? ctDynamic (number, points[i + 1].y, points[i + 1].x - points[i].x)
                                                               : dynamic (voice, pitch, points[i + 1].y, points[i + 1].x - points[i].x));
        }

        // The pitch breakpoints: from every one to the next. Their places run from 0 to 1 along the note.
        for (size_t i = 0; i + 1 < note.breakpoints.size(); ++i)
        {
            const double from = note.breakpoints[i].position * note.duration;
            const double to = note.breakpoints[i + 1].position * note.duration;
            add (onset + std::round (from), byNumber ? ctGlissando (number, note.breakpoints[i + 1].cents / 100.0, to - from)
                                                     : glissando (voice, pitch, note.breakpoints[i + 1].cents / 100.0, to - from));
        }

        // The Max patch: a note without breakpoints still gets a glide, to where it is.
        if (glissandoToZero && note.breakpoints.size() < 2)
            add (onset, byNumber ? ctGlissando (number, 0.0, note.duration) : glissando (voice, pitch, 0.0, note.duration));
    }

    /** The command a marker holds, when its name begins with `cpe`. */
    bool markerCommand (const Marker& marker, OscMessage& message)
    {
        const Llll* words = nullptr;
        if (marker.names.isList() && marker.names.size() > 1 && marker.names[0].isSymbol ("cpe"))
            words = &marker.names;
        else if (marker.names.isList())
            for (const auto& item : marker.names.items)
                if (item.isList() && item.size() > 1 && item[0].isSymbol ("cpe")) { words = &item; break; }

        if (words == nullptr || ! (*words)[1].isSymbol()) return false;

        message = OscMessage ((*words)[1].text);
        for (int i = 2; i < words->size(); ++i) addAtom (message, (*words)[i]);
        return true;
    }
}

std::vector<TimedMessage> messagesForRoll (const Roll& roll, const std::vector<int>& voices, const MappingOptions& options)
{
    std::vector<TimedMessage> out;
    int nextNumber = 1;

    for (size_t v = 0; v < roll.voices.size(); ++v)
    {
        const auto& voice = roll.voices[v];
        if (voice.name.isEmpty() || voice.isMuted()) continue;
        if (! voices.empty() && std::find (voices.begin(), voices.end(), (int) v) == voices.end()) continue;

        for (const auto& chord : voice.chords)
        {
            if (chord.isMuted()) continue;

            // By number: every note of the chord gets its number first, so that both halves of the patch's order use the same.
            const int voiceNumber = (int) v + 1;
            const auto voiceLabel = options.byNumber ? "voice" + juce::String (voiceNumber) : voice.name;
            std::vector<int> numbers;
            for (const auto& note : chord.notes)
                numbers.push_back (options.byNumber && ! note.isMuted() ? nextNumber++ : 0);

            auto each = [&] (Part part)
            {
                for (size_t n = 0; n < chord.notes.size(); ++n)
                    if (! chord.notes[n].isMuted())
                        addNote (out, voiceLabel, (int) v, chord.onset, chord.notes[n], part, options.glissandoToZero, numbers[n], voiceNumber);
            };

            if (options.noteOnFirst)    { each (Part::programAndNote); each (Part::curves); }
            else                        each (Part::all);
        }
    }

    for (const auto& marker : roll.markers)
    {
        OscMessage message;
        if (markerCommand (marker, message)) out.push_back ({ marker.position, std::move (message), -1, marker.position, marker.position });
    }

    // In order of time; commands of one moment stay in the order they were made.
    std::stable_sort (out.begin(), out.end(), [] (const TimedMessage& a, const TimedMessage& b) { return a.time < b.time; });

    // The commands of one voice that are due at the same moment, a little apart from each other.
    if (options.spacing > 0.0)
    {
        std::map<int, std::pair<double, int>> last;     // per voice: the moment, and how many commands it holds so far
        for (auto& m : out)
        {
            auto& [moment, count] = last[m.voice];
            if (count > 0 && std::abs (m.moment - moment) < 1.0e-9)     m.time = m.moment + options.spacing * count++;
            else                                    { moment = m.moment; count = 1; }
        }
        std::stable_sort (out.begin(), out.end(), [] (const TimedMessage& a, const TimedMessage& b) { return a.time < b.time; });
    }

    return out;
}

std::vector<TimedMessage> messagesFrom (const std::vector<TimedMessage>& all, double from)
{
    if (from <= 0.0) return all;

    // The last program and the last gain of every voice before `from`.
    std::map<int, TimedMessage> lastProgram, lastGain;
    std::vector<TimedMessage> out, later;

    for (const auto& m : all)
    {
        if (m.time < from)
        {
            if (m.message.address == "program")     lastProgram[m.voice] = m;
            else if (m.message.address == "gain")   lastGain[m.voice] = m;
        }
        if (m.noteOnset >= from) later.push_back (m);
    }

    for (auto* kept : { &lastProgram, &lastGain })
        for (auto& [voice, m] : *kept)
        {
            juce::ignoreUnused (voice);
            m.time = from;
            m.noteOnset = from;
            m.moment = from;
            out.push_back (m);
        }

    out.insert (out.end(), later.begin(), later.end());
    return out;
}

// --- Held against a log from Max -------------------------------------------------------

namespace
{
    /** A command as its words: `program Vlc "sul tasto"` is three. */
    juce::StringArray wordsOf (const juce::String& text)
    {
        juce::StringArray words;
        words.addTokens (text, " \t\r\n", "\"");
        words.removeEmptyStrings();
        for (auto& w : words) w = w.unquoted().replace ("\\,", ",").replace ("\\;", ";");
        return words;
    }

    bool isNumber (const juce::String& word)
    {
        return word.isNotEmpty() && word.containsOnly ("0123456789.-+e") && word.containsAnyOf ("0123456789");
    }

    bool sameWords (const juce::StringArray& a, const juce::StringArray& b)
    {
        if (a.size() != b.size()) return false;
        for (int i = 0; i < a.size(); ++i)
        {
            if (isNumber (a[i]) && isNumber (b[i]))
            {
                if (std::abs (a[i].getDoubleValue() - b[i].getDoubleValue()) >= 0.002) return false;
            }
            else if (a[i] != b[i])
            {
                return false;
            }
        }
        return true;
    }

    /** Whether a number is whole in one and real in the other, for two commands that are the same. */
    bool differInKind (const juce::StringArray& a, const juce::StringArray& b)
    {
        for (int i = 0; i < a.size() && i < b.size(); ++i)
            if (isNumber (a[i]) && isNumber (b[i]) && a[i].containsChar ('.') != b[i].containsChar ('.'))
                return true;
        return false;
    }

    bool isCommandForThePlayer (const juce::String& message)
    {
        return message.isNotEmpty() && ! message.startsWithChar ('/')                    // not Maket's own ticks
            && ! message.startsWith ("note ") && ! message.startsWith ("chord ")         // not bach.roll's playout
            && juce::CharacterFunctions::isLetter (message[0]);
    }

    struct Item
    {
        double time = 0.0;
        juce::String text;
        juce::StringArray words;
        bool matched = false;
        int partner = -1;       // for a command of the log: which of Maket's it was paired with
        double moment = 0.0;    // for a command of Maket's: when it is due by the roll
        bool known = false;     // not paired, but explained by what is known of the Max patch
    };

    juce::String timeAndText (double seconds, const juce::String& text)
    {
        return juce::String (seconds, 3).paddedLeft (' ', 9) + " s   " + text;
    }
}

Comparison compareWithLog (const std::vector<TimedMessage>& expected, const std::vector<timing::LogLine>& log)
{
    Comparison c;

    std::vector<Item> mine, theirs;
    juce::StringArray myVoices;
    std::vector<int> ownCounts (2, 0);
    for (const auto& e : expected)
    {
        Item item { e.time, e.message.toText(), {}, false };
        item.words = wordsOf (item.text);
        item.moment = e.moment;
        if (item.words.size() > 1) myVoices.addIfNotAlreadyThere (item.words[1]);
        mine.push_back (std::move (item));
    }
    for (const auto& line : log)
    {
        if (! isCommandForThePlayer (line.message)) continue;
        Item item { line.time, line.message, wordsOf (line.message), false };
        if (item.words.isEmpty()) continue;

        // What the ePlayer's own Max side sends is counted and left aside.
        static const juce::StringArray playersOwn { "set_window_titles", "end_dialog" };
        if (const int own = playersOwn.indexOf (item.words[0]); own >= 0)
        {
            ++ownCounts[(size_t) own];
            continue;
        }

        // A command for a voice Maket was not asked to play is set apart.
        static const juce::StringArray perVoice { "noteon", "noteoff", "program", "dynamic", "gain", "glissando" };
        if (perVoice.contains (item.words[0]) && item.words.size() > 1 && ! myVoices.contains (item.words[1]))
        {
            c.otherVoices.addIfNotAlreadyThere (item.words[1]);
            continue;
        }
        theirs.push_back (std::move (item));
    }

    c.inMaket = (int) mine.size();
    c.inLog = (int) theirs.size();
    if (ownCounts[0] > 0) c.playersOwn.add (juce::String (ownCounts[0]) + " set_window_titles");
    if (ownCounts[1] > 0) c.playersOwn.add (juce::String (ownCounts[1]) + " end_dialog");

    if (mine.empty())   { c.problem = "The roll gives no commands for the chosen voices."; return c; }
    if (theirs.empty()) { c.problem = "The log holds no commands for the ePlayer."; return c; }

    // Where the roll's time 0 lies on Max's clock: every pair of equal
    // note-ons votes for a difference, and the difference most voted for wins.
    {
        std::map<juce::int64, int> votes;
        for (const auto& t : theirs)
        {
            if (t.words[0] != "noteon") continue;
            for (const auto& m : mine)
                if (m.words[0] == "noteon" && sameWords (m.words, t.words))
                    ++votes[(juce::int64) std::llround ((t.time - m.time) / 20.0)];
        }

        juce::int64 best = 0;
        int most = 0;
        for (const auto& [bin, count] : votes)
        {
            const int around = count + (votes.count (bin - 1) ? votes.at (bin - 1) : 0) + (votes.count (bin + 1) ? votes.at (bin + 1) : 0);
            if (around > most) { most = around; best = bin; }
        }
        if (most == 0) { c.problem = "No note-on in the log is one of the roll's: is it the same roll, with the same voice names?"; return c; }
        c.offset = (double) best * 20.0;
    }

    // Pair every command of the log with Maket's nearest equal one.
    std::vector<double> differences;
    for (auto& t : theirs)
    {
        Item* nearest = nullptr;
        double nearestDistance = 250.0;
        for (auto& m : mine)
        {
            if (m.matched) continue;
            if (m.time > t.time - c.offset + 250.0) break;      // Maket's are in order of time
            const double distance = std::abs (t.time - c.offset - m.time);
            if (distance < nearestDistance && m.words[0] == t.words[0] && sameWords (m.words, t.words))
            {
                nearest = &m;
                nearestDistance = distance;
            }
        }
        if (nearest != nullptr)
        {
            nearest->matched = true;
            t.matched = true;
            t.partner = (int) (nearest - mine.data());
            differences.push_back (t.time - nearest->time);

            if (differInKind (t.words, nearest->words) && ++c.numKindDifferences <= 4)
            {
                c.kindDifferences.add ("Max:   " + t.text);
                c.kindDifferences.add ("Maket: " + nearest->text);
            }
        }
    }

    // The clocks laid together at the middle pair, and the spread around it.
    {
        auto sorted = differences;
        std::sort (sorted.begin(), sorted.end());
        if (! sorted.empty()) c.offset = sorted[sorted.size() / 2];
        for (auto& d : differences) d -= c.offset;
        c.timing = timing::spreadOf (differences);
    }

    // The order within one moment: the commands of one voice that Maket sends at the same
    // time, in the order of the log and in Maket's own.
    {
        std::map<std::pair<juce::int64, juce::String>, std::vector<std::pair<int, int>>> moments;   // (log index, Maket index)
        for (size_t i = 0; i < theirs.size(); ++i)
        {
            const auto& t = theirs[i];
            if (t.partner < 0 || t.words.size() < 2) continue;
            const auto& m = mine[(size_t) t.partner];
            moments[{ (juce::int64) std::llround (m.moment * 1000.0), m.words[1] }].push_back ({ (int) i, t.partner });
        }

        std::map<juce::String, int> patterns;
        for (auto& [key, pairs] : moments)
        {
            juce::ignoreUnused (key);
            if (pairs.size() < 2) continue;

            juce::StringArray inLog, inMaket;
            for (const auto& p : pairs) inLog.add (theirs[(size_t) p.first].words[0]);       // already in the log's order
            std::sort (pairs.begin(), pairs.end(), [] (const auto& a, const auto& b) { return a.second < b.second; });
            for (const auto& p : pairs) inMaket.add (mine[(size_t) p.second].words[0]);

            if (inLog != inMaket)
            {
                ++c.numOrderDifferences;
                ++patterns["Max: " + inLog.joinIntoString (", ") + "   Maket: " + inMaket.joinIntoString (", ")];
            }
        }

        std::vector<std::pair<int, juce::String>> byCount;
        for (const auto& [pattern, count] : patterns) byCount.push_back ({ count, pattern });
        std::sort (byCount.begin(), byCount.end(), [] (const auto& a, const auto& b) { return a.first > b.first; });
        for (size_t i = 0; i < byCount.size() && i < 8; ++i)
            c.orderDifferences.add (juce::String (byCount[i].first) + " times   " + byCount[i].second);
    }

    // What is known of the Max patch, among the commands that found no partner.
    for (auto& t : theirs)
    {
        if (t.matched) continue;
        const double when = t.time - c.offset;

        // `all_notes_off`: playing was stopped in Max.
        if (t.words[0] == "all_notes_off") { t.known = true; ++c.numStops; continue; }

        if ((t.words[0] != "dynamic" && t.words[0] != "glissando") || t.words.size() != 5) continue;

        // A chord: the same command in everything but the pitch, which in the patch is that of the chord's last note.
        for (auto& m : mine)
        {
            if (m.time > when + 250.0) break;
            if (m.matched || m.known || m.words.size() != 5 || m.words[0] != t.words[0] || m.words[1] != t.words[1]) continue;
            if (std::abs (m.time - when) < 250.0
                && std::abs (m.words[3].getDoubleValue() - t.words[3].getDoubleValue()) < 0.002
                && std::abs (m.words[4].getDoubleValue() - t.words[4].getDoubleValue()) < 0.002)
            {
                t.known = true;
                m.known = true;
                ++c.numChordFault;
                break;
            }
        }
        if (t.known) continue;

        // A glide to 0 for a note that has no breakpoints: the note-on of that pitch is Maket's.
        if (t.words[0] == "glissando" && std::abs (t.words[3].getDoubleValue()) < 1.0e-9)
        {
            for (const auto& m : mine)
            {
                if (m.time > when + 250.0) break;
                if (m.words[0] == "noteon" && std::abs (m.time - when) < 250.0 && m.words[1] == t.words[1]
                    && std::abs (m.words[2].getDoubleValue() - t.words[2].getDoubleValue()) < 0.002)
                {
                    t.known = true;
                    ++c.numZeroGlissandi;
                    break;
                }
            }
        }
    }

    // Counts per kind of command.
    auto indexOf = [&c] (const juce::String& command)
    {
        int i = c.commands.indexOf (command);
        if (i < 0)
        {
            c.commands.add (command);
            c.maketPerCommand.push_back (0);
            c.logPerCommand.push_back (0);
            c.matchedPerCommand.push_back (0);
            i = c.commands.size() - 1;
        }
        return (size_t) i;
    };
    for (const auto& m : mine)   { ++c.maketPerCommand[indexOf (m.words[0])]; }
    for (const auto& t : theirs) { const auto i = indexOf (t.words[0]); ++c.logPerCommand[i]; if (t.matched) { ++c.matchedPerCommand[i]; ++c.matched; } }

    // What is left over, and for a command of the log Maket's nearest one of that kind and voice.
    const int shown = 12;
    for (const auto& t : theirs)
    {
        if (t.matched || t.known) continue;
        if (++c.numOnlyInLog <= shown) c.onlyInLog.add (timeAndText ((t.time - c.offset) / 1000.0, t.text));

        const Item* closest = nullptr;
        double closestDistance = 1000.0;
        for (const auto& m : mine)
        {
            if (m.matched || m.known || m.words[0] != t.words[0] || m.words.size() < 2 || t.words.size() < 2 || m.words[1] != t.words[1]) continue;
            const double distance = std::abs (t.time - c.offset - m.time);
            if (distance < closestDistance) { closest = &m; closestDistance = distance; }
        }
        if (closest != nullptr && c.nearMisses.size() < 2 * shown)
        {
            c.nearMisses.add ("Max:   " + timeAndText ((t.time - c.offset) / 1000.0, t.text));
            c.nearMisses.add ("Maket: " + timeAndText (closest->time / 1000.0, closest->text));
        }
    }
    for (const auto& m : mine)
        if (! m.matched && ! m.known && ++c.numOnlyInMaket <= shown) c.onlyInMaket.add (timeAndText (m.time / 1000.0, m.text));

    c.usable = true;
    return c;
}

juce::String describe (const Comparison& c)
{
    if (! c.usable) return "No result: " + c.problem + "\n";

    juce::String text;
    text << "The same command at the same time in both: " << c.matched << ". Maket makes " << c.inMaket
         << " commands for the roll, the log holds " << c.inLog << ".\n";

    for (int i = 0; i < c.commands.size(); ++i)
        text << "  " << c.commands[i].paddedRight (' ', 14) << "Maket " << juce::String (c.maketPerCommand[(size_t) i]).paddedLeft (' ', 5)
             << "   log " << juce::String (c.logPerCommand[(size_t) i]).paddedLeft (' ', 5)
             << "   the same " << juce::String (c.matchedPerCommand[(size_t) i]).paddedLeft (' ', 5) << "\n";

    if (c.timing.count > 0)
        text << "In time, the pairs lie: half within " << juce::String (c.timing.half, 2) << " ms of each other, 95 in 100 within "
             << juce::String (c.timing.most, 2) << " ms, the farthest " << juce::String (c.timing.worst, 2) << " ms.\n";

    if (c.numKindDifferences > 0)
        text << "In " << c.numKindDifferences << " of the pairs a number is whole on one side and real on the other (the same value, "
             << "another kind of number on its way to the ePlayer). The first:\n  " << c.kindDifferences.joinIntoString ("\n  ") << "\n";

    if (c.numOrderDifferences > 0)
        text << "At " << c.numOrderDifferences << " moments the commands of one voice that are due together come in another order in the log "
             << "than Maket sends them:\n  " << c.orderDifferences.joinIntoString ("\n  ") << "\n";
    else if (c.matched > 0)
        text << "Commands of one voice that are due together come in the same order in both.\n";

    if (c.otherVoices.size() > 0)
        text << "The log also holds commands for voices that were not chosen here: " << c.otherVoices.joinIntoString (", ") << ".\n";

    if (c.numZeroGlissandi + c.numChordFault + c.numStops > 0 || c.playersOwn.size() > 0)
    {
        text << "Known of the Max patch and kept apart:\n";
        if (c.numZeroGlissandi > 0)
            text << "  " << c.numZeroGlissandi << " times a `glissando` to 0 for a note without pitch breakpoints, which Maket does not send\n";
        if (c.numChordFault > 0)
            text << "  " << c.numChordFault << " commands of chords that the patch sends with the pitch of the chord's last note; Maket sends each "
                 << "with the pitch of its own note\n";
        if (c.numStops > 0)
            text << "  " << (c.numStops == 1 ? juce::String ("once") : juce::String (c.numStops) + " times")
                 << " `all_notes_off`, when playing was stopped in Max\n";
        if (c.playersOwn.size() > 0)
            text << "  from the ePlayer's own side in Max: " << c.playersOwn.joinIntoString (", ") << "\n";
    }

    if (c.numOnlyInLog > 0)
        text << "\nOnly in the log (" << c.numOnlyInLog << "; the first):\n  " << c.onlyInLog.joinIntoString ("\n  ") << "\n";
    if (c.numOnlyInMaket > 0)
        text << "\nOnly in Maket (" << c.numOnlyInMaket << "; the first):\n  " << c.onlyInMaket.joinIntoString ("\n  ") << "\n";
    if (c.nearMisses.size() > 0)
        text << "\nSide by side, a command of the log and Maket's nearest one of that kind for that voice:\n  "
             << c.nearMisses.joinIntoString ("\n  ") << "\n";

    if (c.numOnlyInLog == 0 && c.numOnlyInMaket == 0)
        text << "Nothing else is left over on either side.\n";
    return text;
}

} // namespace maket::contimbre
