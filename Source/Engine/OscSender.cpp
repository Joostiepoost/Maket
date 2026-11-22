// OscSender.cpp

#include "OscSender.h"

namespace maket
{

OscSender::OscSender() = default;

void OscSender::setTarget (const juce::String& hostToUse, int portToUse)
{
    const juce::ScopedLock sl (lock);
    host = hostToUse.trim();
    port = portToUse;
}

juce::String OscSender::getHost() const
{
    const juce::ScopedLock sl (lock);
    return host;
}

int OscSender::getPort() const
{
    const juce::ScopedLock sl (lock);
    return port;
}

bool OscSender::send (const OscMessage& message)
{
    const auto bytes = message.encode();

    const juce::ScopedLock sl (lock);
    if (host.isEmpty() || port < 1 || port > 65535) return false;

    return socket.write (host, port, bytes.getData(), (int) bytes.getSize()) == (int) bytes.getSize();
}

} // namespace maket
