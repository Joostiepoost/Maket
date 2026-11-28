// SurfaceEngine.cpp — see SurfaceEngine.h.

#include "SurfaceEngine.h"
#include <algorithm>
#include <cmath>

namespace maket
{

namespace { const double halfPi = 1.57079632679489661923; }

double gainFromDb (double db)       { return db <= -60.0 ? 0.0 : std::pow (10.0, db / 20.0); }

double Clip::fadeAt (double into) const
{
    double level = 1.0;
    if (fadeIn > 0.0 && into < fadeIn)
    {
        const double t = juce::jlimit (0.0, 1.0, into / fadeIn);
        level *= fadeInEqualPower ? std::sin (t * halfPi) : t;
    }
    if (fadeOut > 0.0 && into > length - fadeOut)
    {
        const double t = juce::jlimit (0.0, 1.0, (length - into) / fadeOut);
        level *= fadeOutEqualPower ? std::sin (t * halfPi) : t;
    }
    return level;
}

std::shared_ptr<const Arrangement> buildArrangement (const Surface& surface, double projectRate,
                                                     const std::function<std::shared_ptr<const Sound> (const juce::String&)>& soundOf)
{
    auto arrangement = std::make_shared<Arrangement>();
    arrangement->sampleRate = projectRate;
    const double perMs = projectRate / 1000.0;

    bool anySolo = false;
    for (const auto& t : surface.tracks) anySolo = anySolo || t.solo;

    for (const auto& track : surface.tracks)
    {
        if (track.mute || (anySolo && ! track.solo)) continue;

        // The track's blocks in order of time.
        std::vector<const Placement*> blocks;
        for (const auto& p : surface.placements) if (p.trackId == track.id) blocks.push_back (&p);
        std::stable_sort (blocks.begin(), blocks.end(), [] (const Placement* a, const Placement* b) { return a->start < b->start; });

        // Pan as a balance: in the middle both sides at full level; to one side the other falls away.
        const double trackGain = gainFromDb (track.gainDb);
        const double left = trackGain * juce::jmin (1.0, 1.0 - track.pan), right = trackGain * juce::jmin (1.0, 1.0 + track.pan);

        const size_t firstOfTrack = arrangement->clips.size();
        for (const auto* p : blocks)
        {
            auto sound = soundOf (p->snippetId);
            if (sound == nullptr || sound->numSamples() == 0) continue;
            Clip c;
            c.sound = sound;
            c.start = std::round (p->start * perMs);
            c.offset = juce::jlimit (0.0, (double) sound->numSamples(), std::round (p->offset * perMs));
            c.length = juce::jlimit (0.0, (double) sound->numSamples() - c.offset, std::round (p->length * perMs));
            if (c.length <= 0.0) continue;
            const double blockGain = gainFromDb (p->gainDb);
            c.gainLeft = (float) (blockGain * left);
            c.gainRight = (float) (blockGain * right);
            c.fadeIn = juce::jmin (c.length, p->fadeIn * perMs);
            c.fadeOut = juce::jmin (c.length, p->fadeOut * perMs);
            arrangement->clips.push_back (std::move (c));
        }

        // Where a block begins before the one before it has ended, the two cross over at equal power.
        for (size_t i = firstOfTrack + 1; i < arrangement->clips.size(); ++i)
        {
            auto& a = arrangement->clips[i - 1];
            auto& b = arrangement->clips[i];
            const double overlap = (a.start + a.length) - b.start;
            if (overlap <= 0.0 || b.start + b.length <= a.start + a.length) continue;       // apart, or one wholly inside the other
            const double over = juce::jmin (overlap, a.length, b.length);
            if (over > a.fadeOut) { a.fadeOut = over; a.fadeOutEqualPower = true; }
            if (over > b.fadeIn)  { b.fadeIn = over;  b.fadeInEqualPower = true; }
        }
    }
    for (const auto& c : arrangement->clips) arrangement->length = juce::jmax (arrangement->length, c.start + c.length);
    return arrangement;
}

//==============================================================================

void SurfaceEngine::setArrangement (std::shared_ptr<const Arrangement> a)
{
    std::shared_ptr<const Arrangement> old;         // let go of outside the lock
    const juce::ScopedLock sl (lock);
    old = std::move (current);
    current = std::move (a);
}

void SurfaceEngine::play (double fromMs, bool loopOn, double loopStartMs, double loopEndMs)
{
    const juce::ScopedLock sl (lock);
    if (current == nullptr) return;
    const double perMs = current->sampleRate / 1000.0;
    place = std::round (juce::jmax (0.0, fromMs) * perMs);
    loopStart = std::round (juce::jmax (0.0, loopStartMs) * perMs);
    loopEnd = std::round (juce::jmax (0.0, loopEndMs) * perMs);
    looping = loopOn && loopEnd - loopStart >= current->sampleRate * 0.02;
    if (looping && place >= loopEnd) place = loopStart;
    stopping = false;
    gain = 0.0;
    gainTarget = 1.0;
    position.store (place / perMs);
    playing.store (true);
}

void SurfaceEngine::stop()
{
    const juce::ScopedLock sl (lock);
    if (playing.load()) { stopping = true; gainTarget = 0.0; }
}

void SurfaceEngine::halt()
{
    const juce::ScopedLock sl (lock);
    playing.store (false);
    stopping = false;
}

void SurfaceEngine::render (float* left, float* right, int numSamples, double deviceRate)
{
    const juce::ScopedTryLock sl (lock);
    if (! sl.isLocked() || ! playing.load() || current == nullptr || deviceRate <= 0.0) return;

    const auto& arrangement = *current;
    const double ratio = arrangement.sampleRate / deviceRate;
    const bool sameRate = std::abs (ratio - 1.0) < 1.0e-9;
    if (! sameRate && (reader == nullptr || std::abs (readerRatio - ratio) > 1.0e-12))
    {
        reader = std::make_unique<Resampler> (ratio);
        readerRatio = ratio;
    }
    const double gainStep = 1.0 / (fadeMs * deviceRate / 1000.0);
    const double seamFade = fadeMs * arrangement.sampleRate / 1000.0;
    const double endOfAll = arrangement.length + tailMs * arrangement.sampleRate / 1000.0;

    int done = 0;
    while (done < numSamples && playing.load())
    {
        // A stretch that does not cross the loop's end.
        int count = numSamples - done;
        if (looping) count = juce::jmin (count, juce::jmax (1, (int) std::ceil ((loopEnd - place) / ratio)));
        float master[4096];
        count = juce::jmin (count, 4096);
        const double stretchStart = place;

        // The level of the whole (the start, the stop, the loop's seam), per sample.
        bool ended = false;
        for (int i = 0; i < count; ++i)
        {
            if (gain < gainTarget) gain = std::min (gainTarget, gain + gainStep);
            else if (gain > gainTarget) gain = std::max (gainTarget, gain - gainStep);
            double g = gain;
            if (looping)
            {
                const double at = stretchStart + i * ratio;
                g *= juce::jlimit (0.0, 1.0, std::min ((loopEnd - at) / seamFade, at >= loopStart ? (at - loopStart) / seamFade : 1.0));
            }
            master[i] = (float) g;
            if (stopping && gain <= 0.0) { ended = true; count = i; break; }
        }

        const double stretchEnd = stretchStart + count * ratio;
        for (const auto& c : arrangement.clips)
        {
            if (c.start >= stretchEnd || c.start + c.length <= stretchStart) continue;
            const int first = juce::jmax (0, (int) std::ceil ((c.start - stretchStart) / ratio));
            const int last = juce::jmin (count, (int) std::ceil ((c.start + c.length - stretchStart) / ratio));
            const float* a = c.sound->samples.getReadPointer (0);
            const float* b = c.sound->numChannels() > 1 ? c.sound->samples.getReadPointer (1) : a;
            const int soundLength = c.sound->numSamples();
            const bool fades = c.fadeIn > 0.0 || c.fadeOut > 0.0;
            for (int i = first; i < last; ++i)
            {
                const double into = stretchStart + i * ratio - c.start;
                if (into < 0.0 || into >= c.length) continue;
                const double src = c.offset + into;
                float l, r;
                if (sameRate)
                {
                    const int n = (int) (src + 0.5);
                    if (n >= soundLength) continue;
                    l = a[n]; r = b[n];
                }
                else
                {
                    l = reader->at (a, soundLength, src);
                    r = b == a ? l : reader->at (b, soundLength, src);
                }
                const float level = master[i] * (fades ? (float) c.fadeAt (into) : 1.0f);
                if (left != nullptr)  left[done + i]  += level * c.gainLeft * l;
                if (right != nullptr) right[done + i] += level * c.gainRight * r;
            }
        }

        done += count;
        place = stretchStart + count * ratio;
        if (ended) { playing.store (false); break; }
        if (looping && place >= loopEnd) place = loopStart + (place - loopEnd);
        else if (! looping && place >= endOfAll) { playing.store (false); break; }
        if (count == 0) break;
    }
    position.store (place * 1000.0 / arrangement.sampleRate);
}

} // namespace maket
