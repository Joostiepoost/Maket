// Chords.cpp — see Chords.h.

#include "Chords.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace maket::notation
{

std::vector<bool> farSideHeads (const std::vector<int>& steps, int direction)
{
    std::vector<size_t> order (steps.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort (order.begin(), order.end(), [&steps, direction] (size_t a, size_t b)
                      { return direction == 1 ? steps[a] < steps[b] : steps[a] > steps[b]; });

    std::vector<bool> far (steps.size(), false);
    for (size_t i = 1; i < order.size(); ++i)
        far[order[i]] = std::abs (steps[order[i]] - steps[order[i - 1]]) <= 1 && ! far[order[i - 1]];
    return far;
}

std::vector<double> placeAccidentals (const std::vector<ChordNote>& notes)
{
    std::vector<double> right (notes.size(), 0.0);
    if (notes.empty()) return right;

    const int perStep = 10;                             // bach's refinement: ten heights to a step
    const double stepsPerSpace = 2.0;

    int lowest = notes[0].step, highest = notes[0].step;
    double reach = 0.0;
    for (const auto& n : notes)
    {
        lowest = std::min (lowest, n.step);
        highest = std::max (highest, n.step);
        if (n.hasAccidental) reach = std::max (reach, std::max (n.above, n.below));
    }

    // Heights from under the lowest sign to above the highest: index 0 is `margin` tenths under the lowest note.
    const int margin = (int) std::ceil (reach * stepsPerSpace * perStep) + perStep + 4;
    const int size = (highest - lowest) * perStep + 2 * margin + 1;
    std::vector<double> leftLimit ((size_t) size, 0.0);
    auto row = [&] (int step) { return (step - lowest) * perStep + margin; };
    auto clampRow = [&] (int r) { return std::max (0, std::min (size - 1, r)); };

    for (const auto& n : notes)
        for (int r = clampRow (row (n.step) - perStep); r < clampRow (row (n.step) + perStep); ++r)
            leftLimit[(size_t) r] = std::min (leftLimit[(size_t) r], n.headLeft - accidentalFromNote);

    // The order bach looks through the notes in: lowest, highest, second lowest, second highest ...
    std::vector<size_t> byHeight (notes.size());
    for (size_t i = 0; i < byHeight.size(); ++i) byHeight[i] = i;
    std::stable_sort (byHeight.begin(), byHeight.end(), [&notes] (size_t a, size_t b) { return notes[a].step < notes[b].step; });
    std::vector<size_t> order;
    for (size_t j = 0; j < byHeight.size(); ++j)
        order.push_back (byHeight[j % 2 == 0 ? j / 2 : byHeight.size() - (j - 1) / 2 - 1]);

    auto rowsOf = [&] (const ChordNote& n, int& from, int& to)
    {
        from = row (n.step) - (int) std::floor (n.below * stepsPerSpace * perStep);
        to   = row (n.step) + (int) std::ceil  (n.above * stepsPerSpace * perStep);
    };

    std::vector<bool> done (notes.size(), false);
    for (;;)
    {
        bool found = false;
        size_t best = 0;
        double bestX = 0.0;
        for (size_t i : order)
        {
            if (! notes[i].hasAccidental || done[i]) continue;
            int from, to;
            rowsOf (notes[i], from, to);
            double x = leftLimit[(size_t) clampRow (from - 2)];             // two tenths of a step more, against touching
            for (int r = clampRow (from - 2); r <= clampRow (to + 2); ++r)
                x = std::min (x, leftLimit[(size_t) r]);
            if (! found || x > bestX) { found = true; best = i; bestX = x; }
        }
        if (! found) break;

        right[best] = bestX;
        done[best] = true;
        int from, to;
        rowsOf (notes[best], from, to);
        for (int r = clampRow (from); r <= clampRow (to); ++r)
            leftLimit[(size_t) r] = bestX - notes[best].width - accidentalFromAccidental;
    }
    return right;
}

} // namespace maket::notation
