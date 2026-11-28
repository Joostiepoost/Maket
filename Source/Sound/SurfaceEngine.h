// SurfaceEngine.h — plays a build surface: many sounds at once (stage 3d).
//
// The surface is first turned into an arrangement: a plain list of clips,
// each with its sound, its place, its part of the sound, its level on the
// left and the right (the block's gain, and its track's gain, pan, mute and
// solo, all reckoned in), and its fades. Where two blocks on one track
// overlap, the earlier fades out and the later fades in over the overlap, at
// equal power. The engine plays the arrangement sample for sample at the
// project's rate, and through the sinc of Resampler when the device runs at
// another rate. Its own count of samples is the surface's clock.
// No interface code; `render` is called from the audio thread.

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "Sound.h"
#include "Resampler.h"
#include "../Model/Surface.h"

namespace maket
{

struct Clip
{
    std::shared_ptr<const Sound> sound;
    double start = 0.0, offset = 0.0, length = 0.0;     // in samples at the project's rate
    float gainLeft = 1.0f, gainRight = 1.0f;
    double fadeIn = 0.0, fadeOut = 0.0;                 // in samples
    bool fadeInEqualPower = false, fadeOutEqualPower = false;

    /** The level of the clip `into` samples after its start, from its fades alone (0 to 1). */
    double fadeAt (double into) const;
};

struct Arrangement
{
    double sampleRate = 48000.0;
    std::vector<Clip> clips;
    double length = 0.0;        // samples
};

/** A level in dB as a factor; -60 and under is silence. */
double gainFromDb (double db);

/** The surface as clips. `soundOf` gives the sound of a snippet (null: that block is left out). */
std::shared_ptr<const Arrangement> buildArrangement (const Surface&, double projectRate,
                                                     const std::function<std::shared_ptr<const Sound> (const juce::String& snippetId)>& soundOf);

class SurfaceEngine
{
public:
    /** What is played; may be changed while it plays (an edit is heard at once). */
    void setArrangement (std::shared_ptr<const Arrangement>);
    void play (double fromMs, bool loopOn = false, double loopStartMs = 0.0, double loopEndMs = 0.0);
    void stop();
    /** Ends the play at once, without a fade: for when the audio output is closed and nothing is rendered any more. */
    void halt();
    bool isPlaying() const              { return playing.load(); }
    double positionMs() const           { return position.load(); }

    void render (float* left, float* right, int numSamples, double deviceRate);

    static constexpr double fadeMs = 4.0;       // at a stop, at a start, and at the seam of a loop
    static constexpr double tailMs = 250.0;     // it plays on this long after the last block, then stops by itself

private:
    juce::CriticalSection lock;
    std::shared_ptr<const Arrangement> current;
    std::unique_ptr<Resampler> reader;
    double readerRatio = 0.0;
    double place = 0.0, loopStart = 0.0, loopEnd = 0.0;     // samples at the project's rate
    bool looping = false, stopping = false;
    double gain = 0.0, gainTarget = 1.0;
    std::atomic<bool> playing { false };
    std::atomic<double> position { 0.0 };
};

} // namespace maket
