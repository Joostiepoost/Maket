// SpeedTests.cpp — stage 2f: speed with a large roll.
//
// The test program writes a roll far larger than any of Joost's (24 voices,
// 20 minutes, tens of thousands of notes) and times what Maket does with it:
// reading and writing the file, working out what is drawn, preparing a play,
// and the synth at full load. The times are those of the machine the program
// runs on; the limits are wide, they catch a mistake and not a slow machine.
// How the roll view itself feels with this roll is tried in the app.

#include <JuceHeader.h>
#include <cmath>
#include <limits>
#include <vector>

#include "TestReport.h"
#include "../Source/Model/Roll.h"
#include "../Source/Formats/BachRoll.h"
#include "../Source/Formats/LlllText.h"
#include "../Source/Engine/OscScheduler.h"
#include "../Source/Engine/Playing.h"
#include "../Source/Engine/Synth.h"
#include "../Source/Engine/Transport.h"
#include "../Source/Notation/Chords.h"
#include "../Source/Notation/Glides.h"
#include "../Source/Notation/Spelling.h"
#include "../Source/Notation/Staves.h"

using namespace testreport;
using maket::playing::VoiceEngine;

namespace
{
    const double forever = std::numeric_limits<double>::max();

    double nowMs()      { return juce::Time::getMillisecondCounterHiRes(); }

    maket::HeaderEntry entryOf (const juce::String& text)
    {
        maket::HeaderEntry entry;
        maket::Llll all;
        maket::readLlllText (text, all);
        entry.content = all.size() == 1 && all[0].isList() ? all[0] : all;
        entry.name = entry.content.head();
        return entry;
    }

    void setSlot (maket::Note& note, int number, const juce::String& content)
    {
        maket::SlotValue value;
        value.number = number;
        maket::readLlllText (content, value.content);
        note.slots.push_back (std::move (value));
    }

    /** A roll of `numVoices` voices and `seconds` of music: chords of one to four notes on the eighth-tone grid,
        one note in ten gliding, one in five with a modulation curve. The first half of the voices name a technique
        (conTimbre), the others play on the synth. Always the same roll: the numbers come from a fixed seed. */
    maket::Roll largeRoll (int numVoices, double seconds, const std::vector<maket::SlotInfo>& slotInfo)
    {
        maket::Roll roll;
        juce::Random random (20261126);
        const char* clefs[] = { "G", "F", "FG", "G8vb", "Alto", "FFGG" };
        const char* techniques[] = { "ordinario", "pizz", "[ sul tasto ]", "[ sul ponticello ]" };

        juce::String clefLine = "[ clefs", nameLine = "[ voicenames", channelLine = "[ midichannels";
        roll.voices.resize ((size_t) numVoices);
        for (int v = 0; v < numVoices; ++v)
        {
            auto& voice = roll.voices[(size_t) v];
            voice.clef = clefs[v % 6];
            voice.name = "v" + juce::String (v + 1);
            clefLine << " " << voice.clef;
            nameLine << " " << voice.name;
            channelLine << " " << (v % 16 + 1);

            const bool onConTimbre = v < numVoices / 2;
            const double centre = voice.clef == "F" ? 4800.0 : voice.clef == "Alto" ? 6000.0 : voice.clef == "G8vb" ? 5700.0 : 6700.0;
            double t = random.nextDouble() * 400.0;
            while (t < seconds * 1000.0)
            {
                maket::Chord chord;
                chord.onset = std::round (t);
                const int size = random.nextInt (10) < 6 ? 1 : 2 + random.nextInt (3);
                const double base = centre + 25.0 * (random.nextInt (97) - 48);
                for (int i = 0; i < size; ++i)
                {
                    maket::Note note;
                    note.cents = base + 25.0 * random.nextInt (9) + 300.0 * i;
                    note.duration = std::round (80.0 + random.nextDouble() * random.nextDouble() * 2500.0);
                    note.velocity = 20 + random.nextInt (108);
                    if (random.nextInt (10) == 0)
                    {
                        maket::Breakpoint head, middle, tail;
                        middle.position = 0.3 + 0.4 * random.nextDouble();
                        middle.cents = 25.0 * (random.nextInt (17) - 8);
                        middle.slope = random.nextDouble() - 0.5;
                        tail.position = 1.0;
                        tail.cents = 25.0 * (random.nextInt (17) - 8);
                        note.breakpoints = { head, middle, tail };
                    }
                    if (onConTimbre && i == 0) setSlot (note, 1, techniques[random.nextInt (4)]);
                    if (random.nextInt (5) == 0)
                        setSlot (note, 2, "[ 0. " + juce::String (random.nextInt (128)) + ". 0. ] [ " + juce::String (note.duration * 0.5, 1) + " "
                                          + juce::String (random.nextInt (128)) + ". 0. ] [ " + juce::String (note.duration, 1) + " " + juce::String (random.nextInt (128)) + ". 0. ]");
                    chord.notes.push_back (std::move (note));
                }
                voice.chords.push_back (std::move (chord));
                t += 150.0 + random.nextDouble() * 1100.0;
            }
        }
        roll.header.push_back (entryOf (clefLine + " ]"));
        roll.header.push_back (entryOf (nameLine + " ]"));
        roll.header.push_back (entryOf (channelLine + " ]"));
        roll.slotInfo = slotInfo;

        for (int i = 1; i * 5.0 < seconds; ++i)
        {
            maket::Marker m;
            m.position = i * 5000.0;
            m.names.items.push_back (maket::Llll::makeSymbol ("m" + juce::String (i)));
            m.role = "none";
            roll.markers.push_back (std::move (m));
        }
        return roll;
    }

    juce::String ms (double value)      { return (value < 10.0 ? juce::String (value, 1) : juce::String (juce::roundToInt (value))) + " ms"; }
}

void testSpeed (const juce::File& temp)
{
    line ("## 11. Speed with a large roll (stage 2f)");
    line();
    line ("A roll far larger than Joost's own is written into `Temp/written by Maket` and timed. The times are those of this machine; "
          "the limits are wide and catch a mistake, not a slow machine.");
    line();

    maket::Roll mixed;
    juce::StringArray remarks;
    std::vector<maket::SlotInfo> slotInfo;
    if (maket::readBachRollFile (material (temp, "mixed_roll_example_01h.txt"), mixed, remarks).wasOk()) slotInfo = mixed.slotInfo;

    double t = nowMs();
    const auto roll = largeRoll (24, 1200.0, slotInfo);
    const double made = nowMs() - t;
    const int notes = roll.numNotes();
    int chords = 0;
    for (const auto& v : roll.voices) chords += (int) v.chords.size();

    t = nowMs();
    const auto text = maket::writeBachRollText (roll);
    const double written = nowMs() - t;

    const auto folder = temp.getChildFile ("written by Maket");
    const auto file = folder.getChildFile ("Large roll by Maket.txt");
    const bool saved = temp.isDirectory() && folder.createDirectory().wasOk() && file.replaceWithText (text, false, false, "\n");

    t = nowMs();
    maket::Roll back;
    const bool readOk = saved && maket::readBachRollFile (file, back, remarks).wasOk();
    const double read = nowMs() - t;
    const bool same = readOk && back.numNotes() == notes && maket::writeBachRollText (back) == text;

    // What the roll view works out per note before it draws: the staves, the spelling, heads and accidentals of each chord, the glides.
    t = nowMs();
    long long checksum = 0;
    {
        const auto layout = maket::notation::layoutVoices (roll);
        for (const auto& place : layout.voices)
            for (const auto& chord : roll.voices[(size_t) place.voice].chords)
            {
                std::vector<int> steps;
                std::vector<maket::notation::ChordNote> placing;
                for (const auto& note : chord.notes)
                {
                    const auto spelling = maket::notation::spellAs (note.pitchName, note.cents, 8);
                    steps.push_back (spelling.steps);
                    checksum += (long long) maket::notation::glidePoints (spelling, note.breakpoints).size() + (long long) maket::notation::ledgerSteps (place.clef, spelling.steps).size();
                    maket::notation::ChordNote p;
                    p.step = spelling.steps;
                    p.hasAccidental = maket::notation::accidentalOf (spelling.alteration) != maket::notation::Accidental::none;
                    p.above = 1.4; p.below = 1.4; p.width = 1.0;
                    placing.push_back (p);
                }
                std::sort (steps.begin(), steps.end());
                std::sort (placing.begin(), placing.end(), [] (const maket::notation::ChordNote& a, const maket::notation::ChordNote& b) { return a.step < b.step; });
                const auto heads = maket::notation::placeHeads (steps, 1, 1.18);
                for (size_t i = 0; i < placing.size(); ++i) placing[i].headLeft = heads[i].left;
                checksum += (long long) maket::notation::placeAccidentals (placing).size();
            }
    }
    const double laidOut = nowMs() - t;

    // A play of the whole roll: the commands for the ePlayer and the notes for the synth.
    const auto engines = maket::playing::defaultEngines (roll);
    t = nowMs();
    int numbers = 0;
    const auto messages = maket::playing::messagesForPass (roll, engines, 0.0, forever, 1, numbers);
    const auto synthNotes = maket::playing::synthNotesForPass (roll, engines, 0.0, forever);
    const double prepared = nowMs() - t;

    // As the transport does it: the roll made ready once, then each pass from that.
    t = nowMs();
    const auto ready = maket::playing::prepare (roll, engines);
    const double madeReady = nowMs() - t;
    t = nowMs();
    int fromMiddleNumbers = 0;
    const auto fromMiddle = maket::playing::messagesForPass (ready, 600000.0, forever, 1, fromMiddleNumbers);
    const double preparedMiddle = nowMs() - t;
    t = nowMs();
    const auto loopPass = maket::playing::messagesForPass (ready, 600000.0, 610000.0, 1, fromMiddleNumbers);
    const auto loopNotes = maket::playing::synthNotesForPass (ready, 600000.0, 610000.0);
    const double preparedLoop = nowMs() - t;

    // The prepared way gives what the plain way gives.
    int plainNumbers = 0;
    const auto plainLoop = maket::playing::messagesForPass (roll, engines, 600000.0, 610000.0, 1, plainNumbers);
    bool sameLoop = plainLoop.size() == loopPass.size() && plainNumbers == fromMiddleNumbers
                    && loopNotes.size() == maket::playing::synthNotesForPass (roll, engines, 600000.0, 610000.0).size();
    for (size_t i = 0; sameLoop && i < plainLoop.size(); ++i)
        sameLoop = std::abs (plainLoop[i].time - loopPass[i].time) < 1.0e-9 && plainLoop[i].voice == loopPass[i].voice && plainLoop[i].message.address == loopPass[i].message.address
                   && plainLoop[i].message.arguments.size() == loopPass[i].message.arguments.size();
    int wholeNumbers = 0;
    const auto wholeReady = maket::playing::messagesForPass (ready, 0.0, forever, 1, wholeNumbers);
    bool sameWhole = wholeReady.size() == messages.size() && wholeNumbers == numbers;
    for (size_t i = 0; sameWhole && i < messages.size(); i += 97)
        sameWhole = std::abs (messages[i].time - wholeReady[i].time) < 1.0e-9 && messages[i].message.address == wholeReady[i].message.address;

    line ("| What | Time here |");
    line ("| --- | --- |");
    line ("| The roll: 24 voices, 20 minutes, " + juce::String (chords) + " chords, " + juce::String (notes) + " notes, " + juce::String (roll.numNotesWithBreakpoints())
          + " of them gliding, " + juce::String ((int) roll.markers.size()) + " markers; as a file " + juce::String (text.length() / 1000000.0, 1) + " MB | made in " + ms (made) + " |");
    line ("| Writing it as a bach.roll text | " + ms (written) + " |");
    line ("| Reading the file | " + ms (read) + " |");
    line ("| Working out staves, spelling, heads, accidentals and glides for every note | " + ms (laidOut) + " |");
    line ("| Preparing a play of the whole roll: " + juce::String ((int) messages.size()) + " commands for the ePlayer, " + juce::String ((int) synthNotes.size()) + " notes for the synth | " + ms (prepared) + " |");
    line ("| Making the roll ready for the transport (once after opening, and after a change of who plays a voice) | " + ms (madeReady) + " |");
    line ("| After that: a play from the middle to the end | " + ms (preparedMiddle) + " |");
    line ("| After that: one round of a loop of 10 s (" + juce::String ((int) loopPass.size()) + " commands, " + juce::String ((int) loopNotes.size()) + " synth notes) | " + ms (preparedLoop) + " |");

    // The transport: from the press of Play to everything being scheduled; the first note keeps its lead.
    double pressToReady = 0.0, firstLead = 0.0, stopTook = 0.0;
    bool transportOk = false;
    {
        struct CountingSink : public maket::NoteSink
        {
            void addNotes (std::vector<maket::SynthNote> n) override   { for (const auto& x : n) first = std::min (first, x.start); count += (int) n.size(); }
            void allNotesOff() override                                 {}
            double first = std::numeric_limits<double>::max();
            int count = 0;
        };
        maket::OscScheduler scheduler;
        maket::Transport transport (scheduler, 11);
        CountingSink sink;
        transport.setRoll (roll);
        transport.setSynth (&sink);
        juce::String problem;
        const double before = nowMs();
        const bool began = transport.play (0.0, 442.0, problem);
        const double after = nowMs();
        pressToReady = after - before;
        // The first synth note of the roll, on the clock, against the moment play() came back.
        double firstInRoll = forever;
        for (const auto& n : synthNotes) firstInRoll = std::min (firstInRoll, n.start);
        firstLead = (sink.first - firstInRoll) - after;       // how far ahead of "now" the roll's time 0 lies when everything is ready
        t = nowMs();
        transport.stop();
        stopTook = nowMs() - t;
        transportOk = began && sink.count == (int) synthNotes.size() && firstLead > 0.0;
    }
    line ("| From the press of Play to everything being scheduled | " + ms (pressToReady) + " |");
    line ("| Stop: taking back what was scheduled | " + ms (stopTook) + " |");

    // The synth at full load: 128 notes at once, 10 s of sound.
    double renderTook = 0.0, waitingTook = 0.0;
    {
        maket::Synth synth;
        synth.prepare (48000.0);
        std::vector<maket::SynthNote> many;
        for (int i = 0; i < 128; ++i)
        {
            maket::SynthNote n;
            n.start = 0.0; n.end = 10000.0; n.cents = 3600.0 + 50.0 * i; n.velocity = 100;
            n.glide = { { 0.0, 0.0, 0.0 }, { 0.5, 100.0, 0.3 }, { 1.0, 0.0, -0.3 } };
            n.levels = { { 0.0, 20.0, 0.0 }, { 5000.0, 127.0, 0.0 }, { 10000.0, 0.0, 0.0 } };
            many.push_back (n);
        }
        synth.addNotes (many);
        std::vector<float> left (512), right (512);
        t = nowMs();
        for (int block = 0; block * 512 < 480000; ++block)
        {
            std::fill (left.begin(), left.end(), 0.0f); std::fill (right.begin(), right.end(), 0.0f);
            synth.render (left.data(), right.data(), 512, block * 512 * 1000.0 / 48000.0);
        }
        renderTook = nowMs() - t;

        // With the whole roll's notes waiting, a block of silence must still cost next to nothing.
        maket::Synth waitingSynth;
        waitingSynth.prepare (48000.0);
        auto later = synthNotes;
        for (auto& n : later) { n.start += 1.0e7; n.end += 1.0e7; }
        waitingSynth.addNotes (later);
        t = nowMs();
        for (int block = 0; block < 1000; ++block) waitingSynth.render (left.data(), right.data(), 512, block * 512 * 1000.0 / 48000.0);
        waitingTook = (nowMs() - t) / 1000.0;
    }
    line ("| The synth with 128 gliding notes at once: 10 s of sound | " + ms (renderTook) + " (" + juce::String (renderTook / 100.0, 1) + " % of the time it lasts) |");
    line ("| One block of sound (10.7 ms long) while " + juce::String ((int) synthNotes.size()) + " synth notes wait their turn | " + juce::String (waitingTook, 3) + " ms |");
    line();

    line ("| Check | Passed |");
    line ("| --- | --- |");
    checkRow (saved && same && notes > 40000, "`Temp/written by Maket/Large roll by Maket.txt` is written (" + juce::String (notes) + " notes) and, read back, holds the same");
    checkRow (written < 5000.0 && read < 10000.0 && laidOut < 5000.0, "Writing, reading and working out the notation each take seconds at most, far less on a fast machine");
    checkRow (prepared < 5000.0 && madeReady < 5000.0 && preparedMiddle < 5000.0 && checksum > 0 && ! fromMiddle.empty(), "A play of the whole roll, and of a part of it, is prepared in seconds at most");
    checkRow (sameLoop && sameWhole && preparedLoop < 200.0, "A round of a loop is worked out from its own ten seconds, not from the whole roll (" + ms (preparedLoop)
                                                             + "), and gives the same commands as the plain way");
    checkRow (transportOk, "However long the preparing takes, the first note keeps its lead: when everything is scheduled, the start of the roll still lies ahead (here "
                           + juce::String (juce::roundToInt (firstLead)) + " ms), so the opening is not played late or bunched together");
    checkRow (renderTook < 5000.0 && waitingTook < 2.0, "The synth at full load makes sound several times faster than it is heard, and waiting notes cost next to nothing");
    line();
}
