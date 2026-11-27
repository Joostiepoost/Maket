// Miniature.h — a small picture of a roll for the library (stage 3b).
//
// Every note as a line: across by time, up and down by pitch, a glide as a
// bent line. All in numbers from 0 to 1, so that it can be drawn at any size.
// No interface code.

#pragma once

#include <vector>
#include "../Model/Roll.h"

namespace maket::notation
{

struct MiniLine
{
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;      // 0..1; y = 0 is the highest pitch
    float weight = 1.0f;                        // 0..1 from the note's velocity
};

/** The notes of the roll as lines. With more than `most` notes, only every so-manyth is given: the picture stays the
    same to the eye, and drawing it stays quick. Muted notes are drawn too, at a third of their weight. */
std::vector<MiniLine> miniatureOf (const Roll& roll, int most = 3000);

} // namespace maket::notation
