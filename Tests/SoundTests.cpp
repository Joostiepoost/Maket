// SoundTests.cpp — stage 3c: sound files in the library.
//
// Sound files are made here (plain tones, so that everything can be reckoned),
// imported into a project, read back and played into memory: the change of
// rate, the copy without loss, the audio snippet in the project, and the
// player sample for sample. How it sounds is for Joost's ears.

#include <JuceHeader.h>
#include <cmath>
#include <vector>

#include "TestReport.h"
#include "../Source/Model/Project.h"
#include "../Source/Sound/Resampler.h"
#include "../Source/Sound/Sound.h"
#include "../Source/Sound/SoundPlayer.h"

using namespace testreport;

namespace
{
    const double pi = 3.14159265358979323846;

    std::vector<float> tone (double hz, double rate, double seconds, double level, double phase = 0.0)
    {
        std::vector<float> x ((size_t) std::llround (rate * seconds));
        for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (level * std::sin (2.0 * pi * hz * (double) i / rate + phase));
        return x;
    }

    /** How far `x` lies from a pure tone of that frequency, level and phase, in dB under the tone; the ends left out. */
    double cleanliness (const std::vector<float>& x, double hz, double rate, double level, double phase = 0.0, size_t skip = 2000)
    {
        double signal = 0.0, error = 0.0;
        for (size_t i = skip; i + skip < x.size(); ++i)
        {
            const double want = level * std::sin (2.0 * pi * hz * (double) i / rate + phase);
            signal += want * want;
            error += ((double) x[i] - want) * ((double) x[i] - want);
        }
        return 10.0 * std::log10 (signal / std::max (error, 1.0e-30));
    }

    double levelDb (const std::vector<float>& x, size_t skip = 2000)
    {
        double sum = 0.0; size_t n = 0;
        for (size_t i = skip; i + skip < x.size(); ++i) { sum += (double) x[i] * x[i]; ++n; }
        return 10.0 * std::log10 (std::max (sum / (double) std::max<size_t> (1, n), 1.0e-30));
    }

    bool writeWav (const juce::File& file, const std::vector<std::vector<float>>& channels, double rate, int bits)
    {
        file.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> stream (std::make_unique<juce::FileOutputStream> (file));
        auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (rate).withNumChannels ((int) channels.size()).withBitsPerSample (bits));
        if (writer == nullptr) return false;
        juce::AudioBuffer<float> buffer ((int) channels.size(), (int) channels[0].size());
        for (size_t c = 0; c < channels.size(); ++c) buffer.copyFrom ((int) c, 0, channels[c].data(), (int) channels[c].size());
        return writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
    }

    /** Plays into memory: `seconds` at `deviceRate`, in blocks of `block`. */
    void run (maket::SoundPlayer& player, std::vector<float>& left, std::vector<float>& right, double seconds, double deviceRate, int block)
    {
        const size_t before = left.size(), more = (size_t) std::llround (seconds * deviceRate);
        left.resize (before + more, 0.0f);
        right.resize (before + more, 0.0f);
        for (size_t done = 0; done < more; done += (size_t) block)
            player.render (left.data() + before + done, right.data() + before + done, (int) std::min ((size_t) block, more - done), deviceRate);
    }
}

void testSound (const juce::File& work)
{
    line ("## 14. Sound files in the library (stage 3c)");
    line();
    line ("A sound file is copied into the project without loss, at the project's sample rate, and played from memory. The files here are plain tones made by the test program.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    // --- The change of rate ---
    {
        const auto in441 = tone (1000.0, 44100.0, 1.0, 0.5);
        const auto up = maket::Resampler::convert (in441.data(), (int) in441.size(), 44100.0, 48000.0);
        const double upClean = cleanliness (up, 1000.0, 48000.0, 0.5);

        const auto in48 = tone (15000.0, 48000.0, 1.0, 0.5);
        const auto doubled = maket::Resampler::convert (in48.data(), (int) in48.size(), 48000.0, 96000.0);
        const double doubledClean = cleanliness (doubled, 15000.0, 96000.0, 0.5);

        const auto low = tone (10000.0, 96000.0, 1.0, 0.5);
        const auto high = tone (30000.0, 96000.0, 1.0, 0.5);
        const auto lowDown = maket::Resampler::convert (low.data(), (int) low.size(), 96000.0, 48000.0);
        const auto highDown = maket::Resampler::convert (high.data(), (int) high.size(), 96000.0, 48000.0);
        const double downClean = cleanliness (lowDown, 10000.0, 48000.0, 0.5);
        const double folded = levelDb (highDown) - levelDb (lowDown);

        std::vector<float> steady (20000, 0.25f);
        const auto steadyOut = maket::Resampler::convert (steady.data(), (int) steady.size(), 44100.0, 96000.0);
        float off = 0.0f;
        for (size_t i = 2000; i + 2000 < steadyOut.size(); ++i) off = std::max (off, std::abs (steadyOut[i] - 0.25f));

        checkRow (up.size() == 48000 && doubled.size() == 96000 && lowDown.size() == 48000 && upClean > 85.0 && doubledClean > 85.0 && downClean > 85.0 && folded < -85.0 && off < 2.0e-6f,
                  "The change of rate: a tone of 1 kHz from 44.1 to 48 kHz comes out as that tone, what is not the tone lying " + juce::String (juce::roundToInt (upClean))
                  + " dB under it; 15 kHz from 48 to 96 kHz: " + juce::String (juce::roundToInt (doubledClean)) + " dB; 10 kHz from 96 down to 48 kHz: "
                  + juce::String (juce::roundToInt (downClean)) + " dB; a tone of 30 kHz, which 48 kHz cannot hold, is taken out (" + juce::String (juce::roundToInt (folded))
                  + " dB) instead of folding back; a steady level stays what it was; the lengths are right to the sample");
    }

    const auto folder = work.getChildFile ("sound test project");
    folder.deleteRecursively();
    maket::Project project;
    const bool made = maket::Project::create (folder, 48000.0, 442.0, project).wasOk();

    // --- Import: a copy without loss ---
    const auto stereoFile = work.getChildFile ("test stereo 48.wav"), monoFile = work.getChildFile ("test mono 441.wav");
    const auto leftTone = tone (440.0, 48000.0, 0.5, 0.4), rightTone = tone (660.0, 48000.0, 0.5, 0.3);
    juce::String stereoId, monoId;
    {
        juce::StringArray remarks;
        bool ok = made && writeWav (stereoFile, { leftTone, rightTone }, 48000.0, 16) && writeWav (monoFile, { tone (1000.0, 44100.0, 1.0, 0.5) }, 44100.0, 24);
        ok = ok && project.importSound (stereoFile, stereoId, remarks).wasOk() && project.importSound (monoFile, monoId, remarks).wasOk();

        // The stereo file has the project's rate: the copy holds the file's own numbers, every one.
        maket::Sound original, copy, mono;
        bool exact = ok && maket::readSoundFile (stereoFile, original).wasOk() && maket::readSoundFile (project.soundFile (stereoId), copy).wasOk()
                     && copy.numChannels() == 2 && copy.numSamples() == original.numSamples() && juce::approximatelyEqual (copy.sampleRate, 48000.0);
        for (int c = 0; exact && c < 2; ++c)
            for (int i = 0; exact && i < copy.numSamples(); ++i)
                exact = juce::exactlyEqual (copy.samples.getSample (c, i), original.samples.getSample (c, i));

        // The mono file has another rate: one channel, at 48 kHz, the same tone.
        bool converted = ok && maket::readSoundFile (project.soundFile (monoId), mono).wasOk() && mono.numChannels() == 1 && mono.numSamples() == 48000
                         && juce::approximatelyEqual (mono.sampleRate, 48000.0);
        double monoClean = 0.0;
        if (converted)
        {
            std::vector<float> x (mono.samples.getReadPointer (0), mono.samples.getReadPointer (0) + mono.numSamples());
            monoClean = cleanliness (x, 1000.0, 48000.0, 0.5);
            converted = monoClean > 85.0;
        }

        const auto noSound = work.getChildFile ("not a sound.wav");
        noSound.replaceWithText ("this is no sound file");
        juce::String refusedId;
        const size_t before = project.getSnippets().size();
        const bool refused = project.importSound (noSound, refusedId, remarks).failed() && project.getSnippets().size() == before
                             && folder.getChildFile ("Sounds").findChildFiles (juce::File::findFiles, false, "*").size() == 2;
        noSound.deleteFile();
        checkRow (ok && exact && converted && refused,
                  "Import: a stereo file of 16 bits at the project's rate is kept as FLAC with every sample the very number the file has; a mono file of 44.1 kHz becomes one channel "
                  "at 48 kHz holding the same tone (" + juce::String (juce::roundToInt (monoClean)) + " dB clean); a file that is no sound is refused and leaves nothing behind");
    }

    // --- The audio snippet in the project ---
    {
        juce::StringArray remarks;
        maket::Project opened;
        bool ok = maket::Project::open (folder, opened, remarks).wasOk() && opened.getSnippets().size() == 2 && remarks.isEmpty();
        const auto* s = ok ? opened.find (stereoId) : nullptr;
        const auto* m = ok ? opened.find (monoId) : nullptr;
        ok = ok && s != nullptr && m != nullptr && s->isAudio() && s->channels == 2 && std::abs (s->length - 500.0) < 0.01 && juce::approximatelyEqual (s->sourceRate, 48000.0)
             && m->isAudio() && m->channels == 1 && std::abs (m->length - 1000.0) < 0.05 && juce::approximatelyEqual (m->sourceRate, 44100.0) && m->name == "test mono 441";

        // The file changes outside Maket; the snippet follows on request only. A broken file changes nothing.
        const bool rewritten = writeWav (monoFile, { tone (1000.0, 44100.0, 2.0, 0.5) }, 44100.0, 24);
        const bool stillOld = std::abs (project.find (monoId)->length - 1000.0) < 0.05;
        const bool updated = rewritten && project.updateFromFile (monoId, remarks).wasOk() && std::abs (project.find (monoId)->length - 2000.0) < 0.05;
        monoFile.replaceWithText ("broken");
        const bool kept = project.updateFromFile (monoId, remarks).failed() && std::abs (project.find (monoId)->length - 2000.0) < 0.05 && project.soundFile (monoId).existsAsFile()
                          && folder.getChildFile ("Sounds").findChildFiles (juce::File::findFiles, false, "*").size() == 2;
        const bool removed = project.remove (monoId).wasOk() && ! project.soundFile (monoId).exists() && folder.getChildFile ("Trash").getChildFile (monoId + ".flac").existsAsFile();
        checkRow (ok && stillOld && updated && kept && removed,
                  "An audio snippet in the project opened anew: its kind, one or two channels, its length, the rate of the file it came from. Update from its file takes the new sound "
                  "(1 s becomes 2 s); a broken file leaves the snippet as it was; a removed snippet's sound is in the project's `Trash` folder");
    }

    // --- The player ---
    {
        auto sound = std::make_shared<maket::Sound>();
        sound->sampleRate = 48000.0;
        sound->samples.setSize (2, 48000);
        const auto l = tone (1000.0, 48000.0, 1.0, 0.5), r = tone (1500.0, 48000.0, 1.0, 0.25);
        sound->samples.copyFrom (0, 0, l.data(), 48000);
        sound->samples.copyFrom (1, 0, r.data(), 48000);

        // From the start, at the same rate: sample for sample, whatever the size of the blocks; silent after the end.
        bool exact = true;
        for (int block : { 512, 37 })
        {
            maket::SoundPlayer player;
            std::vector<float> left, right;
            player.play (sound, 0.0);
            run (player, left, right, 1.2, 48000.0, block);
            for (size_t i = 0; exact && i < 48000; ++i) exact = juce::exactlyEqual (left[i], l[i]) && juce::exactlyEqual (right[i], r[i]);
            for (size_t i = 48000; exact && i < left.size(); ++i) exact = juce::exactlyEqual (left[i], 0.0f);
            exact = exact && ! player.isPlaying();
        }

        // One channel goes to both sides.
        auto mono = std::make_shared<maket::Sound>();
        mono->sampleRate = 48000.0;
        mono->samples.setSize (1, 4800);
        mono->samples.copyFrom (0, 0, l.data(), 4800);
        maket::SoundPlayer monoPlayer;
        std::vector<float> ml, mr;
        monoPlayer.play (mono, 0.0);
        run (monoPlayer, ml, mr, 0.1, 48000.0, 512);
        bool both = true;
        for (size_t i = 0; both && i < 4800; ++i) both = juce::exactlyEqual (ml[i], l[i]) && juce::exactlyEqual (mr[i], l[i]);
        checkRow (exact && both, "Playing at the project's rate: every sample out is the sample of the sound, in blocks of 512 and of 37; silence after its end; "
                                 "a sound of one channel goes to both sides alike");

        // From the middle: it comes in over 4 ms, then it is the sound. Stop: out over 4 ms, no click, then silence.
        maket::SoundPlayer player;
        std::vector<float> left, right;
        player.play (sound, 250.0);
        run (player, left, right, 0.1, 48000.0, 512);
        bool middle = std::abs (left[0]) < 1.0e-6f && std::abs (player.positionMs() - 350.0) < 0.5;
        for (size_t i = 200; middle && i < left.size(); ++i) middle = juce::exactlyEqual (left[i], l[12000 + i]);
        player.stop();
        const size_t stopAt = left.size();
        run (player, left, right, 0.05, 48000.0, 512);
        float jump = 0.0f, after = 0.0f;
        for (size_t i = stopAt; i < left.size(); ++i) jump = std::max (jump, std::abs (left[i] - left[i - 1]));
        for (size_t i = stopAt + 200; i < left.size(); ++i) after = std::max (after, std::abs (left[i]));
        const bool stopped = ! player.isPlaying() && juce::exactlyEqual (after, 0.0f) && jump < 0.08f;      // the tone itself steps 0.065 at most

        // A loop from 100 to 200 ms: after a second it is still inside it, and the seam is silent, not a click.
        maket::SoundPlayer looper;
        std::vector<float> ll, lr;
        looper.play (sound, 0.0, true, 100.0, 200.0);
        run (looper, ll, lr, 1.0, 48000.0, 512);
        float loopJump = 0.0f;
        for (size_t i = 1; i < ll.size(); ++i) loopJump = std::max (loopJump, std::abs (ll[i] - ll[i - 1]));
        const bool looped = looper.isPlaying() && looper.positionMs() >= 100.0 && looper.positionMs() < 200.0 && loopJump < 0.08f && std::abs (ll[9600]) < 0.02f;
        checkRow (middle && stopped && looped, "Begun in the middle, the sound comes in over 4 ms and is then itself; Stop takes it out over 4 ms without a click, then silence; "
                                               "a loop of 100 ms keeps going round, with a dip instead of a click at its seam");

        // Through a device at another rate: the same tone.
        juce::String found;
        bool other = true;
        for (double deviceRate : { 96000.0, 44100.0 })
        {
            maket::SoundPlayer p;
            std::vector<float> ol, orr;
            p.play (sound, 0.0);
            run (p, ol, orr, 0.9, deviceRate, 512);
            const double cl = cleanliness (ol, 1000.0, deviceRate, 0.5), cr = cleanliness (orr, 1500.0, deviceRate, 0.25);
            other = other && cl > 85.0 && cr > 85.0;
            found += juce::String (juce::roundToInt (deviceRate / 100.0) / 10.0) + " kHz: " + juce::String (juce::roundToInt (std::min (cl, cr))) + " dB; ";
        }
        checkRow (other, "Played through a device at another rate than the project's 48 kHz, the tones come out as they are (" + found.trimEnd().dropLastCharacters (1) + " clean)");
    }

    folder.deleteRecursively();
    stereoFile.deleteFile();
    monoFile.deleteFile();
    line();
}
