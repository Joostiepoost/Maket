// KeepAwake.h — keeps macOS from slowing Maket down while it has messages
// to send.
//
// macOS saves energy by "napping" an app that is not in front: its timers
// then fire late, by up to a tenth of a second. For most apps that is
// harmless; for Maket it means notes that leave late as soon as another app
// is used while a piece plays (measured on 22 Nov 2026: up to 99 ms late
// while typing in a browser). An app may tell the system that it is busy
// with something the user asked for; then it is left alone. That is what
// this does, for as long as the send thread has messages waiting and a few
// seconds beyond. It does not keep the Mac itself from going to sleep.
//
// On other systems it does nothing.

#pragma once

#include <JuceHeader.h>
#include <memory>

#include "../Engine/OscScheduler.h"

class KeepAwake : private juce::Timer
{
public:
    explicit KeepAwake (maket::OscScheduler& schedulerToWatch) : scheduler (schedulerToWatch)
    {
        startTimer (200);
    }

    ~KeepAwake() override   { stopTimer(); }

    bool isHolding() const  { return holder != nullptr; }

private:
    void timerCallback() override
    {
        const auto now = juce::Time::getMillisecondCounter();

        if (scheduler.numWaiting() > 0)
        {
            lastBusy = now;
            if (holder == nullptr) holder = std::make_unique<juce::ScopedLowPowerModeDisabler>();
        }
        else if (holder != nullptr && now - lastBusy > 5000)
        {
            holder.reset();
        }
    }

    maket::OscScheduler& scheduler;
    std::unique_ptr<juce::ScopedLowPowerModeDisabler> holder;
    juce::uint32 lastBusy = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeepAwake)
};
