// StreamClock.cpp — see StreamClock.h.

#include "StreamClock.h"

#include <cmath>

namespace maket
{

double StreamClock::blockTime (double now, int numSamples, double sampleRate, const std::uint64_t* hostTimeNs, double latencyMs)
{
    const double length = sampleRate > 0.0 ? numSamples * 1000.0 / sampleRate : 0.0;

    if (hostTimeNs != nullptr)
    {
        // The device's own time, when it lies near the clock's (the same clock on a Mac).
        const double device = (double) *hostTimeNs / 1.0e6;
        if (std::abs (device - now) < 500.0)
        {
            fromDevice = true;
            running = true;
            count (device, numSamples, sampleRate, jump);
            next = device + latencyMs + length;
            return device + latencyMs;
        }
    }

    fromDevice = false;
    count (now, numSamples, sampleRate, gap);          // the calls come unevenly: only a real break counts as one
    const double called = now + latencyMs;      // the block is asked for about one block ahead of its sound; the device's delay comes on top
    double time = called;
    if (running)
    {
        const double off = called - next;
        time = std::abs (off) > gap ? called : next + nudge * off;
    }
    running = true;
    next = time + length;
    return time;
}

void StreamClock::count (double blockStart, int numSamples, double sampleRate, double tolerance)
{
    const double length = sampleRate > 0.0 ? numSamples * 1000.0 / sampleRate : 0.0;
    if (! counting)
    {
        counting = true;
        countFrom = blockStart;
        countedMs = 0.0;
        lastOff = 0.0;
    }
    else
    {
        // By the count this block begins at countFrom + countedMs; by the clock at blockStart.
        const double passed = blockStart - countFrom;
        const double off = countedMs - passed;
        if (std::abs (off - lastOff) > tolerance || passed <= 0.0)
        {
            // Blocks were lost, or the device stopped for a while: the count says nothing any more.
            countFrom = blockStart;
            countedMs = 0.0;
            lastOff = 0.0;
            measuredSeconds.store (0.0);
            measuredPpm.store (0.0);
        }
        else
        {
            lastOff = off;
            measuredSeconds.store (passed / 1000.0);
            measuredPpm.store (off / passed * 1.0e6);
        }
    }
    countedMs += length;
}

} // namespace maket
