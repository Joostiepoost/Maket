// Osc.h — one OSC message: what it holds, its bytes on the wire, and back.
//
// OSC (Open Sound Control) is the form in which Maket talks to Max over the
// network: the conTimbre ePlayer, and later the custom engines. A message is
// an address (a word such as `noteon`) followed by arguments: whole numbers,
// real numbers and texts.
//
// Maket writes the bytes itself and does not use JUCE's OSC module, for one
// reason: the ePlayer's commands are plain words (`noteon`, `noteoff`,
// `program`), as Max's own udpsend object sends them, and JUCE refuses an
// address that does not begin with a slash. Here an address is sent exactly
// as it stands, with or without one.
//
// The byte layout is OSC 1.0: the address, then a text of type letters that
// begins with a comma (`,sfi`), then the arguments; every text ends with a
// zero byte and is filled up with zero bytes to a multiple of four; numbers
// are four bytes, highest byte first.
//
// No interface code.

#pragma once

#include <JuceHeader.h>
#include <vector>

namespace maket
{

struct OscArgument
{
    enum class Kind { integer, real, text };

    Kind kind = Kind::integer;
    juce::int32 integer = 0;
    double real = 0.0;          // the value as it was given; it travels as a 32-bit real number
    juce::String text;

    /** Equal as they travel: real numbers are compared as 32-bit numbers. */
    bool operator== (const OscArgument& other) const;
    bool operator!= (const OscArgument& other) const    { return ! (*this == other); }
};

struct OscMessage
{
    juce::String address;                   // `noteon`, or `/maket/custom1`
    std::vector<OscArgument> arguments;

    OscMessage() = default;
    explicit OscMessage (const juce::String& addressToUse) : address (addressToUse) {}

    OscMessage& addInteger (int value);
    OscMessage& addReal (double value);     // sent as a 32-bit real number, which is what Max works with
    OscMessage& addText (const juce::String& value);

    /** The message as it travels: the bytes of one UDP packet. */
    juce::MemoryBlock encode() const;

    /** Reads the bytes of one packet. False when they are not one OSC message
        of whole numbers, real numbers and texts. */
    static bool decode (const void* data, size_t size, OscMessage& result);

    /** The message on one line, as it shows in the Max window: noteon Fl 60. 100 */
    juce::String toText() const;

    /** A message from one typed line, the other way round from toText: the
        first word is the command, a word of digits a whole number, a number
        with a point (or an exponent) a real one, anything else a text; a
        text with spaces goes in double quotes. False when the line is
        empty or a quote is left open. */
    static bool fromText (const juce::String& line, OscMessage& result);

    /** Several commands typed on one line, with a semicolon between them
        (a semicolon inside quotes is text). False, and nothing in `result`,
        when one of them cannot be read or the line holds none. */
    static bool severalFromText (const juce::String& line, std::vector<OscMessage>& result);

    bool operator== (const OscMessage& other) const;
    bool operator!= (const OscMessage& other) const     { return ! (*this == other); }
};

} // namespace maket
