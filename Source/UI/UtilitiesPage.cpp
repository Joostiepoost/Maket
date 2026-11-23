// UtilitiesPage.cpp

#include "UtilitiesPage.h"
#include "Theme.h"

#include <iterator>

namespace
{
    // The pages of the Utilities tab (proposal, section 14). `step` is empty
    // for a page that exists. The OSC test page is a test bench for step 1.
    struct PageEntry { const char* name; const char* step; };

    const PageEntry pageList[] =
    {
        { "Instruments",  "step 7"  },
        { "Plugins",      "step 8"  },
        { "Orchestra",    "step 14" },
        { "Capture",      "step 12" },
        { "Quantize",     "step 13" },
        { "Preferences",  ""        },
        { "OSC test",     ""        },
        { "Timing test",  ""        },
        { "conTimbre test", ""      },
        { "ePlayer study", ""       }
    };

    constexpr int preferencesIndex = 5;
    constexpr int oscTestIndex = 6;
    constexpr int timingTestIndex = 7;
    constexpr int conTimbreTestIndex = 8;
    constexpr int playerStudyIndex = 9;
    constexpr int entryHeight = 30;
    constexpr int listTop = 14;

    bool exists (int index)     { return juce::String (pageList[index].step).isEmpty(); }
}

UtilitiesPage::UtilitiesPage (maket::Settings& settings, maket::OscScheduler& scheduler)
    : currentPage (preferencesIndex), preferences (settings), oscTest (settings, scheduler), timingTest (settings, scheduler),
      conTimbreTest (settings, scheduler),
      playerStudy (settings, scheduler)
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

void UtilitiesPage::showPage (int index)
{
    if (index == currentPage || ! exists (index)) return;

    preferences.commitPendingEdits();       // a value still being typed is taken over before the page goes
    currentPage = index;

    if (index == oscTestIndex)          viewport.setViewedComponent (&oscTest, false);
    else if (index == timingTestIndex)  viewport.setViewedComponent (&timingTest, false);
    else if (index == conTimbreTestIndex) viewport.setViewedComponent (&conTimbreTest, false);
    else if (index == playerStudyIndex) viewport.setViewedComponent (&playerStudy, false);
    else                                viewport.setViewedComponent (&preferences, false);

    resized();
    repaint();
}

void UtilitiesPage::mouseDown (const juce::MouseEvent& e)
{
    if (! listVisible || e.x >= listWidth || e.y < listTop) return;

    const int index = (e.y - listTop) / entryHeight;
    if (index >= 0 && index < (int) std::size (pageList)) showPage (index);
}

void UtilitiesPage::setListVisible (bool shouldShow)
{
    if (shouldShow == listVisible) return;
    listVisible = shouldShow;
    resized();
    repaint();
}

void UtilitiesPage::resized()
{
    viewport.setBounds (getLocalBounds().withTrimmedLeft (listVisible ? listWidth : 0));

    // The page takes the viewport's width and its own height; a scroll bar
    // appears when that is more than the window has.
    const int width = viewport.getMaximumVisibleWidth();
    const int height = viewport.getMaximumVisibleHeight();
    if (currentPage == oscTestIndex)
        oscTest.setSize (width, juce::jmax (height, oscTest.getHeightFor (width)));
    else if (currentPage == timingTestIndex)
        timingTest.setSize (width, juce::jmax (height, timingTest.getHeightFor (width)));
    else if (currentPage == conTimbreTestIndex)
        conTimbreTest.setSize (width, juce::jmax (height, conTimbreTest.getHeightFor (width)));
    else if (currentPage == playerStudyIndex)
        playerStudy.setSize (width, juce::jmax (height, playerStudy.getHeightFor (width)));
    else
        preferences.setSize (width, juce::jmax (height, preferences.getHeightFor (width)));
}

void UtilitiesPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);
    if (! listVisible) return;

    auto list = getLocalBounds().removeFromLeft (listWidth);
    g.setColour (Theme::panel);
    g.fillRect (list);
    g.setColour (Theme::line);
    g.fillRect (list.getRight() - 1, 0, 1, getHeight());

    int y = listTop;
    for (int i = 0; i < (int) std::size (pageList); ++i)
    {
        const auto& entry = pageList[i];
        const juce::Rectangle<int> r (0, y, listWidth - 1, entryHeight);
        const bool there = exists (i);

        if (i == currentPage)       // the page in front
        {
            g.setColour (Theme::panel2);
            g.fillRect (r);
            g.setColour (Theme::accent);
            g.fillRect (r.withWidth (3));
        }

        g.setColour (there ? Theme::text : Theme::textMuted.withAlpha (0.6f));
        g.setFont (Theme::font (14.0f, i == currentPage));
        g.drawText (entry.name, r.withTrimmedLeft (16), juce::Justification::centredLeft, true);

        if (! there)
        {
            g.setFont (Theme::font (11.5f));
            g.drawText (entry.step, r.withTrimmedRight (12), juce::Justification::centredRight, true);
        }

        y += entryHeight;
    }
}
