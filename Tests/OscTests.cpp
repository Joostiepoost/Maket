// OscTests.cpp — stage 1c: OSC messages, the sender and the send thread.
//
// Nothing here needs Max. The bytes of a message are held against bytes
// worked out by hand from the OSC 1.0 description. Messages are then sent
// to a listener inside this program, over the computer's own network
// loop, to see that they arrive whole and in order, and how punctually the
// send thread lets them go. What Max does with them is tested on Joost's
// Mac, with the logging patch.

#include <JuceHeader.h>
#include <algorithm>
#include <cmath>
#include <vector>

#include "TestReport.h"
#include "../Source/Engine/Osc.h"
#include "../Source/Engine/OscSender.h"
#include "../Source/Engine/OscScheduler.h"
#include "../Source/Engine/ConTimbre.h"

using namespace testreport;
using maket::OscMessage;
using maket::OscScheduler;

namespace
{

juce::String hex (const juce::MemoryBlock& bytes)
{
    juce::String s;
    for (size_t i = 0; i < bytes.getSize(); ++i)
    {
        if (i > 0) s << (i % 4 == 0 ? "  " : " ");
        s << juce::String::toHexString ((int) (juce::uint8) bytes[i]).paddedLeft ('0', 2);
    }
    return s;
}

bool sameBytes (const juce::MemoryBlock& bytes, const std::vector<int>& expected)
{
    if (bytes.getSize() != expected.size()) return false;
    for (size_t i = 0; i < expected.size(); ++i)
        if ((int) (juce::uint8) bytes[i] != expected[i]) return false;
    return true;
}

/** A listener on this computer, on a port the system picks. */
struct Listener
{
    Listener()                      { ok = socket.bindToPort (0, "127.0.0.1"); }
    int port() const                { return socket.getBoundPort(); }

    /** Waits up to `timeoutMs` for one packet; false when none came. */
    bool receive (OscMessage& message, int timeoutMs)
    {
        if (socket.waitUntilReady (true, timeoutMs) != 1) return false;
        juce::uint8 buffer[2048];
        const int size = socket.read (buffer, (int) sizeof (buffer), false);
        return size > 0 && OscMessage::decode (buffer, (size_t) size, message);
    }

    juce::DatagramSocket socket { false };
    bool ok = false;
};

double percentile (std::vector<double> values, double fraction)
{
    if (values.empty()) return 0.0;
    std::sort (values.begin(), values.end());
    const auto index = (size_t) std::llround (fraction * (double) (values.size() - 1));
    return values[index];
}

juce::String ms (double value)      { return juce::String (value, 3); }

// --- The bytes ----------------------------------------------------------------------

void testBytes()
{
    line ("### The bytes of a message");
    line();

    const auto noteOn = maket::contimbre::noteOn ("Fl", 60.0, 100);
    const auto noteOnBytes = noteOn.encode();

    line ("`" + noteOn.toText() + "` as it travels, " + juce::String ((int) noteOnBytes.getSize()) + " bytes (in groups of four):");
    line();
    line ("```");
    line (hex (noteOnBytes));
    line ("```");
    line();
    line ("That is: the word `noteon` and two zero bytes; the kinds of the arguments `,sfi` (a text, a real number, a whole "
          "number) and four zero bytes; `Fl` and two zero bytes; 60.0; 100.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    checkRow (sameBytes (noteOnBytes, { 0x6e, 0x6f, 0x74, 0x65,  0x6f, 0x6e, 0x00, 0x00,
                                        0x2c, 0x73, 0x66, 0x69,  0x00, 0x00, 0x00, 0x00,
                                        0x46, 0x6c, 0x00, 0x00,
                                        0x42, 0x70, 0x00, 0x00,
                                        0x00, 0x00, 0x00, 0x64 }),
              "`noteon Fl 60. 100` gives the 28 bytes worked out by hand from the OSC description");

    checkRow (sameBytes (maket::contimbre::allNotesOff().encode(),
                         { 0x61, 0x6c, 0x6c, 0x5f,  0x6e, 0x6f, 0x74, 0x65,  0x73, 0x5f, 0x6f, 0x66,  0x66, 0x00, 0x00, 0x00,
                           0x2c, 0x00, 0x00, 0x00 }),
              "`all_notes_off`, a command without arguments, gives its 20 bytes");

    checkRow (maket::contimbre::noteOn ("Fl", 60.0, 100, maket::contimbre::CommandForm::withSlash).toText() == "/noteon Fl 60. 100"
              && maket::contimbre::noteOff ("Fl", 60.5).toText() == "noteoff Fl 60.5"
              && maket::contimbre::program ("Alto Flute", "alto flute.multiphonic").toText() == "program \"Alto Flute\" \"alto flute.multiphonic\"",
              "The commands read `noteon`, `noteoff`, `program`, with a slash in front only when asked; a name with a space stays one argument");

    // There and back.
    std::vector<OscMessage> messages;
    messages.push_back (noteOn);
    messages.push_back (maket::contimbre::noteOff ("Fl", 60.5));
    messages.push_back (maket::contimbre::program ("Alto Flute", "alto flute.multiphonic"));
    messages.push_back (maket::contimbre::allNotesOff (maket::contimbre::CommandForm::withSlash));
    messages.push_back (OscMessage ("/maket/custom1").addInteger (-5).addReal (-24.0).addReal (71.02)
                            .addText (juce::String::fromUTF8 ("Fl\xc3\xb6te")).addText ("").addText ("abcd").addInteger (2147483647));

    bool allBack = true, allFours = true;
    for (const auto& m : messages)
    {
        const auto bytes = m.encode();
        OscMessage back;
        allBack = allBack && OscMessage::decode (bytes.getData(), bytes.getSize(), back) && back == m;
        allFours = allFours && bytes.getSize() % 4 == 0;
    }
    checkRow (allBack && allFours, "Five messages (texts with spaces, with an accent, empty, of four letters; negative numbers) "
                                   "read back from their bytes as they were, each a multiple of four bytes long");

    OscMessage unused;
    const juce::uint8 noEnd[4]   = { 'a', 'b', 'c', 'd' };
    const juce::uint8 oddSize[5] = { 'a', 0, 0, 0, 0 };
    const juce::uint8 blob[12]   = { 'a', 0, 0, 0,  ',', 'b', 0, 0,  0, 0, 0, 0 };
    checkRow (! OscMessage::decode (noEnd, 4, unused) && ! OscMessage::decode (oddSize, 5, unused)
              && ! OscMessage::decode (blob, 12, unused) && ! OscMessage::decode (nullptr, 0, unused),
              "Bytes that are not such a message are refused");
    line();
}

// --- Sending ------------------------------------------------------------------------

void testSending()
{
    line ("### Sending, on this computer");
    line();
    line ("A listener inside the test program takes the place of Max. This shows that the messages leave whole and in "
          "order, and how punctual the send thread is. It says nothing about Max: that is measured with the logging patch.");
    line();

    Listener listener;
    if (! listener.ok)
    {
        check (false, "A listener could be opened on this computer");
        line ("**Not tested:** no listening port could be opened on 127.0.0.1.");
        line();
        return;
    }

    line ("| Check | Passed |");
    line ("| --- | --- |");

    // 1. The sender alone.
    {
        maket::OscSender sender;
        const bool refused = ! sender.send (maket::contimbre::allNotesOff());      // no target yet
        sender.setTarget ("127.0.0.1", listener.port());

        bool allOk = true;
        const int count = 50;
        for (int i = 0; i < count; ++i)
        {
            const auto out = maket::contimbre::noteOn ("Fl", 60.0 + 0.25 * i, 1 + i);
            OscMessage in;
            allOk = allOk && sender.send (out) && listener.receive (in, 1000) && in == out;
        }
        checkRow (refused && allOk, "Without a target nothing is sent; with one, " + juce::String (count)
                                    + " messages arrive at the listener one by one, each as it was sent");
    }

    // 2. The send thread: a run of messages at their times.
    const int count = 200;
    const double spacing = 5.0;
    std::vector<double> late, arrivedAfterDue;
    bool allArrived = true, inOrder = true, allSentOk = true, sameOrderInLog = true;
    bool realtime = false;
    {
        OscScheduler scheduler;
        realtime = scheduler.isRealtime();
        scheduler.setPlayer ("127.0.0.1", listener.port());

        const double start = OscScheduler::now() + 150.0;
        for (int i = 0; i < count; ++i)
            scheduler.sendAt (start + spacing * i, OscMessage ("/maket/tick").addInteger (i), OscScheduler::toPlayer);

        for (int i = 0; i < count; ++i)
        {
            OscMessage in;
            if (! listener.receive (in, 2000)) { allArrived = false; break; }
            const double arrival = OscScheduler::now();
            inOrder = inOrder && in.address == "/maket/tick" && in.arguments.size() == 1 && in.arguments[0].integer == i;
            arrivedAfterDue.push_back (arrival - (start + spacing * i));
        }

        const auto sent = scheduler.takeSent();
        allSentOk = (int) sent.size() == count;
        for (size_t i = 0; i < sent.size(); ++i)
        {
            allSentOk = allSentOk && sent[i].ok;
            sameOrderInLog = sameOrderInLog && sent[i].text == "/maket/tick " + juce::String ((int) i);
            late.push_back (sent[i].sent - sent[i].due);
        }
    }

    const double earliest = late.empty() ? 0.0 : *std::min_element (late.begin(), late.end());
    const double latest = late.empty() ? 0.0 : *std::max_element (late.begin(), late.end());

    checkRow (allArrived && inOrder && allSentOk && sameOrderInLog,
              juce::String (count) + " messages, one every " + juce::String (spacing, 0) + " ms, all leave and all arrive, in order");
    checkRow (! late.empty() && earliest > -0.05, "None leaves before its time (the earliest: " + ms (earliest) + " ms)");
    checkRow (! late.empty() && percentile (late, 0.95) < 2.0,
              "95 in 100 leave within 2 ms after their time (within " + ms (percentile (late, 0.95)) + " ms; the latest of all: " + ms (latest)
              + " ms, a measurement and not a check, since a busy computer can hold any program up for a moment)");

    // 3. Equal times keep their order; what waits can be sent at once.
    {
        OscScheduler scheduler;
        scheduler.setPlayer ("127.0.0.1", listener.port());

        const double together = OscScheduler::now() + 50.0;
        for (int i = 0; i < 6; ++i)
            scheduler.sendAt (together, OscMessage ("/maket/tick").addInteger (i), OscScheduler::toPlayer);

        bool order = true;
        for (int i = 0; i < 6; ++i)
        {
            OscMessage in;
            order = order && listener.receive (in, 1000) && in.arguments.size() == 1 && in.arguments[0].integer == i;
        }
        checkRow (order, "Six messages with the same time (a chord) leave in the order they were given");

        const double before = OscScheduler::now();
        scheduler.sendAt (before + 60000.0, maket::contimbre::noteOff ("Fl", 60.0), OscScheduler::toPlayer);
        const bool waits = scheduler.numWaiting() == 1;
        scheduler.sendAllWaitingNow();

        OscMessage in;
        const bool came = listener.receive (in, 1000) && in == maket::contimbre::noteOff ("Fl", 60.0);
        checkRow (waits && came && scheduler.numWaiting() == 0 && OscScheduler::now() - before < 1000.0,
                  "A note-off that is due in a minute is sent at once when asked (as when Maket quits)");
    }

    line();
    line ("How punctual the send thread was in the run of " + juce::String (count) + " messages, in milliseconds after the time each was due "
          "(the thread " + (realtime ? "has" : "did not get") + " the system's realtime priority):");
    line();
    line ("| | Half of them within | 95 in 100 within | The latest |");
    line ("| --- | --- | --- | --- |");
    line ("| Left Maket | " + ms (percentile (late, 0.5)) + " | " + ms (percentile (late, 0.95)) + " | " + ms (latest) + " |");
    line ("| Arrived at the listener in this program | " + ms (percentile (arrivedAfterDue, 0.5)) + " | "
          + ms (percentile (arrivedAfterDue, 0.95)) + " | "
          + ms (arrivedAfterDue.empty() ? 0.0 : *std::max_element (arrivedAfterDue.begin(), arrivedAfterDue.end())) + " |");
    line();
}

} // namespace

void testOsc()
{
    line ("## 5. OSC: messages and sending");
    line();
    line ("Stage 1c: Maket's side of the connection to Max. An OSC message is made, turned into the bytes of a network "
          "packet, and sent at its time by a thread of its own.");
    line();

    testBytes();
    testSending();
}
