// LlllText.cpp

#include "LlllText.h"

#include <cstdlib>
#include <string>

namespace maket
{

namespace
{
    using Str = std::string;        // UTF-8 bytes; every character bach treats specially is plain ASCII

    bool isDigit (char c)       { return c >= '0' && c <= '9'; }
    bool isSpace (char c)       { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
    bool isBracket (char c)     { return c == '(' || c == ')' || c == '[' || c == ']'; }
    bool isAccidental (char c)  { return c == '#' || c == 'b' || c == 'x' || c == 'd' || c == 'q' || c == '^' || c == 'v'; }

    // Small scanners over s, each starting at i and moving it on when they match.

    bool scanDigits (const Str& s, size_t& i)
    {
        const size_t start = i;
        while (i < s.size() && isDigit (s[i])) ++i;
        return i > start;
    }

    /** bach's INTEGER: an optional sign, then digits. */
    bool scanInteger (const Str& s, size_t& i)
    {
        size_t j = i;
        if (j < s.size() && (s[j] == '+' || s[j] == '-')) ++j;
        if (! scanDigits (s, j)) return false;
        i = j;
        return true;
    }

    bool isInteger (const Str& s)
    {
        size_t i = 0;
        return scanInteger (s, i) && i == s.size();
    }

    bool isRational (const Str& s)
    {
        size_t i = 0;
        if (! scanInteger (s, i) || i >= s.size() || s[i] != '/') return false;
        ++i;
        return scanInteger (s, i) && i == s.size();
    }

    /** bach's FLOAT: digits with a point (before, after or between), with an
        optional exponent; or digits with an exponent. */
    bool isFloat (const Str& s)
    {
        size_t i = 0;
        if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;

        const bool before = scanDigits (s, i);
        bool point = false, after = false;
        if (i < s.size() && s[i] == '.') { point = true; ++i; after = scanDigits (s, i); }

        bool exponent = false;
        if (i < s.size() && (s[i] == 'e' || s[i] == 'E'))
        {
            size_t j = i + 1;
            if (j < s.size() && (s[j] == '+' || s[j] == '-')) ++j;
            if (scanDigits (s, j)) { exponent = true; i = j; }
        }

        if (i != s.size()) return false;
        if (point) return before || after;
        return before && exponent;
    }

    /** A pitch name in one of bach's four forms; on success `cents` is its value.
        C5 is middle C, 6000 cents. Accidentals: # sharp, b flat, x double sharp,
        q quarter sharp, d quarter flat, ^ eighth sharp, v eighth flat. A tail
        such as +1/4t adds a part of a whole tone. */
    bool readPitch (const Str& s, double& cents)
    {
        static const int degreeCents[] = { 900, 1100, 0, 200, 400, 500, 700 };     // a b c d e f g

        size_t i = 0;
        double sign = 1.0;
        if (i < s.size() && (s[i] == '+' || s[i] == '-')) { if (s[i] == '-') sign = -1.0; ++i; }

        if (i >= s.size()) return false;
        const char name = (char) (s[i] | 0x20);       // lower case
        if (name < 'a' || name > 'g') return false;
        ++i;

        auto alterAt = [&s] (size_t& k)                 // in whole tones
        {
            double alter = 0.0;
            for (; k < s.size() && isAccidental (s[k]); ++k)
                switch (s[k])
                {
                    case 'x': alter += 1.0;    break;
                    case '#': alter += 0.5;    break;
                    case 'b': alter -= 0.5;    break;
                    case 'q': alter += 0.25;   break;
                    case 'd': alter -= 0.25;   break;
                    case '^': alter += 0.125;  break;
                    case 'v': alter -= 0.125;  break;
                    default: break;
                }
            return alter;
        };

        auto pitchOf = [&] (double alter, long octave)
        {
            return sign * (degreeCents[name - 'a'] + 1200.0 * (double) octave + 200.0 * alter);
        };

        // Name, accidentals, octave; then perhaps a tail in tones.
        {
            size_t k = i;
            const double alter = alterAt (k);
            size_t afterOctave = k;
            if (scanInteger (s, afterOctave))
            {
                const long octave = std::strtol (s.c_str() + k, nullptr, 10);

                if (afterOctave == s.size())
                {
                    cents = pitchOf (alter, octave);
                    return true;
                }

                // +3t  or  -1/4t
                size_t t = afterOctave;
                if (s[t] == '+' || s[t] == '-')
                {
                    const size_t numberAt = t;
                    ++t;
                    if (scanDigits (s, t))
                    {
                        double tones = (double) std::strtol (s.c_str() + numberAt, nullptr, 10);
                        if (t < s.size() && s[t] == '/')
                        {
                            const size_t denAt = ++t;
                            if (! scanInteger (s, t)) return false;
                            const long den = std::strtol (s.c_str() + denAt, nullptr, 10);
                            if (den == 0) return false;
                            tones /= (double) den;
                        }
                        if (t + 1 == s.size() && s[t] == 't')
                        {
                            cents = pitchOf (alter, octave) + 200.0 * tones;
                            return true;
                        }
                    }
                }
            }
        }

        // Name, octave, accidentals.
        {
            size_t k = i;
            if (scanInteger (s, k) && k < s.size())
            {
                const long octave = std::strtol (s.c_str() + i, nullptr, 10);
                size_t a = k;
                const double alter = alterAt (a);
                if (a == s.size() && a > k)
                {
                    cents = pitchOf (alter, octave);
                    return true;
                }
            }
        }

        return false;
    }

    /** Takes the backslashes out: each one keeps the character after it. */
    Str unescape (const Str& s)
    {
        Str out;
        out.reserve (s.size());
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '\\' && i + 1 < s.size()) ++i;
            else if (s[i] == '\\') break;
            out += s[i];
        }
        return out;
    }

    juce::String fromUtf8 (const Str& s)    { return juce::String::fromUTF8 (s.data(), (int) s.size()); }

    /** A plain word (no quotes, no backtick) to an atom, by bach's order of trial. */
    bool wordToAtom (const Str& word, Llll& atom)
    {
        atom = Llll();
        atom.spelling = fromUtf8 (word);

        if (isInteger (word))
        {
            atom.kind = Llll::Kind::integer;
            atom.num = std::strtoll (word.c_str(), nullptr, 10);
            atom.value = (double) atom.num;
        }
        else if (isRational (word))
        {
            atom.kind = Llll::Kind::rational;
            const auto slash = word.find ('/');
            atom.num = std::strtoll (word.c_str(), nullptr, 10);
            atom.den = std::strtoll (word.c_str() + slash + 1, nullptr, 10);
            atom.value = atom.den != 0 ? (double) atom.num / (double) atom.den : 0.0;
        }
        else if (isFloat (word))
        {
            atom.kind = Llll::Kind::real;
            atom.value = atom.spelling.getDoubleValue();        // does not depend on the Mac's language settings
        }
        else if (double cents = 0.0; readPitch (word, cents))
        {
            atom.kind = Llll::Kind::pitch;
            atom.value = cents;
        }
        else if (word == "null")
        {
            return false;                                       // stands for nothing at all
        }
        else if (word == "nil")
        {
            atom.kind = Llll::Kind::list;                       // an empty list
        }
        else
        {
            atom.kind = Llll::Kind::symbol;
            atom.text = fromUtf8 (word.find ('\\') == Str::npos ? word : unescape (word));
        }
        return true;
    }
}

Llll readLlllWord (const juce::String& word)
{
    Llll atom;
    if (! wordToAtom (word.toStdString(), atom)) return Llll::makeList();
    return atom;
}

juce::Result readLlllText (const juce::String& text, Llll& result)
{
    result = Llll::makeList();

    const Str s = text.toStdString();
    std::vector<Llll*> open { &result };        // the lists being filled, the innermost last
    std::vector<int> openedAtLine;
    int lineNumber = 1;

    // Note: `open` holds pointers into vectors that grow. A list is only ever
    // appended to while it is the innermost one, so the pointers above it
    // stay valid; the innermost pointer is taken afresh after each append.

    size_t i = 0;
    while (i < s.size())
    {
        const char c = s[i];

        if (isSpace (c))
        {
            if (c == '\n') ++lineNumber;
            ++i;
            continue;
        }

        if (c == '(' || c == '[')
        {
            open.back()->items.emplace_back();
            open.push_back (&open.back()->items.back());
            openedAtLine.push_back (lineNumber);
            ++i;
            continue;
        }

        if (c == ')' || c == ']')
        {
            if (open.size() == 1)
                return juce::Result::fail ("Line " + juce::String (lineNumber) + ": a bracket closes a list that was never opened.");
            open.pop_back();
            openedAtLine.pop_back();
            ++i;
            continue;
        }

        Llll atom;
        bool keep = true;
        const size_t start = i;

        if (c == '"')
        {
            // A quoted symbol: up to the next quote that has no backslash before it.
            const int startLine = lineNumber;
            ++i;
            while (i < s.size() && s[i] != '"')
            {
                if (s[i] == '\\' && i + 1 < s.size()) ++i;
                if (s[i] == '\n') ++lineNumber;
                ++i;
            }
            if (i >= s.size())
                return juce::Result::fail ("Line " + juce::String (startLine) + ": a quote opens a text that never ends.");
            ++i;

            const Str inside = s.substr (start + 1, i - start - 2);
            atom.kind = Llll::Kind::symbol;
            atom.text = fromUtf8 (inside.find ('\\') == Str::npos ? inside : unescape (inside));
            atom.spelling = fromUtf8 (s.substr (start, i - start));
        }
        else if (c == '`')
        {
            // A backticked symbol: everything up to the next space, whatever it looks like.
            ++i;
            while (i < s.size() && ! isSpace (s[i]))
            {
                if (s[i] == '\\' && i + 1 < s.size()) ++i;
                ++i;
            }
            const Str inside = s.substr (start + 1, i - start - 1);
            atom.kind = Llll::Kind::symbol;
            atom.text = fromUtf8 (inside.find ('\\') == Str::npos ? inside : unescape (inside));
            atom.spelling = fromUtf8 (s.substr (start, i - start));
        }
        else
        {
            // A plain word: up to a space or a bracket. A backslash keeps the
            // next character in the word, unless that character is a bracket.
            while (i < s.size() && ! isSpace (s[i]) && ! isBracket (s[i]))
            {
                if (s[i] == '\\' && i + 1 < s.size() && ! isBracket (s[i + 1])) ++i;
                ++i;
            }
            keep = wordToAtom (s.substr (start, i - start), atom);
        }

        if (keep) open.back()->items.push_back (std::move (atom));
    }

    if (open.size() > 1)
        return juce::Result::fail ("Line " + juce::String (openedAtLine.back()) + ": a list is opened here and never closed.");

    return juce::Result::ok();
}

} // namespace maket
