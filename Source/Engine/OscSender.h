// OscSender.h — sends OSC messages to one place: a computer (host) and a
// port on it, over UDP.
//
// UDP has no answer: a message that was handed to the system counts as
// sent, whether or not anything is listening at the other end. So "sent"
// never means "arrived". Whether Max got a message can only be seen in Max.
//
// No interface code.

#pragma once

#include <JuceHeader.h>
#include "Osc.h"

namespace maket
{

class OscSender
{
public:
    OscSender();

    /** Where the messages go. May be called at any time, from any thread. */
    void setTarget (const juce::String& host, int port);

    juce::String getHost() const;
    int getPort() const;

    /** Hands one message to the system. False when the system refused it
        (no target set, a host name that does not exist, a packet too large). */
    bool send (const OscMessage& message);

private:
    mutable juce::CriticalSection lock;
    juce::DatagramSocket socket { false };
    juce::String host;
    int port = 0;

    JUCE_DECLARE_NON_COPYABLE (OscSender)
};

} // namespace maket
