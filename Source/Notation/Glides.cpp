// Glides.cpp — see Glides.h.

#include "Glides.h"

#include <algorithm>
#include <cmath>

namespace maket::notation
{

double slopeCurve (double t, double slope)
{
    if (slope >= 1.0) return 0.0;
    if (slope <= -1.0) return 1.0;
    t = std::max (0.0, std::min (1.0, t));
    if (std::abs (slope) < 1.0e-12) return t;

    const double s = std::abs (slope);
    const double power = (1.0 + s) / (1.0 - s);
    return slope > 0.0 ? std::pow (t, power) : 1.0 - std::pow (1.0 - t, power);
}

double continuousSteps (double cents)
{
    const double octave = std::floor (cents / 1200.0);
    const double pitch = cents - octave * 1200.0;
    double step;
    if (pitch <= 200.0)        step = pitch / 200.0;
    else if (pitch <= 400.0)   step = 1.0 + (pitch - 200.0) / 200.0;
    else if (pitch <= 500.0)   step = 2.0 + (pitch - 400.0) / 100.0;
    else if (pitch <= 700.0)   step = 3.0 + (pitch - 500.0) / 200.0;
    else if (pitch <= 900.0)   step = 4.0 + (pitch - 700.0) / 200.0;
    else if (pitch <= 1100.0)  step = 5.0 + (pitch - 900.0) / 200.0;
    else                       step = 6.0 + (pitch - 1100.0) / 100.0;
    return (octave - 5.0) * 7.0 + step;
}

std::vector<GlidePoint> glidePoints (const Spelling& head, const std::vector<maket::Breakpoint>& breakpoints)
{
    std::vector<GlidePoint> points;
    bool moves = false;
    for (const auto& b : breakpoints)
        if (b.position > 0.0) moves = true;
    if (! moves) return points;

    const double whiteKey = centsOf ({ head.steps, 0 });
    points.push_back ({ 0.0, (double) head.steps, 0.0 });
    for (const auto& b : breakpoints)
    {
        if (b.position <= 0.0) continue;
        points.push_back ({ std::min (1.0, b.position), continuousSteps (whiteKey + std::round (b.cents)), b.slope });
    }
    std::stable_sort (points.begin(), points.end(), [] (const GlidePoint& a, const GlidePoint& b) { return a.position < b.position; });
    if (points.back().position < 1.0)
        points.push_back ({ 1.0, points.back().steps, 0.0 });      // a file without a tail: the line stays where it is
    return points;
}

double glideStepsAt (const std::vector<GlidePoint>& points, double position)
{
    if (points.empty()) return 0.0;
    if (position <= points.front().position) return points.front().steps;
    for (size_t i = 1; i < points.size(); ++i)
        if (position <= points[i].position)
        {
            const auto& a = points[i - 1];
            const auto& b = points[i];
            const double length = b.position - a.position;
            const double t = length > 0.0 ? (position - a.position) / length : 1.0;
            return a.steps + (b.steps - a.steps) * slopeCurve (t, b.slope);
        }
    return points.back().steps;
}

std::vector<int> markerNameLines (const std::vector<double>& x, const std::vector<double>& width, double gap)
{
    std::vector<int> line (x.size(), 0);
    for (size_t i = 1; i < x.size(); ++i)
    {
        if (x[i - 1] + width[i - 1] + gap > x[i])
        {
            line[i] = line[i - 1] + 1;
            if (line[i - 1] > 0)
                for (size_t k = i - 1; k-- > 0;)
                    if (line[k] == 0)
                    {
                        if (x[k] + width[k] + gap <= x[i]) line[i] = 0;
                        break;
                    }
        }
    }
    return line;
}

} // namespace maket::notation
