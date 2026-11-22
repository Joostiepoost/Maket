// TimingTest.cpp

#include "TimingTest.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace maket::timing
{

// --- The passages -------------------------------------------------------------------

double Passage::length() const
{
    double end = 0.0;
    for (const auto& n : notes) end = std::max (end, n.onset + n.duration);
    return end;
}

Passage densePassage()
{
    Passage p;
    p.name = "dense";
    p.parts = { "a line, 40 notes a second",
                "chords of eight notes, four a second",
                "irregular, about 95 notes a second",
                "a burst, 200 notes a second" };

    // 1. A fast line: a note every 25 ms for 6 seconds.
    for (int i = 0; i < 240; ++i)
        p.notes.push_back ({ 500.0 + 25.0 * i, 20.0, 60.0 + (i % 13), 0 });

    // 2. Chords of eight notes, every 250 ms for 6 seconds: eight messages at one time.
    for (int c = 0; c < 24; ++c)
        for (int k = 0; k < 8; ++k)
            p.notes.push_back ({ 7000.0 + 250.0 * c, 200.0, 55.0 + 3.0 * k, 1 });

    // 3. Irregular: gaps of 1 to 20 ms, from a fixed series of numbers, for 6 seconds.
    {
        juce::uint32 state = 20261122u;
        auto next = [&state] { state = state * 1664525u + 1013904223u; return (state >> 16) & 0x7fffu; };

        double t = 14000.0;
        while (t < 20000.0)
        {
            p.notes.push_back ({ t, 15.0, 60.0 + (double) (next() % 12u), 2 });
            t += 1.0 + (double) (next() % 20u);
        }
    }

    // 4. A burst: a note every 5 ms for 2 seconds.
    for (int i = 0; i < 400; ++i)
        p.notes.push_back ({ 21000.0 + 5.0 * i, 4.0, 66.0 + (i % 7), 3 });

    return p;
}

Passage longPassage (double minutes)
{
    Passage p;
    p.name = "long";
    p.parts = { "a note every 100 ms" };

    const int count = (int) std::llround (minutes * 600.0);
    for (int i = 0; i < count; ++i)
        p.notes.push_back ({ 500.0 + 100.0 * i, 80.0, 60.0 + (i % 12), 0 });
    return p;
}

Roll passageAsRoll (const Passage& passage, int velocity)
{
    Roll roll;
    Voice voice;

    for (const auto& n : passage.notes)
    {
        // Notes with one onset make one chord.
        if (voice.chords.empty() || std::abs (voice.chords.back().onset - n.onset) > 1.0e-9)
        {
            Chord chord;
            chord.onset = n.onset;
            voice.chords.push_back (chord);
        }

        Note note;
        note.cents = n.pitch * 100.0;
        note.duration = n.duration;
        note.velocity = velocity;
        voice.chords.back().notes.push_back (note);
    }

    roll.voices.push_back (std::move (voice));
    return roll;
}

OscMessage tickMessage (int noteIndex, bool isNoteOff)
{
    return OscMessage ("/maket/tick").addInteger (2 * noteIndex + (isNoteOff ? 1 : 0));
}

// --- Reading the logs ---------------------------------------------------------------

std::vector<LogLine> readCollLog (const juce::String& text)
{
    std::vector<LogLine> lines;

    juce::StringArray entries;
    entries.addTokens (text, ";", "\"");

    for (const auto& entry : entries)
    {
        const auto e = entry.trim();
        const int comma = e.indexOfChar (',');
        if (comma <= 0 || ! e.substring (0, comma).trim().containsOnly ("0123456789")) continue;

        const auto rest = e.substring (comma + 1).trim();
        const int space = rest.indexOfAnyOf (" \t\r\n");

        LogLine line;
        line.number = e.substring (0, comma).trim().getIntValue();
        line.time = (space < 0 ? rest : rest.substring (0, space)).getDoubleValue();
        line.message = space < 0 ? juce::String() : rest.substring (space + 1).trim();
        lines.push_back (std::move (line));
    }
    return lines;
}

std::vector<SendLogLine> readMaketSendLog (const juce::String& text)
{
    std::vector<SendLogLine> lines;
    for (const auto& row : juce::StringArray::fromLines (text))
    {
        juce::StringArray cells;
        cells.addTokens (row, "\t", "");
        if (cells.size() < 5 || ! cells[0].containsOnly ("0123456789") || cells[0].isEmpty()) continue;

        lines.push_back ({ cells[1].getDoubleValue(), cells[2].getDoubleValue(), cells[4] });
    }
    return lines;
}

// --- Comparing ----------------------------------------------------------------------

namespace
{
    double middleOf (std::vector<double> values)
    {
        if (values.empty()) return 0.0;
        std::sort (values.begin(), values.end());
        const size_t n = values.size();
        return n % 2 == 1 ? values[n / 2] : 0.5 * (values[n / 2 - 1] + values[n / 2]);
    }

    /** The value below which `fraction` of the sorted values lie. */
    double within (const std::vector<double>& sorted, double fraction)
    {
        if (sorted.empty()) return 0.0;
        const auto rank = (size_t) std::ceil (fraction * (double) sorted.size());
        return sorted[std::min (sorted.size() - 1, rank == 0 ? 0 : rank - 1)];
    }

    /** The slope of `y` against `x`, by least squares. */
    double slope (const std::vector<double>& x, const std::vector<double>& y)
    {
        if (x.size() < 2) return 0.0;
        double mx = 0.0, my = 0.0;
        for (size_t i = 0; i < x.size(); ++i) { mx += x[i]; my += y[i]; }
        mx /= (double) x.size();
        my /= (double) y.size();

        double sxy = 0.0, sxx = 0.0;
        for (size_t i = 0; i < x.size(); ++i) { sxy += (x[i] - mx) * (y[i] - my); sxx += (x[i] - mx) * (x[i] - mx); }
        return sxx > 0.0 ? sxy / sxx : 0.0;
    }

    /** One message in both worlds. */
    struct Pair
    {
        double due = 0.0;           // when it had to be there, on the sender's clock
        double arrived = 0.0;       // when it arrived, on Max's clock
        int part = 0;
        bool noteOn = true;
    };

    /** Fills in everything of a Result that follows from the pairs, which
        are in the order of arrival. */
    void measure (Result& result, const std::vector<Pair>& pairs, int numParts)
    {
        result.arrived = (int) pairs.size();
        if (pairs.size() < 2)
        {
            result.usable = false;
            if (result.problem.isEmpty()) result.problem = "The log holds too few of the messages to measure anything.";
            return;
        }

        std::vector<double> raw, due;
        for (const auto& p : pairs) { raw.push_back (p.arrived - p.due); due.push_back (p.due); }

        // The two clocks are laid on each other at the middle message.
        const double shift = middleOf (raw);

        std::vector<double> all, ons;
        std::vector<std::vector<double>> parts ((size_t) std::max (1, numParts));
        double latestDue = -1.0e300;

        for (size_t i = 0; i < pairs.size(); ++i)
        {
            const double d = raw[i] - shift;
            all.push_back (d);
            if (pairs[i].noteOn) ons.push_back (d);
            if (pairs[i].part >= 0 && pairs[i].part < (int) parts.size()) parts[(size_t) pairs[i].part].push_back (d);

            if (pairs[i].due < latestDue - 1.0e-6) ++result.outOfOrder;
            latestDue = std::max (latestDue, pairs[i].due);
        }

        result.all = spreadOf (all);
        result.noteOns = spreadOf (ons);
        for (const auto& part : parts) result.perPart.push_back (spreadOf (part));
        result.driftPerMinute = slope (due, raw) * 60000.0;
        result.span = *std::max_element (due.begin(), due.end()) - *std::min_element (due.begin(), due.end());
        result.usable = true;
    }

    /** The whole number after the first space of `/maket/tick 12`. */
    bool tickNumber (const juce::String& message, int& number)
    {
        if (! message.startsWith ("/maket/tick ")) return false;
        const auto n = message.fromFirstOccurrenceOf (" ", false, false).trim();
        if (n.isEmpty() || ! n.containsOnly ("0123456789")) return false;
        number = n.getIntValue();
        return true;
    }

    bool isBachLine (const juce::String& message)
    {
        return (message.startsWith ("note ") || message.startsWith ("chord ")) && message.containsChar ('[');
    }

    juce::String ms (double value)      { return juce::String (value, 2); }
}

Spread spreadOf (const std::vector<double>& deviations)
{
    Spread s;
    s.count = (int) deviations.size();
    if (deviations.empty()) return s;

    double mean = 0.0;
    for (double d : deviations) mean += d;
    mean /= (double) deviations.size();

    double sum = 0.0;
    std::vector<double> sizes;
    s.earliest = s.latest = deviations.front();
    for (double d : deviations)
    {
        sum += (d - mean) * (d - mean);
        sizes.push_back (std::abs (d));
        s.earliest = std::min (s.earliest, d);
        s.latest = std::max (s.latest, d);
    }
    std::sort (sizes.begin(), sizes.end());

    s.typical = std::sqrt (sum / (double) deviations.size());
    s.half = within (sizes, 0.5);
    s.most = within (sizes, 0.95);
    s.nearlyAll = within (sizes, 0.99);
    s.worst = sizes.back();
    return s;
}

bool hasBachLines (const std::vector<LogLine>& log)
{
    for (const auto& line : log)
        if (isBachLine (line.message)) return true;
    return false;
}

Result compareMaketRun (const Passage& passage, const std::vector<OscScheduler::Sent>& sent, const std::vector<LogLine>& log)
{
    Result result;

    // What Maket sent: tick number -> when it was due and how late it left.
    struct Tick { double due = 0.0, late = 0.0; };
    std::map<int, Tick> ticks;
    std::vector<double> lateness;
    for (const auto& s : sent)
    {
        int number = 0;
        if (! tickNumber (s.text, number)) continue;
        ticks[number] = { s.due, s.sent - s.due };
        lateness.push_back (s.sent - s.due);
    }

    result.expected = (int) ticks.size();
    result.leftMaket = spreadOf (lateness);

    if (ticks.empty())
    {
        result.problem = "Maket sent no ticks in this run.";
        return result;
    }

    // What arrived, in the order of arrival.
    std::vector<Pair> pairs;
    std::map<int, bool> seen;
    for (const auto& line : log)
    {
        int number = 0;
        if (! tickNumber (line.message, number)) continue;

        const auto found = ticks.find (number);
        if (found == ticks.end() || seen[number]) continue;
        seen[number] = true;

        const int note = number / 2;
        Pair pair;
        pair.due = found->second.due;
        pair.arrived = line.time;
        pair.noteOn = (number % 2) == 0;
        pair.part = note < (int) passage.notes.size() ? passage.notes[(size_t) note].part : 0;
        pairs.push_back (pair);
    }

    if (pairs.empty())
        result.problem = "The log holds no ticks from Maket. Is the logging patch open in Max, on the port in the preferences?";

    measure (result, pairs, passage.parts.size());
    return result;
}

Result compareBachTake (const Passage& passage, const std::vector<LogLine>& log)
{
    Result result;

    // Which part an onset belongs to.
    std::map<juce::int64, int> partOfOnset;                    // onset in thousandths of a ms
    for (const auto& n : passage.notes) partOfOnset[(juce::int64) std::llround (n.onset * 1000.0)] = n.part;

    std::vector<Pair> pairs;
    int notewise = 0, chordwise = 0;
    for (const auto& line : log)
    {
        if (! isBachLine (line.message)) continue;
        if (line.message.startsWith ("note ")) ++notewise; else ++chordwise;

        // note <voice> <channel> [ <onset> [ <cents> ... ] <flag> ]
        const auto afterBracket = line.message.fromFirstOccurrenceOf ("[", false, false).trim();
        const double onset = afterBracket.upToFirstOccurrenceOf (" ", false, false).getDoubleValue();

        Pair pair;
        pair.due = onset;
        pair.arrived = line.time;
        const auto found = partOfOnset.find ((juce::int64) std::llround (onset * 1000.0));
        pair.part = found != partOfOnset.end() ? found->second : 0;
        pairs.push_back (pair);
    }

    result.mode = chordwise > notewise ? "chordwise" : "notewise";
    result.expected = result.mode == "chordwise" ? (int) partOfOnset.size() : (int) passage.notes.size();

    if (pairs.empty())
        result.problem = "The log holds no notes from bach.roll. Does its playout go to `s maket_log_in`, as text?";

    measure (result, pairs, passage.parts.size());
    return result;
}

Result compareLogs (const std::vector<SendLogLine>& sent, const std::vector<LogLine>& log)
{
    Result result;
    result.expected = (int) sent.size();

    std::vector<double> lateness;
    for (const auto& s : sent) lateness.push_back (s.late);
    result.leftMaket = spreadOf (lateness);

    std::vector<Pair> pairs;
    const size_t n = std::min (sent.size(), log.size());
    for (size_t i = 0; i < n; ++i)
    {
        Pair pair;
        pair.due = sent[i].due;
        pair.arrived = log[i].time;
        pair.noteOn = sent[i].message.startsWith ("noteon");
        pairs.push_back (pair);
    }

    measure (result, pairs, 1);
    return result;
}

// --- In words -----------------------------------------------------------------------

namespace
{
    juce::String spreadInWords (const Spread& s)
    {
        return "half within " + ms (s.half) + " ms of their time, 95 in 100 within " + ms (s.most)
             + " ms, 99 in 100 within " + ms (s.nearlyAll) + " ms; the farthest " + ms (s.worst)
             + " ms (earliest " + ms (s.earliest) + ", latest +" + ms (s.latest) + ")";
    }
}

juce::String describe (const Result& result, const Passage& passage)
{
    if (! result.usable)
        return "No result: " + result.problem + "\n";

    juce::String text;
    text << "Arrived: " << result.arrived << " of " << result.expected << " messages; out of order: " << result.outOfOrder << "."
         << (result.mode.isNotEmpty() ? " bach.roll played " + result.mode + "." : juce::String()) << "\n";
    text << "All messages: " << spreadInWords (result.all) << ".\n";

    if (result.mode.isEmpty() && result.noteOns.count > 0 && result.noteOns.count != result.all.count)
        text << "The note-ons alone: " << spreadInWords (result.noteOns) << ".\n";

    if (result.perPart.size() > 1)
        for (size_t i = 0; i < result.perPart.size(); ++i)
            if (result.perPart[i].count > 0)
                text << "  Part " << (int) i + 1 << " (" << (i < (size_t) passage.parts.size() ? passage.parts[(int) i] : juce::String())
                     << "): 95 in 100 within " << ms (result.perPart[i].most) << " ms, the farthest " << ms (result.perPart[i].worst) << " ms.\n";

    if (result.leftMaket.count > 0)
        text << "Leaving Maket: 95 in 100 within " << ms (result.leftMaket.most) << " ms of their time, the latest "
             << ms (result.leftMaket.latest) << " ms after it.\n";

    if (result.span >= 60000.0)
        text << "Over the " << juce::String (result.span / 60000.0, 1) << " minutes the two clocks drift apart by "
             << ms (result.driftPerMinute) << " ms a minute.\n";
    return text;
}

juce::String tableHeading()
{
    return "| When | Passage | Played by | Condition | Arrived | Out of order | Typical (ms) | Half within | 95 in 100 within | 99 in 100 within | Farthest | Left Maket at the latest |\n"
           "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |\n";
}

juce::String tableRow (const juce::String& when, const juce::String& passage, const juce::String& source,
                       const juce::String& condition, const Result& result)
{
    if (! result.usable)
        return "| " + when + " | " + passage + " | " + source + " | " + condition + " | no result: " + result.problem + " | | | | | | | |\n";

    return "| " + when + " | " + passage + " | " + source + " | " + condition
         + " | " + juce::String (result.arrived) + " of " + juce::String (result.expected)
         + " | " + juce::String (result.outOfOrder)
         + " | " + ms (result.all.typical) + " | " + ms (result.all.half) + " | " + ms (result.all.most)
         + " | " + ms (result.all.nearlyAll) + " | " + ms (result.all.worst)
         + " | " + (result.leftMaket.count > 0 ? ms (result.leftMaket.latest) : juce::String()) + " |\n";
}

} // namespace maket::timing
