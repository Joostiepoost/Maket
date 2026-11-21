// MainComponent.h — everything inside Maket's main window.
//
// The row of four tabs (Library and edition, Build surface, Render,
// Utilities), a page for each, and the inspector on the right. A tab comes
// in front by a click or by Cmd-1 to Cmd-4 (also in the View menu). Three
// pages are still empty; the Utilities tab holds the preferences page.
// The list on the left of a tab and the inspector can be hidden and shown
// (buttons in the tab row, View menu, Cmd-Alt-L and Cmd-Alt-I).
// The settings are read here when Maket starts.

#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>

#include "UI/Theme.h"
#include "UI/Tabs.h"
#include "UI/TabBar.h"
#include "UI/EmptyPage.h"
#include "UI/Inspector.h"
#include "UI/UtilitiesPage.h"
#include "Model/Settings.h"

class MainComponent : public juce::Component,
                      public juce::ApplicationCommandTarget,
                      public juce::MenuBarModel
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // One command per tab, in the order of Tabs::Index.
    enum CommandIDs
    {
        cmdShowTab = 0x3000,        // cmdShowTab + Tabs::Index
        cmdToggleList = 0x3010,
        cmdToggleInspector
    };

    /** Hides or shows the list on the left of the tabs that have one. */
    void setListShown (bool shouldShow);

    /** Hides or shows the inspector. */
    void setInspectorShown (bool shouldShow);

    /** Brings a tab in front (one of Tabs::Index). */
    void showTab (int index);
    int  currentTab() const            { return tabBar.getCurrent(); }

    juce::ApplicationCommandTarget* getNextCommandTarget() override   { return nullptr; }
    void getAllCommands (juce::Array<juce::CommandID>&) override;
    void getCommandInfo (juce::CommandID, juce::ApplicationCommandInfo&) override;
    bool perform (const InvocationInfo&) override;

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex (int menuIndex, const juce::String& menuName) override;
    void menuItemSelected (int, int) override   {}

private:
    /** True when the tab in front has a list on its left. */
    bool currentTabHasList() const      { return tabBar.getCurrent() == Tabs::utilities; }

    /** Brings the two panel buttons in line with what shows. */
    void updatePanelButtons();

    /** Shows a setting in the inspector. */
    void showSetting (const maket::SettingInfo&);

    Theme::LookAndFeel lookAndFeel;
    juce::TooltipWindow tooltips { this };
    juce::ApplicationCommandManager commandManager;

    maket::Settings settings { maket::Settings::defaultFile() };
    UtilitiesPage* utilities = nullptr;         // owned by `pages`
    bool listShown = true;
    bool inspectorShown = true;

    TabBar tabBar;
    std::array<std::unique_ptr<juce::Component>, Tabs::count> pages;
    Inspector inspector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
