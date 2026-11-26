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

#include <atomic>
#include <cstdint>

namespace maket
{

class StreamClock
{
public:
    void reset()            { running = false; counting = false; measuredSeconds.store (0.0); measuredPpm.store (0.0); }

    /** The moment (ms) at which the first sample of this block is heard.
        `now`: the clock at the call. `hostTimeNs`: when the device says the block leaves, or null.
        `latencyMs`: the output delay the device reports. */
    double blockTime (double now, int numSamples, double sampleRate, const std::uint64_t* hostTimeNs, double latencyMs);

    /** True when the last block's time came from the device itself. */
    bool usesDeviceTime() const     { return fromDevice; }

    /** Stage 3a: the sound card's clock against the computer's. The samples the device has asked for since the count
        began, against the time that passed. Positive: the sound card runs fast (it plays more than a second of samples
        in a second of the computer's clock). Parts in a million; one part in a million is 0.6 ms in ten minutes.
        Safe to read from another thread. */
    double driftPpm() const             { return measuredPpm.load(); }
    /** Over how many seconds the drift was measured; nothing is said under `driftFrom` seconds. */
    double driftSeconds() const         { return measuredSeconds.load(); }
    static constexpr double driftFrom = 20.0;
    static constexpr double jump = 5.0;             // ms: the count and the clock part by this much at once: a break, count anew

    static constexpr double nudge = 0.02;           // the share of the difference taken up with each block
    static constexpr double gap = 60.0;             // ms off the count that count as a break in the stream

private:
    bool running = false, fromDevice = false;
    double next = 0.0;      // without device time: where the count says the coming block begins

    void count (double blockStart, int numSamples, double sampleRate, double tolerance);
    bool counting = false;
    double countFrom = 0.0, countedMs = 0.0, lastOff = 0.0;
    std::atomic<double> measuredPpm { 0.0 }, measuredSeconds { 0.0 };
};

} // namespace maket
