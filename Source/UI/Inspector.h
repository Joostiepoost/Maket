// Inspector.h — the panel on the right of the main window.
//
// One inspector serves all four tabs and shows what belongs to the tab in
// front. While nothing is selected it says what it will show in that tab.
// The first thing that can be selected is a setting on the preferences page.

#pragma once

#include <JuceHeader.h>

class Inspector : public juce::Component
{
public:
    Inspector();

    static constexpr int width = 280;

    /** The tab in front (one of Tabs::Index). Changing tab clears the detail. */
    void setTab (int tabIndex);

    /** Shows what is selected: a heading (what kind of thing), its name, and
        lines of label and value. */
    void showDetail (const juce::String& kind, const juce::String& name,
                     const juce::StringPairArray& labelledLines);

    /** Back to "nothing is selected". */
    void clearDetail();

    void paint (juce::Graphics&) override;

private:
    int tab = 0;
    bool hasDetail = false;
    juce::String detailKind, detailName;
    juce::StringPairArray detailLines;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Inspector)
};
