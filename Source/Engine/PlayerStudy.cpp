// PlayerStudy.cpp

#include "PlayerStudy.h"
#include "ConTimbre.h"

#include <algorithm>

namespace maket::study
{

std::vector<Way> ways (const Options& o, int run)
{
    std::vector<Way> all;

    if (run == 2)
    {
        // Single notes, no dynamic: the same velocity by pitch and by number.
        for (int velocity : { 15, 36, 60, 100, 127 })
            for (bool byNumber : { false, true })
            {
                Way w;
                w.name = juce::String (byNumber ? "one note by number" : "one note by pitch") + ", velocity " + juce::String (velocity);
                w.byNumber = byNumber;
                w.singleVelocity = velocity;
                w.share = 0.4;
                all.push_back (w);
            }

        auto figureWay = [&all] (const char* name, bool byNumber, double firstDelay, double lead, double restBefore, double share)
        {
            Way w;
            w.name = name;
            w.byNumber = byNumber;
            w.firstDelay = firstDelay;
            w.lead = lead;
            w.restBefore = restBefore;
            w.share = share;
            all.push_back (w);
        };
        figureWay ("figure by pitch, as Maket sends now",                          false, 0.0, 0.3,    0.0, 1.0);
        figureWay ("figure by number, as Maket sends now",                         true,  0.0, 0.3,    0.0, 1.0);
        figureWay ("figure by number, timed as ScAst",                             true,  0.7, 1.4,    0.0, 1.0);
        figureWay ("figure by number, only the first dynamic as ScAst (0.7 ms)",   true,  0.7, 0.3,    0.0, 1.0);
        figureWay ("figure by number, only the next ramps as ScAst (1.4 ms)",      true,  0.0, 1.4,    0.0, 1.0);
        figureWay ("figure by pitch, after 3 seconds of silence",                  false, 0.0, 0.3, 2400.0, 0.75);
        return all;
    }

    auto add = [&all] (const char* name, bool byNumber, double firstDelay, double lead, bool correction)
    {
        all.push_back ({ name, byNumber, firstDelay, lead, correction });
    };

    add ("by pitch, as Maket sends now",                        false,  0.0,  0.3, true);
    add ("by pitch, timed as ScAst",                            false,  0.7,  1.4, true);
    add ("by pitch, first dynamic 5 ms after the note-on",      false,  5.0,  0.3, true);
    add ("by pitch, first dynamic 20 ms after the note-on",     false, 20.0,  0.3, true);
    add ("by pitch, next ramp 0.6 ms after the end",            false,  0.0, -0.6, true);
    add ("by pitch, next ramp 5 ms before the end",             false,  0.0,  5.0, true);

    if (o.withNumber)
    {
        add ("by number, as Maket sends now",                   true,   0.0,  0.3, true);
        add ("by number, timed as ScAst",                       true,   0.7,  1.4, true);
    }
    if (o.withCorrectionOff)
    {
        add ("by pitch, dynamic correction off",                false,  0.0,  0.3, false);
        if (o.withNumber)
            add ("by number, dynamic correction off",           true,   0.0,  0.3, false);
    }
    return all;
}

std::vector<contimbre::TimedMessage> figure (const Options& o, const Way& way, double start, int& nextNumber)
{
    std::vector<contimbre::TimedMessage> out;
    const auto voice = o.voiceName();

    struct Ramp { double target, length; };
    auto note = [&] (double onset, double pitch, int velocity, double length, const std::vector<Ramp>& ramps)
    {
        auto add = [&] (double time, OscMessage message) { out.push_back ({ time, std::move (message), 0, onset, time }); };
        const int number = nextNumber++;

        add (onset, way.byNumber ? contimbre::ctNote (o.voiceNumber, number, pitch, velocity, length)
                                 : contimbre::noteOn (voice, pitch, velocity));

        // Each ramp's command: the first after the note-on, the others before (or after) the end of the ramp before.
        double when = onset + way.firstDelay;
        for (const auto& ramp : ramps)
        {
            add (when, way.byNumber ? contimbre::ctDynamic (number, ramp.target, ramp.length)
                                    : contimbre::dynamic (voice, pitch, ramp.target, ramp.length));
            when += ramp.length - way.lead;
        }

        add (onset + length, way.byNumber ? contimbre::ctNoteOff (number) : contimbre::noteOff (voice, pitch));
    };

    if (way.singleVelocity > 0)
    {
        note (start, 67.4, way.singleVelocity, singleNoteLength, {});
    }
    else
    {
        note (start,          67.4,  15, 1700.0, { { 60.0, 1500.0 } });
        note (start + 1500.0, 67.68, 36, 1900.0, { { 70.0, 700.0 }, { 100.0, 600.0 }, { 127.0, 400.0 } });
    }

    std::stable_sort (out.begin(), out.end(), [] (const auto& a, const auto& b) { return a.time < b.time; });
    return out;
}

Plan makePlan (const Options& o, int onlyWay, int run)
{
    Plan plan;
    plan.options = o;
    plan.run = run;
    plan.ways = ways (o, run);

    auto add = [&plan] (double time, OscMessage message) { plan.messages.push_back ({ time, std::move (message), 0, time, time }); };

    // The voice as the figure needs it, half a second before the first note.
    const double lead = 500.0;
    add (0.0, contimbre::program (o.voiceName(), o.program));
    add (0.0, contimbre::gain (o.voiceName(), o.gain));

    const int numWays = (int) plan.ways.size();
    const int repetitions = juce::jmax (1, o.repetitions);

    // How many of each way, and in turns: each round begins one way further, so that no way always follows the same one.
    std::vector<int> order;
    if (onlyWay >= 0)
    {
        order.push_back (juce::jlimit (0, numWays - 1, onlyWay));
    }
    else
    {
        std::vector<int> count ((size_t) numWays);
        int most = 0;
        for (int w = 0; w < numWays; ++w)
        {
            count[(size_t) w] = juce::jmax (1, juce::roundToInt (plan.ways[(size_t) w].share * repetitions));
            most = juce::jmax (most, count[(size_t) w]);
        }
        for (int r = 0; r < most; ++r)
            for (int i = 0; i < numWays; ++i)
            {
                const int w = (i + r) % numWays;
                if (r < count[(size_t) w]) order.push_back (w);
            }
    }

    int nextNumber = 1;
    double start = lead, end = lead;

    for (int w : order)
    {
        const auto& way = plan.ways[(size_t) w];
        start += way.restBefore;
        add (start - 150.0, contimbre::dynamicCorrection (way.correction));

        const auto notes = figure (o, way, start, nextNumber);
        plan.messages.insert (plan.messages.end(), notes.begin(), notes.end());
        plan.repetitions.push_back ({ w, start });

        end = start + (way.singleVelocity > 0 ? singleNoteLength : figureLength);
        start += way.singleVelocity > 0 ? singleRepetitionLength : repetitionLength;
    }

    // The player is left as it was found: the correction on.
    plan.length = end + 300.0;
    add (plan.length, contimbre::dynamicCorrection (true));

    std::stable_sort (plan.messages.begin(), plan.messages.end(), [] (const auto& a, const auto& b) { return a.time < b.time; });
    return plan;
}

juce::String planAsText (const Plan& plan)
{
    juce::String text;
    text << "Maket, study of the ePlayer, run " << plan.run << (plan.run == 2 ? " (stage 1f.3)\n" : " (stage 1f.2)\n")
         << "voice\t" << plan.options.voiceName() << "\n"
         << "program\t" << plan.options.program << "\n"
         << "gain\t" << juce::String (plan.options.gain, 6) << "\n"
         << "repetition length (ms)\t" << juce::String (repetitionLength, 1) << "\n"
         << "figure\tnote 1: 67.4, velocity 15, 1700 ms, ramp to 60 over 1500 ms; note 2 at 1500 ms: 67.68, velocity 36, 1900 ms, "
            "ramps to 70 over 700 ms, to 100 over 600 ms, to 127 over 400 ms\n"
         << "\nWays. Columns, separated by a tab: number, by pitch or number, first dynamic after the note-on (ms), "
            "next ramp's command before the end of the ramp before (ms), dynamic correction, name, "
            "velocity of a single note (0: the figure), silence added before (ms)\n";

    for (size_t i = 0; i < plan.ways.size(); ++i)
    {
        const auto& w = plan.ways[i];
        text << (int) i + 1 << "\t" << (w.byNumber ? "number" : "pitch") << "\t" << juce::String (w.firstDelay, 1) << "\t"
             << juce::String (w.lead, 1) << "\t" << (w.correction ? "on" : "off") << "\t" << w.name << "\t"
             << w.singleVelocity << "\t" << juce::String (w.restBefore, 1) << "\n";
    }

    text << "\nRepetitions. Columns, separated by a tab: number, way, begins at (ms from the first command)\n";
    for (size_t i = 0; i < plan.repetitions.size(); ++i)
        text << (int) i + 1 << "\t" << plan.repetitions[i].way + 1 << "\t" << juce::String (plan.repetitions[i].start, 1) << "\n";

    return text;
}

} // namespace maket::study
