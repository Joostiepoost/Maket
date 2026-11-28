// SurfaceTests.cpp — stage 3d: the build surface and its engine.
//
// Surfaces are made here with sounds whose every sample is known (a steady
// level, a count, a tone), turned into arrangements and played into memory.

#include <JuceHeader.h>
#include <cmath>
#include <map>
#include <vector>

#include "TestReport.h"
#include "../Source/Model/Project.h"
#include "../Source/Model/Surface.h"
#include "../Source/Sound/SurfaceEngine.h"

using namespace testreport;
using maket::Surface;

namespace
{
    const double rate = 48000.0;

    std::shared_ptr<const maket::Sound> made (int channels, int length, const std::function<float (int channel, int index)>& value)
    {
        auto s = std::make_shared<maket::Sound>();
        s->sampleRate = rate;
        s->samples.setSize (channels, length);
        for (int c = 0; c < channels; ++c)
            for (int i = 0; i < length; ++i) s->samples.setSample (c, i, value (c, i));
        return s;
    }

    struct Played { std::vector<float> left, right; };

    Played play (maket::SurfaceEngine& engine, double seconds, double deviceRate = rate, int block = 512)
    {
        Played p;
        const size_t total = (size_t) std::llround (seconds * deviceRate);
        p.left.assign (total, 0.0f);
        p.right.assign (total, 0.0f);
        for (size_t done = 0; done < total; done += (size_t) block)
            engine.render (p.left.data() + done, p.right.data() + done, (int) std::min ((size_t) block, total - done), deviceRate);
        return p;
    }
}

void testSurface (const juce::File& work)
{
    line ("## 15. The build surface and its engine (stage 3d)");
    line();
    line ("Blocks of sound on tracks, played many at once. The sounds here are made by the test program, so that every sample that should come out is known.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    // Three sounds: one that counts (sample i holds i / 100000), a steady level, a tone.
    std::map<juce::String, std::shared_ptr<const maket::Sound>> sounds;
    sounds["count"] = made (2, 48000, [] (int c, int i) { return (c == 0 ? 1.0f : -1.0f) * (float) i / 100000.0f; });
    sounds["steady"] = made (1, 96000, [] (int, int) { return 0.5f; });
    sounds["tone"] = made (1, 96000, [] (int, int i) { return (float) (0.5 * std::sin (2.0 * 3.14159265358979323846 * 1000.0 * i / rate)); });
    const auto soundOf = [&sounds] (const juce::String& id) { const auto f = sounds.find (id); return f != sounds.end() ? f->second : std::shared_ptr<const maket::Sound>(); };

    // --- The surface as a file ---
    {
        Surface s;
        const auto id1 = s.addTrack().id;       // (the id is taken at once: the next addTrack may move the list)
        const auto id2 = s.addTrack().id;
        s.tracks[0].name = "Strings"; s.tracks[0].gainDb = -3.5; s.tracks[0].pan = -0.25; s.tracks[1].solo = true;
        auto& p = s.addPlacement ("count", id1, 1500.0, 1000.0);
        p.offset = 100.0; p.length = 300.0; p.gainDb = -6.0; p.fadeIn = 20.0; p.fadeOut = 50.0;
        s.addPlacement ("tone", id2, 0.0, 2000.0);

        Surface back;
        bool ok = Surface::fromJson (s.toJson(), back).wasOk() && back.toJson() == s.toJson() && back.tracks.size() == 2 && back.placements.size() == 2
                  && back.tracks[0].name == "Strings" && back.tracks[1].solo && std::abs (back.placements[0].offset - 100.0) < 1.0e-9
                  && std::abs (back.length() - 2000.0) < 1.0e-9 && id1 != id2 && back.placements[0].id != back.placements[1].id;

        // In a project: kept in Surfaces/main.json; a project that has none gives an empty one.
        const auto folder = work.getChildFile ("surface test project");
        folder.deleteRecursively();
        maket::Project project;
        Surface none, loaded;
        ok = ok && maket::Project::create (folder, rate, 442.0, project).wasOk() && project.loadSurface (none).wasOk() && none.tracks.empty()
             && project.saveSurface (s).wasOk() && project.loadSurface (loaded).wasOk() && loaded.toJson() == s.toJson();
        folder.deleteRecursively();

        // A track taken away takes its blocks with it; a file that is no surface is refused.
        back.removeTrack (id1);
        Surface wrong;
        // A track added after one was taken away gets a number that no track has ("Track 2" is there: it becomes "Track 3").
        Surface named;
        named.addTrack(); named.addTrack();
        named.removeTrack (named.tracks[0].id);
        ok = ok && named.addTrack().name == "Track 3";
        ok = ok && back.tracks.size() == 1 && back.placements.size() == 1 && back.placements[0].snippetId == "tone" && Surface::fromJson ("{ \"x\": 1 }", wrong).failed();
        checkRow (ok, "A surface with two tracks and two blocks is written and read back the same (names, gain, pan, solo; each block's place, part, gain and fades), "
                      "also through a project; a track taken away takes its blocks with it; a new track never gets a name that is in use");
    }

    // --- One block, sample for sample ---
    {
        Surface s;
        const auto track = s.addTrack().id;
        auto& p = s.addPlacement ("count", track, 1000.0, 1000.0);
        p.offset = 100.0; p.length = 300.0;
        maket::SurfaceEngine engine;
        engine.setArrangement (maket::buildArrangement (s, rate, soundOf));
        engine.play (0.0);
        bool exact = true;
        for (int block : { 512, 37 })
        {
            engine.play (0.0);
            const auto out = play (engine, 1.6, rate, block);
            for (size_t i = 0; exact && i < out.left.size(); ++i)
            {
                const bool inside = i >= 48000 && i < 48000 + 14400;
                const float want = inside ? (float) (4800 + (i - 48000)) / 100000.0f : 0.0f;
                exact = juce::exactlyEqual (out.left[i], want) && juce::exactlyEqual (out.right[i], -want);
            }
        }
        // It plays on a quarter of a second past the last block, then stops by itself.
        const bool stops = ! engine.isPlaying();
        checkRow (exact && stops, "A block at 1 s that uses 300 ms of a sound from 100 ms in: every sample out is that sample of the sound, left and right, in blocks of 512 and of 37; "
                                  "silence before and after; the play ends by itself a quarter of a second after the last block");
    }

    // --- Levels: the block's gain, the track's gain, pan, mute and solo ---
    {
        auto levelOf = [&] (const std::function<void (Surface&)>& change, float& l, float& r)
        {
            Surface s;
            const auto a = s.addTrack().id;
            const auto b = s.addTrack().id;
            s.addPlacement ("steady", a, 0.0, 2000.0);
            s.addPlacement ("steady", b, 0.0, 2000.0).gainDb = -60.0;          // the second track is silent unless the change says otherwise
            change (s);
            maket::SurfaceEngine engine;
            engine.setArrangement (maket::buildArrangement (s, rate, soundOf));
            engine.play (0.0);
            const auto out = play (engine, 0.5);
            l = out.left[12000]; r = out.right[12000];
        };
        float l = 0, r = 0;
        bool ok = true;
        levelOf ([] (Surface&) {}, l, r);                                                   ok = ok && std::abs (l - 0.5f) < 1.0e-6f && std::abs (r - 0.5f) < 1.0e-6f;
        levelOf ([] (Surface& s) { s.placements[0].gainDb = -6.0206; }, l, r);              ok = ok && std::abs (l - 0.25f) < 1.0e-5f;
        levelOf ([] (Surface& s) { s.tracks[0].gainDb = 6.0206; }, l, r);                   ok = ok && std::abs (l - 1.0f) < 1.0e-4f;
        levelOf ([] (Surface& s) { s.tracks[0].pan = -1.0; }, l, r);                        ok = ok && std::abs (l - 0.5f) < 1.0e-6f && std::abs (r) < 1.0e-9f;
        levelOf ([] (Surface& s) { s.tracks[0].pan = 0.5; }, l, r);                         ok = ok && std::abs (l - 0.25f) < 1.0e-6f && std::abs (r - 0.5f) < 1.0e-6f;
        levelOf ([] (Surface& s) { s.tracks[0].mute = true; }, l, r);                       ok = ok && std::abs (l) < 1.0e-9f;
        levelOf ([] (Surface& s) { s.tracks[1].solo = true; }, l, r);                       ok = ok && std::abs (l) < 1.0e-9f;
        levelOf ([] (Surface& s) { s.tracks[0].solo = true; s.placements[1].gainDb = 0.0; }, l, r);   ok = ok && std::abs (l - 0.5f) < 1.0e-6f;
        levelOf ([] (Surface& s) { s.placements[1].gainDb = 0.0; }, l, r);                  ok = ok && std::abs (l - 1.0f) < 1.0e-6f;      // two tracks add up
        checkRow (ok, "Levels: a sound of one channel goes to both sides at its own level; a block at -6 dB is half; a track at +6 dB is double; pan hard left silences the right, "
                      "pan half right halves the left; a muted track is silent; with a track on solo only that one sounds; two tracks add up");
    }

    // --- Fades, and the crossing over of two blocks on one track ---
    {
        Surface s;
        const auto a = s.addTrack().id;
        auto& p = s.addPlacement ("steady", a, 0.0, 1000.0);
        p.fadeIn = 100.0; p.fadeOut = 200.0;
        maket::SurfaceEngine engine;
        engine.setArrangement (maket::buildArrangement (s, rate, soundOf));
        engine.play (0.0);
        auto out = play (engine, 1.1);
        const bool own = std::abs (out.left[2400] - 0.25f) < 2.0e-4f && std::abs (out.left[24000] - 0.5f) < 1.0e-6f
                         && std::abs (out.left[48000 - 4800] - 0.25f) < 2.0e-4f && std::abs (out.left[48000 - 1]) < 1.0e-4f;

        // Two blocks on one track, the second beginning 200 ms before the first ends.
        Surface x;
        const auto t = x.addTrack().id;
        x.addPlacement ("steady", t, 0.0, 1000.0);
        x.addPlacement ("steady", t, 800.0, 1000.0);
        const auto crossing = maket::buildArrangement (x, rate, soundOf);
        engine.setArrangement (crossing);
        engine.play (0.0);
        out = play (engine, 1.9);
        // At equal power: in the middle each is at 0.7071, and the squares of the two levels always add up to one.
        bool power = crossing->clips.size() == 2;
        for (int i = 0; power && i <= 9600; i += 480)
        {
            const double first = crossing->clips[0].fadeAt (38400.0 + i), second = crossing->clips[1].fadeAt ((double) i);
            power = std::abs (first * first + second * second - 1.0) < 1.0e-9;
        }
        const bool crossed = power && std::abs (out.left[24000] - 0.5f) < 1.0e-6f && std::abs (out.left[38400 + 4800] - 0.5f * 1.41421356f) < 1.0e-4f
                             && std::abs (out.left[72000] - 0.5f) < 1.0e-6f;

        // On two tracks they do not cross over: they add up.
        Surface y;
        const auto t1 = y.addTrack().id;
        const auto t2 = y.addTrack().id;
        y.addPlacement ("steady", t1, 0.0, 1000.0);
        y.addPlacement ("steady", t2, 800.0, 1000.0);
        engine.setArrangement (maket::buildArrangement (y, rate, soundOf));
        engine.play (0.0);
        out = play (engine, 1.0);
        checkRow (own && crossed && std::abs (out.left[38400 + 4800] - 1.0f) < 1.0e-6f,
                  "A block's own fades run in a straight line (half way up at half the fade). Two blocks on one track that overlap by 200 ms cross over at equal power: "
                  "the earlier goes out as the later comes in, each at 0.707 in the middle; on two tracks they simply add up");
    }

    // --- Loop, stop, a change while it plays, another device rate ---
    {
        Surface s;
        const auto a = s.addTrack().id;
        s.addPlacement ("tone", a, 0.0, 2000.0);
        maket::SurfaceEngine engine;
        engine.setArrangement (maket::buildArrangement (s, rate, soundOf));

        engine.play (0.0, true, 500.0, 700.0);
        auto out = play (engine, 1.5);
        float jump = 0.0f;
        for (size_t i = 1; i < out.left.size(); ++i) jump = std::max (jump, std::abs (out.left[i] - out.left[i - 1]));
        const bool looped = engine.isPlaying() && engine.positionMs() >= 500.0 && engine.positionMs() < 700.0 && jump < 0.08f;

        engine.stop();
        out = play (engine, 0.1);
        float after = 0.0f;
        for (size_t i = 400; i < out.left.size(); ++i) after = std::max (after, std::abs (out.left[i]));
        const bool stopped = ! engine.isPlaying() && juce::exactlyEqual (after, 0.0f);

        // The track is muted while it plays: heard at once.
        engine.play (0.0);
        play (engine, 0.2);
        s.tracks[0].mute = true;
        engine.setArrangement (maket::buildArrangement (s, rate, soundOf));
        out = play (engine, 0.1);
        float muted = 0.0f;
        for (float v : out.left) muted = std::max (muted, std::abs (v));
        s.tracks[0].mute = false;

        // Through a device at 96 kHz the tone is the tone.
        engine.setArrangement (maket::buildArrangement (s, rate, soundOf));
        engine.play (0.0);
        out = play (engine, 1.0, 96000.0);
        double signal = 0.0, error = 0.0;
        for (size_t i = 4000; i + 4000 < out.left.size(); ++i)
        {
            const double want = 0.5 * std::sin (2.0 * 3.14159265358979323846 * 1000.0 * (double) i / 96000.0);
            signal += want * want;
            error += (out.left[i] - want) * (out.left[i] - want);
        }
        const double clean = 10.0 * std::log10 (signal / std::max (error, 1.0e-30));
        checkRow (looped && stopped && juce::exactlyEqual (muted, 0.0f) && clean > 85.0,
                  "A loop of 200 ms goes round without a click; Stop ends in silence; a track muted while it plays falls silent at once; through a device at 96 kHz a tone "
                  "comes out as it is (" + juce::String (juce::roundToInt (clean)) + " dB clean)");
    }

    // --- What it costs ---
    {
        // 24 tracks, each with ten seconds of a stereo sound, all sounding at once.
        auto stereo = made (2, 480000, [] (int c, int i) { return (float) (0.02 * std::sin (2.0 * 3.14159265358979323846 * (200.0 + 50.0 * c) * i / rate)); });
        Surface s;
        for (int t = 0; t < 24; ++t)
        {
            const auto id = s.addTrack().id;
            s.tracks.back().pan = (t % 5 - 2) / 2.0;
            s.addPlacement ("stereo", id, 0.0, 10000.0);
        }
        maket::SurfaceEngine engine;
        engine.setArrangement (maket::buildArrangement (s, rate, [&stereo] (const juce::String&) { return stereo; }));
        double share[2] = { 0.0, 0.0 };
        const double deviceRates[2] = { 48000.0, 96000.0 };
        for (int r = 0; r < 2; ++r)
        {
            engine.play (0.0);
            const double before = juce::Time::getMillisecondCounterHiRes();
            const auto out = play (engine, 9.5, deviceRates[r]);
            share[r] = (juce::Time::getMillisecondCounterHiRes() - before) / 9500.0 * 100.0;
            engine.stop();
            play (engine, 0.1, deviceRates[r]);
        }
        checkRow (share[0] < 25.0 && share[1] < 80.0,
                  "24 tracks of stereo sound at once take " + juce::String (share[0], 1) + " % of real time through a device at the project's rate, and "
                  + juce::String (share[1], 1) + " % through a device at 96 kHz (where every sample is worked out anew)");
    }
    line();
}
