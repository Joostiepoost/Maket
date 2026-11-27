// Miniature.cpp — see Miniature.h.

#include "Miniature.h"
#include <algorithm>
#include <cmath>

namespace maket::notation
{

std::vector<MiniLine> miniatureOf (const Roll& roll, int most)
{
    std::vector<MiniLine> lines;
    const double length = roll.length();
    const int total = roll.numNotes();
    if (length <= 0.0 || total <= 0) return lines;

    // The range of pitches, glides included.
    double low = 1.0e9, high = -1.0e9;
    for (const auto& voice : roll.voices)
        for (const auto& chord : voice.chords)
            for (const auto& note : chord.notes)
            {
                low = std::min (low, note.cents); high = std::max (high, note.cents);
                for (const auto& b : note.breakpoints) { low = std::min (low, note.cents + b.cents); high = std::max (high, note.cents + b.cents); }
            }
    const double range = std::max (1200.0, high - low);         // at least an octave, so that one pitch is not a line across the top
    const double middle = 0.5 * (low + high);
    auto yOf = [range, middle] (double cents) { return (float) (0.5 - (cents - middle) / range * 0.92); };
    auto xOf = [length] (double ms) { return (float) (ms / length); };

    const int step = std::max (1, (total + most - 1) / std::max (1, most));
    int index = 0;
    for (const auto& voice : roll.voices)
    {
        for (const auto& chord : voice.chords)
            for (const auto& note : chord.notes)
            {
                if (index++ % step != 0) continue;
                // What is muted is drawn too, paler: a snippet whose voices are all muted still shows what it holds.
                const bool muted = voice.isMuted() || chord.isMuted() || note.isMuted();
                const float weight = (float) std::clamp (note.velocity / 127.0, 0.0, 1.0) * (muted ? 0.35f : 1.0f);
                if (note.breakpoints.size() >= 2)
                {
                    for (size_t i = 1; i < note.breakpoints.size(); ++i)
                    {
                        const auto& a = note.breakpoints[i - 1];
                        const auto& b = note.breakpoints[i];
                        lines.push_back ({ xOf (chord.onset + a.position * note.duration), yOf (note.cents + a.cents),
                                           xOf (chord.onset + b.position * note.duration), yOf (note.cents + b.cents), weight });
                    }
                }
                else
                {
                    const float y = yOf (note.cents);
                    lines.push_back ({ xOf (chord.onset), y, xOf (chord.onset + note.duration), y, weight });
                }
            }
    }
    return lines;
}

} // namespace maket::notation
