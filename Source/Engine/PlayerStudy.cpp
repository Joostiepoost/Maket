// PlayerStudy.cpp

#include "PlayerStudy.h"
#include "ConTimbre.h"

#include <algorithm>
#include <cmath>

namespace maket::study
{

double Way::soundLength() const
{
    if (repeatCount > 0)    return (repeatCount - 1) * period + juce::jmax (20.0, period + overlap);
    if (std::abs (glide) > 0.0)   return 2000.0;
    if (singleVelocity > 0 || chord) return singleNoteLength;
    return figureLength;
}

double Way::itemLength() const
{
    if (repeatCount > 0 || singleVelocity > 0 || chord) return singleRepetitionLength;
    if (std::abs (glide) > 0.0) return 2800.0;
    return repetitionLength;
}

std::vector<Way> ways (const Options& o, int run)
{
    std::vector<Way> all;

    if (run == 3)
    {
        const int cello = 3, flute = 4;

        for (double period : { 120.0, 70.0 })
            for (double overlap : { 30.0, 0.0, -20.0 })
                for (bool byNumber : { false, true })
                {
                    Way w;
                    w.name = juce::String ("repeated note ") + (byNumber ? "by number" : "by pitch") + ", one every " + juce::String ((int) period)
                             + " ms, " + (overlap > 0.0 ? "overlapping by 30 ms" : overlap < 0.0 ? "20 ms apart" : "meeting exactly");
                    w.byNumber = byNumber;
                    w.voiceNumber = cello;  w.program = "ordinario";  w.gain = o.gain;
                    w.repeatCount = 8;  w.period = period;  w.overlap = overlap;
                    w.share = 0.4;
                    all.push_back (w);
                }

        for (bool byNumber : { false, true })
        {
            Way w;
            w.name = juce::String ("glide of two semitones ") + (byNumber ? "by number" : "by pitch");
            w.byNumber = byNumber;
            w.voiceNumber = cello;  w.program = "ordinario";  w.gain = o.gain;
            w.glide = 2.0;
            w.share = 0.4;
            all.push_back (w);
        }

        {
            Way w;
            w.name = "one note by number, with the number 2000000";
            w.byNumber = true;
            w.voiceNumber = cello;  w.program = "ordinario";  w.gain = o.gain;
            w.singleVelocity = 80;  w.fixedNumber = 2000000;
            w.share = 0.2;
            all.push_back (w);
        }

        for (bool chord : { true, false })
        {
            Way w;
            w.name = chord ? "flute, program harmonic (chord mode): chordon 60 79" : "flute, program harmonic (chord mode): plain note-on 79";
            w.voiceNumber = flute;  w.program = "harmonic";  w.gain = 0.0;
            w.chord = chord;
            if (! chord) { w.singleVelocity = 80; w.singlePitch = 79.0; }
            w.share = 0.2;
            all.push_back (w);
        }
        return all;
    }

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
        Way w;
        w.name = name;
        w.byNumber = byNumber;
        w.firstDelay = firstDelay;
        w.lead = lead;
        w.correction = correction;
        all.push_back (w);
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
    const int voiceNumber = way.voiceNumber > 0 ? way.voiceNumber : o.voiceNumber;
    const auto voice = "voice" + juce::String (voiceNumber);
    auto addPlain = [&] (double time, OscMessage message) { out.push_back ({ time, std::move (message), 0, start, time }); };

    // A program in chord mode: two pitches as one command, ended by a note-off of the lower one.
    if (way.chord)
    {
        addPlain (start, contimbre::chordOn (voice, { 60.0, 79.0 }));
        addPlain (start + singleNoteLength, contimbre::noteOff (voice, 60.0));
        return out;
    }

    // One note that glides.
    if (std::abs (way.glide) > 0.0)
    {
        const double pitch = 67.4;
        const int number = nextNumber++;
        addPlain (start, way.byNumber ? contimbre::ctNote (voiceNumber, number, pitch, 80, 2000.0) : contimbre::noteOn (voice, pitch, 80));
        addPlain (start, way.byNumber ? contimbre::ctGlissando (number, way.glide, 1500.0) : contimbre::glissando (voice, pitch, way.glide, 1500.0));
        addPlain (start + 2000.0, way.byNumber ? contimbre::ctNoteOff (number) : contimbre::noteOff (voice, pitch));
        return out;
    }

    // One pitch struck several times. A note-off that falls at the moment of the next note-on is sent before it.
    if (way.repeatCount > 0)
    {
        const double pitch = 67.4, length = juce::jmax (20.0, way.period + way.overlap);
        for (int k = 0; k < way.repeatCount; ++k)
        {
            const double onset = start + k * way.period;
            const int number = nextNumber++;
            addPlain (onset + length, way.byNumber ? contimbre::ctNoteOff (number) : contimbre::noteOff (voice, pitch));
            addPlain (onset, way.byNumber ? contimbre::ctNote (voiceNumber, number, pitch, 90, length) : contimbre::noteOn (voice, pitch, 90));
        }
        std::stable_sort (out.begin(), out.end(), [] (const auto& a, const auto& b) { return a.time < b.time; });
        return out;
    }

    struct Ramp { double target, length; };
    auto note = [&] (double onset, double pitch, int velocity, double length, const std::vector<Ramp>& ramps)
    {
        auto add = [&] (double time, OscMessage message) { out.push_back ({ time, std::move (message), 0, onset, time }); };
        const int number = way.fixedNumber > 0 ? way.fixedNumber : nextNumber++;

        add (onset, way.byNumber ? contimbre::ctNote (voiceNumber, number, pitch, velocity, length)
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
        note (start, way.singlePitch, way.singleVelocity, singleNoteLength, {});
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
    if (run != 3)
    {
        add (0.0, contimbre::program (o.voiceName(), o.program));
        add (0.0, contimbre::gain (o.voiceName(), o.gain));
    }

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
        if (way.program.isNotEmpty())
        {
            const auto voice = "voice" + juce::String (way.voiceNumber > 0 ? way.voiceNumber : o.voiceNumber);
            add (start - 140.0, contimbre::program (voice, way.program));
            add (start - 140.0, contimbre::gain (voice, way.gain));
        }

        const auto notes = figure (o, way, start, nextNumber);
        plan.messages.insert (plan.messages.end(), notes.begin(), notes.end());
        plan.repetitions.push_back ({ w, start });

        end = start + way.soundLength();
        start += way.itemLength();
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
    text << "Maket, study of the ePlayer, run " << plan.run << (plan.run == 3 ? " (stage 1f.4)\n" : plan.run == 2 ? " (stage 1f.3)\n" : " (stage 1f.2)\n")
         << "voice\t" << plan.options.voiceName() << "\n"
         << "program\t" << plan.options.program << "\n"
         << "gain\t" << juce::String (plan.options.gain, 6) << "\n"
         << "repetition length (ms)\t" << juce::String (repetitionLength, 1) << "\n"
         << "figure\tnote 1: 67.4, velocity 15, 1700 ms, ramp to 60 over 1500 ms; note 2 at 1500 ms: 67.68, velocity 36, 1900 ms, "
            "ramps to 70 over 700 ms, to 100 over 600 ms, to 127 over 400 ms\n"
         << "\nWays. Columns, separated by a tab: number, by pitch or number, first dynamic after the note-on (ms), "
            "next ramp's command before the end of the ramp before (ms), dynamic correction, name, "
            "velocity of a single note (0: the figure), silence added before (ms), voice, repeated notes, their period (ms), "
            "their overlap (ms), glide (semitones), chord\n";

    for (size_t i = 0; i < plan.ways.size(); ++i)
    {
        const auto& w = plan.ways[i];
        text << (int) i + 1 << "\t" << (w.byNumber ? "number" : "pitch") << "\t" << juce::String (w.firstDelay, 1) << "\t"
             << juce::String (w.lead, 1) << "\t" << (w.correction ? "on" : "off") << "\t" << w.name << "\t"
             << w.singleVelocity << "\t" << juce::String (w.restBefore, 1) << "\t"
             << (w.voiceNumber > 0 ? w.voiceNumber : plan.options.voiceNumber) << "\t" << w.repeatCount << "\t" << juce::String (w.period, 1) << "\t"
             << juce::String (w.overlap, 1) << "\t" << juce::String (w.glide, 1) << "\t" << (w.chord ? 1 : 0) << "\n";
    }

    text << "\nRepetitions. Columns, separated by a tab: number, way, begins at (ms from the first command)\n";
    for (size_t i = 0; i < plan.repetitions.size(); ++i)
        text << (int) i + 1 << "\t" << plan.repetitions[i].way + 1 << "\t" << juce::String (plan.repetitions[i].start, 1) << "\n";

    return text;
}

} // namespace maket::study
