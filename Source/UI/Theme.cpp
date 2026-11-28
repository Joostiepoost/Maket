// Theme.cpp

#include "Theme.h"

namespace Theme
{

juce::Font font (float height, bool bold)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font mono (float height)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain));
}

LookAndFeel::LookAndFeel()
{
    auto scheme = juce::LookAndFeel_V4::getDarkColourScheme();
    scheme.setUIColour (ColourScheme::windowBackground, background);
    scheme.setUIColour (ColourScheme::widgetBackground, panel2);
    scheme.setUIColour (ColourScheme::menuBackground, panel);
    scheme.setUIColour (ColourScheme::outline, line);
    scheme.setUIColour (ColourScheme::defaultText, text);
    scheme.setUIColour (ColourScheme::defaultFill, lineStrong);
    scheme.setUIColour (ColourScheme::highlightedText, background);
    scheme.setUIColour (ColourScheme::highlightedFill, accent);
    scheme.setUIColour (ColourScheme::menuText, text);
    setColourScheme (scheme);

    setColour (juce::TextButton::buttonColourId, panel2);
    setColour (juce::TextButton::buttonOnColourId, accent);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, background);
    setColour (juce::ComboBox::backgroundColourId, panel2);
    setColour (juce::ComboBox::outlineColourId, line);
    setColour (juce::TextEditor::backgroundColourId, panel2);
    setColour (juce::TextEditor::outlineColourId, line);
    setColour (juce::TextEditor::focusedOutlineColourId, accent);
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::highlightColourId, accent.withAlpha (0.35f));
    setColour (juce::TextEditor::highlightedTextColourId, text);
    setColour (juce::CaretComponent::caretColourId, accent);
    setColour (juce::Label::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour (juce::PopupMenu::highlightedTextColourId, background);
    setColour (juce::ScrollBar::thumbColourId, lineStrong);
}

juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return font (juce::jmin (14.0f, (float) buttonHeight * 0.55f));
}

juce::Font LookAndFeel::getLabelFont (juce::Label& l)
{
    return l.getFont();
}

juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return font (13.0f);
}

juce::Font LookAndFeel::getPopupMenuFont()
{
    return font (14.0f);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& backgroundColour,
                                        bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    auto c = b.getToggleState() ? b.findColour (juce::TextButton::buttonOnColourId) : backgroundColour;      // (the button's own colour, when it has one)
    if (down) c = c.brighter (0.15f);
    else if (highlighted) c = c.brighter (0.07f);
    g.setColour (c);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (b.getToggleState() ? c : line);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

} // namespace Theme
