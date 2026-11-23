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
}
