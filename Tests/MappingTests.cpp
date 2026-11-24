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
#include <set>
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
    const auto file = material (temp, "doodsbed final v1c.txt");
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

void testSecondRoll (const juce::File& temp)
{
    line ("### A second piece: Bult");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    Roll roll;
    juce::StringArray remarks;
    const auto file = material (temp, "Bult - maquette final.txt");
    if (! file.existsAsFile() || maket::readBachRollFile (file, roll, remarks).failed())
    {
        line ("| (`Bult - maquette final.txt` is not in Temp, or cannot be read: not checked.) | |");
        line();
        return;
    }

    const auto all = maket::contimbre::messagesForRoll (roll);

    // Pitches written as names in the file: the first note-on of the flute is a G6, 7900 cents.
    bool named = false;
    for (const auto& voice : roll.voices)
        for (const auto& chord : voice.chords)
            for (const auto& note : chord.notes)
                named = named || note.pitchName.isNotEmpty();

    bool inRange = true;
    for (const auto& m : all)
        if (m.message.address == "noteon")
            inRange = inRange && m.message.arguments.size() >= 3 && m.message.arguments[1].real > 12.0 && m.message.arguments[1].real < 127.0;

    checkRow ((int) roll.voices.size() == 11 && (int) all.size() == 2478 && count (all, "noteon") == 991 && count (all, "noteoff") == 991
              && count (all, "program") == 325 && count (all, "gain") == 92 && count (all, "dynamic") == 79 && count (all, "glissando") == 0
              && named && inRange,
              "Bult, eleven voices of which seven are played (three have no name, one is muted): 2478 commands, 991 note-ons, 325 programs, "
              "92 gains, 79 dynamics; pitches written as names (`G6`, `C#7`) become numbers");
    line();
}

/** Stage 1f.1: the three ways of the Max patch, each by its own switch. */
void testPatchWays (const juce::File& temp)
{
    line ("### The ways of the Max patch, by switch (stage 1f.1)");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    Roll roll;
    juce::StringArray remarks;
    if (maket::readBachRollFile (material (temp, "doodsbed final v1c.txt"), roll, remarks).failed())
    {
        check (false, "doodsbed final v1c is in Temp and can be read");
        line();
        return;
    }

    const auto own = maket::contimbre::messagesForRoll (roll);
    const double onset = 7200.802301;

    // The order: note-on before gain.
    {
        maket::contimbre::MappingOptions o;
        o.noteOnFirst = true;
        const auto vlc = ofVoice (maket::contimbre::messagesForRoll (roll, {}, o), 2);
        checkRow (vlc.size() > 5 && own.size() == maket::contimbre::messagesForRoll (roll, {}, o).size()
                  && at (vlc[0], onset, "program Vlc \"sul tasto\"")
                  && at (vlc[1], onset, "noteon Vlc 66.9 1")
                  && at (vlc[2], onset, "gain Vlc 5.777778")
                  && at (vlc[3], onset, "dynamic Vlc 66.9 16.846475 4267.")
                  && at (vlc[4], onset, "glissando Vlc 66.9 0.5 4511."),
                  "The patch's order: program, note-on, gain, dynamic, glissando; the same commands, none more or fewer");

        // A double stop of the viola: both note-ons before either note's curves.
        const auto vla = ofVoice (maket::contimbre::messagesForRoll (roll, {}, o), 5);
        juce::StringArray kinds;
        for (const auto& m : vla)
            if (std::abs (m.time - 19372.0) < 1.0) kinds.add (m.message.address);
        line ("| (The viola's double stop at 19.4 s in the patch's order: " + kinds.joinIntoString (", ") + ".) | |");
        int lastNoteOn = -1, firstCurve = 1000;
        for (int i = 0; i < kinds.size(); ++i)
        {
            if (kinds[i] == "noteon") lastNoteOn = i;
            else if (kinds[i] != "program" && kinds[i] != "noteoff") firstCurve = juce::jmin (firstCurve, i);
        }
        checkRow (kinds.size() >= 6 && lastNoteOn >= 1 && lastNoteOn < firstCurve,
                  "In a chord every note-on comes before the first gain, dynamic or glissando");
    }

    // A glide to 0 for every note without breakpoints.
    {
        maket::contimbre::MappingOptions o;
        o.glissandoToZero = true;
        const auto all = maket::contimbre::messagesForRoll (roll, {}, o);

        int withoutBreakpoints = 0;
        for (const auto& voice : roll.voices)
        {
            if (voice.name.isEmpty() || voice.isMuted()) continue;
            for (const auto& chord : voice.chords)
                if (! chord.isMuted())
                    for (const auto& note : chord.notes)
                        if (! note.isMuted() && note.breakpoints.size() < 2) ++withoutBreakpoints;
        }

        int toZero = 0;
        for (const auto& m : all)
            if (m.message.address == "glissando" && m.message.arguments.size() == 4 && std::abs (m.message.arguments[2].real) < 1.0e-12) ++toZero;
        int ownToZero = 0;
        for (const auto& m : own)
            if (m.message.address == "glissando" && std::abs (m.message.arguments[2].real) < 1.0e-12) ++ownToZero;

        checkRow (withoutBreakpoints > 200 && (int) all.size() == (int) own.size() + withoutBreakpoints && toZero - ownToZero == withoutBreakpoints,
                  "With the glide to 0 switched on, every note without pitch breakpoints gets one more command ("
                  + juce::String (withoutBreakpoints) + " in doodsbed), the others none");
    }

    // The commands of a moment 0.1 ms apart.
    {
        maket::contimbre::MappingOptions o;
        o.spacing = 0.1;
        const auto vlc = ofVoice (maket::contimbre::messagesForRoll (roll, {}, o), 2);
        bool inOrder = true, momentsKept = true;
        const auto all = maket::contimbre::messagesForRoll (roll, {}, o);
        for (size_t i = 1; i < all.size(); ++i) inOrder = inOrder && all[i].time >= all[i - 1].time;
        for (const auto& m : all) momentsKept = momentsKept && m.time >= m.moment && m.time - m.moment < 2.0;
        checkRow (vlc.size() > 5 && inOrder && momentsKept
                  && at (vlc[0], onset, "program Vlc \"sul tasto\"") && at (vlc[1], onset + 0.1, "gain Vlc 5.777778")
                  && at (vlc[2], onset + 0.2, "noteon Vlc 66.9 1") && at (vlc[3], onset + 0.3, "dynamic Vlc 66.9 16.846475 4267.")
                  && at (vlc[4], onset + 0.4, "glissando Vlc 66.9 0.5 4511."),
                  "With a spacing of 0.1 ms the five commands of the cello's first note leave at 0, 0.1, 0.2, 0.3 and 0.4 ms after its onset");
    }
    line();
}

/** Stage 1f.4: every note of a roll addressed by number. */
void testByNumber (const juce::File& temp)
{
    line ("### A roll by number (stage 1f.4)");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    Roll roll;
    juce::StringArray remarks;
    if (maket::readBachRollFile (material (temp, "doodsbed final v1c.txt"), roll, remarks).failed())
    {
        check (false, "doodsbed final v1c is in Temp and can be read");
        line();
        return;
    }

    maket::contimbre::MappingOptions o;
    o.byNumber = true;
    const auto byPitch = maket::contimbre::messagesForRoll (roll);
    const auto all = maket::contimbre::messagesForRoll (roll, {}, o);
    const auto vlc = ofVoice (all, 2);
    const double onset = 7200.802301;

    // The cello is the third voice of the roll: voice3. Its first note is note number 12 (after the eleven of Mezzo).
    juce::String first;
    for (size_t i = 0; i < 5 && i < vlc.size(); ++i) first << vlc[i].message.toText() << (i < 4 ? "; " : "");
    line ("| (The cello's first note by number: " + first + ".) | |");
    checkRow (vlc.size() > 5 && at (vlc[0], onset, "program voice3 \"sul tasto\"") && at (vlc[1], onset, "gain voice3 5.777778")
              && at (vlc[2], onset, "ctnote 3 12 66.9 1 4511.") && at (vlc[3], onset, "ctdynamic 12 17 4267.")
              && at (vlc[4], onset, "ctglissando 12 0.5 4511."),
              "The cello, third voice of the roll, plays on `voice3`: program and gain by that name, the note and its curves by the note's number");

    // The same number of commands, at the same times, kind for kind.
    bool sameShape = all.size() == byPitch.size();
    for (size_t i = 0; sameShape && i < all.size(); ++i)
    {
        const auto a = byPitch[i].message.address, b = all[i].message.address;
        sameShape = std::abs (all[i].time - byPitch[i].time) < 1.0e-9
                    && (b == a || b == "ct" + a || (a == "noteon" && b == "ctnote") || (a == "noteoff" && b == "ctnoteoff"));
    }

    // Every note-on has a number of its own, and its note-off, dynamics and glissandi carry that number.
    std::set<int> numbers;
    int noteOns = 0;
    bool known = true;
    for (const auto& m : all)
    {
        const auto& a = m.message.arguments;
        if (m.message.address == "ctnote") { ++noteOns; numbers.insert ((int) a[1].integer); }
    }
    for (const auto& m : all)
        if (m.message.address == "ctnoteoff" || m.message.address == "ctdynamic" || m.message.address == "ctglissando")
            known = known && numbers.count ((int) m.message.arguments[0].integer) == 1;

    checkRow (sameShape && noteOns == 306 && (int) numbers.size() == 306 && *numbers.begin() == 1 && *numbers.rbegin() == 306 && known,
              "By number the roll gives the same commands at the same times as by pitch, each in its form by number; the 306 notes are "
              "numbered 1 to 306, and every note-off, dynamic and glissando carries the number of its note");

    // A ramp's target by number: a whole number, the rounded target of the same ramp by pitch; with the switch off, as by pitch.
    {
        auto real = o;
        real.wholeTargets = false;
        const auto asReal = maket::contimbre::messagesForRoll (roll, {}, real);
        using Kind = maket::OscArgument::Kind;
        bool whole = true, rounded = true, realOnes = asReal.size() == all.size();
        int ramps = 0;
        for (size_t i = 0; i < all.size() && i < byPitch.size() && realOnes; ++i)
            if (all[i].message.address == "ctdynamic")
            {
                ++ramps;
                const auto& t = all[i].message.arguments[1];
                const auto& p = byPitch[i].message.arguments[2];
                const auto& r = asReal[i].message.arguments[1];
                whole = whole && t.kind == Kind::integer;
                rounded = rounded && t.integer == juce::roundToInt (p.real);
                realOnes = realOnes && r.kind == Kind::real && std::abs (r.real - p.real) < 1.0e-9;
            }
        checkRow (ramps == 218 && whole && rounded && realOnes,
                  "By number the target of each of the 218 dynamic ramps is sent as a whole number, the target by pitch rounded; "
                  "with the switch off it is the real number as by pitch (stage 1f.5)");
    }

    // The viola's double stop: two notes of one moment, each with its own number and its own curve.
    {
        const auto vla = ofVoice (all, 5);
        std::set<int> on, curved;
        for (const auto& m : vla)
            if (std::abs (m.time - 19372.0) < 1.0)
            {
                if (m.message.address == "ctnote") on.insert ((int) m.message.arguments[1].integer);
                if (m.message.address == "ctdynamic") curved.insert ((int) m.message.arguments[0].integer);
            }
        checkRow (on.size() == 2 && curved == on, "In the viola's double stop at 19.4 s each of the two notes has its own number, and each gets its own dynamic");
    }
    line();
}

/** Joost's takes of 23 Nov: the logs of ScAst playing, held against Maket's commands. */
void testJoostsTakes (const juce::File& temp)
{
    line ("### Joost's ScAst takes of 23 Nov, compared here");
    line();
    line ("The logs in `Temp/Mapping 1e`, with every voice but Mezzo (which is not a conTimbre voice).");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    struct Take { const char* rollName; const char* log; int same, zero, chord; };
    const Take takes[] = {
        { "doodsbed final v1c.txt",    "20261123-091638 doodsbed final v1c scast max log.txt",    1146, 238, 20 },
        { "Bult - maquette final.txt", "20261123-121314 Bult - maquette final scast max log.txt", 2438, 971,  0 } };

    for (const auto& take : takes)
    {
        Roll roll;
        juce::StringArray remarks;
        const auto logFile = temp.getChildFile ("Mapping 1e").getChildFile (take.log);
        if (! logFile.existsAsFile() || maket::readBachRollFile (material (temp, take.rollName), roll, remarks).failed())
        {
            line ("| (`" + juce::String (take.log) + "` or its roll is not in Temp: not checked.) | |");
            continue;
        }

        std::vector<int> voices;
        for (int v = 0; v < (int) roll.voices.size(); ++v)
            if (roll.voices[(size_t) v].name.isNotEmpty() && roll.voices[(size_t) v].name != "Mezzo") voices.push_back (v);

        const auto log = maket::timing::readCollLog (logFile.loadFileAsString());
        const auto c = maket::contimbre::compareWithLog (maket::contimbre::messagesForRoll (roll, voices), log);

        line ("| (" + juce::String (take.rollName) + ": the same " + juce::String (c.matched) + " of " + juce::String (c.inMaket)
              + "; glides to 0: " + juce::String (c.numZeroGlissandi) + "; chord commands with the last note's pitch: " + juce::String (c.numChordFault)
              + "; stops: " + juce::String (c.numStops) + "; the ePlayer's own: " + c.playersOwn.joinIntoString (", ")
              + "; left over: " + juce::String (c.numOnlyInLog) + " in the log, " + juce::String (c.numOnlyInMaket) + " in Maket.) | |");
        checkRow (c.usable && c.matched == take.same && c.numZeroGlissandi == take.zero && c.numChordFault == take.chord
                  && c.inMaket == take.same + take.chord && c.numOnlyInLog == 0 && c.numOnlyInMaket == 0,
                  juce::String (take.rollName) + ": every command of Maket's is in ScAst's log, apart from the " + juce::String (take.chord)
                  + " chord commands that the patch sends with another pitch; the log's " + juce::String (take.zero)
                  + " glides to 0 are told apart; nothing else is left over");

        // With the glide to 0 switched on, those are Maket's as well.
        maket::contimbre::MappingOptions o;
        o.glissandoToZero = true;
        o.noteOnFirst = true;
        const auto asPatch = maket::contimbre::compareWithLog (maket::contimbre::messagesForRoll (roll, voices, o), log);
        checkRow (asPatch.usable && asPatch.inMaket == take.same + take.chord + take.zero
                  && asPatch.matched + asPatch.numChordFault == asPatch.inMaket && asPatch.numZeroGlissandi == 0
                  && asPatch.numOnlyInLog == 0 && asPatch.numOnlyInMaket == 0 && asPatch.numOrderDifferences < c.numOrderDifferences,
                  juce::String (take.rollName) + ", with the patch's order and its glides to 0 switched on: " + juce::String (asPatch.matched)
                  + " the same, " + juce::String (asPatch.numChordFault) + " chord commands with another pitch (the patch's glides to 0 of a chord "
                  + "carry the last note's pitch too), nothing left over; the order within a moment differs at "
                  + juce::String (asPatch.numOrderDifferences) + " moments instead of " + juce::String (c.numOrderDifferences));
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
    if (maket::readBachRollFile (material (temp, "doodsbed final v1c.txt"), roll, remarks).failed())
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
    testSecondRoll (temp);
    testPatchWays (temp);
    testByNumber (temp);
    testSmallCases();
    testComparison (temp);
    testJoostsTakes (temp);
    testPlayerStudy();
}
