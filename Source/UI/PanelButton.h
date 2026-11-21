// PanelButton.h — a small button in the tab row that hides or shows a panel:
// the list on the left of a tab, or the inspector on the right.
//
// It draws a window outline with a bar on the side of its panel. The bar is
// filled while the panel shows and empty while it is hidden.

#pragma once

#include <JuceHeader.h>

class PanelButton : public juce::Button
{
public:
    enum class Side { left, right };

    PanelButton (const juce::String& name, Side sideOfPanel);

    /** Whether the panel shows; the button only draws it, the main window decides. */
    void setPanelShown (bool shown);

    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    Side side;
    bool panelShown = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanelButton)
};
