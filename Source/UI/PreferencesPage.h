// PreferencesPage.h — the preferences page of the Utilities tab.
//
// One row per setting, built from the table in Model/Settings.h: a folder
// row has a text field, a Choose button and a Default button; a host or a
// port has a text field and a Default button. A change is checked, and if it
// is valid it is written to the settings file at once (on Return, on leaving
// the field, on choosing a folder, on Default, and when Maket quits).

#pragma once

#include <JuceHeader.h>
#include <functional>

#include "../Model/Settings.h"

class PreferencesPage : public juce::Component
{
public:
    explicit PreferencesPage (maket::Settings&);
    ~PreferencesPage() override;

    /** Called when a setting's field gets the keyboard, with that setting. */
    std::function<void (const maket::SettingInfo&)> onSettingSelected;

    /** A message shown at the top of the page (a problem met when the
        settings file was read); empty for none. */
    void setNotice (const juce::String& text);

    /** Takes over what is still being typed in a field. Called before Maket
        quits, so that a value typed just before Cmd-Q is not lost. */
    void commitPendingEdits();

    /** The height the page needs at a given width. */
    int getHeightFor (int width) const;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    class Row;

    struct Layout
    {
        juce::Rectangle<int> title, subtitle, notice, fileLabel, filePath, fileButton;
        juce::Array<juce::Rectangle<int>> groupHeadings, rowBounds;
        int height = 0;
    };
    Layout computeLayout (int width) const;

    maket::Settings& settings;
    juce::OwnedArray<Row> rows;
    juce::StringArray groups;               // the group headings, in order
    juce::TextButton showFileButton;
    juce::String notice;
    Layout layout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreferencesPage)
};
