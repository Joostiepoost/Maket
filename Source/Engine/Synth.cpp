// Synth.cpp — see Synth.h.

#include "Synth.h"
#include "../Notation/Glides.h"

#include <algorithm>
#include <cmath>

namespace maket
{

namespace
{
    constexpr int controlStep = 16;         // samples between two readings of pitch and loudness; in between, a straight line
    constexpr size_t mostWaiting = 16384, mostFinished = 4096;
}

Synth::Synth()
{
    waiting.reserve (mostWaiting);
    voices.reserve ((size_t) maxVoices);
    finished.reserve (mostFinished);
}

void Synth::prepare (double sampleRateToUse)
{
    const juce::ScopedLock sl (lock);
    sampleRate = sampleRateToUse > 0.0 ? sampleRateToUse : 48000.0;
}

void Synth::addNotes (std::vector<SynthNote> notes)
{
    std::vector<SynthNote> old;             // freed here, outside the lock and outside the audio thread
    {
        const juce::ScopedLock sl (lock);
        old.swap (finished);
        finished.reserve (mostFinished);
        // Room for 16384 waiting notes is kept ready; a pass with more makes the list grow here, which may hold up one block of sound.
        for (auto& n : notes)
            if (n.end > n.start) waiting.push_back (std::move (n));
    }
}

void Synth::allNotesOff()
{
    std::vector<SynthNote> old, were;
    {
        const juce::ScopedLock sl (lock);
        old.swap (finished);
        finished.reserve (mostFinished);
        were.swap (waiting);
        waiting.reserve (mostWaiting);
        offWanted.store (true);             // what sounds is released by the audio thread at its next block
    }
}

int Synth::numNotes() const
{
    const juce::ScopedLock sl (lock);
    return (int) (waiting.size() + voices.size());
}

double Synth::glideAt (const SynthNote& note, double position)
{
    if (note.glide.size() < 2) return 0.0;
    const auto* before = &note.glide.front();
    if (position <= before->position) return before->cents;
    for (size_t i = 1; i < note.glide.size(); ++i)
    {
        const auto& p = note.glide[i];
        if (position <= p.position)
        {
            const double length = p.position - before->position;
            const double t = length > 0.0 ? (position - before->position) / length : 1.0;
            return before->cents + (p.cents - before->cents) * notation::slopeCurve (t, p.slope);
        }
        before = &p;
    }
    return note.glide.back().cents;
}

double Synth::levelAt (const SynthNote& note, double ms)
{
    if (note.levels.empty()) return 127.0;
    const auto* before = &note.levels.front();
    if (ms <= before->ms) return before->value;
    for (size_t i = 1; i < note.levels.size(); ++i)
    {
        const auto& p = note.levels[i];
        if (ms <= p.ms)
        {
            const double length = p.ms - before->ms;
            const double t = length > 0.0 ? (ms - before->ms) / length : 1.0;
            return before->value + (p.value - before->value) * notation::slopeCurve (t, p.slope);
        }
        before = &p;
    }
    return note.levels.back().value;
}

void Synth::render (float* left, float* right, int numSamples, double clock)
{
    const juce::ScopedLock sl (lock);
    if (numSamples <= 0) return;

    const double msPerSample = 1000.0 / sampleRate;
    const double blockEnd = clock + numSamples * msPerSample;
    const double pitch = standardPitch.load(), level = gain.load();

    // Stop: what sounds is released from now.
    if (offWanted.exchange (false))
        for (auto& v : voices)
            if (! v.cut) { v.cut = true; v.cutAt = clock; }

    // Notes that begin before this block ends get a voice.
    {
        size_t kept = 0;
        for (size_t i = 0; i < waiting.size(); ++i)
        {
            auto& n = waiting[i];
            if (n.start < blockEnd)
            {
                if (n.end + releaseMs > clock && (int) voices.size() < maxVoices)
                {
                    voices.emplace_back();
                    voices.back().note = std::move (n);
                }
                else if (finished.size() < mostFinished) finished.push_back (std::move (n));     // over already, or no voice free
            }
            else
            {
                if (kept != i) waiting[kept] = std::move (n);
                ++kept;
            }
        }
        waiting.resize (kept);
    }

    for (auto& v : voices)
    {
        const auto& n = v.note;
        const double end = v.cut ? std::min (n.end, std::max (v.cutAt, n.start)) : n.end;
        const double length = n.end - n.start;
        const double velocity = juce::jlimit (0, 127, n.velocity) / 127.0;

        auto readAt = [&] (double t, double& amplitude, double& increment)
        {
            const double into = t - n.start;
            if (into < 0.0 || t >= end + releaseMs) { amplitude = 0.0; }
            else
            {
                const double held = std::min (into, end - n.start);
                double envelope = std::min (1.0, into / attackMs);
                if (t > end) envelope *= std::max (0.0, 1.0 - (t - end) / releaseMs);
                amplitude = envelope * velocity * (levelAt (n, held) / 127.0) * level;
            }
            const double position = length > 0.0 ? juce::jlimit (0.0, 1.0, into / length) : 0.0;
            increment = frequencyOf (n.cents + glideAt (n, position), pitch) / sampleRate;
        };

        int done = 0;
        if (! v.started)
        {
            readAt (clock, v.lastAmplitude, v.lastIncrement);
            v.started = true;
        }
        while (done < numSamples)
        {
            const int count = std::min (controlStep, numSamples - done);
            double amplitude, increment;
            readAt (clock + (done + count) * msPerSample, amplitude, increment);

            // The note begins inside this piece: no straight line from silence across its first sample.
            const double pieceStart = clock + done * msPerSample;
            const bool beginsHere = n.start >= pieceStart && n.start < pieceStart + count * msPerSample;

            // Sample i of the piece sounds at pieceStart + i samples and takes the values of that moment:
            // the piece's first values were read for pieceStart, its last are those of the next piece's start.
            const double amplitudeStep = (amplitude - v.lastAmplitude) / count, incrementStep = (increment - v.lastIncrement) / count;
            double a = v.lastAmplitude, inc = v.lastIncrement;
            for (int i = 0; i < count; ++i)
            {
                double out = 0.0;
                if (beginsHere)
                {
                    // Exact, sample by sample, for the few samples around the start.
                    double exactA, exactInc;
                    readAt (pieceStart + i * msPerSample, exactA, exactInc);
                    if (exactA > 0.0) { v.phase += exactInc; out = exactA * std::sin (juce::MathConstants<double>::twoPi * v.phase); }
                }
                else if (a > 0.0 || amplitude > 0.0)
                {
                    v.phase += inc;
                    out = a * std::sin (juce::MathConstants<double>::twoPi * v.phase);
                }
                a += amplitudeStep;
                inc += incrementStep;
                if (v.phase >= 1.0) v.phase -= std::floor (v.phase);
                const auto sample = (float) out;
                if (left != nullptr)  left[done + i] += sample;
                if (right != nullptr) right[done + i] += sample;
            }
            v.lastAmplitude = amplitude;
            v.lastIncrement = increment;
            done += count;
        }
    }

    // Voices that are over leave; their notes are freed outside the audio thread.
    {
        size_t kept = 0;
        for (size_t i = 0; i < voices.size(); ++i)
        {
            auto& v = voices[i];
            const double end = v.cut ? std::min (v.note.end, std::max (v.cutAt, v.note.start)) : v.note.end;
            if (blockEnd >= end + releaseMs)
            {
                if (finished.size() < mostFinished) finished.push_back (std::move (v.note));
            }
            else
            {
                if (kept != i) voices[kept] = std::move (v);
                ++kept;
            }
        }
        voices.resize (kept);
    }

    // Many notes together can pass full scale: the top is rounded off instead of cut.
    for (auto* channel : { left, right })
        if (channel != nullptr)
            for (int i = 0; i < numSamples; ++i)
            {
                const float x = channel[i];
                if (x > 0.8f)        channel[i] = 0.8f + 0.2f * std::tanh ((x - 0.8f) / 0.2f);
                else if (x < -0.8f)  channel[i] = -0.8f - 0.2f * std::tanh ((-x - 0.8f) / 0.2f);
            }
}

} // namespace maket
