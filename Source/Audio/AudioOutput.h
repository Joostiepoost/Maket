// AudioOutput.h — the audio device Maket's own sound goes to.
//
// Opened when a roll has a voice on the synth, and kept open. With every
// block the device asks for, the synth is told at which moment the block
// will be heard (StreamClock), on the clock the ePlayer's commands are
// timed on.
//
// Only the app has this; the console test program renders the synth into
// memory instead.

#pragma once

#include <JuceHeader.h>

#include "../Engine/StreamClock.h"
#include "../Engine/Synth.h"
#include "../Sound/SoundPlayer.h"
#include "../Sound/SurfaceEngine.h"

class AudioOutput : private juce::AudioIODeviceCallback
{
public:
    explicit AudioOutput (maket::Synth&);
    /** Stage 3c: a player of sound files, mixed in with the synth. Set once, before the output is opened. */
    void setSoundPlayer (maket::SoundPlayer* p)     { soundPlayer = p; }
    /** Stage 3d: the engine of the build surface, mixed in as well. Set once, before the output is opened. */
    void setSurfaceEngine (maket::SurfaceEngine* e)     { surfaceEngine = e; }
    ~AudioOutput() override;

    /** Opens the output with this name (empty: the computer's default output), unless it is open already.
        Returns what went wrong, or nothing. */
    juce::String open (const juce::String& deviceName);
    void close();
    bool isOpen() const;

    /** The outputs this computer has now. */
    juce::StringArray outputNames();

    /** In a few words: the device, its rate, its block and its delay; or that none is open. */
    juce::String describe() const;
    /** Stage 3a: how far the sound card's clock runs from the computer's, in words. */
    juce::String driftInWords() const;

private:
    void audioDeviceIOCallbackWithContext (const float* const* inputs, int numInputs, float* const* outputs, int numOutputs,
                                           int numSamples, const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart (juce::AudioIODevice*) override;
    void audioDeviceStopped() override;

    maket::Synth& synth;
    maket::SoundPlayer* soundPlayer = nullptr;
    maket::SurfaceEngine* surfaceEngine = nullptr;
    juce::AudioDeviceManager manager;
    maket::StreamClock clock;
    bool initialised = false;
    juce::String wanted;                // the name asked for at the last open
    double sampleRate = 48000.0, latencyMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioOutput)
};
