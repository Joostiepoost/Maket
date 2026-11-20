// UtilitiesPage.h — the Utilities tab.
//
// A list on the left chooses the page; the page fills the rest. At stage 0c
// there is one page, Preferences. The pages still to come are listed in
// grey with the step that brings them.

#pragma once

#include <JuceHeader.h>
#include <functional>

#include "PreferencesPage.h"
#include "../Model/Settings.h"

class UtilitiesPage : public juce::Component
{
public:
    explicit UtilitiesPage (maket::Settings&);

    /** Called when a setting's field gets the keyboard. */
    std::function<void (const maket::SettingInfo&)> onSettingSelected;

    /** A problem met when the settings file was read; shown on the preferences page. */
    void setSettingsNotice (const juce::String& text)    { preferences.setNotice (text); }

    /** See PreferencesPage::commitPendingEdits. */
    void commitPendingEdits()                             { preferences.commitPendingEdits(); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int listWidth = 200;

    PreferencesPage preferences;
    juce::Viewport viewport;        // the page scrolls when the window is low

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UtilitiesPage)
};
