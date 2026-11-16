// EmptyPage.cpp

#include "EmptyPage.h"
#include "Tabs.h"
#include "Theme.h"

EmptyPage::EmptyPage (int tabIndex) : tab (tabIndex)
{
    setOpaque (true);
}

void EmptyPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    const auto& info = Tabs::info (tab);

    // A column of text in the middle of the page.
    const int width = juce::jmin (560, getWidth() - 80);
    if (width < 120) return;
    auto column = getLocalBounds().withSizeKeepingCentre (width, 200);

    g.setColour (Theme::text);
    g.setFont (Theme::font (22.0f, true));
    g.drawText (info.name, column.removeFromTop (32), juce::Justification::centredLeft, true);

    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (13.0f));
    g.drawText ("Empty at this stage.", column.removeFromTop (24), juce::Justification::centredLeft, true);

    column.removeFromTop (14);
    g.setColour (Theme::line);
    g.fillRect (column.removeFromTop (1));
    column.removeFromTop (14);

    // What it will hold, then when; the text wraps to the column.
    juce::AttributedString body;
    body.setLineSpacing (3.0f);
    body.append ("What comes here\n", Theme::font (12.0f), Theme::textMuted);
    body.append (juce::String (info.holds) + "\n\n", Theme::font (14.0f), Theme::text);
    body.append (info.arrives, Theme::font (13.0f), Theme::textMuted);

    juce::TextLayout layout;
    layout.createLayout (body, (float) column.getWidth());
    layout.draw (g, column.toFloat());
}
