// SoundPlayer.cpp — see SoundPlayer.h.

#include "SoundPlayer.h"
#include <cmath>

namespace maket
{

void SoundPlayer::play (std::shared_ptr<const Sound> sound, double fromMs, bool loopOn, double loopStartMs, double loopEndMs)
{
    std::shared_ptr<const Sound> old;           // let go of outside the lock
    {
        const juce::ScopedLock sl (lock);
        old = std::move (current);
        current = std::move (sound);
        if (current == nullptr || current->numSamples() == 0) { playing.store (false); return; }

        const double perMs = current->sampleRate / 1000.0;
        place = juce::jlimit (0.0, (double) current->numSamples(), fromMs * perMs);
        loopStart = juce::jlimit (0.0, (double) current->numSamples(), loopStartMs * perMs);
        loopEnd = juce::jlimit (0.0, (double) current->numSamples(), loopEndMs * perMs);
        looping = loopOn && loopEnd - loopStart >= current->sampleRate * 0.02;      // a loop is 20 ms at least
        if (looping && place >= loopEnd) place = loopStart;
        stopping = false;
        // From the very start a sound begins as it is; from the middle it comes in over a few ms, without a click.
        gain = place <= 0.0 ? 1.0 : 0.0;
        gainTarget = 1.0;
        position.store (place / perMs);
        playing.store (true);
    }
}

void SoundPlayer::stop()
{
    const juce::ScopedLock sl (lock);
    if (playing.load()) { stopping = true; gainTarget = 0.0; }
}

void SoundPlayer::render (float* left, float* right, int numSamples, double deviceRate)
{
    const juce::ScopedTryLock sl (lock);
    if (! sl.isLocked() || ! playing.load() || current == nullptr || deviceRate <= 0.0) return;

    const auto& sound = *current;
    const int length = sound.numSamples();
    const double ratio = sound.sampleRate / deviceRate;
    const bool sameRate = std::abs (ratio - 1.0) < 1.0e-9;
    if (! sameRate && (reader == nullptr || std::abs (readerRatio - ratio) > 1.0e-12))
    {
        reader = std::make_unique<Resampler> (ratio);       // only when the device's rate changes
        readerRatio = ratio;
    }

    const float* a = sound.samples.getReadPointer (0);
    const float* b = sound.numChannels() > 1 ? sound.samples.getReadPointer (1) : a;       // one channel goes to both sides
    const double gainStep = 1.0 / (fadeMs * deviceRate / 1000.0);
    const double seamFade = fadeMs * sound.sampleRate / 1000.0;                              // in samples of the sound

    for (int i = 0; i < numSamples; ++i)
    {
        if (gain < gainTarget) gain = std::min (gainTarget, gain + gainStep);
        else if (gain > gainTarget) gain = std::max (gainTarget, gain - gainStep);
        if (stopping && gain <= 0.0) { playing.store (false); current.reset(); break; }

        // At the seam of a loop the sound dips for a few ms instead of clicking.
        double g = gain;
        if (looping)
        {
            g *= std::min (1.0, std::min ((loopEnd - place) / seamFade, (place - loopStart) / seamFade + (place < loopStart ? 1.0e9 : 0.0)));
            if (g < 0.0) g = 0.0;
        }

        float l, r;
        if (sameRate)
        {
            const int n = (int) place;
            l = n < length ? a[n] : 0.0f;
            r = n < length ? b[n] : 0.0f;
        }
        else
        {
            l = reader->at (a, length, place);
            r = b == a ? l : reader->at (b, length, place);
        }
        if (left != nullptr)  left[i]  += (float) g * l;
        if (right != nullptr) right[i] += (float) g * r;

        place += ratio;
        if (looping && place >= loopEnd) place = loopStart + (place - loopEnd);
        else if (place >= (double) length) { playing.store (false); current.reset(); break; }
    }
    if (playing.load()) position.store (place * 1000.0 / sound.sampleRate);
}

} // namespace maket
