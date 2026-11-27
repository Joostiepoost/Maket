// Sound.h — a sound held whole in memory, and bringing a sound file into a project (stage 3c).
//
// Joost's sound files run from a few seconds to two minutes, so a sound is
// read whole: one or two channels of numbers at the project's sample rate.
// Importing reads a file of any kind JUCE can read (WAV, AIFF, FLAC, Ogg; on
// a Mac also MP3, AAC and CAF), brings it to the project's rate when it has
// another, and writes a copy without loss: FLAC, 24 bits. No interface code.

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

namespace maket
{

struct Sound
{
    double sampleRate = 48000.0;
    juce::AudioBuffer<float> samples;       // one or two channels

    int numChannels() const     { return samples.getNumChannels(); }
    int numSamples() const      { return samples.getNumSamples(); }
    double lengthMs() const     { return sampleRate > 0.0 ? numSamples() * 1000.0 / sampleRate : 0.0; }

    /** The lowest and highest value of every stretch of `overviewStep` samples, per channel: for drawing. */
    struct Range { float low = 0.0f, high = 0.0f; };
    static constexpr int overviewStep = 64;
    std::vector<std::vector<Range>> overview;
    void makeOverview();
};

struct SoundFacts
{
    int channels = 0;
    double lengthMs = 0.0;
    double sourceRate = 0.0;        // the rate of the file it came from
    int sourceChannels = 0;
    int sourceBits = 0;
};

/** The kinds of sound file this Mac's Maket can read, as a pattern for a file dialog ("*.wav;*.aif;..."). */
juce::String soundFilePattern();
bool looksLikeSoundFile (const juce::File&);

/** Reads `source`, brings it to `projectRate`, and writes it as `target` (FLAC, 24 bits). More than two channels:
    the first two are taken, with a remark. The source file is not touched. */
juce::Result importSoundFile (const juce::File& source, const juce::File& target, double projectRate, SoundFacts& facts, juce::StringArray& remarks,
                              Sound* imported = nullptr);

/** The sound as `columns` pairs of lowest and highest value (over all channels), for the small picture in the library. */
std::vector<float> soundMiniature (const Sound&, int columns = 240);

/** Reads a sound file whole, as it is (no change of rate). */
juce::Result readSoundFile (const juce::File& file, Sound& sound);

/** What the file's own head says, without reading the sound. */
juce::Result readSoundFacts (const juce::File& file, SoundFacts& facts);

} // namespace maket
