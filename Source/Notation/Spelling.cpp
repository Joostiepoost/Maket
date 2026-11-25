// Spelling.cpp

#include "Spelling.h"

#include <cmath>

namespace maket::notation
{

namespace
{
    // The accidental of each eighth-tone step of an octave from C, in eighths of a tone (bach's table for C major, "auto").
    constexpr int table[48] =
    {
        0, 1, 2, 3,  4, -3, -2, -1,      // C ... C#, then towards D
        0, 1, 2, 3, -4, -3, -2, -1,      // D ... then Eb, towards E
        0, 1, 2, -1,                     // E, then F an eighth tone low
        0, 1, 2, 3,  4, -3, -2, -1,      // F ... F#, towards G
        0, 1, 2, 3, -4, -3, -2, -1,      // G ... then Ab, towards A
        0, 1, 2, 3, -4, -3, -2, -1,      // A ... then Bb, towards B
        0, 1, 2, -1                      // B, then C an eighth tone low
    };

    // Eighth-tone step of each natural note from C, and one octave further.
    constexpr int naturals[8] = { 0, 8, 16, 20, 28, 36, 44, 48 };
}

Spelling spell (double cents, int toneDivision)
{
    if (toneDivision != 2 && toneDivision != 4) toneDivision = 8;

    const double octave = std::floor (cents / 1200.0);
    const double stepCents = 200.0 / toneDivision;
    const long numSteps = 6L * toneDivision;

    long gridStep = std::lround ((cents - 1200.0 * octave) / stepCents);    // halves away from zero, as C's round()
    long octaves = (long) octave - 5;                                       // bach's octave 5 begins at middle C
    if (gridStep >= numSteps) { gridStep -= numSteps; ++octaves; }

    const int step48 = (int) (gridStep * 48 / numSteps);
    const int alteration = table[step48];
    int natural48 = step48 - alteration;                                    // 48: the C of the next octave

    int degree = 0;
    while (degree < 7 && naturals[degree] != natural48) ++degree;
    if (degree == 7) { degree = 0; ++octaves; }

    return { (int) octaves * 7 + degree, alteration };
}

double centsOf (const Spelling& s)
{
    const int octaves = (int) std::floor (s.steps / 7.0);
    const int degree = s.steps - octaves * 7;
    return 6000.0 + 1200.0 * octaves + 25.0 * (naturals[degree] + s.alteration);
}

Spelling spellAs (const juce::String& pitchName, double cents, int toneDivision)
{
    const auto automatic = spell (cents, toneDivision);
    const auto name = pitchName.trim();
    if (name.length() < 2) return automatic;

    const int degree = juce::String ("CDEFGAB").indexOfChar (juce::CharacterFunctions::toUpperCase (name[0]));
    if (degree < 0) return automatic;

    // After the letter the accidentals, then the octave (middle C is C5).
    int i = 1;
    while (i < name.length() && ! juce::CharacterFunctions::isDigit (name[i]) && name[i] != '-') ++i;
    int end = i;
    if (end < name.length() && name[end] == '-') ++end;
    const int firstDigit = end;
    while (end < name.length() && juce::CharacterFunctions::isDigit (name[end])) ++end;
    if (end == firstDigit) return automatic;
    const int octave = name.substring (i, end).getIntValue();

    Spelling named { (octave - 5) * 7 + degree, 0 };
    const double off = (centsOf (automatic) - centsOf (named)) / 25.0;
    const int alteration = (int) std::lround (off);
    if (std::abs (off - alteration) > 1.0e-6 || std::abs (alteration) > 4) return automatic;
    named.alteration = alteration;
    return named;
}

Accidental accidentalOf (int alteration)
{
    switch (alteration)
    {
        case 1:  return Accidental::naturalUp;
        case 2:  return Accidental::quarterSharp;
        case 3:  return Accidental::quarterSharpUp;
        case 4:  return Accidental::sharp;
        case -1: return Accidental::naturalDown;
        case -2: return Accidental::quarterFlat;
        case -3: return Accidental::quarterFlatDown;
        case -4: return Accidental::flat;
        default: return Accidental::none;
    }
}

juce::String spellingName (const Spelling& s)
{
    static const char* letters[7] = { "C", "D", "E", "F", "G", "A", "B" };
    static const char* words[9] = { " flat", " three eighths flat", " quarter flat", " eighth flat", "",
                                    " eighth sharp", " quarter sharp", " three eighths sharp", " sharp" };
    const int octaves = (int) std::floor (s.steps / 7.0);
    const int degree = s.steps - octaves * 7;
    const int a = s.alteration < -4 ? -4 : s.alteration > 4 ? 4 : s.alteration;
    return juce::String (letters[degree]) + juce::String (octaves + 5) + words[a + 4];
}

} // namespace maket::notation
