// Sound.cpp — see Sound.h.

#include "Sound.h"
#include "Resampler.h"

namespace maket
{

namespace
{
    juce::AudioFormatManager& formats()
    {
        static juce::AudioFormatManager manager;
        static bool ready = false;
        if (! ready) { manager.registerBasicFormats(); ready = true; }
        return manager;
    }
}

void Sound::makeOverview()
{
    overview.assign ((size_t) numChannels(), {});
    for (int c = 0; c < numChannels(); ++c)
    {
        const float* data = samples.getReadPointer (c);
        auto& ranges = overview[(size_t) c];
        ranges.reserve ((size_t) (numSamples() / overviewStep + 1));
        for (int i = 0; i < numSamples(); i += overviewStep)
        {
            const auto r = juce::FloatVectorOperations::findMinAndMax (data + i, juce::jmin (overviewStep, numSamples() - i));
            ranges.push_back ({ r.getStart(), r.getEnd() });
        }
    }
}

juce::String soundFilePattern()         { return formats().getWildcardForAllFormats(); }

bool looksLikeSoundFile (const juce::File& file)
{
    const auto ext = file.getFileExtension().toLowerCase();
    return ext.isNotEmpty() && ext != ".txt" && ext != ".llll" && ext != ".json" && formats().findFormatForFileExtension (ext) != nullptr;
}

juce::Result readSoundFacts (const juce::File& file, SoundFacts& facts)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats().createReaderFor (file));
    if (reader == nullptr) return juce::Result::fail (file.getFileName() + " is not a sound file Maket can read.");
    facts.channels = juce::jmin (2, (int) reader->numChannels);
    facts.sourceChannels = (int) reader->numChannels;
    facts.sourceRate = reader->sampleRate;
    facts.sourceBits = (int) reader->bitsPerSample;
    facts.lengthMs = reader->sampleRate > 0.0 ? (double) reader->lengthInSamples * 1000.0 / reader->sampleRate : 0.0;
    return juce::Result::ok();
}

juce::Result readSoundFile (const juce::File& file, Sound& sound)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats().createReaderFor (file));
    if (reader == nullptr) return juce::Result::fail (file.getFileName() + " is not a sound file Maket can read.");
    if (reader->lengthInSamples <= 0 || reader->numChannels == 0) return juce::Result::fail (file.getFileName() + " holds no sound.");
    if (reader->lengthInSamples > 60 * 60 * 192000ll) return juce::Result::fail (file.getFileName() + " is too long to hold in memory.");

    const int channels = juce::jmin (2, (int) reader->numChannels);
    sound.sampleRate = reader->sampleRate;
    sound.samples.setSize (channels, (int) reader->lengthInSamples);
    if (! reader->read (&sound.samples, 0, (int) reader->lengthInSamples, 0, true, channels > 1))
        return juce::Result::fail (file.getFileName() + " could not be read to its end.");
    sound.makeOverview();
    return juce::Result::ok();
}

std::vector<float> soundMiniature (const Sound& sound, int columns)
{
    std::vector<float> pairs;
    const int length = sound.numSamples();
    if (length <= 0 || columns <= 0) return pairs;
    pairs.reserve ((size_t) columns * 2);
    for (int c = 0; c < columns; ++c)
    {
        const int from = (int) ((juce::int64) length * c / columns), to = juce::jmax (from + 1, (int) ((juce::int64) length * (c + 1) / columns));
        float low = 1.0f, high = -1.0f;
        for (int ch = 0; ch < sound.numChannels(); ++ch)
        {
            const auto r = juce::FloatVectorOperations::findMinAndMax (sound.samples.getReadPointer (ch) + from, juce::jmin (to, length) - from);
            low = juce::jmin (low, r.getStart()); high = juce::jmax (high, r.getEnd());
        }
        pairs.push_back (juce::jlimit (-1.0f, 1.0f, low));
        pairs.push_back (juce::jlimit (-1.0f, 1.0f, high));
    }
    return pairs;
}

juce::Result importSoundFile (const juce::File& source, const juce::File& target, double projectRate, SoundFacts& facts, juce::StringArray& remarks, Sound* imported)
{
    auto read = readSoundFacts (source, facts);
    if (read.failed()) return read;
    if (facts.sourceChannels > 2) remarks.add (source.getFileName() + " has " + juce::String (facts.sourceChannels) + " channels; the first two are taken.");

    Sound sound;
    read = readSoundFile (source, sound);
    if (read.failed()) return read;

    // To the project's rate, when the file has another.
    if (std::abs (sound.sampleRate - projectRate) > 0.5)
    {
        juce::AudioBuffer<float> converted;
        for (int c = 0; c < sound.numChannels(); ++c)
        {
            const auto out = Resampler::convert (sound.samples.getReadPointer (c), sound.numSamples(), sound.sampleRate, projectRate);
            if (c == 0) converted.setSize (sound.numChannels(), (int) out.size());
            converted.copyFrom (c, 0, out.data(), (int) out.size());
        }
        sound.samples = std::move (converted);
        sound.sampleRate = projectRate;
    }

    // Written beside the target first, so that a failed import leaves nothing half done.
    if (! target.getParentDirectory().createDirectory().wasOk()) return juce::Result::fail ("The project's Sounds folder could not be made.");
    const auto temporary = target.getSiblingFile (target.getFileNameWithoutExtension() + ".part");
    temporary.deleteFile();
    {
        juce::FlacAudioFormat flac;
        auto file = std::make_unique<juce::FileOutputStream> (temporary);
        if (file->failedToOpen()) return juce::Result::fail ("The copy could not be written: " + target.getFullPathName());
        std::unique_ptr<juce::OutputStream> stream (std::move (file));
        auto writer = flac.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (projectRate)
                                                        .withNumChannels (sound.numChannels()).withBitsPerSample (24));
        if (writer == nullptr) return juce::Result::fail ("The copy could not be written as FLAC.");
        if (! writer->writeFromAudioSampleBuffer (sound.samples, 0, sound.numSamples()))
            return juce::Result::fail ("The copy could not be written to its end.");
    }
    if (! temporary.moveFileTo (target)) return juce::Result::fail ("The copy could not be put in its place: " + target.getFullPathName());

    facts.channels = sound.numChannels();
    facts.lengthMs = sound.lengthMs();
    if (imported != nullptr) { sound.makeOverview(); *imported = std::move (sound); }
    return juce::Result::ok();
}

} // namespace maket
