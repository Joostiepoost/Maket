// SoundPlayer.h — plays one sound through Maket's audio output (stage 3c).
//
// For listening to an audio snippet in the library: from any point, with a
// loop, stopped without a click. It reads the sound with the sinc of
// Resampler when the device runs at another rate than the project (Joost's
// runs at 96 kHz), and sample for sample when the rates are the same. The
// build surface (3d) gets an engine of its own that plays many at once.
// No interface code; `render` is called from the audio thread.

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>

#include "Sound.h"
#include "Resampler.h"

namespace maket
{

class SoundPlayer
{
public:
    /** Begins to play `sound` at `fromMs`. With `loopOn`, it goes back to `loopStartMs` whenever it reaches `loopEndMs`. */
    void play (std::shared_ptr<const Sound> sound, double fromMs, bool loopOn = false, double loopStartMs = 0.0, double loopEndMs = 0.0);
    /** Fades out over a few milliseconds and stops. */
    void stop();
    bool isPlaying() const              { return playing.load(); }
    /** Where in the sound the player is, in ms. */
    double positionMs() const           { return position.load(); }

    /** Adds the sound to the two channels (either may be null). `deviceRate`: the rate the device runs at. */
    void render (float* left, float* right, int numSamples, double deviceRate);

    static constexpr double fadeMs = 4.0;       // at a stop, at the seam of a loop, and at a start in the middle of a sound

private:
    juce::CriticalSection lock;
    std::shared_ptr<const Sound> current;
    std::unique_ptr<Resampler> reader;
    double readerRatio = 0.0;
    double place = 0.0;                         // in samples of the sound
    double loopStart = 0.0, loopEnd = 0.0;      // in samples
    bool looping = false, stopping = false;
    double gain = 1.0, gainTarget = 1.0;
    std::atomic<bool> playing { false };
    std::atomic<double> position { 0.0 };
};

} // namespace maket
