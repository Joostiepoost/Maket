// SynthTests.cpp — stage 2e: Maket's own sound.
//
// The synth is rendered into memory and measured: pitch (microtones, the
// standard pitch, a glide), the moment a note begins and ends, its level.
// Also: which engine a voice gets, what the transport hands to the synth,
// and the clock of the audio stream on made-up calls. How it sounds, and
// whether it is in time with the ePlayer, is settled on Joost's Mac.

#include <JuceHeader.h>
#include <cmath>
#include <limits>
#include <vector>

#include "TestReport.h"
#include "../Source/Model/Roll.h"
#include "../Source/Model/Settings.h"
#include "../Source/Formats/BachRoll.h"
#include "../Source/Formats/LlllText.h"
#include "../Source/Engine/OscScheduler.h"
#include "../Source/Engine/Playing.h"
#include "../Source/Engine/StreamClock.h"
#include "../Source/Engine/Synth.h"
#include "../Source/Engine/Transport.h"

using namespace testreport;
using maket::SynthNote;
using maket::playing::VoiceEngine;

namespace
{
    const double forever = std::numeric_limits<double>::max();
    const double rate = 48000.0;

    SynthNote tone (double start, double end, double cents, int velocity = 127)
    {
        SynthNote n;
        n.start = start; n.end = end; n.cents = cents; n.velocity = velocity;
        return n;
    }

    /** The synth's sound for these notes, from clock 0, in blocks of `block` samples. */
    std::vector<float> rendered (std::vector<SynthNote> notes, double seconds, int block = 512, double standardPitch = 442.0, double levelDb = -18.0,
                                 double offAt = -1.0)
    {
        maket::Synth synth;
        synth.prepare (rate);
        synth.setStandardPitch (standardPitch);
        synth.setLevel (levelDb);
        synth.addNotes (std::move (notes));

        const int total = (int) (seconds * rate);
        std::vector<float> out ((size_t) total, 0.0f);
        bool off = false;
        for (int done = 0; done < total; done += block)
        {
            const double clock = done * 1000.0 / rate;
            if (offAt >= 0.0 && ! off && clock >= offAt) { synth.allNotesOff(); off = true; }
            synth.render (out.data() + done, nullptr, std::min (block, total - done), clock);
        }
        return out;
    }

    /** The frequency between two moments (ms), from the upward crossings of zero. */
    double frequencyBetween (const std::vector<float>& x, double fromMs, double toMs)
    {
        const int a = (int) (fromMs * rate / 1000.0), b = std::min ((int) x.size() - 1, (int) (toMs * rate / 1000.0));
        double first = -1.0, last = -1.0;
        int crossings = 0;
        for (int i = std::max (1, a); i <= b; ++i)
            if (x[(size_t) i - 1] < 0.0f && x[(size_t) i] >= 0.0f)
            {
                const double t = (i - 1) + (double) -x[(size_t) i - 1] / ((double) x[(size_t) i] - (double) x[(size_t) i - 1]);
                if (first < 0.0) first = t;
                last = t;
                ++crossings;
            }
        return crossings >= 2 ? (crossings - 1) * rate / (last - first) : 0.0;
    }

    float peakBetween (const std::vector<float>& x, double fromMs, double toMs)
    {
        float peak = 0.0f;
        const int a = std::max (0, (int) (fromMs * rate / 1000.0)), b = std::min ((int) x.size(), (int) (toMs * rate / 1000.0));
        for (int i = a; i < b; ++i) peak = std::max (peak, std::abs (x[(size_t) i]));
        return peak;
    }

    // "Sound": more than a ten-millionth of full scale (-140 dB).
    int firstSound (const std::vector<float>& x)
    {
        for (size_t i = 0; i < x.size(); ++i) if (std::abs (x[i]) > 1.0e-7f) return (int) i;
        return -1;
    }

    int lastSound (const std::vector<float>& x)
    {
        for (size_t i = x.size(); i-- > 0;) if (std::abs (x[i]) > 1.0e-7f) return (int) i;
        return -1;
    }

    maket::Note& addNote (maket::Voice& voice, double onset, double cents, double duration, int velocity = 100)
    {
        maket::Chord chord;
        chord.onset = onset;
        maket::Note note;
        note.cents = cents; note.duration = duration; note.velocity = velocity;
        chord.notes.push_back (note);
        voice.chords.push_back (chord);
        return voice.chords.back().notes.back();
    }

    void setSlot (maket::Note& note, int number, const juce::String& content)
    {
        maket::SlotValue value;
        value.number = number;
        maket::readLlllText (content, value.content);
        note.slots.push_back (std::move (value));
    }

    maket::HeaderEntry entryOf (const juce::String& text)
    {
        maket::HeaderEntry entry;
        maket::Llll all;
        maket::readLlllText (text, all);
        entry.content = all.size() == 1 && all[0].isList() ? all[0] : all;
        entry.name = entry.content.head();
        return entry;
    }

    /** The roll for the ear and for a recording at stage 2e: a cello on conTimbre and the synth, together.
        `slotInfo`: Joost's slot definitions, from one of his rolls. */
    maket::Roll togetherRoll (const std::vector<maket::SlotInfo>& slotInfo)
    {
        maket::Roll roll;
        roll.header.push_back (entryOf ("[ clefs F G G ]"));
        roll.header.push_back (entryOf ("[ voicenames cello synth [ synth alone ] ]"));
        roll.header.push_back (entryOf ("[ midichannels 1 2 3 ]"));
        roll.slotInfo = slotInfo;
        roll.voices.resize (3);
        roll.voices[0].name = "cello";       roll.voices[0].clef = "F";
        roll.voices[1].name = "synth";       roll.voices[1].clef = "G";
        roll.voices[2].name = "synth alone"; roll.voices[2].clef = "G";
        auto& cello = roll.voices[0];
        auto& synth = roll.voices[1];
        auto& alone = roll.voices[2];

        // 1 to 8 s: eight short notes together, a plucked A on the cello and an E two octaves and a fifth higher on the synth.
        for (int i = 1; i <= 8; ++i)
        {
            setSlot (addNote (cello, 1000.0 * i, 5700.0, 300.0, 110), 1, "pizz");
            addNote (synth, 1000.0 * i, 8800.0, 150.0, 110);
        }
        // 10 to 16 s: the same A, bowed and on the synth, held: a difference in tuning is heard as beating.
        setSlot (addNote (cello, 10000.0, 5700.0, 6000.0, 90), 1, "ordinario");
        addNote (synth, 10000.0, 5700.0, 6000.0, 60);

        // 18 s: eighth tones up from the A, on the synth alone.
        for (int i = 0; i <= 8; ++i) addNote (alone, 18000.0 + 700.0 * i, 6900.0 + 25.0 * i, 600.0, 100);
        // 25 s: a glide up a fifth and back, with bach's curves.
        {
            auto& note = addNote (alone, 25000.0, 6900.0, 4000.0, 100);
            maket::Breakpoint head, top, back;
            top.position = 0.5; top.cents = 700.0; top.slope = 0.5;
            back.position = 1.0; back.cents = 0.0; back.slope = -0.5;
            note.breakpoints = { head, top, back };
        }
        // 30 s: a note that swells and fades by its modulation curve (slot 2, in ms).
        setSlot (addNote (alone, 30000.0, 6400.0, 4000.0, 127), 2, "[ 0. 0. 0. ] [ 2000. 127. 0. ] [ 4000. 0. 0. ]");
        // 35 s: eight notes at once.
        {
            maket::Chord chord;
            chord.onset = 35000.0;
            for (double c : { 4800.0, 5500.0, 6000.0, 6400.0, 6700.0, 7000.0, 7200.0, 7600.0 })
            {
                maket::Note note;
                note.cents = c; note.duration = 3000.0; note.velocity = 127;
                chord.notes.push_back (note);
            }
            alone.chords.push_back (chord);
        }
        return roll;
    }

    struct FakeSink : public maket::NoteSink
    {
        void addNotes (std::vector<SynthNote> notes) override   { for (auto& n : notes) got.push_back (n); }
        void allNotesOff() override                             { ++offs; }
        std::vector<SynthNote> got;
        int offs = 0;
    };
}

void testSynth (const juce::File& temp)
{
    line ("## 10. Maket's own sound: the first synth (stage 2e)");
    line();
    line ("A voice plays on conTimbre, on Maket's synth or not at all. The synth is one sine per note, timed on the clock of the ePlayer's commands. "
          "It is rendered into memory here and measured; how it sounds with the ePlayer is settled on the Mac.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    // --- The settings ---
    {
        using maket::Settings;
        juce::String tidied;
        const auto* level = Settings::find ("synthLevel");
        const auto* offset = Settings::find ("synthOffset");
        const auto* device = Settings::find ("audioOutput");
        bool ok = level != nullptr && offset != nullptr && device != nullptr;
        if (ok)
        {
            ok = Settings::validate (*level, "-12,5", tidied).wasOk() && tidied == "-12.5"
                 && Settings::validate (*level, "-18.0", tidied).wasOk() && tidied == "-18"
                 && Settings::validate (*level, "7", tidied).failed() && Settings::validate (*level, "loud", tidied).failed()
                 && Settings::validate (*offset, "15", tidied).wasOk() && tidied == "15"
                 && Settings::validate (*offset, "-7.5", tidied).wasOk() && tidied == "-7.5"
                 && Settings::validate (*offset, "250", tidied).failed() && Settings::validate (*offset, "1-2", tidied).failed()
                 && Settings::validate (*device, "", tidied).wasOk() && tidied.isEmpty()
                 && Settings::validate (*device, " MacBook Pro Speakers ", tidied).wasOk() && tidied == "MacBook Pro Speakers";
        }
        checkRow (ok, "The three new settings: the audio output (not set means the Mac's default output), the synth's level (-60 to 6 dB, default -18) "
                      "and the synth's timing against the ePlayer (-200 to 200 ms, default 0); a comma is read as a point, a wrong value is refused");
    }

    // --- Pitch ---
    {
        struct Pitch { double cents, standard, hz; };
        const Pitch pitches[] = { { 6900, 442, 442.0 }, { 6900, 440, 440.0 }, { 6950, 442, 442.0 * std::pow (2.0, 50.0 / 1200.0) },
                                  { 6925, 442, 442.0 * std::pow (2.0, 25.0 / 1200.0) }, { 6000, 442, 442.0 * std::pow (2.0, -900.0 / 1200.0) },
                                  { 8137, 442, 442.0 * std::pow (2.0, 1237.0 / 1200.0) }, { 3600, 442, 442.0 * std::pow (2.0, -3300.0 / 1200.0) } };
        bool ok = true;
        double worst = 0.0;
        for (const auto& p : pitches)
        {
            const auto x = rendered ({ tone (0.0, 1500.0, p.cents) }, 1.5, 512, p.standard);
            const double cents = 1200.0 * std::log2 (frequencyBetween (x, 100.0, 1400.0) / p.hz);
            worst = std::max (worst, std::abs (cents));
            ok = ok && std::abs (cents) < 0.05;
        }
        checkRow (ok, "Pitch: the A is 442 Hz, and 440 Hz when the standard pitch is 440; a quarter tone and an eighth tone above it, middle C, a pitch off the grid "
                      "and a low C are right within 0.05 cent (furthest off: " + juce::String (worst, 3) + " cent)");

        // A glide: two semitones up in a straight line over 2 s; and bach's curve.
        auto straight = tone (0.0, 2000.0, 6900.0);
        straight.glide = { { 0.0, 0.0, 0.0 }, { 1.0, 200.0, 0.0 } };
        const auto g = rendered ({ straight }, 2.0);
        const double atStart = 1200.0 * std::log2 (frequencyBetween (g, 80.0, 120.0) / 442.0);
        const double atHalf = 1200.0 * std::log2 (frequencyBetween (g, 980.0, 1020.0) / 442.0);
        const double atEnd = 1200.0 * std::log2 (frequencyBetween (g, 1880.0, 1920.0) / 442.0);
        auto curved = straight;
        curved.glide = { { 0.0, 0.0, 0.0 }, { 1.0, 200.0, 0.5 } };
        const auto c = rendered ({ curved }, 2.0);
        const double curvedHalf = 1200.0 * std::log2 (frequencyBetween (c, 980.0, 1020.0) / 442.0);
        const bool glideOk = std::abs (atStart - 10.0) < 0.5 && std::abs (atHalf - 100.0) < 0.5 && std::abs (atEnd - 190.0) < 0.5 && std::abs (curvedHalf - 25.0) < 1.0;
        checkRow (glideOk, "A glide of two semitones over 2 s: " + juce::String (atStart, 1) + ", " + juce::String (atHalf, 1) + " and " + juce::String (atEnd, 1)
                           + " cents above the A after 0.1, 1 and 1.9 s (10, 100, 190); with slope 0.5 the pitch is " + juce::String (curvedHalf, 1)
                           + " cents up at half time (25: an eighth of the way, bach's curve)");
    }

    // --- Time ---
    {
        bool ok = true;
        juce::String found;
        for (int block : { 512, 64, 37, 2048 })
        {
            const auto x = rendered ({ tone (10.0, 110.0, 6900.0) }, 0.3, block);
            const int first = firstSound (x), last = lastSound (x);
            // The note begins at 10 ms, which is sample 480: the first sample with sound is 481. It is over 50 ms after
            // its end at 110 ms, sample 7680; loudness is read every 16 samples, so the very end may come a third of a ms late.
            ok = ok && first == 481 && last <= 7680 + 16 && last >= 7600;
            found += juce::String (block) + ": " + juce::String (first) + " to " + juce::String (last) + "; ";
        }
        checkRow (ok, "Time: a note from 10 ms to 110 ms has its first sound at sample 481 of 48000 a second, exactly, and is silent 50 ms after its end (within a third of a ms), whatever the size of the blocks "
                      "(blocks of " + found.trimEnd().dropLastCharacters (1) + ")");

        const auto a = rendered ({ tone (5.0, 400.0, 6000.0), tone (100.0, 300.0, 6700.0, 80) }, 0.5, 512);
        const auto b = rendered ({ tone (5.0, 400.0, 6000.0), tone (100.0, 300.0, 6700.0, 80) }, 0.5, 96);
        float difference = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) difference = std::max (difference, std::abs (a[i] - b[i]));
        checkRow (difference < 2.0e-3f, "The same notes in blocks of 512 and of 96 samples give the same sound (largest difference "
                                        + juce::String (20.0 * std::log10 (std::max (1.0e-9f, difference)), 0) + " dB)");
    }

    // --- Level ---
    {
        const float full = peakBetween (rendered ({ tone (0.0, 400.0, 6900.0, 127) }, 0.4), 100.0, 300.0);
        const float half = peakBetween (rendered ({ tone (0.0, 400.0, 6900.0, 64) }, 0.4), 100.0, 300.0);
        const float louder = peakBetween (rendered ({ tone (0.0, 400.0, 6900.0, 127) }, 0.4, 512, 442.0, -12.0), 100.0, 300.0);
        const auto attack = rendered ({ tone (0.0, 400.0, 6900.0, 127) }, 0.4);
        const float early = peakBetween (attack, 4.0, 6.0);

        auto shaped = tone (0.0, 1000.0, 6900.0, 127);
        shaped.levels = { { 0.0, 0.0, 0.0 }, { 500.0, 127.0, 0.0 }, { 1000.0, 63.5, 0.0 } };
        const auto s = rendered ({ shaped }, 1.1);
        const float quarter = peakBetween (s, 245.0, 255.0), top = peakBetween (s, 495.0, 505.0), late = peakBetween (s, 745.0, 755.0);

        const double expected = std::pow (10.0, -18.0 / 20.0);
        const bool ok = std::abs (full - expected) < 0.002 && std::abs (half / full - 64.0 / 127.0) < 0.01 && std::abs (louder / full - 2.0) < 0.02
                        && early > 0.35f * full && early < 0.65f * full
                        && std::abs (quarter / full - 0.5) < 0.03 && std::abs (top / full - 1.0) < 0.02 && std::abs (late / full - 0.75) < 0.03;
        checkRow (ok, "Level: a note at full velocity peaks at -18 dB under full scale (the setting), velocity 64 at half of that; half way up the attack of 10 ms after 5 ms; "
                      "a modulation curve from 0 to 127 and back to 63.5 gives half, all and three quarters of the level at its quarter, middle and three quarters");

        std::vector<SynthNote> many;
        for (int i = 0; i < 12; ++i) many.push_back (tone (0.0, 300.0, 6000.0 + 100.0 * i, 127));
        const float crowd = peakBetween (rendered (many, 0.3), 0.0, 300.0);
        const auto stopped = rendered ({ tone (0.0, 2000.0, 6900.0) }, 0.6, 512, 442.0, -18.0, 200.0);
        const int silentFrom = lastSound (stopped);
        checkRow (crowd < 1.0f && crowd > 0.8f && silentFrom > (int) (0.2 * rate) && silentFrom < (int) (0.27 * rate),
                  "Twelve full notes together stay under full scale (the top is rounded off, peak " + juce::String (crowd, 2)
                  + "); Stop ends a sounding note within its release (silent " + juce::String (silentFrom * 1000.0 / rate - 200.0, 0) + " ms after a stop at 200 ms)");
    }

    // --- The clock of the audio stream ---
    {
        maket::StreamClock clock;
        const double length = 512 * 1000.0 / rate;

        // With the device's own time: taken as it is, plus the device's delay.
        std::uint64_t device = 5'000'123'000'000ull;
        const double fromDevice = clock.blockTime (5000120.0, 512, rate, &device, 4.5);
        const bool deviceOk = std::abs (fromDevice - (5000123.0 + 4.5)) < 1.0e-6 && clock.usesDeviceTime();

        // Without it: calls that come up to 4 ms early or late; the blocks still follow each other evenly.
        clock.reset();
        juce::Random random (7);
        double previous = 0.0, worstStep = 0.0, worstOff = 0.0;
        for (int i = 0; i < 400; ++i)
        {
            const double ideal = 1000.0 + i * length;
            const double t = clock.blockTime (ideal + (random.nextDouble() * 8.0 - 4.0), 512, rate, nullptr, 3.0);
            if (i > 0) worstStep = std::max (worstStep, std::abs ((t - previous) - length));
            if (i > 50) worstOff = std::max (worstOff, std::abs (t - (ideal + 3.0)));
            previous = t;
        }
        // A break of half a second: the count starts anew.
        const double after = clock.blockTime (1000.0 + 400 * length + 500.0, 512, rate, nullptr, 3.0);
        const bool callsOk = worstStep < 0.2 && worstOff < 4.5 && std::abs (after - (1000.0 + 400 * length + 500.0 + 3.0)) < 1.0e-6 && ! clock.usesDeviceTime();
        checkRow (deviceOk && callsOk, "When a block is heard: the device's own time plus its delay where the device gives one (the Mac); otherwise counted on from block to block, "
                                       "even when the calls come up to 4 ms early or late (largest step off " + juce::String (worstStep, 2) + " ms), and anew after a break");
    }

    // --- Which engine a voice gets, and what each engine is handed ---
    {
        maket::Roll roll;
        juce::StringArray remarks;
        if (maket::readBachRollFile (material (temp, "doodsbed final v1c.txt"), roll, remarks).failed())
        {
            checkRow (false, "doodsbed final v1c is in Temp and can be read");
        }
        else
        {
            const auto engines = maket::playing::defaultEngines (roll);
            bool expected = engines.size() == 8 && engines[0] == VoiceEngine::silent && engines[1] == VoiceEngine::silent;
            for (size_t v = 2; expected && v < 8; ++v) expected = engines[v] == VoiceEngine::conTimbre;

            int numbers = 0;
            const auto all = maket::playing::messagesForPass (roll, 0.0, forever, 1, numbers);
            const auto split = maket::playing::messagesForPass (roll, engines, 0.0, forever, 1, numbers);
            int mezzo = 0, others = 0;
            for (const auto& m : all)   { if (m.voice == 0) ++mezzo; else ++others; }      // voices count from 0 here
            bool noMezzo = true;
            for (const auto& m : split) if (m.voice == 0) noMezzo = false;
            checkRow (expected && noMezzo && (int) split.size() == others && mezzo > 0
                      && maket::playing::synthNotesForPass (roll, engines, 0.0, forever).empty(),
                      "doodsbed: its six instruments name techniques and play on conTimbre, on the same ePlayer voices as before; the Mezzo (no technique, no clef) "
                      "and the voice without a name are silent (" + juce::String (mezzo) + " commands fewer than at 2d, which went to a voice the orchestra does not have)");

            auto withSynth = engines;
            withSynth[0] = VoiceEngine::synth;
            const auto notes = maket::playing::synthNotesForPass (roll, withSynth, 0.0, forever);
            const auto late = maket::playing::synthNotesForPass (roll, withSynth, 60000.0, 70000.0);
            bool inside = true;
            for (const auto& n : late) inside = inside && n.start >= 60000.0 - 1.0e-9 && n.end <= 70000.0 + 1.0e-9 && n.end > n.start;
            checkRow ((int) notes.size() == (int) roll.voices[0].numNotes() && inside,
                      "With the Mezzo set to the synth, its " + juce::String ((int) notes.size()) + " notes go to the synth; in a loop from 60 s to 70 s every synth note lies within the loop");
        }

        maket::Roll mixed;
        if (maket::readBachRollFile (material (temp, "mixed_roll_example_01h.txt"), mixed, remarks).wasOk())
        {
            const auto engines = maket::playing::defaultEngines (mixed);
            int conTimbre = 0, synthVoices = 0, silent = 0;
            for (auto e : engines) { if (e == VoiceEngine::conTimbre) ++conTimbre; else if (e == VoiceEngine::synth) ++synthVoices; else ++silent; }
            const auto notes = maket::playing::synthNotesForPass (mixed, engines, 0.0, forever);
            int gliding = 0, shaped = 0;
            for (const auto& n : notes) { if (! n.glide.empty()) ++gliding; if (! n.levels.empty()) ++shaped; }
            juce::StringArray onConTimbre;
            for (size_t v = 0; v < engines.size(); ++v) if (engines[v] == VoiceEngine::conTimbre) onConTimbre.add (mixed.voices[v].name);
            checkRow (onConTimbre.joinIntoString (" ") == "L1a L2a" && synthVoices == 13 && silent == 3 && ! notes.empty() && gliding > 0 && shaped > 0,
                      "The mixed example: " + juce::String (conTimbre) + " voices on conTimbre (" + onConTimbre.joinIntoString (", ")
                      + ": their notes name a technique in words; the numbers on the iac voices are not one), " + juce::String (synthVoices) + " on the synth, "
                      + juce::String (silent) + " silent (the percussion clefs); the synth gets " + juce::String ((int) notes.size()) + " notes, "
                      + juce::String (gliding) + " with a glide and " + juce::String (shaped) + " with a modulation curve");
        }
        else checkRow (false, "mixed_roll_example_01h is in Temp and can be read");
    }

    // --- The roll for the ear and for a recording ---
    {
        maket::Roll mixed;
        juce::StringArray remarks;
        std::vector<maket::SlotInfo> slotInfo;
        if (maket::readBachRollFile (material (temp, "mixed_roll_example_01h.txt"), mixed, remarks).wasOk()) slotInfo = mixed.slotInfo;

        const auto roll = togetherRoll (slotInfo);
        const auto folder = temp.getChildFile ("written by Maket");
        const auto file = folder.getChildFile ("Synth and conTimbre by Maket.txt");
        const bool written = temp.isDirectory() && folder.createDirectory().wasOk() && maket::writeBachRollFile (file, roll).wasOk();

        maket::Roll back;
        const bool same = written && maket::readBachRollFile (file, back, remarks).wasOk() && back.voices.size() == 3 && back.numNotes() == roll.numNotes()
                          && maket::writeBachRollText (back) == maket::writeBachRollText (roll) && back.voices[2].name == "synth alone";
        bool split = false;
        int forSynth = 0, commands = 0;
        if (same)
        {
            const auto engines = maket::playing::defaultEngines (back);
            split = engines.size() == 3 && engines[0] == VoiceEngine::conTimbre && engines[1] == VoiceEngine::synth && engines[2] == VoiceEngine::synth;
            int numbers = 0;
            const auto messages = maket::playing::messagesForPass (back, engines, 0.0, forever, 1, numbers);
            for (const auto& m : messages) if (m.message.address == "ctnote") ++commands;
            forSynth = (int) maket::playing::synthNotesForPass (back, engines, 0.0, forever).size();
        }
        checkRow (same && split && commands == 9 && forSynth == 28 && ! slotInfo.empty(),
                  "`Temp/written by Maket/Synth and conTimbre by Maket.txt` is written and, read back, holds the same: a cello on conTimbre (" + juce::String (commands)
                  + " notes: eight plucked, one bowed) with the synth on the same moments, then the synth alone (eighth tones, a glide, a swell, eight notes at once; "
                  + juce::String (forSynth) + " synth notes in all)");
    }

    // --- The transport hands both engines the same moments ---
    {
        maket::Roll roll;
        roll.voices.resize (2);
        roll.voices[0].name = "a"; roll.voices[0].clef = "G";
        roll.voices[1].name = "b"; roll.voices[1].clef = "G";
        for (int v = 0; v < 2; ++v)
        {
            maket::Chord chord;
            chord.onset = 3000.0;
            maket::Note note;
            note.cents = 6000.0 + 700.0 * v; note.duration = 500.0; note.velocity = 90;
            chord.notes.push_back (note);
            roll.voices[(size_t) v].chords.push_back (chord);
        }

        maket::OscScheduler scheduler;
        maket::Transport transport (scheduler, 9);
        FakeSink sink;
        transport.setRoll (roll);
        transport.setEngines ({ VoiceEngine::conTimbre, VoiceEngine::synth });
        transport.setSynth (&sink);
        const auto pass = transport.nextPass (nullptr, 2500.0, 100000.0);
        const auto onClock = maket::Transport::onClock (pass, 12.0);
        double noteOn = -1.0;
        for (const auto& m : pass.messages)
            if (m.message.address == "ctnote") noteOn = pass.clockStart + (m.time - pass.rollFrom);
        const bool together = onClock.size() == 1 && std::abs (noteOn - 100500.0) < 1.0e-9
                              && std::abs (onClock[0].start - (100500.0 + 12.0)) < 1.0e-9 && std::abs (onClock[0].end - (101000.0 + 12.0)) < 1.0e-9
                              && std::abs (onClock[0].cents - 6700.0) < 1.0e-9 && onClock[0].velocity == 90;
        checkRow (together, "Two notes that begin together, one on conTimbre and one on the synth: the command of the first is timed 500 ms after the start of play, "
                            "the synth's note at the same moment, moved by the timing setting (here 12 ms)");

        // A real play and stop, with a stand-in for the synth.
        juce::String problem;
        transport.setSynthOffset (0.0);
        const double before = maket::OscScheduler::now();
        const bool began = transport.play (2500.0, 442.0, problem);
        const bool handed = began && sink.got.size() == 1
                            && sink.got[0].start >= before + maket::Transport::leadTime + 500.0 - 1.0e-6
                            && sink.got[0].start <= maket::OscScheduler::now() + maket::Transport::leadTime + 500.0 + 1.0e-6
                            && std::abs ((sink.got[0].end - sink.got[0].start) - 500.0) < 1.0e-6;
        transport.stop();
        // Only synth voices: play still begins, and nothing is left for the ePlayer but the standard pitch.
        transport.setEngines ({ VoiceEngine::silent, VoiceEngine::synth });
        const auto synthOnly = transport.nextPass (nullptr, 0.0, 100000.0);
        const bool begins = transport.play (0.0, 442.0, problem);
        transport.stop();
        transport.setEngines ({ VoiceEngine::silent, VoiceEngine::silent });
        const bool refuses = ! transport.play (0.0, 442.0, problem) && problem.isNotEmpty();
        checkRow (handed && sink.offs == 2 && begins && synthOnly.messages.empty() && synthOnly.synthNotes.size() == 1 && refuses,
                  "Play hands the synth its note on the clock of the commands (120 ms ahead, as for the ePlayer) and Stop silences the synth; a roll with only synth voices "
                  "plays without a single note command to the ePlayer; with every voice silent, play says that nothing plays");
    }
    line();
}
