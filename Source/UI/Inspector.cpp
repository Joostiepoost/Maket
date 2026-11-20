// Inspector.cpp

#include "Inspector.h"
#include "Tabs.h"
#include "Theme.h"

Inspector::Inspector()
{
    setOpaque (true);
}

void Inspector::setTab (int tabIndex)
{
    if (tabIndex == tab) return;
    tab = tabIndex;
    hasDetail = false;
    repaint();
}

void Inspector::showDetail (const juce::String& kind, const juce::String& name,
                            const juce::StringPairArray& labelledLines)
{
    hasDetail = true;
    detailKind = kind;
    detailName = name;
    detailLines = labelledLines;
    repaint();
}

void Inspector::clearDetail()
{
    if (! hasDetail) return;
    hasDetail = false;
    repaint();
}

void Inspector::paint (juce::Graphics& g)
{
    g.fillAll (Theme::panel);
    g.setColour (Theme::line);
    g.fillRect (0, 0, 1, getHeight());

    const auto& info = Tabs::info (tab);
    auto area = getLocalBounds().reduced (16, 14);

    g.setColour (Theme::text);
    g.setFont (Theme::font (14.0f, true));
    g.drawText ("Inspector", area.removeFromTop (22), juce::Justification::centredLeft, true);

    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (12.0f));
    g.drawText (info.name, area.removeFromTop (18), juce::Justification::centredLeft, true);

    area.removeFromTop (12);
    g.setColour (Theme::line);
    g.fillRect (area.removeFromTop (1));
    area.removeFromTop (12);

    juce::AttributedString body;
    body.setLineSpacing (3.0f);

    if (hasDetail)
    {
        // What is selected: its kind, its name, then label and value pairs.
        g.setColour (Theme::textMuted);
        g.setFont (Theme::font (12.0f));
        g.drawText (detailKind, area.removeFromTop (18), juce::Justification::centredLeft, true);

        g.setColour (Theme::text);
        g.setFont (Theme::font (15.0f, true));
        g.drawText (detailName, area.removeFromTop (24), juce::Justification::centredLeft, true);

        area.removeFromTop (8);

        const auto& labels = detailLines.getAllKeys();
        const auto& texts  = detailLines.getAllValues();
        for (int i = 0; i < labels.size(); ++i)
        {
            if (labels[i].isNotEmpty())
                body.append (labels[i] + "\n", Theme::font (12.0f), Theme::textMuted);
            body.append (texts[i] + (i + 1 < labels.size() ? "\n\n" : ""), Theme::font (13.0f), Theme::text);
        }
    }
    else
    {
        g.setColour (Theme::textMuted);
        g.setFont (Theme::font (13.0f));
        g.drawText ("Nothing is selected.", area.removeFromTop (20), juce::Justification::centredLeft, true);

        area.removeFromTop (10);

        // What it will show in this tab; the text wraps to the panel.
        body.append ("What comes here in this tab\n", Theme::font (12.0f), Theme::textMuted);
        body.append (info.inspectorShows, Theme::font (13.0f), Theme::text);
    }

    juce::TextLayout layout;
    layout.createLayout (body, (float) area.getWidth());
    layout.draw (g, area.toFloat());
}
