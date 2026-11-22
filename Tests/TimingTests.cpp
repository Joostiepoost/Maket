// TimingTests.cpp — stage 1d: the passages of the timing test, the reader
// of the logging patch's file, and the comparison of arrival times with
// due times.
//
// The measurements themselves are made on Joost's Mac, with Max. What is
// checked here is that the measuring is right: the comparison is held
// against the two logs of 22 Nov (stage 1c), for which the numbers were
// worked out separately, against made-up logs with known faults in them,
// and against a short run through this computer's own network loop.

#include <JuceHeader.h>
#include <cmath>
#include <vector>

#include "TestReport.h"
#include "../Source/Model/Roll.h"
#include "../Source/Formats/BachRoll.h"
#include "../Source/Formats/LlllText.h"
#include "../Source/Engine/Osc.h"
#include "../Source/Engine/OscScheduler.h"
#include "../Source/Engine/TimingTest.h"

using namespace testreport;
using namespace maket::timing;
using maket::OscMessage;
using maket::OscScheduler;
using maket::Roll;

namespace
{

bool close (double a, double b, double tolerance = 0.0006)     { return std::abs (a - b) <= tolerance; }

juce::String ms (double value)      { return juce::String (value, 2); }

void testPassages()
{
    line ("### The passages");
    line();

    const auto dense = densePassage();
    const auto again = densePassage();
    const auto longOne = longPassage (5.0);

    std::vector<int> perPart ((size_t) dense.parts.size(), 0);
    bool inOrder = true, same = dense.notes.size() == again.notes.size();
    for (size_t i = 0; i < dense.notes.size(); ++i)
    {
        ++perPart[(size_t) dense.notes[i].part];
        if (i > 0) inOrder = inOrder && dense.notes[i].onset >= dense.notes[i - 1].onset;
        if (same) same = close (dense.notes[i].onset, again.notes[i].onset, 0.0) && close (dense.notes[i].pitch, again.notes[i].pitch, 0.0);
    }

    line ("The dense passage: " + juce::String ((int) dense.notes.size()) + " notes in " + juce::String (dense.length() / 1000.0, 1)
          + " seconds, so " + juce::String (dense.numMessages()) + " messages (a note-on and a note-off each):");
    line();
    for (int i = 0; i < dense.parts.size(); ++i)
        line ("- part " + juce::String (i + 1) + ": " + dense.parts[i] + " (" + juce::String (perPart[(size_t) i]) + " notes)");
    line();
    line ("The long passage: " + longOne.parts[0] + ", " + juce::String ((int) longOne.notes.size()) + " notes in "
          + juce::String (longOne.length() / 60000.0, 1) + " minutes.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    checkRow (inOrder && same && perPart[0] == 240 && perPart[1] == 192 && perPart[3] == 400 && perPart[2] > 400
              && (int) longOne.notes.size() == 3000,
              "The passages are the same notes at every run, in order of time");

    // As a bach.roll file, for the reference take.
    const auto roll = passageAsRoll (dense, 90);
    const auto text = maket::writeBachRollText (roll);
    maket::Llll list;
    Roll back;
    juce::StringArray remarks;
    const bool readsBack = maket::readLlllText (text, list).wasOk() && maket::readBachRoll (list, back, remarks).wasOk()
                           && back.voices.size() == 1 && back.numNotes() == (int) dense.notes.size()
                           && back.numChords() == 240 + 24 + perPart[2] + 400
                           && back.voices[0].chords[240].notes.size() == 8
                           && close (back.voices[0].chords[0].onset, 500.0, 0.0)
                           && close (back.voices[0].chords[0].notes[0].cents, 6000.0, 0.0);
    checkRow (readsBack, "Written as a bach.roll file and read back, the dense passage holds the same notes, the chords as chords of eight");
    line();
}

void testLogReading (const juce::File& temp)
{
    line ("### Reading the logs, and the logs of 22 Nov");
    line();

    const auto maxFile = temp.getChildFile ("1c max log.txt");
    const auto maketFile = temp.getChildFile ("1c maket log.txt");
    if (! maxFile.existsAsFile() || ! maketFile.existsAsFile())
    {
        check (false, "The two logs of stage 1c are in Temp");
        line ("**Not tested:** `1c max log.txt` or `1c maket log.txt` is not in `Temp`.");
        line();
        return;
    }

    const auto log = readCollLog (maxFile.loadFileAsString());
    const auto sent = readMaketSendLog (maketFile.loadFileAsString());

    bool numbered = log.size() == 72;
    for (size_t i = 0; i < log.size(); ++i) numbered = numbered && log[i].number == (int) i + 1;

    line ("| Check | Passed |");
    line ("| --- | --- |");
    checkRow (numbered && close (log[0].time, 31929.31275, 1.0e-9)
              && log[0].message == "program Cl \"bass clarinet boehm system.key noise.tremolo\""
              && log[71].message == "noteoff Cl 41.700001" && close (log[71].time, 110023.873958, 1.0e-9),
              "The patch's file of 22 Nov reads as 72 lines, each with its number, time and message");
    checkRow (sent.size() == 72 && close (sent[1].due, 0.0, 0.0) && close (sent[1].late, 0.183, 1.0e-9)
              && sent[1].message == "noteon Cl 80.75 126" && close (sent[71].due, 78095.095, 1.0e-9),
              "Maket's send log of 22 Nov reads as 72 lines, each with its due time, lateness and message");

    bool sameMessages = sent.size() == log.size();
    for (size_t i = 0; sameMessages && i < sent.size(); ++i) sameMessages = sent[i].message == log[i].message;
    checkRow (sameMessages, "Both hold the same 72 messages in the same order");

    // The numbers below were worked out separately (a short Python script, 22 Nov).
    const auto result = compareLogs (sent, log);
    checkRow (result.usable && result.arrived == 72 && result.outOfOrder == 0
              && close (result.all.typical, 0.2415) && close (result.all.half, 0.1420) && close (result.all.most, 0.5257)
              && close (result.all.worst, 0.6643) && close (result.all.earliest, -0.2317) && close (result.all.latest, 0.6643),
              "The comparison of the two gives the numbers worked out separately: typical " + juce::String (result.all.typical, 4)
              + " ms, half within " + juce::String (result.all.half, 4) + ", 95 in 100 within " + juce::String (result.all.most, 4)
              + ", the farthest " + juce::String (result.all.worst, 4));
    line();
}

void testComparing()
{
    line ("### The comparison, on made-up logs with known faults");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    // A small passage: 100 notes, 10 ms apart, 5 ms long; so 200 ticks.
    Passage passage;
    passage.name = "made up";
    passage.parts = { "first half", "second half" };
    for (int i = 0; i < 100; ++i) passage.notes.push_back ({ 10.0 * i, 5.0, 60.0, i < 50 ? 0 : 1 });

    // What Maket sent: every tick 0.01 ms late.
    std::vector<OscScheduler::Sent> sent;
    for (int i = 0; i < 100; ++i)
        for (int off = 0; off < 2; ++off)
        {
            OscScheduler::Sent s;
            s.due = 7000.0 + passage.notes[(size_t) i].onset + (off == 1 ? passage.notes[(size_t) i].duration : 0.0);
            s.sent = s.due + 0.01;
            s.text = tickMessage (i, off == 1).toText();
            s.targets = OscScheduler::toLog;
            sent.push_back (s);
        }

    // The patch's log: Max's clock runs 123456.789 ms ahead; every tenth tick
    // comes 0.5 ms late, tick 150 comes 2 ms early, tick 60 is lost, ticks 20
    // and 21 change places, and one line is something else.
    auto makeLog = [&] (bool withFaults)
    {
        std::vector<LogLine> log;
        int number = 0;
        auto add = [&] (int tick)
        {
            double deviation = 0.0;
            if (withFaults && tick % 10 == 0) deviation = 0.5;
            if (withFaults && tick == 150)    deviation = -2.0;
            log.push_back ({ ++number, sent[(size_t) tick].due + 123456.789 + deviation, "/maket/tick " + juce::String (tick) });
        };

        for (int tick = 0; tick < 200; ++tick)
        {
            if (withFaults && tick == 60) continue;
            if (withFaults && tick == 20) { add (21); add (20); ++tick; continue; }
            add (tick);
            if (tick == 5) log.push_back ({ ++number, 1.0, "noteon Cl 60. 100" });
        }
        return log;
    };

    const auto clean = compareMaketRun (passage, sent, makeLog (false));
    checkRow (clean.usable && clean.expected == 200 && clean.arrived == 200 && clean.outOfOrder == 0
              && close (clean.all.worst, 0.0, 1.0e-6) && close (clean.leftMaket.latest, 0.01, 1.0e-6) && close (clean.driftPerMinute, 0.0, 1.0e-3),
              "A log without faults, on a clock that runs two minutes ahead: every message exactly on time, whatever the clocks say");

    const auto faulty = compareMaketRun (passage, sent, makeLog (true));
    // 199 arrive; 18 of them 0.5 late (of the 20 tenth ticks, 60 is lost and 150 is the early one), one 2 early, the rest on time.
    checkRow (faulty.usable && faulty.arrived == 199 && faulty.outOfOrder == 1
              && close (faulty.all.latest, 0.5, 1.0e-6) && close (faulty.all.earliest, -2.0, 1.0e-6) && close (faulty.all.worst, 2.0, 1.0e-6)
              && close (faulty.all.half, 0.0, 1.0e-6) && close (faulty.all.most, 0.5, 1.0e-6)
              && faulty.perPart.size() == 2 && close (faulty.perPart[0].worst, 0.5, 1.0e-6) && close (faulty.perPart[1].worst, 2.0, 1.0e-6),
              "A log with a lost message, two that changed places, late ones and an early one: each fault is found and counted "
              "(199 of 200, 1 out of order, latest +0.50, earliest -2.00), per part of the passage too");

    // bach.roll's playout, note by note and chord by chord.
    Passage chords;
    chords.parts = { "chords" };
    for (int c = 0; c < 10; ++c)
        for (int k = 0; k < 3; ++k) chords.notes.push_back ({ 500.0 + 250.0 * c, 200.0, 60.0 + k, 0 });

    std::vector<LogLine> notewise, chordwise;
    int n = 0, m = 0;
    for (int c = 0; c < 10; ++c)
    {
        const double onset = 500.0 + 250.0 * c;
        const double time = 99000.0 + onset + (c == 4 ? 1.25 : 0.0);
        for (int k = 0; k < 3; ++k)
            notewise.push_back ({ ++n, time, "note 1 1 [ " + juce::String (onset, 1) + " [ " + juce::String (6000 + 100 * k) + ". 200. 90 0 ] 0 ]" });
        chordwise.push_back ({ ++m, time, "chord 1 1 [ " + juce::String (onset, 1) + " [ 6000. 200. 90 0 ] [ 6100. 200. 90 0 ] [ 6200. 200. 90 0 ] 0 ]" });
    }
    notewise.push_back ({ ++n, 105000.0, "end" });

    const auto byNote = compareBachTake (chords, notewise);
    const auto byChord = compareBachTake (chords, chordwise);
    checkRow (byNote.usable && byNote.mode == "notewise" && byNote.expected == 30 && byNote.arrived == 30 && close (byNote.all.latest, 1.25, 1.0e-6)
              && byChord.usable && byChord.mode == "chordwise" && byChord.expected == 10 && byChord.arrived == 10 && close (byChord.all.latest, 1.25, 1.0e-6)
              && hasBachLines (notewise) && ! hasBachLines (makeLog (false)),
              "bach.roll's playout is read note by note and chord by chord, each line against the onset it carries");

    const auto empty = compareMaketRun (passage, sent, {});
    checkRow (! empty.usable && empty.problem.isNotEmpty() && describe (empty, passage).startsWith ("No result"),
              "An empty log gives no numbers, and says why");
    line();
}

void testOnThisComputer()
{
    line ("### A short run on this computer");
    line();
    line ("The first three seconds of the dense passage go through the send thread to a listener inside the test program, "
          "which notes the arrival times on a clock of its own, as the patch does. This checks the whole chain of the "
          "measuring; the numbers say nothing about Max.");
    line();

    juce::DatagramSocket listener (false);
    if (! listener.bindToPort (0, "127.0.0.1"))
    {
        check (false, "A listener could be opened on this computer");
        line();
        return;
    }

    auto passage = densePassage();
    passage.notes.erase (std::remove_if (passage.notes.begin(), passage.notes.end(),
                                         [] (const PassageNote& n) { return n.onset >= 3500.0; }), passage.notes.end());

    std::vector<LogLine> log;
    std::vector<OscScheduler::Sent> sent;
    {
        OscScheduler scheduler;
        scheduler.setLog ("127.0.0.1", listener.getBoundPort());

        const double start = OscScheduler::now() + 200.0;
        for (size_t i = 0; i < passage.notes.size(); ++i)
        {
            scheduler.sendAt (start + passage.notes[i].onset, tickMessage ((int) i, false), OscScheduler::toLog);
            scheduler.sendAt (start + passage.notes[i].onset + passage.notes[i].duration, tickMessage ((int) i, true), OscScheduler::toLog);
        }

        const double clockOffset = 55555.5 - OscScheduler::now();      // the listener's clock has its own zero
        for (int i = 0; i < passage.numMessages(); ++i)
        {
            if (listener.waitUntilReady (true, 3000) != 1) break;
            juce::uint8 buffer[512];
            const int size = listener.read (buffer, (int) sizeof (buffer), false);
            const double arrival = OscScheduler::now() + clockOffset;

            OscMessage message;
            if (size > 0 && OscMessage::decode (buffer, (size_t) size, message))
                log.push_back ({ i + 1, arrival, message.toText() });
        }
        sent = scheduler.takeSent();
    }

    const auto result = compareMaketRun (passage, sent, log);

    line ("| Check | Passed |");
    line ("| --- | --- |");
    checkRow (result.usable && result.expected == passage.numMessages() && result.arrived == result.expected && result.outOfOrder == 0,
              "All " + juce::String (passage.numMessages()) + " messages are found in the listener's log, none out of order");
    checkRow (result.usable && result.all.most < 2.0,
              "95 in 100 arrive within 2 ms of their time (within " + ms (result.all.most) + " ms; the farthest " + ms (result.all.worst) + " ms)");
    line();
    line ("```");
    line (describe (result, passage).trimEnd());
    line ("```");
    line();
}

/** Joost's takes of 22 Nov, read from the logs in Temp/Timing 1d and
    compared here once more. For the logs he saved by hand (bach.roll's
    takes) this is the only place they are compared. */
void testTakes (const juce::File& folder)
{
    line ("### The takes of 22 Nov on Joost's Mac");
    line();

    if (! folder.isDirectory())
    {
        line ("(The folder `Temp/" + folder.getFileName() + "` is not there; nothing to show.)");
        line();
        return;
    }

    const auto dense = densePassage();
    const auto longOne = longPassage (5.0);

    auto files = folder.findChildFiles (juce::File::findFiles, false, "*");
    files.sort();

    struct Row { juce::String take, playedBy; maket::timing::Result result; };
    std::vector<Row> rows;

    for (const auto& file : files)
    {
        const auto name = file.getFileName();
        const bool byHand = name.startsWith ("test");
        if (! byHand && ! name.endsWith ("max log.txt")) continue;

        const auto log = readCollLog (file.loadFileAsString());
        const auto take = byHand ? name : name.upToLastOccurrenceOf (" max log.txt", false, false);

        const int highest = highestTick (log);
        if (highest >= 0)
            rows.push_back ({ take, "Maket", compareTicksToPassage (highest >= dense.numMessages() ? longOne : dense, log) });

        if (hasBachLines (log))
        {
            const auto result = compareBachTake (dense, log);
            if (result.usable && result.arrived == result.expected)        // a take bach.roll played to its end
                rows.push_back ({ take, "bach.roll", result });
        }
    }

    line ("Every log in `Temp/" + folder.getFileName() + "`, compared here again: how far the messages arrived in Max from their time, in ms. "
          "The conditions are in the names Joost gave the takes.");
    line();
    line ("| Take | Played by | Arrived | Half within | 95 in 100 within | 99 in 100 within | Farthest |");
    line ("| --- | --- | --- | --- | --- | --- | --- |");
    for (const auto& row : rows)
        line ("| " + row.take + " | " + row.playedBy + " | " + juce::String (row.result.arrived) + " of " + juce::String (row.result.expected)
              + " | " + ms (row.result.all.half) + " | " + ms (row.result.all.most) + " | " + ms (row.result.all.nearlyAll)
              + " | " + ms (row.result.all.worst) + " |");
    line();

    // Against the numbers worked out separately (a Python script, 22 Nov).
    auto find = [&rows] (const juce::String& start, const juce::String& playedBy) -> const maket::timing::Result*
    {
        for (const auto& row : rows)
            if (row.take.startsWith (start) && row.playedBy == playedBy) return &row.result;
        return nullptr;
    };
    auto is = [] (const maket::timing::Result* r, int arrived, double most, double worst)
    {
        return r != nullptr && r->arrived == arrived && close (r->all.most, most, 0.006) && close (r->all.worst, worst, 0.006);
    };

    line ("| Check | Passed |");
    line ("| --- | --- |");
    checkRow (is (find ("test2", "Maket"), 2806, 0.31, 3.24) && is (find ("test2", "bach.roll"), 1403, 0.15, 5.27)
              && is (find ("test5", "bach.roll"), 1403, 13.74, 14.09)
              && is (find ("20261122-200039", "Maket"), 6000, 38.88, 99.36) && is (find ("20261122-204355", "Maket"), 6000, 0.12, 0.64),
              "Five of these rows (takes 2, 5, 7 and 7d) give the numbers worked out separately");
    line();
}

} // namespace

void testTiming (const juce::File& temp)
{
    line ("## 6. Timing tests: the measuring");
    line();
    line ("Stage 1d measures on Joost's Mac how evenly Maket's messages arrive in Max. This section checks the measuring "
          "itself. A deviation is always: how far a message arrived from the time it was due, after the two clocks have "
          "been laid on each other at the middle message.");
    line();

    testPassages();
    testLogReading (temp);
    testComparing();
    testOnThisComputer();
    testTakes (temp.getChildFile ("Timing 1d"));
}
