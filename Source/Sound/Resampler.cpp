// Resampler.cpp — see Resampler.h.

#include "Resampler.h"
#include <algorithm>
#include <cmath>

namespace maket
{

namespace
{
    const double pi = 3.14159265358979323846;

    double bessel0 (double x)
    {
        double sum = 1.0, term = 1.0;
        for (int k = 1; k < 40; ++k)
        {
            term *= (x / (2.0 * k)) * (x / (2.0 * k));
            sum += term;
            if (term < 1.0e-12 * sum) break;
        }
        return sum;
    }
}

Resampler::Resampler (double ratio)
{
    scale = ratio > 1.0 ? 1.0 / ratio : 1.0;                    // going down: the cutoff goes down with the rate
    halfWidth = (int) std::ceil (crossings / scale);
    const int taps = 2 * halfWidth;
    const double beta = 10.0, reach = (double) halfWidth;
    table.assign ((size_t) (phases + 1) * (size_t) taps, 0.0f);

    for (int p = 0; p <= phases; ++p)
    {
        const double fraction = (double) p / phases;
        double sum = 0.0;
        std::vector<double> row ((size_t) taps);
        for (int k = 0; k < taps; ++k)
        {
            // Tap k sits at source sample (floor (position) - halfWidth + 1 + k); its distance from the position:
            const double d = (double) (k - halfWidth + 1) - fraction;
            const double x = d * scale;
            const double sinc = std::abs (x) < 1.0e-12 ? 1.0 : std::sin (pi * x) / (pi * x);
            const double w = std::abs (d) >= reach ? 0.0 : bessel0 (beta * std::sqrt (1.0 - (d / reach) * (d / reach))) / bessel0 (beta);
            row[(size_t) k] = sinc * w * scale;
            sum += row[(size_t) k];
        }
        // Each row sums to exactly one: a steady level comes out as it went in.
        for (int k = 0; k < taps; ++k) table[(size_t) p * (size_t) taps + (size_t) k] = (float) (row[(size_t) k] / sum);
    }
}

float Resampler::at (const float* data, int length, double position) const
{
    const double whole = std::floor (position);
    const double fraction = position - whole;
    const int taps = 2 * halfWidth;
    const int first = (int) whole - halfWidth + 1;

    const double phase = fraction * phases;
    const int p = (int) phase;
    const float mix = (float) (phase - p);
    const float* a = table.data() + (size_t) p * (size_t) taps;
    const float* b = a + taps;

    const int from = std::max (0, -first), to = std::min (taps, length - first);
    float sum = 0.0f;
    for (int k = from; k < to; ++k)
        sum += data[first + k] * (a[k] + mix * (b[k] - a[k]));
    return sum;
}

std::vector<float> Resampler::convert (const float* data, int length, double sourceRate, double targetRate)
{
    if (length <= 0 || sourceRate <= 0.0 || targetRate <= 0.0) return {};
    const double ratio = sourceRate / targetRate;
    const Resampler r (ratio);
    const auto count = (size_t) std::llround ((double) length / ratio);
    std::vector<float> out (count);
    for (size_t n = 0; n < count; ++n) out[n] = r.at (data, length, (double) n * ratio);
    return out;
}

} // namespace maket
