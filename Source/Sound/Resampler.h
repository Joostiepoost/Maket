// Resampler.h — reading a sound at any point between its samples (stage 3c).
//
// A windowed sinc: 16 zero crossings to each side at the sound's own rate, a
// Kaiser window, the kernel kept in a table of 512 phases. One and the same
// reckoning serves the conversion of a file to the project's rate when it is
// imported, and playing a sound through a device that runs at another rate.
// Going down in rate the kernel is widened, so that what the lower rate cannot
// hold is taken out and does not fold back. No interface code.

#pragma once

#include <vector>

namespace maket
{

class Resampler
{
public:
    /** `ratio`: steps in the source for one sample out (source rate / target rate). */
    explicit Resampler (double ratio);

    /** The value of the sound at `position` (in samples, may lie between two). Outside the sound is silence. */
    float at (const float* data, int length, double position) const;

    /** All of a sound at the new rate. */
    static std::vector<float> convert (const float* data, int length, double sourceRate, double targetRate);

    static constexpr int crossings = 16, phases = 512;

private:
    double scale = 1.0;         // 1 going up; target rate / source rate going down
    int halfWidth = crossings;  // taps to each side
    std::vector<float> table;   // (phases + 1) rows of 2 * halfWidth taps
};

} // namespace maket
