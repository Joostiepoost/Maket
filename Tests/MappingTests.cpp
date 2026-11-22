// MappingTests.cpp — stage 1e: the notes of a roll as commands for the
// conTimbre ePlayer, and the comparison with a log from Max.
//
// Whether the commands are the ones Joost's Max patch sends is settled on
// his Mac, by holding them against a log of the patch playing the same
// roll. What is checked here: that the mapping does what the patch
// `cpe_poly` was read to do, on notes of doodsbed worked out by hand, and
// that the comparison finds what differs.

#include <JuceHeader.h>
#include <cmath>
#include <vector>

#include "TestReport.h"
#include "../Source/Model/Roll.h"
#include "../Source/Formats/BachRoll.h"
#include "../Source/Formats/LlllText.h"
#include "../Source/Engine/Osc.h"
#include "../Source/Engine/ConTimbreMapping.h"

using namespace testreport;
using maket::Roll;
using maket::contimbre::TimedMessage;

namespace
{

bool at (const TimedMessage& m, double time, const char* text)
{
    return std::abs (m.time - time) < 1.0e-6 && m.message.toText() == juce::String::fromUTF8 (text);
}

int count (const std::vector<TimedMessage>& messages, const char* command)
{
    int n = 0;
    for (const auto& m : messages) if (m.message.address == command) ++n;
    return n;
}

std::vector<TimedMessage> ofVoice (const std::vector<TimedMessage>& all, int voice)
{
    std::vector<TimedMessage> out;
    for (const auto& m : all) if (m.voice == voice) out.push_back (m);
    return out;
}

void testMapping (const juce::File& temp)
{
    line ("### The mapping, on doodsbed");
    line();

    Roll roll;
    juce::StringArray remarks;
    const auto file = temp.getChildFile ("doodsbed final v1c.txt");
    if (maket::readBachRollFile (file, roll, remarks).failed())
    {
        check (false, "doodsbed final v1c is in Temp and can be read");
        line();
        return;
    }

    const auto all = maket::contimbre::messagesForRoll (roll);
    const auto vlc = ofVoice (all, 2);

    line ("The first note of the cello (`Vlc`) in the file: 6690 cents, 4511 ms long, velocity 1, at 7200.802301 ms; "
          "slot 1 `sul tasto`; slot 2 two points (0 ms: 0, 4267 ms: 16.846475); slot 3 one point (0 ms: 5.777778 dB); "
          "two pitch breakpoints (start: 0 cents, end: +50 cents). The commands Maket makes of it, in the order of the Max patch:");
    line();
    line ("```");
    for (size_t i = 0; i < 5 && i < vlc.size(); ++i)
        line (juce::String (vlc[i].time, 3).paddedLeft (' ', 10) + " ms   " + vlc[i].message.toText());
    for (const auto& m : vlc)
        if (m.message.address == "noteoff") { line (juce::String (m.time, 3).paddedLeft (' ', 10) + " ms   " + m.message.toText()); break; }
    line ("```");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    const double onset = 7200.802301;
    bool noteOff = false;
    for (const auto& m : vlc) noteOff = noteOff || at (m, onset + 4511.0, "noteoff Vlc 66.9");

    checkRow (vlc.size() > 6
              && at (vlc[0], onset, "program Vlc \"sul tasto\"")
              && at (vlc[1], onset, "gain Vlc 5.777778")
              && at (vlc[2], onset, "noteon Vlc 66.9 1")
              && at (vlc[3], onset, "dynamic Vlc 66.9 16.846475 4267.")
              && at (vlc[4], onset, "glissando Vlc 66.9 0.5 4511.")
              && noteOff,
              "That note gives program, gain, note-on, dynamic and glissando at its onset, in that order, and its note-off at its end");

    // The fourth note: slot 2 begins at 34.106086 ms, so the first dynamic leaves 34 ms after the onset
    // (the moment is rounded) and lasts 761.55103 - 34.106086 ms (the duration is not).
    {
        const double on = 19378.120319;
        bool first = false, second = false, last = false;
        for (const auto& m : vlc)
        {
            first  = first  || at (m, on + 34.0,   "dynamic Vlc 67.86 28.222222 727.444944");
            second = second || at (m, on + 762.0,  "dynamic Vlc 67.86 11.288889 1091.167417");
            last   = last   || at (m, on + 3005.0, "dynamic Vlc 67.86 0. 1585.493725");
        }
        checkRow (first && second && last,
                  "A dynamic curve of five points gives four commands, each to the value of the next point; the moment it is sent is "
                  "rounded to a whole millisecond, its duration is not");
    }

    // A note with three breakpoints: 0, 0 at 0.834274 of its length, +25 cents at its end.
    {
        const double on = 119724.941656, length = 12463.70905;
        const double corner = 0.834274 * length;
        bool first = false, second = false;
        for (const auto& m : vlc)
        {
            if (m.message.address != "glissando" || m.message.arguments.size() != 4) continue;
            const auto& a = m.message.arguments;
            first  = first  || (std::abs (m.time - on) < 1.0e-6 && std::abs (a[2].real) < 1.0e-9 && std::abs (a[3].real - corner) < 1.0e-6);
            second = second || (std::abs (m.time - (on + std::round (corner))) < 1.0e-6 && std::abs (a[2].real - 0.25) < 1.0e-9
                                && std::abs (a[3].real - (length - corner)) < 1.0e-6);
        }
        checkRow (first && second, "Three pitch breakpoints give two glissandi: to 0 semitones over the first stretch, then to 0.25 over the rest");
    }

    // Counts over the whole roll, against the roll itself.
    {
        int notes = 0, programs = 0, gains = 0, dynamics = 0, glissandi = 0;
        for (const auto& voice : roll.voices)
        {
            if (voice.name.isEmpty() || voice.isMuted()) continue;
            for (const auto& chord : voice.chords)
            {
                if (chord.isMuted()) continue;
                for (const auto& note : chord.notes)
                {
                    if (note.isMuted()) continue;
                    ++notes;
                    if (note.slot (1) != nullptr) ++programs;
                    if (const auto* s = note.slot (3)) gains += s->content.size();
                    if (const auto* s = note.slot (2)) dynamics += juce::jmax (0, s->content.size() - 1);
                    glissandi += juce::jmax (0, (int) note.breakpoints.size() - 1);
                }
            }
        }

        bool inOrder = true;
        for (size_t i = 1; i < all.size(); ++i) inOrder = inOrder && all[i].time >= all[i - 1].time;

        line ("| (The whole roll: " + juce::String ((int) all.size()) + " commands: " + juce::String (count (all, "noteon")) + " note-ons and as many note-offs, "
              + juce::String (count (all, "program")) + " programs, " + juce::String (count (all, "dynamic")) + " dynamics, "
              + juce::String (count (all, "gain")) + " gains, " + juce::String (count (all, "glissando")) + " glissandi.) | |");
        checkRow (inOrder && notes == 306 && count (all, "noteon") == 306 && count (all, "noteoff") == 306
                  && programs == 109 && count (all, "program") == 109 && count (all, "glissando") == glissandi && glissandi == 69
                  && count (all, "dynamic") == dynamics && count (all, "gain") == gains,
                  "Every note that is not muted gives a note-on and a note-off, every filled slot and every breakpoint its commands, in order of time; "
                  "the muted note and the voice without a name give none");
    }

    // Playing from the middle: 60 seconds in.
    {
        const auto from = maket::contimbre::messagesFrom (all, 60000.0);
        bool nothingEarlier = true, cutNotes = true;
        int programsAtStart = 0, gainsAtStart = 0, noteOns = 0, expectedNoteOns = 0;
        juce::String celloProgram;
        for (const auto& m : from)
        {
            nothingEarlier = nothingEarlier && m.time >= 60000.0;
            if (m.message.address == "noteon") ++noteOns;
        }
        for (size_t i = 0; i < from.size() && std::abs (from[i].time - 60000.0) < 1.0e-9 && from[i].noteOnset <= 60000.0; ++i)
        {
            if (from[i].message.address == "program") { ++programsAtStart; if (from[i].voice == 2) celloProgram = from[i].message.toText(); }
            else if (from[i].message.address == "gain") ++gainsAtStart;
            else cutNotes = false;
        }
        juce::String lastCello;
        for (const auto& m : all)
        {
            if (m.message.address == "noteon" && m.time >= 60000.0) ++expectedNoteOns;
            if (m.message.address == "program" && m.voice == 2 && m.time < 60000.0) lastCello = m.message.toText();
        }
        checkRow (nothingEarlier && cutNotes && noteOns == expectedNoteOns && noteOns > 0 && noteOns < 306
                  && programsAtStart >= 5 && gainsAtStart >= 5 && celloProgram.isNotEmpty() && celloProgram == lastCello
                  && maket::contimbre::messagesFrom (all, 0.0).size() == all.size(),
                  "Playing from 60 seconds in: only the notes that begin from there on, and before them each voice gets the last program and "
                  "the last gain it was given earlier (the cello: `" + lastCello + "`)");
    }

    // A chosen voice alone.
    {
        const auto onlyFlute = maket::contimbre::messagesForRoll (roll, { 3 });
        bool allFlute = ! onlyFlute.empty();
        for (const auto& m : onlyFlute) allFlute = allFlute && m.voice == 3 && m.message.arguments.size() > 0 && m.message.arguments[0].text == "Fl";
        checkRow (allFlute && onlyFlute.size() == ofVoice (all, 3).size(), "Asked for one voice, Maket makes the commands of that voice only");
    }
    line();
}

void testSmallCases()
{
    line ("### Note-on options, a command in a marker, muted notes");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    const char* text =
        "roll [ voicenames A B [ ] ] "
        "[ markers [ 500. [ cpe all_notes_off ] none ] [ 700. [ marker 2 ] barline ] ] "
        "[ [ 0. [ 6000. 100. 90 [ slots [ 4 time_fraction 0.5 ] ] 0 ] 0 ] "
        "  [ 200. [ 6100. 100. 80 0 ] [ 6200. 100. 80 2 ] 0 ] "
        "  [ 400. [ 6300. 100. 80 0 ] 2 ] 0 ] "
        "[ [ 0. [ 7000. 50. 70 0 ] 0 ] 2 ] "
        "[ [ 0. [ 7100. 50. 70 0 ] 0 ] 0 ]";

    maket::Llll list;
    Roll roll;
    juce::StringArray remarks;
    const bool read = maket::readLlllText (text, list).wasOk() && maket::readBachRoll (list, roll, remarks).wasOk();
    const auto m = maket::contimbre::messagesForRoll (roll);

    checkRow (read && m.size() == 5
              && at (m[0], 0.0, "noteon A 60. 90 time_fraction 0.5")
              && at (m[1], 100.0, "noteoff A 60.")
              && at (m[2], 200.0, "noteon A 61. 80")
              && at (m[3], 300.0, "noteoff A 61.")
              && at (m[4], 500.0, "all_notes_off"),
              "The items of slot 4 follow the note-on; a marker named `cpe all_notes_off` sends that command; a muted note, "
              "a muted chord, a muted voice and a voice without a name send nothing");
    line();
}

void testComparison (const juce::File& temp)
{
    line ("### The comparison with a log from Max, on made-up logs");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    Roll roll;
    juce::StringArray remarks;
    if (maket::readBachRollFile (temp.getChildFile ("doodsbed final v1c.txt"), roll, remarks).failed())
    {
        check (false, "doodsbed final v1c is in Temp and can be read");
        line();
        return;
    }

    // Maket plays every voice but the first (Mezzo); the patch plays that one too.
    std::vector<int> chosen;
    for (int v = 1; v < (int) roll.voices.size(); ++v) chosen.push_back (v);
    const auto expected = maket::contimbre::messagesForRoll (roll, chosen);
    const auto everything = maket::contimbre::messagesForRoll (roll);

    // A log as Max would write it: its own clock, a little unevenness in time.
    auto makeLog = [&] (bool asReceivedOverOsc, bool withFaults, bool wholeTargets = false)
    {
        std::vector<maket::timing::LogLine> log;
        int n = 0, seen = 0;
        for (const auto& e : everything)
        {
            auto message = e.message;
            if (asReceivedOverOsc)
            {
                const auto bytes = message.encode();
                maket::OscMessage::decode (bytes.getData(), bytes.getSize(), message);
            }

            ++seen;
            if (withFaults && e.voice != 0)
            {
                if (seen == 400) continue;                                                      // one command lost
                if (message.address == "noteon" && seen > 600 && seen < 620 && message.arguments.size() == 3)
                {
                    message.arguments[2].integer += 1;                                          // one velocity different
                    seen = 10000;
                }
            }
            if (wholeTargets && message.address == "dynamic" && message.arguments.size() == 4
                && std::abs (message.arguments[2].real) < 1.0e-9)
            {
                message.arguments[2].kind = maket::OscArgument::Kind::integer;                  // 0 instead of 0.
                message.arguments[2].integer = 0;
            }
            log.push_back ({ ++n, 123456.7 + e.time + 0.3 * std::sin ((double) n), message.toText() });
        }
        log.push_back ({ ++n, 1.0, "/maket/tick 3" });
        log.push_back ({ ++n, 2.0, "note 1 1 [ 500. [ 6000. 20. 60 0 ] 0 ]" });
        return log;
    };

    const auto clean = maket::contimbre::compareWithLog (expected, makeLog (false, false));
    checkRow (clean.usable && clean.matched == (int) expected.size() && clean.inLog == clean.matched
              && clean.numOnlyInLog == 0 && clean.numOnlyInMaket == 0
              && clean.otherVoices.size() == 1 && clean.otherVoices[0] == "Mezzo"
              && std::abs (clean.offset - 123456.7) < 0.5 && clean.timing.worst < 0.7,
              "A log of the same commands on another clock: all " + juce::String ((int) expected.size())
              + " are paired, nothing is left over, and the commands for a voice that was not chosen (Mezzo) are named apart");

    const auto overOsc = maket::contimbre::compareWithLog (expected, makeLog (true, false));
    checkRow (overOsc.usable && overOsc.matched == (int) expected.size() && overOsc.numOnlyInLog == 0 && overOsc.numOnlyInMaket == 0,
              "The same commands as they read after travelling as OSC (numbers of 32 bits, such as 66.900002 for 66.9) are still paired");

    const auto faulty = maket::contimbre::compareWithLog (expected, makeLog (false, true));
    checkRow (faulty.usable && faulty.matched == (int) expected.size() - 2 && faulty.numOnlyInLog == 1 && faulty.numOnlyInMaket == 2
              && faulty.nearMisses.size() == 2 && faulty.nearMisses[0].contains ("noteon") && faulty.nearMisses[1].contains ("noteon"),
              "A log with one command lost and one velocity changed: both are found, and the changed note-on is shown beside Maket's");

    const auto kinds = maket::contimbre::compareWithLog (expected, makeLog (false, false, true));
    checkRow (kinds.usable && kinds.matched == (int) expected.size() && kinds.numKindDifferences > 0
              && kinds.kindDifferences.size() >= 2 && kinds.kindDifferences[0].contains ("dynamic"),
              "A log in which a dynamic goes to `0` where Maket sends `0.`: still paired, and the difference in the kind of number is told ("
              + juce::String (kinds.numKindDifferences) + " times)");

    // A log in which the gain of a note comes after its note-on.
    {
        auto log = makeLog (false, false);
        int swapped = 0;
        for (size_t i = 0; i + 1 < log.size(); ++i)
            if (log[i].message.startsWith ("gain Vlc ") && log[i + 1].message.startsWith ("noteon Vlc ")
                && std::abs (log[i].time - log[i + 1].time) < 1.0)                               // of the same moment
            {
                std::swap (log[i].message, log[i + 1].message);
                ++swapped;
                ++i;
            }
        const auto order = maket::contimbre::compareWithLog (expected, log);
        checkRow (clean.numOrderDifferences == 0 && order.usable && order.matched == (int) expected.size() && swapped > 10
                  && order.numOrderDifferences == swapped && order.orderDifferences.size() > 0
                  && order.orderDifferences[0].contains ("noteon, gain") && order.orderDifferences[0].contains ("gain, noteon"),
                  "A log in which the cello's gain comes after its note-on instead of before: paired all the same, and the other order is told ("
                  + juce::String (swapped) + " moments)");
    }

    const auto wrongRoll = maket::contimbre::compareWithLog (expected, { { 1, 5.0, "noteon Tuba 30. 64" }, { 2, 9.0, "noteoff Tuba 30." } });
    checkRow (! wrongRoll.usable && wrongRoll.problem.isNotEmpty(), "A log of another piece gives no result, and says why");
    line();
}

} // namespace

void testConTimbreMapping (const juce::File& temp)
{
    line ("## 7. The conTimbre mapping");
    line();
    line ("Stage 1e: every note of a roll becomes the commands for the conTimbre ePlayer that Joost's Max patch `cpe_poly` makes of it. "
          "Whether they are the same commands is settled on his Mac, against a log of the patch playing the roll; this section "
          "checks the mapping on notes worked out by hand and the comparison itself.");
    line();

    testMapping (temp);
    testSmallCases();
    testComparison (temp);
}
