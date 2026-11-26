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
#include "UI/EditionPage.h"
#include "UI/LibraryPage.h"
#include "Engine/OscScheduler.h"
#include "Engine/Synth.h"
#include "Audio/AudioOutput.h"
#include "UI/KeepAwake.h"
#include "Model/Settings.h"
#include "Model/WindowState.h"
#include "UI/PanelEdge.h"

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
        cmdToggleInspector,
        cmdOpen = 0x3020,
        cmdZoomAcrossIn, cmdZoomAcrossOut, cmdZoomDownIn, cmdZoomDownOut, cmdZoomToFit, cmdDrawingTimes,
        cmdPlayStop = 0x3030,       // stage 2d: the transport of the roll
        cmdPlayPrescheduled, cmdPlayOn, cmdToggleLoop, cmdToStart,
        cmdToggleAudio = 0x3040,    // stage 3a
        cmdNewProject = 0x3050, cmdOpenProject, cmdCloseProject, cmdImportRolls, cmdProjectPitch, cmdCloseTab     // stage 3b
    };

    /** Audio on: Maket's audio output is open. Off: it is closed and Maket makes no sound of its own and spends no
        time on it; conTimbre over OSC is not touched. Returns what went wrong when turning on, or nothing. */
    juce::String setAudioOn (bool shouldBeOn);
    bool isAudioOn() const              { return audioOutput.isOpen(); }

    /** The window's place and size as last left (for the window that holds this), and the way to keep it. */
    juce::String storedWindowBounds() const             { return windowState.windowBounds(); }
    void storeWindowBounds (const juce::String& b)      { windowState.setWindowBounds (b); windowState.save(); }

    /** Opens a bach.roll text file in the tab Library and edition. */
    bool openRollFile (const juce::File&);
    /** Several at once: a project's folder first if there is one, then rolls (into the open project's library, or shown by themselves). */
    void openFiles (const juce::Array<juce::File>&);

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
    bool currentTabHasList() const      { return tabBar.getCurrent() == Tabs::utilities
                                                 || (tabBar.getCurrent() == Tabs::library && library != nullptr && library->hasList()); }
    /** The editor of the snippet in front in the Library and edition tab; null when none is open. */
    EditionPage* editor() const         { return library != nullptr ? library->front() : nullptr; }

    /** Brings the two panel buttons in line with what shows. */
    void updatePanelButtons();

    /** Shows a setting in the inspector. */
    void showSetting (const maket::SettingInfo&);

    Theme::LookAndFeel lookAndFeel;
    juce::TooltipWindow tooltips { this };
    juce::ApplicationCommandManager commandManager;

    /** The widths of the list and the inspector, as kept. */
    void applyPanelWidths();

    maket::Settings settings { maket::Settings::defaultFile() };
    maket::WindowState windowState { maket::WindowState::defaultFile() };
    maket::OscScheduler oscScheduler;           // sends OSC messages at their time, on a thread of its own
    maket::Synth synth;                 // Maket's own sound (stage 2e); before the pages, which use it
    AudioOutput audioOutput { synth };
    KeepAwake keepAwake { oscScheduler };       // tells macOS not to slow Maket down while messages wait
    UtilitiesPage* utilities = nullptr;         // owned by `pages`
    LibraryPage* library = nullptr;             // owned by `pages`
    bool listShown = true;
    bool inspectorShown = true;

    TabBar tabBar;
    std::array<std::unique_ptr<juce::Component>, Tabs::count> pages;
    Inspector inspector;
    PanelEdge listEdge, inspectorEdge;          // dragged to change a panel's width
    int widthAtDragStart = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
