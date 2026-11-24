// Osc.cpp

#include "Osc.h"

#include <vector>

#include <cmath>
#include <cstring>

namespace maket
{

bool OscArgument::operator== (const OscArgument& other) const
{
    if (kind != other.kind) return false;
    switch (kind)
    {
        case Kind::integer: return integer == other.integer;
        case Kind::real:
        {
            const float mine = (float) real, theirs = (float) other.real;
            return std::memcmp (&mine, &theirs, sizeof (float)) == 0;
        }
        case Kind::text:    return text == other.text;
    }
    return false;
}

OscMessage& OscMessage::addInteger (int value)
{
    OscArgument a;
    a.kind = OscArgument::Kind::integer;
    a.integer = (juce::int32) value;
    arguments.push_back (a);
    return *this;
}

OscMessage& OscMessage::addReal (double value)
{
    OscArgument a;
    a.kind = OscArgument::Kind::real;
    a.real = value;
    arguments.push_back (a);
    return *this;
}

OscMessage& OscMessage::addText (const juce::String& value)
{
    OscArgument a;
    a.kind = OscArgument::Kind::text;
    a.text = value;
    arguments.push_back (a);
    return *this;
}

bool OscMessage::operator== (const OscMessage& other) const
{
    return address == other.address && arguments == other.arguments;
}

namespace
{
    /** A text, its closing zero byte, and zero bytes up to a multiple of four. */
    void appendText (juce::MemoryBlock& out, const juce::String& text)
    {
        const size_t length = text.getNumBytesAsUTF8();
        out.append (text.toRawUTF8(), length);

        const char zeros[4] = { 0, 0, 0, 0 };
        out.append (zeros, 4 - (length % 4));
    }

    void appendFourBytes (juce::MemoryBlock& out, juce::uint32 value)
    {
        const juce::uint8 bytes[4] = { (juce::uint8) (value >> 24), (juce::uint8) (value >> 16),
                                       (juce::uint8) (value >> 8),  (juce::uint8) value };
        out.append (bytes, 4);
    }

    /** Reads a text at `position` and moves on past its filling. */
    bool readText (const juce::uint8* data, size_t size, size_t& position, juce::String& text)
    {
        size_t end = position;
        while (end < size && data[end] != 0) ++end;
        if (end >= size) return false;                              // no closing zero byte

        text = juce::String::fromUTF8 ((const char*) data + position, (int) (end - position));
        position = end + (4 - ((end - position) % 4));
        return position <= size;
    }

    bool readFourBytes (const juce::uint8* data, size_t size, size_t& position, juce::uint32& value)
    {
        if (position + 4 > size) return false;
        value = ((juce::uint32) data[position] << 24) | ((juce::uint32) data[position + 1] << 16)
              | ((juce::uint32) data[position + 2] << 8) | (juce::uint32) data[position + 3];
        position += 4;
        return true;
    }
}

juce::MemoryBlock OscMessage::encode() const
{
    juce::MemoryBlock out;
    appendText (out, address);

    juce::String types (",");
    for (const auto& a : arguments)
        types << (a.kind == OscArgument::Kind::integer ? "i" : a.kind == OscArgument::Kind::real ? "f" : "s");
    appendText (out, types);

    for (const auto& a : arguments)
    {
        if (a.kind == OscArgument::Kind::integer)
        {
            appendFourBytes (out, (juce::uint32) a.integer);
        }
        else if (a.kind == OscArgument::Kind::real)
        {
            const float asSent = (float) a.real;
            juce::uint32 bits = 0;
            std::memcpy (&bits, &asSent, sizeof (bits));
            appendFourBytes (out, bits);
        }
        else
        {
            appendText (out, a.text);
        }
    }
    return out;
}

bool OscMessage::decode (const void* rawData, size_t size, OscMessage& result)
{
    result = OscMessage();
    const auto* data = static_cast<const juce::uint8*> (rawData);
    if (data == nullptr || size == 0 || size % 4 != 0) return false;

    size_t position = 0;
    juce::String types;
    if (! readText (data, size, position, result.address)) return false;
    if (position == size) return true;                              // an address alone
    if (! readText (data, size, position, types) || ! types.startsWithChar (',')) return false;

    for (int i = 1; i < types.length(); ++i)
    {
        const auto type = types[i];
        juce::uint32 bits = 0;

        if (type == 'i')
        {
            if (! readFourBytes (data, size, position, bits)) return false;
            result.addInteger ((int) (juce::int32) bits);
        }
        else if (type == 'f')
        {
            if (! readFourBytes (data, size, position, bits)) return false;
            float value = 0.0f;
            std::memcpy (&value, &bits, sizeof (value));
            result.addReal ((double) value);
        }
        else if (type == 's')
        {
            juce::String text;
            if (! readText (data, size, position, text)) return false;
            result.addText (text);
        }
        else
        {
            return false;                                           // a kind of argument Maket does not send
        }
    }
    return position == size;
}

bool OscMessage::fromText (const juce::String& line, OscMessage& result)
{
    juce::StringArray words;
    juce::String word;
    bool inQuotes = false, quoted = false;
    std::vector<bool> wasQuoted;

    auto finish = [&]
    {
        if (word.isNotEmpty() || quoted) { words.add (word); wasQuoted.push_back (quoted); }
        word.clear();
        quoted = false;
    };

    for (auto c : line.trim())
    {
        if (c == '"')                                           { inQuotes = ! inQuotes; quoted = true; }
        else if (! inQuotes && (c == ' ' || c == '\t'))         finish();
        else                                                    word << juce::String::charToString (c);
    }
    if (inQuotes) return false;
    finish();
    if (words.isEmpty() || words[0].isEmpty()) return false;

    result = OscMessage (words[0]);
    for (int i = 1; i < words.size(); ++i)
    {
        const auto& w = words[i];
        const auto digits = w.trimCharactersAtStart ("-+");
        const bool whole = ! wasQuoted[(size_t) i] && digits.isNotEmpty() && digits.containsOnly ("0123456789") && w.length() - digits.length() <= 1;
        const bool real = ! wasQuoted[(size_t) i] && ! whole && digits.isNotEmpty() && digits.containsOnly ("0123456789.eE-+")
                          && digits.containsAnyOf ("0123456789") && w.length() - digits.length() <= 1
                          && juce::CharacterFunctions::isDigit (digits.replaceCharacter ('.', '0')[0]);
        if (whole)      result.addInteger (w.getIntValue());
        else if (real)  result.addReal (w.getDoubleValue());
        else            result.addText (w);
    }
    return true;
}

bool OscMessage::severalFromText (const juce::String& line, std::vector<OscMessage>& result)
{
    result.clear();
    juce::StringArray parts;
    juce::String part;
    bool inQuotes = false;
    for (auto c : line)
    {
        if (c == '"') inQuotes = ! inQuotes;
        if (c == ';' && ! inQuotes)     { parts.add (part); part.clear(); }
        else                            part << juce::String::charToString (c);
    }
    parts.add (part);

    for (const auto& p : parts)
    {
        if (p.trim().isEmpty()) continue;
        OscMessage m;
        if (! fromText (p, m)) { result.clear(); return false; }
        result.push_back (m);
    }
    return ! result.empty();
}

juce::String OscMessage::toText() const
{
    juce::String line (address);
    for (const auto& a : arguments)
    {
        line << " ";
        if (a.kind == OscArgument::Kind::integer)
        {
            line << juce::String (a.integer);
        }
        else if (a.kind == OscArgument::Kind::real)
        {
            // As Max shows a real number: with a point, without zeros after it.
            auto number = juce::String (a.real, 6);
            while (number.endsWithChar ('0')) number = number.dropLastCharacters (1);
            line << number;
        }
        else
        {
            line << (a.text.containsAnyOf (" \t") ? a.text.quoted() : a.text);
        }
    }
    return line;
}

} // namespace maket
