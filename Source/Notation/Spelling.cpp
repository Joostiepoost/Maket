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

} // namespace maket::notation
