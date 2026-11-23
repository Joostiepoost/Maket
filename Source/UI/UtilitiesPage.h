// UtilitiesPage.h — the Utilities tab.
//
// A list on the left chooses the page; the page fills the rest. At stage 0c
// there is one page, Preferences. The pages still to come are listed in
// grey with the step that brings them.

#pragma once

#include <JuceHeader.h>
#include <functional>

#include "PreferencesPage.h"
#include "OscTestPage.h"
#include "TimingTestPage.h"
#include "ConTimbreTestPage.h"
#include "PlayerStudyPage.h"
#include "../Model/Settings.h"

class UtilitiesPage : public juce::Component
{
public:
    UtilitiesPage (maket::Settings&, maket::OscScheduler&);

    /** Called when a setting's field gets the keyboard. */
    std::function<void (const maket::SettingInfo&)> onSettingSelected;

    /** A problem met when the settings file was read; shown on the preferences page. */
    void setSettingsNotice (const juce::String& text)    { preferences.setNotice (text); }

    /** Hides or shows the list of pages on the left. */
    void setListVisible (bool shouldShow);

    /** See PreferencesPage::commitPendingEdits. */
    void commitPendingEdits()                             { preferences.commitPendingEdits(); }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    static constexpr int listWidth = 200;

    void showPage (int index);

    bool listVisible = true;
    int currentPage = 0;            // an index into the list of pages
    PreferencesPage preferences;
    OscTestPage oscTest;
    TimingTestPage timingTest;
    ConTimbreTestPage conTimbreTest;
    PlayerStudyPage playerStudy;
    juce::Viewport viewport;        // the page scrolls when the window is low

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UtilitiesPage)
};
