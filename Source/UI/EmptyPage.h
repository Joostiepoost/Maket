// EmptyPage.h — what a tab shows until its build step fills it.
//
// It names the tab, says what it will hold and at which step that arrives.
// Each of the four tabs has one; a page is replaced by the real thing when
// its step is built.

#pragma once

#include <JuceHeader.h>

class EmptyPage : public juce::Component
{
public:
    explicit EmptyPage (int tabIndex);

    void paint (juce::Graphics&) override;

private:
    int tab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EmptyPage)
};
