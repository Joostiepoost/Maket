// StreamClock.h — at which moment a block of audio will be heard, on the
// clock of the send thread (OscScheduler::now).
//
// The audio device asks for sound a block at a time, a little before it is
// heard. On a Mac it says with every block when the block's first sample
// leaves for the hardware, on the very clock Maket's send thread uses; that
// time is taken as it is, plus the delay the hardware reports. Where the
// device says nothing (other systems), the time of the call is used
// instead: the calls come unevenly, so the block's time is counted on from
// the block before and only nudged towards the call's time, and set anew
// after a gap.
//
// No interface code and no audio device here: the console test program
// feeds it made-up calls.

#pragma once

#include <cstdint>

namespace maket
{

class StreamClock
{
public:
    void reset()            { running = false; }

    /** The moment (ms) at which the first sample of this block is heard.
        `now`: the clock at the call. `hostTimeNs`: when the device says the block leaves, or null.
        `latencyMs`: the output delay the device reports. */
    double blockTime (double now, int numSamples, double sampleRate, const std::uint64_t* hostTimeNs, double latencyMs);

    /** True when the last block's time came from the device itself. */
    bool usesDeviceTime() const     { return fromDevice; }

    static constexpr double nudge = 0.02;           // the share of the difference taken up with each block
    static constexpr double gap = 60.0;             // ms off the count that count as a break in the stream

private:
    bool running = false, fromDevice = false;
    double next = 0.0;      // without device time: where the count says the coming block begins
};

} // namespace maket
