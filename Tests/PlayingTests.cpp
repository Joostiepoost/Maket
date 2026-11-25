// PlayingTests.cpp — stage 2d: what of a roll is played, from where to
// where, and the transport's reckoning.
//
// Whether it sounds right is settled on Joost's Mac with the ePlayer. What
// is checked here: which notes play (muted, solo), what a note that is
// entered in its middle becomes, what a loop's pass holds, and where the
// transport says the sound is.

#include <JuceHeader.h>
#include <cmath>
#include <limits>
#include <set>
#include <vector>

#include "TestReport.h"
#include "../Source/Model/Roll.h"
#include "../Source/Formats/BachRoll.h"
#include "../Source/Formats/LlllText.h"
#include "../Source/Engine/ConTimbreMapping.h"
#include "../Source/Engine/OscScheduler.h"
#include "../Source/Engine/Playing.h"
#include "../Source/Engine/Transport.h"

using namespace testreport;
using maket::Roll;
using maket::contimbre::TimedMessage;

namespace
{
    const double forever = std::numeric_limits<double>::max();

    maket::SlotValue slotOf (int number, const char* text)
    {
        maket::SlotValue s;
        s.number = number;
        maket::readLlllText (text, s.content);
        return s;
    }

    maket::Note noteOf (double cents, double duration, int velocity = 80)
    {
        maket::Note n;
        n.cents = cents; n.duration = duration; n.velocity = velocity;
        return n;
    }

    void addNote (maket::Voice& voice, double onset, const maket::Note& note, int chordFlags = 0)
    {
        maket::Chord chord;
        chord.onset = onset;
        chord.flags = chordFlags;
        chord.notes.push_back (note);
        voice.chords.push_back (chord);
    }

    maket::Breakpoint bp (double position, double cents, double slope = 0.0)
    {
        maket::Breakpoint b;
        b.position = position; b.cents = cents; b.slope = slope;
        return b;
    }

    bool has (const std::vector<TimedMessage>& messages, double time, const char* text)
    {
        for (const auto& m : messages)
            if (std::abs (m.time - time) < 1.0e-6 && m.message.toText() == juce::String::fromUTF8 (text)) return true;
        return false;
    }

    int count (const std::vector<TimedMessage>& messages, const char* command)
    {
        int n = 0;
        for (const auto& m : messages) if (m.message.address == command) ++n;
        return n;
    }

    juce::String listing (const std::vector<TimedMessage>& messages)
    {
        juce::StringArray lines;
        for (const auto& m : messages) lines.add (juce::String (m.time, 1) + "  " + m.message.toText());
        return lines.joinIntoString ("; ");
    }

    /** One long note with a glide, dynamics and gain, in a voice named Vc. */
    Roll longNoteRoll()
    {
        Roll roll;
        roll.voices.resize (1);
        roll.voices[0].name = "Vc";
        auto note = noteOf (6000.0, 4000.0, 80);
        note.breakpoints = { bp (0.0, 0.0), bp (0.5, 200.0), bp (1.0, 200.0) };
        note.slots.push_back (slotOf (1, "ordinario"));
        note.slots.push_back (slotOf (2, "[ 0 40 0 ] [ 2000 100 0 ] [ 4000 20 0 ]"));
        note.slots.push_back (slotOf (3, "[ 0 -6 0 ] [ 1000 -3 0 ] [ 3000 0 0 ]"));
        addNote (roll.voices[0], 1000.0, note);
        return roll;
    }
}

void testPlaying (const juce::File& temp)
{
    line ("## 9. Playing a roll: from a point, round a loop (stage 2d)");
    line();
    line ("The roll view plays through the same mapping as the conTimbre test page, by number. Before the mapping the roll is cut to what "
          "has to sound: muted and solo, the point play begins at, the end of a loop. Whether it sounds right is settled with the ePlayer.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    // --- Muted and solo ---
    {
        Roll roll;
        roll.voices.resize (3);
        for (auto& v : roll.voices) v.name = "v";
        addNote (roll.voices[0], 0.0, noteOf (6000, 500));
        auto muted = noteOf (6100, 500); muted.flags = maket::flagMuted;
        addNote (roll.voices[0], 1000.0, muted);
        addNote (roll.voices[0], 2000.0, noteOf (6200, 500), maket::flagMuted);
        addNote (roll.voices[1], 0.0, noteOf (6300, 500));
        roll.voices[1].flags = maket::flagMuted;
        addNote (roll.voices[2], 0.0, noteOf (6400, 500));

        const auto plain = maket::playing::audible (roll);
        const bool mutedOut = plain.numNotes() == 2 && plain.voices[0].chords.size() == 1 && plain.voices[1].chords.empty() && ! maket::playing::anySolo (roll);

        auto withSolo = roll;
        withSolo.voices[2].chords[0].notes[0].flags = maket::flagSolo;
        const auto solo = maket::playing::audible (withSolo);
        auto soloVoice = roll;
        soloVoice.voices[0].flags = maket::flagSolo;
        const auto voice = maket::playing::audible (soloVoice);
        auto both = roll;
        both.voices[2].chords[0].notes[0].flags = maket::flagSolo | maket::flagMuted;
        checkRow (mutedOut && solo.numNotes() == 1 && solo.voices[2].chords.size() == 1
                  && voice.numNotes() == 1 && voice.voices[0].chords.size() == 1 && maket::playing::audible (both).numNotes() == 0,
                  "Muted and solo, bach's rule: a muted note, a note of a muted chord and of a muted voice do not play; when a note or a voice is solo "
                  "only that plays, and what is muted within it still does not");
    }

    // --- A note entered in its middle ---
    {
        const auto roll = longNoteRoll();
        const auto cut = maket::playing::excerpt (roll, 2000.0, forever);
        const auto& chord = cut.voices[0].chords[0];
        const auto& note = chord.notes[0];
        const bool shape = std::abs (chord.onset - 2000.0) < 1.0e-9 && std::abs (note.duration - 3000.0) < 1.0e-9 && std::abs (note.cents - 6100.0) < 1.0e-9
                           && note.breakpoints.size() == 3 && std::abs (note.breakpoints[1].position - 1.0 / 3.0) < 1.0e-9
                           && std::abs (note.breakpoints[1].cents - 100.0) < 1.0e-9 && std::abs (note.breakpoints[2].cents - 100.0) < 1.0e-9;
        checkRow (shape, "A note of 4 s that glides up a tone in its first half, entered after 1 s: it begins there, lasts 3 s, starts a semitone higher "
                         "and still has a semitone to go, as bach cuts a partial note");

        int numbers = 0;
        const auto messages = maket::playing::messagesForPass (roll, 2000.0, forever, 1, numbers);
        const bool sent = has (messages, 2000.0, "program voice1 ordinario") && has (messages, 2000.0, "gain voice1 -3.") && has (messages, 4000.0, "gain voice1 0.")
                          && has (messages, 2000.0, "ctnote 1 1 61. 80 3000.") && has (messages, 5000.0, "ctnoteoff 1")
                          && has (messages, 2000.0, "ctglissando 1 1. 1000.") && has (messages, 3000.0, "ctglissando 1 1. 2000.")
                          && count (messages, "ctdynamic") == 3 && numbers == 1;
        // The dynamics: at once to where the curve is (70 of 127), then on to 100 and to 20.
        bool dynamics = false;
        {
            std::vector<const TimedMessage*> d;
            for (const auto& m : messages) if (m.message.address == "ctdynamic") d.push_back (&m);
            dynamics = d.size() == 3
                       && std::abs (d[0]->time - 2000.0) < 1.0e-6 && std::abs (d[0]->message.arguments[1].real - 70.0 / 127.0) < 1.0e-6 && std::abs (d[0]->message.arguments[2].real - 20.0) < 1.0e-6
                       && std::abs (d[1]->time - 2020.0) < 1.0e-6 && std::abs (d[1]->message.arguments[1].real - 100.0 / 127.0) < 1.0e-6 && std::abs (d[1]->message.arguments[2].real - 980.0) < 1.0e-6
                       && std::abs (d[2]->time - 3000.0) < 1.0e-6 && std::abs (d[2]->message.arguments[1].real - 20.0 / 127.0) < 1.0e-6 && std::abs (d[2]->message.arguments[2].real - 2000.0) < 1.0e-6;
        }
        if (! (sent && dynamics)) line ("| _sent: " + listing (messages) + "_ | |");
        checkRow (sent && dynamics, "Its commands: the program and the gain of that moment, the note at the pitch reached, the rest of the glide, and the dynamics: "
                                    "in 20 ms to the value the curve has there (70), then on to 100 and to 20 at their own times");

        const auto whole = maket::playing::messagesForPass (roll, 0.0, forever, 1, numbers);
        maket::contimbre::MappingOptions byNumber;
        byNumber.byNumber = true;
        const auto mapped = maket::contimbre::messagesForRoll (roll, {}, byNumber);
        bool same = whole.size() == mapped.size();
        for (size_t i = 0; same && i < whole.size(); ++i)
            same = std::abs (whole[i].time - mapped[i].time) < 1.0e-9 && whole[i].message.toText() == mapped[i].message.toText();
        const auto late = maket::playing::messagesForPass (roll, 5500.0, forever, 1, numbers);
        checkRow (same && count (late, "ctnote") == 0, "From the start the note is sent whole, exactly as the mapping gives it; from after its end it is not sent");
    }

    // --- The end of a loop ---
    {
        auto roll = longNoteRoll();
        addNote (roll.voices[0], 2500.0, noteOf (6700.0, 300.0));
        addNote (roll.voices[0], 3800.0, noteOf (7200.0, 300.0));         // begins after the loop's end

        int numbers = 0;
        const auto pass = maket::playing::messagesForPass (roll, 2000.0, 3500.0, 11, numbers);
        bool inside = true;
        std::set<int> on, off;
        for (const auto& m : pass)
        {
            if (m.time < 2000.0 - 1.0e-9 || m.time > 3500.0 + 1.0e-9) inside = false;
            if (m.message.address == "ctnote")      on.insert (m.message.arguments[1].integer);
            if (m.message.address == "ctnoteoff")   off.insert (m.message.arguments[0].integer);
        }
        const bool ended = has (pass, 2000.0, "ctnote 1 11 61. 80 1500.") && has (pass, 3500.0, "ctnoteoff 11")
                           && has (pass, 2500.0, "ctnote 1 12 67. 80 300.") && has (pass, 2800.0, "ctnoteoff 12");
        if (! (inside && ended)) line ("| _sent: " + listing (pass) + "_ | |");
        checkRow (inside && ended && on == off && numbers == 2 && on == std::set<int> ({ 11, 12 }),
                  "One pass of a loop from 2 s to 3.5 s: the long note enters at 2 s and is ended at 3.5 s, the short one plays whole, the note after the loop is left out; "
                  "nothing is sent outside the loop, every note is ended, and the notes carry the numbers the pass was given (11, 12)");

        const auto sounding = maket::playing::soundingAt (pass, 2600.0);
        const auto later = maket::playing::soundingAt (pass, 3000.0);
        checkRow (sounding == std::vector<int> ({ 11, 12 }) && later == std::vector<int> ({ 11 }) && maket::playing::soundingAt (pass, 3600.0).empty(),
                  "Which notes sound at a moment (for Stop, which ends each by its number): both at 2.6 s, the long one at 3 s, none after the loop's end");
    }

    // --- The transport's reckoning ---
    {
        maket::OscScheduler scheduler;
        maket::Transport transport (scheduler, 9);
        transport.setRoll (longNoteRoll());
        transport.setLoop (3000.0, 2000.0);         // given backwards: start and end are sorted
        transport.setLoopOn (true);

        const auto first = transport.nextPass (nullptr, 1500.0, 100.0);
        const auto second = transport.nextPass (&first, 1500.0, 100.0 + 1500.0);
        const auto beyond = transport.nextPass (nullptr, 3200.0, 100.0);
        const bool passes = std::abs (first.rollFrom - 1500.0) < 1.0e-9 && std::abs (first.rollTo - 3000.0) < 1.0e-9 && first.firstNumber == 1 && first.numbers == 1
                            && std::abs (second.rollFrom - 2000.0) < 1.0e-9 && std::abs (second.rollTo - 3000.0) < 1.0e-9 && second.firstNumber == 2
                            && beyond.rollTo >= forever;

        std::deque<maket::Transport::Pass> queue { first, second };
        const bool where = std::abs (maket::Transport::positionAt (queue, 50.0) - 1500.0) < 1.0e-9
                           && std::abs (maket::Transport::positionAt (queue, 600.0) - 2000.0) < 1.0e-9
                           && std::abs (maket::Transport::positionAt (queue, 1600.0) - 2000.0) < 1.0e-9
                           && std::abs (maket::Transport::positionAt (queue, 2100.0) - 2500.0) < 1.0e-9;
        checkRow (passes && where, "A play from 1.5 s with a loop from 2 s to 3 s: the first pass runs from 1.5 s to the loop's end, each later one round the loop with new note numbers; "
                                   "a play from beyond the loop runs to the end of the roll. The position follows from the clock: 1.5 s before the first note, back at 2 s when a pass begins");

        // The send thread forgets one user's waiting commands and keeps the others'.
        const double later = maket::OscScheduler::now() + 60000.0;
        scheduler.sendAt (later, maket::OscMessage ("one"), maket::OscScheduler::toPlayer, 9);
        scheduler.sendAt (later, maket::OscMessage ("two"), maket::OscScheduler::toPlayer, 3);
        scheduler.sendAt (later, maket::OscMessage ("three"), maket::OscScheduler::toPlayer, 9);
        scheduler.clear (9);
        const int left = scheduler.numWaiting();
        scheduler.clear();
        checkRow (left == 1, "Stop takes back the transport's own waiting commands and leaves what others have waiting");
    }

    // --- doodsbed ---
    {
        Roll roll;
        juce::StringArray remarks;
        if (maket::readBachRollFile (material (temp, "doodsbed final v1c.txt"), roll, remarks).failed())
        {
            checkRow (false, "doodsbed final v1c is in Temp and can be read");
        }
        else
        {
            int numbers = 0;
            const auto whole = maket::playing::messagesForPass (roll, 0.0, forever, 1, numbers);
            maket::contimbre::MappingOptions byNumber;
            byNumber.byNumber = true;
            const auto mapped = maket::contimbre::messagesForRoll (roll, {}, byNumber);
            bool same = whole.size() == mapped.size();
            for (size_t i = 0; same && i < whole.size(); ++i)
                same = std::abs (whole[i].time - mapped[i].time) < 1.0e-9 && whole[i].message.toText() == mapped[i].message.toText();
            checkRow (same, "doodsbed from the start: the roll view sends the same " + juce::String ((int) mapped.size())
                            + " commands at the same times as the conTimbre test page does by number");

            // From 60 s: every note that sounds there enters at 60 s.
            const auto playing = maket::playing::audible (roll);
            int soundingThere = 0, laterNotes = 0;
            for (const auto& v : playing.voices)
            {
                if (v.name.isEmpty()) continue;
                for (const auto& c : v.chords)
                    for (const auto& n : c.notes)
                    {
                        if (c.onset < 60000.0 && c.onset + n.duration > 60000.0) ++soundingThere;
                        if (c.onset >= 60000.0) ++laterNotes;
                    }
            }
            const auto from60 = maket::playing::messagesForPass (roll, 60000.0, forever, 1, numbers);
            int entering = 0;
            bool early = false;
            for (const auto& m : from60)
            {
                if (m.time < 60000.0 - 1.0e-9) early = true;
                if (m.message.address == "ctnote" && std::abs (m.time - 60000.0) < 1.0e-9 && m.noteOnset <= 60000.0 + 1.0e-9) ++entering;
            }
            int startingAt60 = 0;
            for (const auto& v : playing.voices)
                if (v.name.isNotEmpty())
                    for (const auto& c : v.chords)
                        if (std::abs (c.onset - 60000.0) < 1.0e-9) startingAt60 += (int) c.notes.size();
            checkRow (! early && soundingThere > 0 && entering == soundingThere + startingAt60 && count (from60, "ctnote") == soundingThere + laterNotes,
                      "doodsbed from 60 s: the " + juce::String (soundingThere) + " notes that sound there enter at 60 s, the " + juce::String (laterNotes)
                      + " later ones follow at their own times, and nothing is sent for an earlier moment");

            // A loop from 60 s to 70 s.
            const auto loop = maket::playing::messagesForPass (roll, 60000.0, 70000.0, 1001, numbers);
            bool inside = true;
            std::set<int> on, off;
            for (const auto& m : loop)
            {
                if (m.time < 60000.0 - 1.0e-9 || m.time > 70000.0 + 1.0e-9) inside = false;
                if (m.message.address == "ctnote")      on.insert (m.message.arguments[1].integer);
                if (m.message.address == "ctnoteoff")   off.insert (m.message.arguments[0].integer);
            }
            checkRow (inside && on == off && ! on.empty() && *on.begin() == 1001 && (int) on.size() == numbers,
                      "doodsbed, one pass of a loop from 60 s to 70 s: " + juce::String ((int) loop.size()) + " commands, all within the loop; each of its "
                      + juce::String (numbers) + " notes is ended by the loop's end");
        }
    }
    line();
}
