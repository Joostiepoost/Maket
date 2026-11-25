// Chords.cpp — see Chords.h.

#include "Chords.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace maket::notation
{

std::vector<HeadPlace> placeHeads (const std::vector<int>& steps, int direction, double w)
{
    const int count = (int) steps.size();
    std::vector<HeadPlace> result ((size_t) count);
    if (count == 0) return result;

    // bach's own order: by height, lowest first.
    std::vector<int> order ((size_t) count);
    for (int i = 0; i < count; ++i) order[(size_t) i] = i;
    std::stable_sort (order.begin(), order.end(), [&steps] (int a, int b) { return steps[(size_t) a] < steps[(size_t) b]; });
    std::vector<int> sp ((size_t) count);
    for (int i = 0; i < count; ++i) sp[(size_t) i] = steps[(size_t) order[(size_t) i]];

    const int lowest = sp.front();
    const int extension = sp.back() - lowest + 1;
    std::vector<double> leftLimit ((size_t) extension, 0.0), rightLimit ((size_t) extension, 0.0);      // per height, from the stem
    std::vector<int> stemPos ((size_t) count, 0);       // 0 at the stem on the usual side, -1 at the stem on the far side, others further out
    std::vector<double> x ((size_t) count, direction == 1 ? -w : 0.0);

    auto spread = [&] (int rel)
    {
        if (rel - 1 >= 0)
        {
            rightLimit[(size_t) rel - 1] = std::max (rightLimit[(size_t) rel - 1], rightLimit[(size_t) rel]);
            leftLimit[(size_t) rel - 1] = std::min (leftLimit[(size_t) rel - 1], leftLimit[(size_t) rel]);
        }
        if (rel + 1 < extension)
        {
            rightLimit[(size_t) rel + 1] = std::max (rightLimit[(size_t) rel + 1], rightLimit[(size_t) rel]);
            leftLimit[(size_t) rel + 1] = std::min (leftLimit[(size_t) rel + 1], leftLimit[(size_t) rel]);
        }
    };

    // Groups of notes a second or less apart.
    std::vector<std::pair<int, int>> groups;
    {
        bool inGroup = false;
        int start = 0;
        for (int i = 0; i < count; ++i)
        {
            if (i < count - 1 && sp[(size_t) i + 1] <= sp[(size_t) i] + 1)
            {
                if (! inGroup) { start = i; inGroup = true; }
            }
            else if (inGroup)
            {
                groups.push_back ({ start, i });
                inGroup = false;
            }
        }
    }

    for (const auto& group : groups)
    {
        const int start = group.first, end = group.second;
        const int size = end - start + 1;

        // The number of each note among the heights of the group (notes at one height share a number).
        std::vector<int> height ((size_t) size, 0);
        for (int j = start + 1; j <= end; ++j)
            height[(size_t) (j - start)] = height[(size_t) (j - start - 1)] + (sp[(size_t) j] == sp[(size_t) j - 1] + 1 ? 1 : 0);
        const int heights = height.back() + 1;
        std::vector<int> minLimit ((size_t) heights, 0), maxLimit ((size_t) heights, 0);

        auto lowerMin = [&] (int h, int value)
        {
            for (int k = h - 1; k <= h + 1; ++k)
                if (k >= 0 && k < heights) minLimit[(size_t) k] = std::min (minLimit[(size_t) k], value);
        };

        // First the notes that are the first at their height: by turns at the stem's usual side and its far side.
        int previous = 0;
        bool first = true;
        for (int j = start; j <= end; ++j)
        {
            if (! (first || sp[(size_t) j] == sp[(size_t) j - 1] + 1)) continue;
            const int h = height[(size_t) (j - start)];
            const int rel = sp[(size_t) j] - lowest;

            if (first)                  { stemPos[(size_t) j] = 0; first = false; }
            else if (previous == 0)     { stemPos[(size_t) j] = -1; lowerMin (h, -1); }
            else                        { stemPos[(size_t) j] = 0; }
            previous = stemPos[(size_t) j];

            const bool usual = stemPos[(size_t) j] >= 0;
            if (direction == 1)
            {
                x[(size_t) j] = (-stemPos[(size_t) j] - 1) * w;
                rightLimit[(size_t) rel] = std::max (rightLimit[(size_t) rel], usual ? 0.0 : x[(size_t) j] + w);
                leftLimit[(size_t) rel] = std::min (leftLimit[(size_t) rel], usual ? x[(size_t) j] : 0.0);
            }
            else
            {
                x[(size_t) j] = stemPos[(size_t) j] * w;
                rightLimit[(size_t) rel] = std::max (rightLimit[(size_t) rel], usual ? x[(size_t) j] + w : 0.0);
                leftLimit[(size_t) rel] = std::min (leftLimit[(size_t) rel], usual ? 0.0 : x[(size_t) j]);
            }
            spread (rel);
        }

        // Then the further notes at a height: outwards, to the side that has had fewer.
        for (int j = start + 1; j <= end; ++j)
        {
            if (sp[(size_t) j] != sp[(size_t) j - 1]) continue;
            const int h = height[(size_t) (j - start)];
            const int rel = sp[(size_t) j] - lowest;

            if (minLimit[(size_t) h] == 0)
            {
                stemPos[(size_t) j] = -1;
                lowerMin (h, -1);
            }
            else if (std::abs (minLimit[(size_t) h]) > maxLimit[(size_t) h])
            {
                stemPos[(size_t) j] = maxLimit[(size_t) h] + 1;
                maxLimit[(size_t) h] += 1;
                if (h - 1 >= 0)      maxLimit[(size_t) h - 1] = std::max (maxLimit[(size_t) h - 1], maxLimit[(size_t) h]);
                if (h + 1 < heights) maxLimit[(size_t) h + 1] = std::max (maxLimit[(size_t) h + 1], maxLimit[(size_t) h]);
            }
            else
            {
                stemPos[(size_t) j] = minLimit[(size_t) h] - 1;
                minLimit[(size_t) h] -= 1;
                if (h - 1 >= 0)      minLimit[(size_t) h - 1] = std::min (minLimit[(size_t) h - 1], minLimit[(size_t) h]);
                if (h + 1 < heights) minLimit[(size_t) h + 1] = std::min (minLimit[(size_t) h + 1], minLimit[(size_t) h]);
            }

            const bool toTheLeft = (direction == 1) == (stemPos[(size_t) j] >= 0);
            if (toTheLeft)  { x[(size_t) j] = leftLimit[(size_t) rel] - w; leftLimit[(size_t) rel] -= w; }
            else            { x[(size_t) j] = rightLimit[(size_t) rel];    rightLimit[(size_t) rel] += w; }
            spread (rel);
        }
    }

    // Stem down: the lower note of a group is the left one.
    if (direction == -1)
        for (const auto& group : groups)
            if (stemPos[(size_t) group.first] >= 0 && stemPos[(size_t) group.second] < 0)
                for (int j = group.first; j <= group.second; ++j)
                {
                    x[(size_t) j] = -x[(size_t) j] - w;
                    stemPos[(size_t) j] = -stemPos[(size_t) j] - 1;
                }

    for (int i = 0; i < count; ++i)
    {
        result[(size_t) order[(size_t) i]].left = x[(size_t) i];
        result[(size_t) order[(size_t) i]].auxiliaryStem = stemPos[(size_t) i] != 0 && stemPos[(size_t) i] != -1;
    }
    return result;
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
