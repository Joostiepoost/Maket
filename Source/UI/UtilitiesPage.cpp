// UtilitiesPage.cpp

#include "UtilitiesPage.h"
#include "Theme.h"

namespace
{
    // The pages of the Utilities tab (proposal, section 14). `step` is empty
    // for a page that exists.
    struct PageEntry { const char* name; const char* step; };

    const PageEntry pageList[] =
    {
        { "Instruments",  "step 7"  },
        { "Plugins",      "step 8"  },
        { "Orchestra",    "step 14" },
        { "Capture",      "step 12" },
        { "Quantize",     "step 13" },
        { "Preferences",  ""        }
    };

    constexpr int entryHeight = 30;
    constexpr int listTop = 14;
}

UtilitiesPage::UtilitiesPage (maket::Settings& settings) : preferences (settings)
{
    setOpaque (true);

    preferences.onSettingSelected = [this] (const maket::SettingInfo& info)
    {
        if (onSettingSelected) onSettingSelected (info);
    };

    viewport.setViewedComponent (&preferences, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);
}

void UtilitiesPage::resized()
{
    viewport.setBounds (getLocalBounds().withTrimmedLeft (listWidth));

    // The page takes the viewport's width and its own height; a scroll bar
    // appears when that is more than the window has.
    const int width = viewport.getMaximumVisibleWidth();
    preferences.setSize (width, juce::jmax (viewport.getMaximumVisibleHeight(), preferences.getHeightFor (width)));
}

void UtilitiesPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    auto list = getLocalBounds().removeFromLeft (listWidth);
    g.setColour (Theme::panel);
    g.fillRect (list);
    g.setColour (Theme::line);
    g.fillRect (list.getRight() - 1, 0, 1, getHeight());

    int y = listTop;
    for (const auto& entry : pageList)
    {
        const juce::Rectangle<int> r (0, y, listWidth - 1, entryHeight);
        const bool exists = juce::String (entry.step).isEmpty();

        if (exists)     // the page in front (the only one for now)
        {
            g.setColour (Theme::panel2);
            g.fillRect (r);
            g.setColour (Theme::accent);
            g.fillRect (r.withWidth (3));
        }

        g.setColour (exists ? Theme::text : Theme::textMuted.withAlpha (0.6f));
        g.setFont (Theme::font (14.0f, exists));
        g.drawText (entry.name, r.withTrimmedLeft (16), juce::Justification::centredLeft, true);

        if (! exists)
        {
            g.setFont (Theme::font (11.5f));
            g.drawText (entry.step, r.withTrimmedRight (12), juce::Justification::centredRight, true);
        }

        y += entryHeight;
    }
}
