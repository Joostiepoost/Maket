// StudyTests.cpp — stage 1f.2: the plan of the ePlayer study.

#include <JuceHeader.h>
#include <cmath>
#include <set>

#include "TestReport.h"
#include "../Source/Engine/PlayerStudy.h"

using namespace testreport;

namespace
{
    bool at (const maket::contimbre::TimedMessage& m, double time, const char* text)
    {
        return std::abs (m.time - time) < 1.0e-6 && m.message.toText() == juce::String::fromUTF8 (text);
    }
}

void testPlayerStudy()
{
    line ("### The study of the ePlayer (stage 1f.2)");
    line();
    line ("One figure of two notes, played in several ways in turns. What the ePlayer does with it is heard and measured on "
          "Joost's Mac; checked here is that the plan holds what it should.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    maket::study::Options o;
    const auto ways = maket::study::ways (o);

    // One figure, by pitch, timed as ScAst: first dynamic 0.7 ms after the note-on, the next ramps 1.4 ms before the end.
    {
        int number = 1;
        const auto f = maket::study::figure (o, ways[1], 1000.0, number);
        checkRow (f.size() == 8
                  && at (f[0], 1000.0,  "noteon voice1 67.4 15")
                  && at (f[1], 1000.7,  "dynamic voice1 67.4 60. 1500.")
                  && at (f[2], 2500.0,  "noteon voice1 67.68 36")
                  && at (f[3], 2500.7,  "dynamic voice1 67.68 70. 700.")
                  && at (f[4], 2700.0,  "noteoff voice1 67.4")
                  && at (f[5], 3199.3,  "dynamic voice1 67.68 100. 600.")
                  && at (f[6], 3797.9,  "dynamic voice1 67.68 127. 400.")
                  && at (f[7], 4400.0,  "noteoff voice1 67.68"),
                  "The figure by pitch, timed as ScAst: two notes that overlap by 0.2 s, the first dynamic 0.7 ms after each note-on, "
                  "each next ramp 1.4 ms before the end of the one before");
    }

    // The same figure by number.
    {
        int number = 7;
        const auto f = maket::study::figure (o, ways[6], 0.0, number);
        checkRow (ways[6].byNumber && f.size() == 8 && number == 9
                  && at (f[0], 0.0,     "ctnote 1 7 67.4 15 1700.")
                  && at (f[1], 0.0,     "ctdynamic 7 60. 1500.")
                  && at (f[2], 1500.0,  "ctnote 1 8 67.68 36 1900.")
                  && at (f[3], 1500.0,  "ctdynamic 8 70. 700.")
                  && at (f[4], 1700.0,  "ctnoteoff 7")
                  && at (f[5], 2199.7,  "ctdynamic 8 100. 600.")
                  && at (f[6], 2799.4,  "ctdynamic 8 127. 400.")
                  && at (f[7], 3400.0,  "ctnoteoff 8"),
                  "The figure by number: `ctnote`, `ctdynamic` and `ctnoteoff` with a number of its own for each note");
    }

    // The whole plan.
    {
        const auto plan = maket::study::makePlan (o);
        const int n = (int) plan.ways.size();

        bool inOrder = true, apart = true, eachWayEqual = true;
        for (size_t i = 1; i < plan.messages.size(); ++i) inOrder = inOrder && plan.messages[i].time >= plan.messages[i - 1].time;
        for (size_t i = 1; i < plan.repetitions.size(); ++i)
            apart = apart && plan.repetitions[i].start - plan.repetitions[i - 1].start >= maket::study::figureLength + 500.0;

        std::vector<int> perWay ((size_t) n, 0);
        for (const auto& r : plan.repetitions) ++perWay[(size_t) r.way];
        for (int c : perWay) eachWayEqual = eachWayEqual && c == o.repetitions;

        // No way always comes after the same one.
        std::set<int> beforeFirstWay;
        for (size_t i = 1; i < plan.repetitions.size(); ++i)
            if (plan.repetitions[i].way == 0) beforeFirstWay.insert (plan.repetitions[i - 1].way);

        std::set<int> numbers;
        int notesByNumber = 0, corrections = 0;
        for (const auto& m : plan.messages)
        {
            if (m.message.address == "ctnote") { ++notesByNumber; numbers.insert ((int) m.message.arguments[1].integer); }
            if (m.message.address == "dynamic_correction") ++corrections;
        }

        const auto& last = plan.messages.back();
        line ("| (The run: " + juce::String (n) + " ways, " + juce::String (o.repetitions) + " figures of each, "
              + juce::String ((int) plan.messages.size()) + " commands, " + juce::String (plan.length / 60000.0, 1) + " minutes.) | |");
        checkRow (n == 10 && (int) plan.repetitions.size() == n * o.repetitions && eachWayEqual && inOrder && apart
                  && (int) plan.messages.size() == 2 + n * o.repetitions * 9 + 1
                  && plan.messages[0].message.toText() == "program voice1 \"sul tasto\""
                  && plan.messages[1].message.toText() == "gain voice1 5.777778",
                  "Every way is played as often as every other, the figures follow each other at 4 seconds, the commands are in order of time; "
                  "program and gain of the voice come first");
        checkRow (beforeFirstWay.size() > 1, "The ways are played in turns, and a way does not always follow the same one");
        checkRow (notesByNumber == 3 * o.repetitions * 2 && (int) numbers.size() == notesByNumber,
                  "Every note addressed by number has a number no other note has");
        checkRow (corrections == n * o.repetitions + 1 && last.message.toText() == "dynamic_correction 1"
                  && std::abs (last.time - plan.length) < 1.0e-6,
                  "The dynamic correction is set before every figure, and the last command of the run switches it back on");

        // Without the ways by number and without the correction off.
        maket::study::Options plain;
        plain.withNumber = false;
        plain.withCorrectionOff = false;
        const auto small = maket::study::makePlan (plain);
        bool none = true;
        for (const auto& m : small.messages) none = none && ! m.message.address.startsWith ("ct") && m.message.toText() != "dynamic_correction 0";
        checkRow (small.ways.size() == 6 && none, "With both switched off, the run holds the six ways by pitch only, and never switches the correction off");

        // One figure to try the sound.
        const auto one = maket::study::makePlan (o, 6);
        checkRow (one.repetitions.size() == 1 && one.repetitions[0].way == 6 && one.messages.size() == 2 + 9 + 1
                  && one.messages.back().message.toText() == "dynamic_correction 1",
                  "A single figure of one way can be played by itself, and leaves the correction on");

        const auto text = maket::study::planAsText (plan);
        checkRow (text.contains ("7\tnumber\t0.0\t0.3\ton\tby number, as Maket sends now") && text.contains ("\n200\t"),
                  "The plan as a text file lists the ways and when each of the 200 figures begins");
    }
    line();

    // --- The second run (stage 1f.3) ---
    line ("### The second run of the study (stage 1f.3)");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");
    {
        const auto w2 = maket::study::ways (o, 2);
        int number = 3;
        const auto byPitch = maket::study::figure (o, w2[6], 100.0, number);     // one note by pitch, velocity 100
        const auto byNumber = maket::study::figure (o, w2[7], 100.0, number);    // the same by number
        checkRow (w2.size() == 16 && byPitch.size() == 2 && byNumber.size() == 2
                  && at (byPitch[0], 100.0, "noteon voice1 67.4 100") && at (byPitch[1], 1600.0, "noteoff voice1 67.4")
                  && at (byNumber[0], 100.0, "ctnote 1 4 67.4 100 1500.") && at (byNumber[1], 1600.0, "ctnoteoff 4"),
                  "A single note without any dynamic, 1.5 s long, the same by pitch and by number");

        number = 1;
        const auto firstOnly = maket::study::figure (o, w2[13], 0.0, number);    // by number, only the first dynamic as ScAst
        const auto nextOnly = maket::study::figure (o, w2[14], 0.0, number);     // by number, only the next ramps as ScAst
        checkRow (firstOnly.size() == 8 && nextOnly.size() == 8
                  && at (firstOnly[1], 0.7, "ctdynamic 1 60. 1500.") && at (firstOnly[5], 2200.4, "ctdynamic 2 100. 600.")
                  && at (nextOnly[1], 0.0, "ctdynamic 3 60. 1500.") && at (nextOnly[5], 2198.6, "ctdynamic 4 100. 600."),
                  "The two distances of ScAst's timing can be had one at a time: the first dynamic 0.7 ms after the note-on, "
                  "or each next ramp 1.4 ms before the end of the one before");

        const auto plan = maket::study::makePlan (o, -1, 2);
        std::vector<int> perWay (w2.size(), 0);
        bool inOrder = true, restKept = true, apart = true;
        for (size_t i = 1; i < plan.messages.size(); ++i) inOrder = inOrder && plan.messages[i].time >= plan.messages[i - 1].time;
        for (size_t i = 0; i < plan.repetitions.size(); ++i)
        {
            const auto& r = plan.repetitions[i];
            ++perWay[(size_t) r.way];
            if (i == 0) continue;
            const auto& before = plan.repetitions[i - 1];
            const bool beforeSingle = plan.ways[(size_t) before.way].singleVelocity > 0;
            const double silence = r.start - (before.start + (beforeSingle ? 1500.0 : 3400.0));
            apart = apart && silence >= 499.0;
            if (r.way == 15) restKept = restKept && silence >= 2899.0;
        }
        bool counts = perWay[15] == 15;
        for (int w = 0; w < 10; ++w) counts = counts && perWay[(size_t) w] == 8;
        for (int w = 10; w < 15; ++w) counts = counts && perWay[(size_t) w] == 20;

        int dynamicsOfSingles = 0;
        for (const auto& m : plan.messages)
            if ((m.message.address == "dynamic" || m.message.address == "ctdynamic") && m.message.arguments.size() > 0) ++dynamicsOfSingles;

        line ("| (The second run: " + juce::String ((int) plan.repetitions.size()) + " items, " + juce::String ((int) plan.messages.size())
              + " commands, " + juce::String (plan.length / 60000.0, 1) + " minutes.) | |");
        checkRow (counts && inOrder && apart && restKept && plan.repetitions.size() == 195
                  && dynamicsOfSingles == 115 * 4 && plan.messages.back().message.toText() == "dynamic_correction 1"
                  && maket::study::planAsText (plan).contains ("run 2 (stage 1f.3)"),
                  "The second run: 8 single notes of each of the ten kinds, 20 figures of each of five ways, 15 figures after three seconds "
                  "of silence; in turns, in order of time, at least half a second of silence between any two");
    }
    line();

    // --- The third run (stage 1f.4) ---
    line ("### The third run of the study (stage 1f.4)");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");
    {
        const auto w3 = maket::study::ways (o, 3);
        int number = 1;
        // way 1: by pitch, every 120 ms, overlapping by 30 ms; way 4: by number, every 120 ms, meeting exactly
        const auto overlapping = maket::study::figure (o, w3[0], 0.0, number);
        number = 1;
        const auto meeting = maket::study::figure (o, w3[3], 0.0, number);
        checkRow (w3.size() == 17 && overlapping.size() == 16 && meeting.size() == 16
                  && at (overlapping[0], 0.0, "noteon voice3 67.4 90") && at (overlapping[1], 120.0, "noteon voice3 67.4 90")
                  && at (overlapping[2], 150.0, "noteoff voice3 67.4")
                  && at (meeting[0], 0.0, "ctnote 3 1 67.4 90 120.") && at (meeting[1], 120.0, "ctnoteoff 1")
                  && at (meeting[2], 120.0, "ctnote 3 2 67.4 90 120.") && at (meeting[15], 960.0, "ctnoteoff 8"),
                  "One pitch struck eight times: by pitch the note-off of an overlapping note comes 30 ms after the next note-on; "
                  "where the notes meet exactly, the note-off is sent before the next note-on");

        number = 5;
        const auto glidePitch = maket::study::figure (o, w3[12], 0.0, number);
        const auto glideNumber = maket::study::figure (o, w3[13], 0.0, number);
        checkRow (glidePitch.size() == 3 && glideNumber.size() == 3
                  && at (glidePitch[1], 0.0, "glissando voice3 67.4 2. 1500.") && at (glideNumber[0], 0.0, "ctnote 3 6 67.4 80 2000.")
                  && at (glideNumber[1], 0.0, "ctglissando 6 2. 1500.") && at (glideNumber[2], 2000.0, "ctnoteoff 6"),
                  "A glide of two semitones over 1.5 s, by pitch (`glissando`) and by number (`ctglissando`)");

        const auto big = maket::study::figure (o, w3[14], 0.0, number);
        const auto chord = maket::study::figure (o, w3[15], 0.0, number);
        const auto plain = maket::study::figure (o, w3[16], 0.0, number);
        checkRow (big.size() == 2 && at (big[0], 0.0, "ctnote 3 2000000 67.4 80 1500.") && at (big[1], 1500.0, "ctnoteoff 2000000")
                  && chord.size() == 2 && at (chord[0], 0.0, "chordon voice4 60. 79.") && at (chord[1], 1500.0, "noteoff voice4 60.")
                  && plain.size() == 2 && at (plain[0], 0.0, "noteon voice4 79. 80"),
                  "A note with the number 2000000; the flute's chord-mode program by `chordon` with two pitches, ended by a note-off of the lower");

        const auto plan = maket::study::makePlan (o, -1, 3);
        std::vector<int> perWay (w3.size(), 0);
        bool inOrder = true, programs = true;
        for (size_t i = 1; i < plan.messages.size(); ++i) inOrder = inOrder && plan.messages[i].time >= plan.messages[i - 1].time;
        for (const auto& r : plan.repetitions) ++perWay[(size_t) r.way];
        bool counts = true;
        for (int w = 0; w < 14; ++w) counts = counts && perWay[(size_t) w] == 8;
        for (int w = 14; w < 17; ++w) counts = counts && perWay[(size_t) w] == 4;
        int programCommands = 0;
        for (const auto& m : plan.messages)
            if (m.message.address == "program")
            {
                ++programCommands;
                const auto t = m.message.toText();
                programs = programs && (t == "program voice3 ordinario" || t == "program voice4 harmonic");
            }
        line ("| (The third run: " + juce::String ((int) plan.repetitions.size()) + " items, " + juce::String ((int) plan.messages.size())
              + " commands, " + juce::String (plan.length / 60000.0, 1) + " minutes.) | |");
        checkRow (counts && inOrder && programs && programCommands == (int) plan.repetitions.size() && plan.repetitions.size() == 124,
                  "The third run: 8 of each repeated-note and glide way, 4 of the others, each preceded by the program and gain of its voice "
                  "(the cello on voice3, the flute on voice4)");
    }
    line();
}
