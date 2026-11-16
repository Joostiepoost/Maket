// TabBar.h — the row of tabs along the top of the main window.
//
// The tab in front is drawn in bold with an accent line under it. A click on
// a tab calls onSelect; the main component decides what happens. The right
// end of the row carries a short note (for now: the version and build stage;
// the transport comes here at step 2).

#pragma once

#include <JuceHeader.h>
#include <functional>

class TabBar : public juce::Component
{
public:
    TabBar();

    static constexpr int height = 38;

    void setCurrent (int index);
    int  getCurrent() const            { return current; }

    /** The muted text at the right end of the row. */
    void setNote (const juce::String& text);

    std::function<void (int)> onSelect;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::Rectangle<int> tabBounds (int index) const;
    int tabAt (juce::Point<int> p) const;

    int current = 0;
    int hovered = -1;
    juce::String note;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabBar)
};
