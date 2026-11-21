// TabBar.cpp

#include "TabBar.h"
#include "Tabs.h"
#include "Theme.h"

namespace
{
    constexpr int   leftMargin = 16;
    constexpr int   tabPadding = 14;      // room on each side of a tab's name
    constexpr float nameHeight = 14.0f;

    int nameWidth (int index)
    {
        // Measured in bold, so that a tab keeps its width when it comes in front.
        return juce::roundToInt (juce::GlyphArrangement::getStringWidth (Theme::font (nameHeight, true),
                                                                         Tabs::info (index).name));
    }
}

TabBar::TabBar()
{
    setOpaque (true);
    addAndMakeVisible (listButton);
    addAndMakeVisible (inspectorButton);
}

void TabBar::resized()
{
    // The two panel buttons at the far right, in the order of their panels.
    auto right = getLocalBounds().reduced (0, 5).withTrimmedRight (10);
    inspectorButton.setBounds (right.removeFromRight (30));
    right.removeFromRight (2);
    listButton.setBounds (right.removeFromRight (30));
}

void TabBar::setCurrent (int index)
{
    index = juce::jlimit (0, (int) Tabs::count - 1, index);
    if (index == current) return;
    current = index;
    repaint();
}

void TabBar::setNote (const juce::String& text)
{
    note = text;
    repaint();
}

juce::Rectangle<int> TabBar::tabBounds (int index) const
{
    int x = leftMargin;
    for (int i = 0; i < index; ++i)
        x += nameWidth (i) + 2 * tabPadding;
    return { x, 0, nameWidth (index) + 2 * tabPadding, getHeight() };
}

int TabBar::tabAt (juce::Point<int> p) const
{
    for (int i = 0; i < Tabs::count; ++i)
        if (tabBounds (i).contains (p)) return i;
    return -1;
}

void TabBar::paint (juce::Graphics& g)
{
    g.fillAll (Theme::header);
    g.setColour (Theme::line);
    g.fillRect (0, getHeight() - 1, getWidth(), 1);

    for (int i = 0; i < Tabs::count; ++i)
    {
        const auto r = tabBounds (i);
        const bool front = (i == current);

        g.setColour (front ? Theme::text : (i == hovered ? Theme::text.withAlpha (0.85f) : Theme::textMuted));
        g.setFont (Theme::font (nameHeight, front));
        g.drawText (Tabs::info (i).name, r, juce::Justification::centred, false);

        if (front)
        {
            g.setColour (Theme::accent);
            g.fillRect (r.getX() + tabPadding, getHeight() - 3, r.getWidth() - 2 * tabPadding, 3);
        }
    }

    if (note.isNotEmpty())
    {
        const int left = tabBounds (Tabs::count - 1).getRight() + 12;
        g.setColour (Theme::textMuted);
        g.setFont (Theme::font (12.0f));
        g.drawText (note, juce::Rectangle<int> (left, 0, listButton.getX() - left - 14, getHeight()),
                    juce::Justification::centredRight, true);
    }
}

void TabBar::mouseMove (const juce::MouseEvent& e)
{
    const int t = tabAt (e.getPosition());
    if (t != hovered) { hovered = t; repaint(); }
}

void TabBar::mouseExit (const juce::MouseEvent&)
{
    if (hovered != -1) { hovered = -1; repaint(); }
}

void TabBar::mouseDown (const juce::MouseEvent& e)
{
    const int t = tabAt (e.getPosition());
    if (t >= 0 && onSelect) onSelect (t);
}
