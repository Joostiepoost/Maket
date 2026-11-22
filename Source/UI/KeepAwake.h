// KeepAwake.h — keeps macOS from slowing Maket down while it has messages
// to send.
//
// macOS "naps" an app that is not in front: its timers then fire late, by
// up to a tenth of a second. For Maket that means notes that leave late as
// soon as another app is used while a piece plays (measured on 22 Nov 2026:
// up to 99 ms, while typing in a browser). This tells the system that Maket
// is busy for as long as the send thread has messages waiting, and a few
// seconds beyond. See Source/Platform/BusyActivity.h for how.
//
// On other systems it does nothing.

#pragma once

#include <JuceHeader.h>

#include "../Engine/OscScheduler.h"
#include "../Platform/BusyActivity.h"

class KeepAwake : private juce::Timer
{
public:
    explicit KeepAwake (maket::OscScheduler& schedulerToWatch) : scheduler (schedulerToWatch)
    {
        startTimer (200);
    }

    ~KeepAwake() override
    {
        stopTimer();
        release();
    }

private:
    void timerCallback() override
    {
        const auto now = juce::Time::getMillisecondCounter();

        if (scheduler.numWaiting() > 0)
        {
            lastBusy = now;
            if (activity == nullptr) activity = maket::platform::beginBusyActivity();
        }
        else if (activity != nullptr && now - lastBusy > 5000)
        {
            release();
        }
    }

    void release()
    {
        maket::platform::endBusyActivity (activity);
        activity = nullptr;
    }

    maket::OscScheduler& scheduler;
    void* activity = nullptr;       // from beginBusyActivity; null while not busy
    juce::uint32 lastBusy = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeepAwake)
};
