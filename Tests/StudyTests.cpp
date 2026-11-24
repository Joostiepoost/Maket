// StudyTests.cpp — stage 1f.2: the plan of the ePlayer study.

#include <JuceHeader.h>
#include <cmath>
#include <set>

#include "TestReport.h"
#include "../Source/Engine/ConTimbre.h"
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

    // --- The fourth run (stage 1f.5) ---
    line ("### The fourth run of the study (stage 1f.5)");
    line();
    line ("After the recording of doodsbed by number (24 Nov): every ramp by number went to about the same loud level whatever its "
          "target, and only a ramp to 0 came down. The suspect is the form of the target, sent as a real number. One note with one "
          "ramp, in each form; and the repeated notes once more, plucked.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");
    {
        const auto w4 = maket::study::ways (o, 4);
        int number = 1;
        const auto pitchReal   = maket::study::figure (o, w4[2], 0.0, number);       // struck 40, to 10, by pitch
        number = 1;
        const auto numberWhole = maket::study::figure (o, w4[3], 0.0, number);       // the same by number, target whole
        const auto numberReal  = maket::study::figure (o, w4[20], 0.0, number);      // by number, target real
        const auto pitchWhole  = maket::study::figure (o, w4[25], 0.0, number);
        number = 3;
        const auto noRamp      = maket::study::figure (o, w4[27], 0.0, number);
        checkRow (w4.size() == 37
                  && pitchReal.size() == 3 && at (pitchReal[0], 0.0, "noteon voice3 67.4 40") && at (pitchReal[1], 200.0, "dynamic voice3 67.4 10. 800.")
                  && at (pitchReal[2], 1900.0, "noteoff voice3 67.4")
                  && numberWhole.size() == 3 && at (numberWhole[0], 0.0, "ctnote 3 1 67.4 40 1900.") && at (numberWhole[1], 200.0, "ctdynamic 1 10 800.")
                  && at (numberWhole[2], 1900.0, "ctnoteoff 1")
                  && numberReal.size() == 3 && at (numberReal[1], 200.0, "ctdynamic 2 10. 800.")
                  && pitchWhole.size() == 3 && at (pitchWhole[1], 200.0, "dynamic voice3 67.4 10 800.")
                  && noRamp.size() == 2 && at (noRamp[0], 0.0, "ctnote 3 3 67.4 40 1900.") && at (noRamp[1], 1900.0, "ctnoteoff 3"),
                  "One note with one ramp, 0.2 s after the note-on, over 0.8 s: by pitch, by number with the target as a whole number "
                  "(`ctdynamic 1 10 800.`) and as a real one (`ctdynamic 2 10. 800.`), by pitch with a whole target, and the note without a ramp");

        number = 1;
        const auto plucked = maket::study::figure (o, w4[30], 0.0, number);          // by number, every 120 ms, overlapping
        checkRow (w4[30].program == "pizz" && w4[30].byNumber && plucked.size() == 16
                  && at (plucked[0], 0.0, "ctnote 3 1 67.4 90 150.") && at (plucked[1], 120.0, "ctnote 3 2 67.4 90 150.") && at (plucked[2], 150.0, "ctnoteoff 1"),
                  "The repeated notes on the plucked program `pizz`, by pitch and by number, every 120 or 70 ms, overlapping or apart");

        const auto plan = maket::study::makePlan (o, -1, 4);
        std::vector<int> perWay (w4.size(), 0);
        bool inOrder = true, programs = true, counts = true;
        for (size_t i = 1; i < plan.messages.size(); ++i) inOrder = inOrder && plan.messages[i].time >= plan.messages[i - 1].time;
        for (const auto& r : plan.repetitions) ++perWay[(size_t) r.way];
        for (int w = 0; w < 29; ++w) counts = counts && perWay[(size_t) w] == 4;
        for (int w = 29; w < 37; ++w) counts = counts && perWay[(size_t) w] == 6;
        int wholeRamps = 0, realRamps = 0;
        for (const auto& m : plan.messages)
        {
            if (m.message.address == "program")
            {
                const auto t = m.message.toText();
                programs = programs && (t == "program voice3 \"sul tasto\"" || t == "program voice3 pizz");
            }
            if (m.message.address == "ctdynamic")
                ++(m.message.arguments[1].kind == maket::OscArgument::Kind::integer ? wholeRamps : realRamps);
        }
        line ("| (The fourth run: " + juce::String ((int) plan.repetitions.size()) + " items, " + juce::String ((int) plan.messages.size())
              + " commands, " + juce::String (plan.length / 60000.0, 1) + " minutes.) | |");
        checkRow (counts && inOrder && programs && plan.repetitions.size() == 164 && wholeRamps == 40 && realRamps == 20,
                  "The fourth run: 4 of each of the 29 ramp ways and 6 of each of the 8 plucked ways, in turns; 40 ramps by number with a "
                  "whole target, 20 with a real one");
    }
    line();

    // --- The fifth run (stage 1f.6) ---
    line ("### The fifth run of the study (stage 1f.6)");
    line();
    line ("Joost found by ear (24 Nov) that `ctdynamic` moves a note from where it is: up for a number above 0, down for one below, "
          "unlike the manual. The fifth run measures it: single steps, sequences of steps, and the ramps by pitch to the same levels.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");
    {
        const auto w5 = maket::study::ways (o, 5);
        int number = 1;
        const auto down = maket::study::figure (o, w5[1], 0.0, number);          // struck 60, one step of -40
        const auto real = maket::study::figure (o, w5[16], 0.0, number);         // -20 as a real number
        const auto slow = maket::study::figure (o, w5[18], 0.0, number);         // +40 over 1.5 s
        const auto back = maket::study::figure (o, w5[21], 0.0, number);         // -30, -30, +60
        const auto pitch = maket::study::figure (o, w5[25], 0.0, number);        // by pitch to 0
        checkRow (w5.size() == 44
                  && down.size() == 3 && at (down[0], 0.0, "ctnote 3 1 67.4 60 1900.") && at (down[1], 200.0, "ctdynamic 1 -40 800.") && at (down[2], 1900.0, "ctnoteoff 1")
                  && real.size() == 3 && at (real[1], 200.0, "ctdynamic 2 -20. 800.")
                  && slow.size() == 3 && at (slow[1], 200.0, "ctdynamic 3 40 1500.")
                  && back.size() == 5 && at (back[0], 0.0, "ctnote 3 4 67.4 60 3300.") && at (back[1], 200.0, "ctdynamic 4 -30 600.")
                  && at (back[2], 1100.0, "ctdynamic 4 -30 600.") && at (back[3], 2000.0, "ctdynamic 4 60 600.") && at (back[4], 3300.0, "ctnoteoff 4")
                  && pitch.size() == 3 && at (pitch[0], 0.0, "noteon voice3 67.4 60") && at (pitch[1], 200.0, "dynamic voice3 67.4 0. 800."),
                  "A note struck at 60 with one step by number (`ctdynamic 1 -40 800.`), the same as a real number, over 1.5 s, "
                  "three steps one after the other (-30, -30, +60, each over 0.6 s, 0.9 s apart), and the ramp by pitch beside them");

        const auto plan = maket::study::makePlan (o, -1, 5);
        std::vector<int> perWay (w5.size(), 0);
        bool inOrder = true, counts = true;
        for (size_t i = 1; i < plan.messages.size(); ++i) inOrder = inOrder && plan.messages[i].time >= plan.messages[i - 1].time;
        for (const auto& r : plan.repetitions) ++perWay[(size_t) r.way];
        for (int c : perWay) counts = counts && c == 4;
        int negative = 0, steps = 0;
        for (const auto& m : plan.messages)
            if (m.message.address == "ctdynamic")
            {
                ++steps;
                const auto& a = m.message.arguments[1];
                if ((a.kind == maket::OscArgument::Kind::integer ? (double) a.integer : a.real) < 0.0) ++negative;
            }
        line ("| (The fifth run: " + juce::String ((int) plan.repetitions.size()) + " items, " + juce::String ((int) plan.messages.size())
              + " commands, " + juce::String (plan.length / 60000.0, 1) + " minutes; " + juce::String (steps) + " steps by number, "
              + juce::String (negative) + " of them down.) | |");
        checkRow (counts && inOrder && plan.repetitions.size() == 176 && steps == 4 * (19 + 18) && negative == 4 * (4 + 3 + 1 + 6),
                  "The fifth run: 4 of each of its 44 ways, in turns: 19 single steps, 6 sequences of three, 8 ramps by pitch, "
                  "3 struck notes without a ramp, 8 plucked ways");
    }
    line();

    // --- The sixth run (stage 1f.7) ---
    line ("### The sixth run of the study (stage 1f.7)");
    line();
    line ("Joost found (24 Nov) that `ctdynamic` goes to an absolute target after all, on a scale from 0. to 1. The sixth run holds a ramp "
          "by number to a target on that scale beside the ramp by pitch to 127 times it, on the cello and on the bass clarinet.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");
    {
        const auto w6 = maket::study::ways (o, 6);
        int number = 1;
        const auto byNumber = maket::study::figure (o, w6[6], 0.0, number);       // cello, struck 40, to 0.5
        number = 1;
        const auto byPitch = maket::study::figure (o, w6[7], 0.0, number);        // the same by pitch: to 63.5
        number = 1;
        const auto sequence = maket::study::figure (o, w6[20], 0.0, number);      // cello: 0.25, 0.75, 0.25
        number = 1;
        const auto clarinet = maket::study::figure (o, w6[22 + 10], 0.0, number); // bass clarinet, struck 40, to 1
        checkRow (w6.size() == 52
                  && byNumber.size() == 3 && at (byNumber[0], 0.0, "ctnote 3 1 67.4 40 1900.") && at (byNumber[1], 200.0, "ctdynamic 1 0.5 800.")
                  && at (byNumber[2], 1900.0, "ctnoteoff 1")
                  && byPitch.size() == 3 && at (byPitch[0], 0.0, "noteon voice3 67.4 40") && at (byPitch[1], 200.0, "dynamic voice3 67.4 63.5 800.")
                  && sequence.size() == 5 && at (sequence[1], 200.0, "ctdynamic 1 0.25 600.") && at (sequence[2], 1100.0, "ctdynamic 1 0.75 600.")
                  && at (sequence[3], 2000.0, "ctdynamic 1 0.25 600.")
                  && clarinet.size() == 3 && at (clarinet[0], 0.0, "ctnote 5 1 50. 40 1900.") && at (clarinet[1], 200.0, "ctdynamic 1 1. 800."),
                  "A note with one ramp by number to a target from 0 to 1 (`ctdynamic 1 0.5 800.`) and the same by pitch to 127 times it "
                  "(`dynamic voice3 67.4 63.5 800.`); a sequence of three targets; the same on the bass clarinet (voice5, pitch 50)");

        const auto plan = maket::study::makePlan (o, -1, 6);
        std::vector<int> perWay (w6.size(), 0);
        bool inOrder = true, counts = true, inRange = true, programs = true;
        for (size_t i = 1; i < plan.messages.size(); ++i) inOrder = inOrder && plan.messages[i].time >= plan.messages[i - 1].time;
        for (const auto& r : plan.repetitions) ++perWay[(size_t) r.way];
        for (int w = 0; w < 44; ++w) counts = counts && perWay[(size_t) w] == 3;
        for (int w = 44; w < 52; ++w) counts = counts && perWay[(size_t) w] == 4;
        for (const auto& m : plan.messages)
        {
            if (m.message.address == "ctdynamic")
                inRange = inRange && m.message.arguments[1].kind == maket::OscArgument::Kind::real
                          && m.message.arguments[1].real >= 0.0 && m.message.arguments[1].real <= 1.0;
            if (m.message.address == "program")
            {
                const auto t = m.message.toText();
                programs = programs && (t == "program voice3 \"sul tasto\"" || t == "program voice3 pizz" || t == "program voice5 ordinario");
            }
        }
        line ("| (The sixth run: " + juce::String ((int) plan.repetitions.size()) + " items, " + juce::String ((int) plan.messages.size())
              + " commands, " + juce::String (plan.length / 60000.0, 1) + " minutes.) | |");
        checkRow (counts && inOrder && inRange && programs && plan.repetitions.size() == 164,
                  "The sixth run: 3 of each of the 44 ramp ways (22 on the cello, 22 on the bass clarinet) and 4 of each of the 8 plucked ways, "
                  "in turns; every target by number lies from 0 to 1");
    }
    line();

    // --- The tuning check (stage 1f.8) ---
    line ("### The tuning check (stage 1f.8)");
    line();
    line ("`kammerton <Hz>` sets the ePlayer's standard pitch. The check plays the A above middle C after `kammerton` with several "
          "frequencies, as a real number and as a whole one, and once sends it while the note sounds.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");
    {
        auto withPitch = o;
        withPitch.standardPitch = 441.0;
        const auto plan = maket::study::makePlan (withPitch, -1, 7);
        juce::StringArray tunings;
        bool before = true;
        double lastNote = -1.0e9;
        for (size_t i = 0; i < plan.messages.size(); ++i)
        {
            const auto& m = plan.messages[i];
            if (m.message.address == "kammerton") tunings.add (m.message.toText());
            if (m.message.address == "ctnote")
            {
                lastNote = m.time;
                // the command before this note: a kammerton 145 ms earlier
                bool found = false;
                for (size_t k = 0; k < i; ++k)
                    found = found || (plan.messages[k].message.address == "kammerton" && std::abs (plan.messages[k].time - (m.time - 145.0)) < 1.0e-6);
                before = before && found && m.message.toText().startsWith ("ctnote 3 ") && m.message.toText().contains (" 69. 80 ");
            }
        }
        juce::ignoreUnused (lastNote);
        const auto last = plan.messages.back().message.toText();
        line ("| (The tuning check: " + juce::String ((int) plan.repetitions.size()) + " notes, " + juce::String (plan.length / 1000.0, 0)
              + " seconds. First commands: " + tunings[0] + "; " + tunings[1] + "; " + tunings[2] + ".) | |");
        checkRow (plan.ways.size() == 8 && plan.repetitions.size() == 24 && before && tunings.size() == 24 + 3 + 1
                  && tunings.contains ("kammerton 442.") && tunings.contains ("kammerton 440.") && tunings.contains ("kammerton 440")
                  && tunings.contains ("kammerton 430") && tunings.contains ("kammerton 415.3") && tunings.contains ("kammerton 466.16")
                  && last == "kammerton 441.",
                  "24 notes of the A on the cello, each 145 ms after its `kammerton` (442, 440, 430, 415.3, 466.16; 440 and 430 also as whole "
                  "numbers); three notes get a second `kammerton` while they sound; the run ends with the standard pitch of the settings");
        checkRow (maket::contimbre::kammerton (442.0).toText() == "kammerton 442." && maket::contimbre::kammerton (440.5).toText() == "kammerton 440.5"
                  && maket::contimbre::kammerton (440.0, true).toText() == "kammerton 440",
                  "`kammerton` is sent as a real number (`kammerton 442.`, `kammerton 440.5`)");
    }
    line();
}
