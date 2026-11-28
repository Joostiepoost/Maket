// AudioOutput.cpp — see AudioOutput.h.

#include "AudioOutput.h"
#include "../Engine/OscScheduler.h"

AudioOutput::AudioOutput (maket::Synth& s) : synth (s) {}

AudioOutput::~AudioOutput()
{
    close();
}

bool AudioOutput::isOpen() const
{
    return initialised && manager.getCurrentAudioDevice() != nullptr;
}

juce::StringArray AudioOutput::outputNames()
{
    juce::StringArray names;
    if (manager.getAvailableDeviceTypes().isEmpty()) return names;
    if (auto* type = manager.getCurrentDeviceTypeObject())
    {
        type->scanForDevices();
        names = type->getDeviceNames (false);
    }
    return names;
}

juce::String AudioOutput::open (const juce::String& deviceName)
{
    if (isOpen() && deviceName == wanted) return {};

    juce::String problem;
    if (! initialised)
    {
        // No inputs, two outputs; the device's own rate and block.
        problem = manager.initialise (0, 2, nullptr, true, deviceName, nullptr);
        manager.addAudioCallback (this);
        initialised = true;
    }
    else
    {
        auto setup = manager.getAudioDeviceSetup();
        juce::String name = deviceName;
        if (name.isEmpty())
            if (auto* type = manager.getCurrentDeviceTypeObject())
            {
                const auto names = type->getDeviceNames (false);
                name = names[juce::jmax (0, type->getDefaultDeviceIndex (false))];
            }
        setup.outputDeviceName = name;
        setup.inputDeviceName.clear();
        problem = manager.setAudioDeviceSetup (setup, true);
    }

    wanted = deviceName;
    if (problem.isEmpty() && manager.getCurrentAudioDevice() == nullptr)
        problem = "No audio output could be opened.";
    if (problem.isEmpty() && deviceName.isNotEmpty()
        && ! manager.getCurrentAudioDevice()->getName().containsIgnoreCase (deviceName))
        problem = "The audio output \"" + deviceName + "\" is not there; using " + manager.getCurrentAudioDevice()->getName() + ".";
    return problem;
}

void AudioOutput::close()
{
    if (! initialised) return;
    manager.removeAudioCallback (this);
    manager.closeAudioDevice();
    initialised = false;
}

juce::String AudioOutput::describe() const
{
    auto* device = manager.getCurrentAudioDevice();
    if (! initialised || device == nullptr) return "no audio output open";
    return device->getName() + ", " + juce::String (device->getCurrentSampleRate() / 1000.0, 1) + " kHz, blocks of "
           + juce::String (device->getCurrentBufferSizeSamples()) + ", "
           + juce::String (latencyMs, 1) + " ms delay" + (clock.usesDeviceTime() ? "" : ", timed by the calls") + driftInWords();
}

juce::String AudioOutput::driftInWords() const
{
    const double seconds = clock.driftSeconds(), ppm = clock.driftPpm();
    if (seconds < maket::StreamClock::driftFrom) return "; its clock against the Mac's: being measured";
    const int whole = (int) seconds;
    return "; its clock " + juce::String (std::abs (ppm), 1) + " millionths " + (ppm >= 0.0 ? "fast" : "slow") + " against the Mac's (over "
           + juce::String (whole / 60) + "'" + juce::String (whole % 60).paddedLeft ('0', 2) + "\")";
}

void AudioOutput::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate() > 0.0 ? device->getCurrentSampleRate() : 48000.0;
    latencyMs = device->getOutputLatencyInSamples() * 1000.0 / sampleRate;
    synth.prepare (sampleRate);
    clock.reset();
}

void AudioOutput::audioDeviceStopped()
{
    clock.reset();
}

void AudioOutput::audioDeviceIOCallbackWithContext (const float* const*, int, float* const* outputs, int numOutputs,
                                                    int numSamples, const juce::AudioIODeviceCallbackContext& context)
{
    for (int c = 0; c < numOutputs; ++c)
        if (outputs[c] != nullptr) juce::FloatVectorOperations::clear (outputs[c], numSamples);

    const double heardAt = clock.blockTime (maket::OscScheduler::now(), numSamples, sampleRate, context.hostTimeNs, latencyMs);
    synth.render (numOutputs > 0 ? outputs[0] : nullptr, numOutputs > 1 ? outputs[1] : nullptr, numSamples, heardAt);
    if (soundPlayer != nullptr)
        soundPlayer->render (numOutputs > 0 ? outputs[0] : nullptr, numOutputs > 1 ? outputs[1] : nullptr, numSamples, sampleRate);
    if (surfaceEngine != nullptr)
        surfaceEngine->render (numOutputs > 0 ? outputs[0] : nullptr, numOutputs > 1 ? outputs[1] : nullptr, numSamples, sampleRate);
}
