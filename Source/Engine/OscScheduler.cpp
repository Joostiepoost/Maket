// OscScheduler.cpp

#include "OscScheduler.h"

#include <chrono>
#include <thread>

namespace maket
{

namespace
{
    constexpr size_t maxLogSize = 200000;       // the log is emptied by takeSent(); this is only a ceiling
}

OscScheduler::OscScheduler() : juce::Thread ("Maket OSC")
{
    // A realtime thread, as audio threads are: the system then wakes it at
    // the time it asked for and not somewhat later. Where the system does
    // not grant that, the highest ordinary priority.
    realtime = startRealtimeThread (RealtimeOptions().withPriority (9)
                                                     .withPeriodMs (1.0)
                                                     .withProcessingTimeMs (0.1)
                                                     .withMaximumProcessingTimeMs (0.5));
    if (! realtime)
        startThread (Priority::highest);
}

OscScheduler::~OscScheduler()
{
    signalThreadShouldExit();
    wake.signal();
    stopThread (2000);
}

void OscScheduler::sendAt (double time, const OscMessage& message, int targets, int tag)
{
    {
        const juce::ScopedLock sl (lock);
        waiting.insert ({ time, Event { message, targets, tag } });
    }
    wake.signal();
}

void OscScheduler::sendImmediately (const OscMessage& message, int targets, int tag)
{
    sendNow (now(), Event { message, targets, tag });
}

int OscScheduler::numWaiting() const
{
    const juce::ScopedLock sl (lock);
    return (int) waiting.size();
}

void OscScheduler::clear()
{
    const juce::ScopedLock sl (lock);
    waiting.clear();
}

std::vector<OscScheduler::Sent> OscScheduler::takeSent()
{
    std::vector<Sent> result;
    const juce::ScopedLock sl (lock);
    result.swap (sentLog);
    return result;
}

std::vector<OscScheduler::Sent> OscScheduler::takeSent (int tag)
{
    std::vector<Sent> result, others;
    const juce::ScopedLock sl (lock);
    for (auto& record : sentLog)
        (record.tag == tag ? result : others).push_back (std::move (record));
    sentLog.swap (others);
    return result;
}

void OscScheduler::sendNow (double due, const Event& event)
{
    Sent record;
    record.due = due;
    record.targets = event.targets;
    record.tag = event.tag;
    record.text = event.message.toText();
    record.sent = now();

    if ((event.targets & toPlayer) != 0) record.ok = player.send (event.message) && record.ok;
    if ((event.targets & toLog) != 0)    record.ok = log.send (event.message) && record.ok;

    const juce::ScopedLock sl (lock);
    if (sentLog.size() < maxLogSize) sentLog.push_back (std::move (record));
}

void OscScheduler::sendAllWaitingNow()
{
    std::multimap<double, Event> all;
    {
        const juce::ScopedLock sl (lock);
        all.swap (waiting);
    }
    for (const auto& [due, event] : all) sendNow (due, event);
}

void OscScheduler::run()
{
    while (! threadShouldExit())
    {
        double due = 0.0;
        bool any = false;
        {
            const juce::ScopedLock sl (lock);
            any = ! waiting.empty();
            if (any) due = waiting.begin()->first;
        }

        if (! any)
        {
            wake.wait (500.0);
            continue;
        }

        const double ahead = due - now();

        if (ahead > 1.0)
        {
            // Sleep until just before the time. A message given in the
            // meantime wakes the thread, which then looks again.
            wake.wait (ahead - 0.8);
            continue;
        }

        if (ahead > 0.03)
        {
            // The last millisecond in short naps, so as not to be late.
            std::this_thread::sleep_for (std::chrono::microseconds (ahead > 0.25 ? 100 : 20));
            continue;
        }

        Event event;
        bool taken = false;
        {
            const juce::ScopedLock sl (lock);
            if (! waiting.empty() && waiting.begin()->first <= due)
            {
                due = waiting.begin()->first;
                event = waiting.begin()->second;
                waiting.erase (waiting.begin());
                taken = true;
            }
        }
        if (taken) sendNow (due, event);
    }
}

} // namespace maket
