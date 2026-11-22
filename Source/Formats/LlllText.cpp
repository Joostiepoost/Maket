// LlllText.cpp

#include "LlllText.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

// --- Writing ----------------------------------------------------------------------
//
// What follows does what bach does when it saves text (`writetxt`): the
// layout of llll_to_text_buf_pretty and the symbol rules of
// prepare_string_for_text_buf and chkparser.l, in bach's source, with the
// settings bach.roll uses (a tab per level, every list on a line of its own,
// no line wrapping, six decimals).

namespace
{
    /** A real number as bach prints it: at most `maxDecimals` decimals, the
        zeros at the end taken off, but one digit left after the point
        (7100.0, 0.5, 35407.0514). */
    Str realText (double x, int maxDecimals)
    {
        char buffer[400];
        const int n = std::snprintf (buffer, sizeof (buffer), "%.*f", maxDecimals, x);
        Str s (buffer, (size_t) juce::jlimit (0, (int) sizeof (buffer) - 1, n));

        for (auto& c : s)
            if (c == ',') c = '.';      // whatever the language settings of the machine

        while (s.size() > 1 && s.back() == '0' && s[s.size() - 2] != '.')
            s.pop_back();
        return s;
    }

    // What bach's check finds in a symbol (chkparser.l).
    enum SymbolFlag { hasBrackets = 1, hasSeparators = 2, hasSpecials = 4, startsWithBacktick = 8 };

    /** How many characters of s, from `from` on, are not in `stop`. */
    size_t runOutside (const Str& s, size_t from, const char* stop)
    {
        size_t i = from;
        while (i < s.size() && std::strchr (stop, s[i]) == nullptr) ++i;        // (a zero byte is in every `stop`)
        return i - from;
    }

    /** bach tries a list of patterns and takes the one that covers most of
        the symbol; of two that cover as much, the first. */
    int symbolFlags (const Str& s)
    {
        struct Rule { const char* stop; int flags; };

        static const char* const all       = "()[] \t\n\r,;\"\\";
        static const char* const sepsSpecs = " \t\n\r,;\"\\";
        static const char* const brksSpecs = "()[]\"\\";
        static const char* const specs     = "\"\\";
        static const char* const brksSeps  = "()[] \t\n\r,;";
        static const char* const seps      = " \t\n\r,;";
        static const char* const brks      = "()[]";
        static const char* const newline   = "\n";

        if (s.empty()) return 0;

        int best = 0;
        size_t bestLength = 0;

        if (s[0] != '`')
        {
            const Rule rules[] =
            {
                { all,       0 },
                { sepsSpecs, hasBrackets },
                { brksSpecs, hasSeparators },
                { specs,     hasBrackets | hasSeparators },
                { brksSeps,  hasSpecials },
                { seps,      hasBrackets | hasSpecials },
                { brks,      hasSeparators | hasSpecials },
                { newline,   hasBrackets | hasSeparators | hasSpecials }
            };

            for (const auto& rule : rules)
            {
                const size_t length = runOutside (s, 0, rule.stop);
                if (length > bestLength) { bestLength = length; best = rule.flags; }
            }
            return best;
        }

        // A symbol that itself begins with a backtick. (The second and the last
        // two of bach's patterns are not what their comments say; they are
        // followed here as they are.)
        const Rule rules[] =
        {
            { all,       startsWithBacktick },
            { nullptr,   startsWithBacktick | hasBrackets },
            { brksSpecs, startsWithBacktick | hasSeparators },
            { specs,     startsWithBacktick | hasBrackets | hasSeparators },
            { brksSeps,  startsWithBacktick | hasSpecials },
            { seps,      startsWithBacktick | hasSpecials },
            { brks,      startsWithBacktick | hasSeparators | hasSpecials },
            { newline,   startsWithBacktick | hasBrackets | hasSpecials }
        };

        for (const auto& rule : rules)
        {
            size_t length = 1;
            if (rule.stop != nullptr)
                length += runOutside (s, 1, rule.stop);
            else
                while (length < s.size() && std::strchr (sepsSpecs, s[length]) != nullptr) ++length;

            if (length > bestLength) { bestLength = length; best = rule.flags; }
        }
        return best;
    }

    Str withBackslashBefore (const Str& s, const char* which)
    {
        Str out;
        for (const char c : s)
        {
            if (std::strchr (which, c) != nullptr && c != 0) out += '\\';
            out += c;
        }
        return out;
    }

    /** True when `written` reads back as the one symbol `name`. */
    bool readsBackAs (const Str& written, const Str& name)
    {
        Llll back;
        return readLlllText (fromUtf8 (written), back).wasOk()
               && back.size() == 1 && back[0].isSymbol() && back[0].text == fromUtf8 (name);
    }

    /** A symbol as bach writes it: as it is when that reads back as a symbol;
        with a backtick when it looks like a number, a pitch or holds
        brackets; in quotes when it holds spaces, commas or semicolons. */
    Str symbolText (const Str& name)
    {
        Llll probe;
        const bool plainSymbol = wordToAtom (name, probe) && probe.isSymbol();
        const int flags = symbolFlags (name);

        Str out;
        const char* closing = "";
        enum class Escape { none, quotes, quotesAndSeparators } escape = Escape::none;

        if (! plainSymbol || (flags & (hasBrackets | startsWithBacktick)) != 0)
        {
            out = "`";
            escape = Escape::quotesAndSeparators;
        }
        else if ((flags & hasSeparators) != 0)
        {
            out = "\"";
            closing = "\"";
            escape = Escape::quotes;
        }

        if (escape == Escape::quotes && (flags & hasSpecials) != 0)
            out += withBackslashBefore (name, "\"'");
        else if (escape == Escape::quotesAndSeparators && (flags & (hasSpecials | hasSeparators)) != 0)
            out += withBackslashBefore (name, ",; \n\t\"'");
        else
            out += name;

        out += closing;

        // bach's rules leave a few symbols that do not read back as themselves
        // (an empty one, one with a backslash in it). Those are written in
        // quotes with every quote and backslash kept by a backslash, which
        // bach reads as the same symbol.
        if (! readsBackAs (out, name))
            out = "\"" + withBackslashBefore (name, "\"\\") + "\"";

        return out;
    }

    Str atomText (const Llll& atom)
    {
        if (atom.spelling.isNotEmpty())
            return atom.spelling.toStdString();

        switch (atom.kind)
        {
            case Llll::Kind::integer:   return std::to_string ((long long) atom.num);
            case Llll::Kind::rational:  return std::to_string ((long long) atom.num) + "/" + std::to_string ((long long) atom.den);
            case Llll::Kind::real:      return realText (atom.value, 6);
            case Llll::Kind::pitch:     return realText (atom.value, 6);       // Maket makes no note names: in cents
            case Llll::Kind::symbol:    return symbolText (atom.text.toStdString());
            case Llll::Kind::list:      break;
        }
        return {};
    }

    void addTabs (Str& out, int depth)
    {
        out.append ((size_t) std::max (0, depth - 1), '\t');
    }

    /** bach's layout. Every atom and every closing bracket is followed by one
        character, a space or a line end, which the next thing written may
        turn into a line end. */
    void writeItems (const Llll& list, Str& out, int& depth, bool& justClosedAList)
    {
        for (const auto& item : list.items)
        {
            if (item.isList())
            {
                ++depth;
                if (! out.empty()) out.back() = '\n';
                addTabs (out, depth);
                justClosedAList = false;
                out += "[ ";

                writeItems (item, out, depth, justClosedAList);

                if (justClosedAList)
                {
                    out.back() = '\n';
                    addTabs (out, depth);
                }
                --depth;
                out += "]\n";
                justClosedAList = true;
            }
            else
            {
                // After a list, an atom begins a new line. bach leaves a real
                // number out of this rule (it stays at the start of the line
                // the bracket ended); the same is done here.
                if (justClosedAList && item.kind != Llll::Kind::real && ! out.empty())
                {
                    out.back() = '\n';
                    addTabs (out, depth);
                    justClosedAList = false;
                }
                out += atomText (item);
                out += ' ';
            }
        }
    }
}

juce::String writeLlllText (const Llll& items)
{
    Str out;
    int depth = 1;
    bool justClosedAList = false;
    writeItems (items, out, depth, justClosedAList);

    if (! out.empty()) out.pop_back();      // the last space or line end
    return fromUtf8 (out);
}

juce::String writeLlllAtom (const Llll& atom)
{
    return atom.isList() ? juce::String() : fromUtf8 (atomText (atom));
}

} // namespace maket
