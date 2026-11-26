// PanelEdge.h — the edge of a panel that can be dragged to make it wider or
// narrower (stage 3a). A strip a few pixels wide that lies over the border
// between a panel (the list, the inspector) and what is beside it. A double
// click sets the panel back to its first width.

#pragma once

#include <JuceHeader.h>
#include <functional>

#include "Theme.h"

class PanelEdge : public juce::Component
{
public:
    PanelEdge()
    {
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
        setRepaintsOnMouseActivity (true);
    }

    static constexpr int thickness = 7;

    std::function<void()> onDragStart;                  // the owner notes the width the drag starts from
    std::function<void (int)> onDrag;                   // pixels moved to the right since the start
    std::function<void()> onDragEnd;
    std::function<void()> onReset;                      // a double click

    void paint (juce::Graphics& g) override
    {
        if (isMouseOverOrDragging())
        {
            g.setColour (Theme::accent.withAlpha (isMouseButtonDown() ? 0.9f : 0.55f));
            g.fillRect (getLocalBounds().withSizeKeepingCentre (2, getHeight()));
        }
    }

    void mouseDown (const juce::MouseEvent&) override           { if (onDragStart) onDragStart(); }
    void mouseDrag (const juce::MouseEvent& e) override         { if (onDrag) onDrag (e.getDistanceFromDragStartX()); }
    void mouseUp (const juce::MouseEvent&) override             { if (onDragEnd) onDragEnd(); }
    void mouseDoubleClick (const juce::MouseEvent&) override    { if (onReset) onReset(); if (onDragEnd) onDragEnd(); }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanelEdge)
};
