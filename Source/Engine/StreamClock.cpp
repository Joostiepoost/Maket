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
            next = device + latencyMs + length;
            return device + latencyMs;
        }
    }

    fromDevice = false;
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

} // namespace maket
