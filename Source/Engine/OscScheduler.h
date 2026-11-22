// OscScheduler.h — sends OSC messages at their time.
//
// A message is given a time and one or two places to go. A thread of its
// own waits for that time and sends it; the interface never waits, and a
// busy interface does not hold a message up. Of every message it keeps when
// it was due and when it really left, so that Maket's own punctuality can be
// seen apart from what happens afterwards in the network and in Max.
//
// Times are in milliseconds on the clock that now() reads.
//
// From step 2 on the audio device's sample clock is the reference for all
// playback; this thread is then told its times from that clock. For the
// tests of step 1 it runs on the computer's own high-resolution clock.
//
// No interface code.

#pragma once

#include <JuceHeader.h>
#include <map>
#include <vector>

#include "Osc.h"
#include "OscSender.h"

namespace maket
{

class OscScheduler : private juce::Thread
{
public:
    /** Where a message goes: one of these, or both added up. */
    enum Target
    {
        toPlayer = 1,       // the conTimbre ePlayer in Max
        toLog    = 2        // Maket's logging patch in Max
    };

    /** One message that left. */
    struct Sent
    {
        double due = 0.0;       // when it had to go
        double sent = 0.0;      // when it was handed to the system
        juce::String text;      // the message on one line
        int targets = 0;
        int tag = 0;            // who asked for it (see sendAt)
        bool ok = true;         // false when the system refused it
    };

    OscScheduler();
    ~OscScheduler() override;

    void setPlayer (const juce::String& host, int port)     { player.setTarget (host, port); }
    void setLog (const juce::String& host, int port)        { log.setTarget (host, port); }

    /** The time now, in milliseconds. */
    static double now()                                     { return juce::Time::getMillisecondCounterHiRes(); }

    /** Sends a message at `time`; a time that has passed means at once.
        Messages with the same time leave in the order they were given.
        `tag` is any number of the caller's choice; it comes back in the
        record of what was sent, so that several users of the thread can
        each ask for their own messages. */
    void sendAt (double time, const OscMessage& message, int targets, int tag = 0);

    /** Sends a message now, from the thread that calls, without waiting for
        the send thread. For the last word when Maket quits. */
    void sendImmediately (const OscMessage& message, int targets, int tag = 0);

    /** How many messages are still waiting for their time. */
    int numWaiting() const;

    /** Sends everything that is still waiting at once, in order. */
    void sendAllWaitingNow();

    /** Forgets everything that is still waiting. */
    void clear();

    /** What left since the last call, in the order it left. */
    std::vector<Sent> takeSent();

    /** The same, for the messages with this tag only; the others stay. */
    std::vector<Sent> takeSent (int tag);

    /** True when the thread got the system's realtime priority. */
    bool isRealtime() const                                 { return realtime; }

private:
    struct Event
    {
        OscMessage message;
        int targets = 0;
        int tag = 0;
    };

    void run() override;
    void sendNow (double due, const Event& event);

    mutable juce::CriticalSection lock;
    std::multimap<double, Event> waiting;       // by time; equal times keep their order
    std::vector<Sent> sentLog;
    juce::WaitableEvent wake;
    OscSender player, log;
    bool realtime = false;

    JUCE_DECLARE_NON_COPYABLE (OscScheduler)
};

} // namespace maket
