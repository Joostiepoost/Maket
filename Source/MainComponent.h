// MainComponent.h — everything inside Maket's main window.
//
// The row of four tabs (Library and edition, Build surface, Render,
// Utilities), an empty page for each, and the inspector on the right. A tab
// comes in front by a click or by Cmd-1 to Cmd-4 (also in the View menu).

#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>

#include "UI/Theme.h"
#include "UI/Tabs.h"
#include "UI/TabBar.h"
#include "UI/EmptyPage.h"
#include "UI/Inspector.h"

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
        cmdShowTab = 0x3000        // cmdShowTab + Tabs::Index
    };

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
    Theme::LookAndFeel lookAndFeel;
    juce::ApplicationCommandManager commandManager;

    TabBar tabBar;
    std::array<std::unique_ptr<juce::Component>, Tabs::count> pages;
    Inspector inspector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
