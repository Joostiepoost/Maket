// PlayheadFlag.h — the running time, riding on the playhead (stage 3d.2).
//
// While something plays, only what moves is drawn again: the playhead. On the
// Mac the parts of a window that are to be drawn again within one frame are
// taken together as ONE rectangle around them all. A running time in the bar
// at the top and a playhead somewhere in the view therefore meant that nearly
// the whole page was drawn again thirty times a second (measured on Joost's
// Mac at 3d.1: 25 to 40 % of a processor, against 11 % for Reaper). So the
// running time is drawn beside the playhead, and the two together are one
// small strip. Nothing else may be redrawn on every tick of a play.

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

namespace playheadflag
{
    constexpr int width = 60, height = 15, reach = width + 10;     // reach: how far it stands out to either side of the playhead

    /** 1'02.3" */
    inline juce::String timeText (double ms)
    {
        const double seconds = juce::jmax (0.0, ms) / 1000.0;
        const int minutes = (int) (seconds / 60.0);
        return juce::String (minutes) + "'" + juce::String (seconds - minutes * 60.0, 1).paddedLeft ('0', 4) + "\"";
    }

    /** Where the flag stands for a playhead at x: to its right, or to its left when there is no room. */
    inline juce::Rectangle<int> bounds (int x, int top, int rightLimit)
    {
        return x + 9 + width <= rightLimit ? juce::Rectangle<int> (x + 9, top + 1, width, height)
                                           : juce::Rectangle<int> (x - 8 - width, top + 1, width, height);
    }

    inline void draw (juce::Graphics& g, juce::Rectangle<int> area, double ms, juce::Colour colour)
    {
        g.setColour (colour);
        g.fillRoundedRectangle (area.toFloat(), 3.0f);
        g.setColour (juce::Colour (0xff10240a));
        g.setFont (Theme::font (11.5f, true));
        g.drawText (timeText (ms), area, juce::Justification::centred, false);
    }
}
