// PanelButton.cpp

#include "PanelButton.h"
#include "Theme.h"

PanelButton::PanelButton (const juce::String& name, Side sideOfPanel)
    : juce::Button (name), side (sideOfPanel)
{
}

void PanelButton::setPanelShown (bool shown)
{
    if (shown == panelShown) return;
    panelShown = shown;
    repaint();
}

void PanelButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto bounds = getLocalBounds().toFloat();

    if (isEnabled() && (highlighted || down))
    {
        g.setColour (Theme::panel2.brighter (down ? 0.15f : 0.05f));
        g.fillRoundedRectangle (bounds.reduced (1.0f), 5.0f);
    }

    // The window outline, 18 by 13, in the middle of the button.
    const auto frame = juce::Rectangle<float> (18.0f, 13.0f).withCentre (bounds.getCentre());
    const auto colour = ! isEnabled() ? Theme::textMuted.withAlpha (0.35f)
                        : panelShown  ? Theme::text
                                      : Theme::textMuted;
    g.setColour (colour);
    g.drawRoundedRectangle (frame, 2.5f, 1.3f);

    // The bar on the panel's side: filled while the panel shows.
    auto inner = frame.reduced (2.2f);
    const auto bar = side == Side::left ? inner.removeFromLeft (4.5f) : inner.removeFromRight (4.5f);
    if (panelShown && isEnabled()) g.fillRoundedRectangle (bar, 1.0f);
    else                           g.drawRoundedRectangle (bar.reduced (0.5f), 1.0f, 1.0f);
}
