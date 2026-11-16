// Inspector.h — the panel on the right of the main window.
//
// One inspector serves all four tabs and shows what belongs to the tab in
// front. At this stage nothing can be selected, so it says what it will show.

#pragma once

#include <JuceHeader.h>

class Inspector : public juce::Component
{
public:
    Inspector();

    static constexpr int width = 280;

    /** The tab in front (one of Tabs::Index). */
    void setTab (int tabIndex);

    void paint (juce::Graphics&) override;

private:
    int tab = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Inspector)
};
