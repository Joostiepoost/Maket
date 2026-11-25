// Glides.h — a note's duration line when the pitch moves along it (bach's
// pitch breakpoints), and the lines the names of markers are written on.
//
// As bach.roll draws them (`paint_duration_line`, `mc_to_yposition`,
// `rescale_with_slope` and the marker loop of its paint function).
// No interface code.

#pragma once

#include <vector>

#include "../Model/Roll.h"
#include "Spelling.h"

namespace maket::notation
{

/** bach's curve between two points: `t` from 0 to 1 gives 0 to 1. Slope 0 is a straight line; a positive slope
    stays low and rises late (t to the power (1+s)/(1-s)), a negative one rises early; 1 never leaves the start,
    -1 is at the end at once. */
double slopeCurve (double t, double slope);

/** The height of a pitch in diatonic steps above middle C, between the notes too: a white key sits on its step,
    and the way to the next white key is divided evenly (so C sharp is half a step above C, and half a step above
    E is already F). This is how bach finds the height of a glide. */
double continuousSteps (double cents);

/** A point of a note's duration line. */
struct GlidePoint
{
    double position = 0.0;      // 0 at the head, 1 at the tail
    double steps = 0.0;         // height in diatonic steps above middle C
    double slope = 0.0;         // the curve of the line that leads to this point
};

/** The duration line of a note that glides: the head first, the tail last; empty for a note without glide points.
    The line starts at the head, so at the height of the note as it is spelled; each point lies at the height of
    that white key's pitch plus the point's offset in whole cents, as in bach. */
std::vector<GlidePoint> glidePoints (const Spelling& head, const std::vector<maket::Breakpoint>& breakpoints);

/** The height at one place along such a line (0 to 1). */
double glideStepsAt (const std::vector<GlidePoint>& points, double position);

/** On which line each marker's name is written, bach's "smart" placing: a name that would run into the one
    before it goes one line lower, and back to the first line as soon as there is room after the last name there.
    `x` is each marker's place across (rising), `width` the width of its name, `gap` the room kept between names. */
std::vector<int> markerNameLines (const std::vector<double>& x, const std::vector<double>& width, double gap);

} // namespace maket::notation
