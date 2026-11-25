// MainComponent.cpp

#include "MainComponent.h"
#include "Model/AppInfo.h"
#include "Engine/ConTimbre.h"

MainComponent::MainComponent()
{
    juce::LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);
    setOpaque (true);

    const auto& app = maket::appInfo();
    tabBar.setNote (juce::String (app.name) + " " + app.version
                    + juce::String::fromUTF8 ("  \xc2\xb7  stage ") + app.stage);
    tabBar.onSelect = [this] (int index) { showTab (index); };
    tabBar.listButton.onClick      = [this] { setListShown (! listShown); };
    tabBar.inspectorButton.onClick = [this] { setInspectorShown (! inspectorShown); };
    addAndMakeVisible (tabBar);

    // Read the settings before anything uses them. A problem with the file
    // is shown on the preferences page; Maket starts all the same.
    const auto settingsRead = settings.load();

    for (int i = 0; i < Tabs::count; ++i)
    {
        if (i == Tabs::utilities)
        {
            auto page = std::make_unique<UtilitiesPage> (settings, oscScheduler);
            utilities = page.get();
            utilities->setSettingsNotice (settingsRead.getErrorMessage());
            utilities->onSettingSelected = [this] (const maket::SettingInfo& info) { showSetting (info); };
            pages[(size_t) i] = std::move (page);
        }
        else if (i == Tabs::library)
        {
            auto page = std::make_unique<EditionPage> (settings, oscScheduler, synth, audioOutput);
            PreferencesPage::audioOutputNames = [this] { return audioOutput.outputNames(); };
            PreferencesPage::audioOutputState = [this] { return audioOutput.describe(); };
            edition = page.get();
            pages[(size_t) i] = std::move (page);
        }
        else
        {
            pages[(size_t) i] = std::make_unique<EmptyPage> (i);
        }
        addChildComponent (*pages[(size_t) i]);
    }
    addAndMakeVisible (inspector);

    commandManager.registerAllCommandsForTarget (this);
    commandManager.setFirstCommandTarget (this);
    addKeyListener (commandManager.getKeyMappings());
    setApplicationCommandManagerToWatch (&commandManager);
   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (this);
   #endif

    pages[(size_t) tabBar.getCurrent()]->setVisible (true);
    inspector.setTab (tabBar.getCurrent());
    updatePanelButtons();

    setWantsKeyboardFocus (true);
    setSize (1400, 860);
}

MainComponent::~MainComponent()
{
    // A value still being typed on the preferences page is taken over, so
    // that quitting right after typing does not lose it.
    if (utilities != nullptr) utilities->commitPendingEdits();
    PreferencesPage::audioOutputNames = nullptr;
    PreferencesPage::audioOutputState = nullptr;

    // Messages that still wait are dropped, and the ePlayer is told to
    // stop every note: nothing is left sounding when Maket has gone.
    if (oscScheduler.numWaiting() > 0)
    {
        oscScheduler.clear();
        oscScheduler.sendImmediately (maket::contimbre::allNotesOff(), maket::OscScheduler::toPlayer);
    }

   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (nullptr);
   #endif
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    tabBar.setBounds (area.removeFromTop (TabBar::height));
    if (inspectorShown)
        inspector.setBounds (area.removeFromRight (Inspector::width));
    for (auto& page : pages)
        page->setBounds (area);
}

void MainComponent::showTab (int index)
{
    index = juce::jlimit (0, (int) Tabs::count - 1, index);
    if (index == tabBar.getCurrent()) return;

    for (int i = 0; i < Tabs::count; ++i)
        pages[(size_t) i]->setVisible (i == index);

    tabBar.setCurrent (index);
    inspector.setTab (index);
    updatePanelButtons();
    commandManager.commandStatusChanged();        // the View menu ticks the tab in front
}

void MainComponent::setListShown (bool shouldShow)
{
    listShown = shouldShow;
    if (utilities != nullptr) utilities->setListVisible (listShown);
    updatePanelButtons();
    commandManager.commandStatusChanged();
}

void MainComponent::setInspectorShown (bool shouldShow)
{
    inspectorShown = shouldShow;
    inspector.setVisible (inspectorShown);
    resized();
    updatePanelButtons();
    commandManager.commandStatusChanged();
}

void MainComponent::updatePanelButtons()
{
    const juce::String alt (juce::String::fromUTF8 ("\xe2\x8c\xa5"));       // the Option key's sign
    const juce::String cmd (juce::String::fromUTF8 ("\xe2\x8c\x98"));       // the Command key's sign

    auto& list = tabBar.listButton;
    list.setEnabled (currentTabHasList());
    list.setPanelShown (listShown);
    list.setTooltip (! currentTabHasList() ? juce::String ("This tab has no list yet")
                                           : (listShown ? "Hide the list  (" : "Show the list  (") + alt + cmd + "L)");

    auto& insp = tabBar.inspectorButton;
    insp.setPanelShown (inspectorShown);
    insp.setTooltip ((inspectorShown ? "Hide the inspector  (" : "Show the inspector  (") + alt + cmd + "I)");
}

void MainComponent::showSetting (const maket::SettingInfo& info)
{
    juce::StringPairArray lines;
    lines.set ("What it is for", info.help);
    lines.set ("Default", juce::String (info.defaultValue).isEmpty() ? "not set" : info.defaultValue);
    lines.set ("Name in the settings file", info.key);
    inspector.showDetail (juce::String::fromUTF8 ("Setting  \xc2\xb7  ") + info.group, info.label, lines);
}

// --- Commands and the menu bar -----------------------------------------------

void MainComponent::getAllCommands (juce::Array<juce::CommandID>& c)
{
    for (int i = 0; i < Tabs::count; ++i)
        c.add (cmdShowTab + i);
    c.add (cmdToggleList);
    c.add (cmdToggleInspector);
    for (auto id : { cmdOpen, cmdZoomAcrossIn, cmdZoomAcrossOut, cmdZoomDownIn, cmdZoomDownOut, cmdZoomToFit,
                     cmdPlayStop, cmdPlayPrescheduled, cmdPlayOn, cmdToggleLoop, cmdToStart })
        c.add (id);
}

bool MainComponent::openRollFile (const juce::File& file)
{
    showTab (Tabs::library);
    const bool opened = edition != nullptr && edition->openFile (file);
    commandManager.commandStatusChanged();
    return opened;
}

void MainComponent::getCommandInfo (juce::CommandID id, juce::ApplicationCommandInfo& info)
{
    const auto cmdAlt = juce::ModifierKeys::commandModifier | juce::ModifierKeys::altModifier;

    if (id == cmdToggleList)
    {
        info.setInfo (listShown ? "Hide the list" : "Show the list",
                      "Hide or show the list on the left of the tab", "View", 0);
        info.addDefaultKeypress ('l', cmdAlt);
        info.setActive (currentTabHasList());
        return;
    }
    if (id == cmdToggleInspector)
    {
        info.setInfo (inspectorShown ? "Hide the inspector" : "Show the inspector",
                      "Hide or show the inspector on the right", "View", 0);
        info.addDefaultKeypress ('i', cmdAlt);
        return;
    }

    const auto cmd = juce::ModifierKeys::commandModifier;
    const bool rollInFront = edition != nullptr && edition->hasRoll() && tabBar.getCurrent() == Tabs::library;
    switch (id)
    {
        case cmdOpen:
            info.setInfo (juce::String::fromUTF8 ("Openâ¦"), "Open a bach.roll text file", "File", 0);
            info.addDefaultKeypress ('o', cmd);
            return;
        case cmdZoomAcrossIn:
            info.setInfo ("More time detail", "Zoom in along the time axis", "View", 0);
            info.addDefaultKeypress ('=', cmd);
            info.addDefaultKeypress ('+', cmd);
            info.setActive (rollInFront);
            return;
        case cmdZoomAcrossOut:
            info.setInfo ("Less time detail", "Zoom out along the time axis", "View", 0);
            info.addDefaultKeypress ('-', cmd);
            info.setActive (rollInFront);
            return;
        case cmdZoomDownIn:
            info.setInfo ("Larger staves", "Zoom in on the staves", "View", 0);
            info.addDefaultKeypress ('=', cmdAlt);
            info.setActive (rollInFront);
            return;
        case cmdZoomDownOut:
            info.setInfo ("Smaller staves", "Zoom out on the staves", "View", 0);
            info.addDefaultKeypress ('-', cmdAlt);
            info.setActive (rollInFront);
            return;
        case cmdZoomToFit:
            info.setInfo ("The whole roll in the window", "Zoom so that the whole roll shows", "View", 0);
            info.addDefaultKeypress ('0', cmd);
            info.setActive (rollInFront);
            return;
        case cmdPlayStop:
            info.setInfo (edition != nullptr && edition->isPlaying() ? "Stop" : "Play", "Play the roll from the playhead on the conTimbre ePlayer, or stop", "Transport", 0);
            info.addDefaultKeypress (juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers);
            info.setActive (rollInFront);
            return;
        case cmdPlayPrescheduled:
            info.setInfo ("Play prescheduled", "Play from the playhead without anything on screen following the sound", "Transport", 0);
            info.addDefaultKeypress (juce::KeyPress::spaceKey, juce::ModifierKeys::shiftModifier);
            info.setActive (rollInFront);
            return;
        case cmdPlayOn:
            info.setInfo ("Play on from where it stopped", "Play from where the last play was stopped", "Transport", 0);
            info.addDefaultKeypress (juce::KeyPress::spaceKey, juce::ModifierKeys::altModifier);
            info.setActive (rollInFront && ! edition->isPlaying());
            return;
        case cmdToggleLoop:
            info.setInfo ("Go round the loop", "Play goes round the loop region marked in the ruler", "Transport", 0);
            info.addDefaultKeypress ('l', juce::ModifierKeys::noModifiers);
            info.setTicked (edition != nullptr && edition->isLoopOn());
            info.setActive (rollInFront);
            return;
        case cmdToStart:
            info.setInfo ("Playhead to the start", "Put the playhead at the start of the roll", "Transport", 0);
            info.addDefaultKeypress (juce::KeyPress::returnKey, juce::ModifierKeys::noModifiers);
            info.setActive (rollInFront);
            return;
        default: break;
    }

    const int tab = (int) id - cmdShowTab;
    if (tab < 0 || tab >= Tabs::count) return;

    const juce::String name (Tabs::info (tab).name);
    info.setInfo (name, "Bring the tab " + name + " in front", "View", 0);
    info.setTicked (tab == tabBar.getCurrent());
    info.addDefaultKeypress ('1' + tab, juce::ModifierKeys::commandModifier);
}

bool MainComponent::perform (const InvocationInfo& info)
{
    if (info.commandID == cmdToggleList)      { setListShown (! listShown); return true; }
    if (info.commandID == cmdToggleInspector) { setInspectorShown (! inspectorShown); return true; }

    if (edition != nullptr)
        switch (info.commandID)
        {
            case cmdOpen:           showTab (Tabs::library); edition->chooseFile(); return true;
            case cmdZoomAcrossIn:   edition->zoomAcross (1);    return true;
            case cmdZoomAcrossOut:  edition->zoomAcross (-1);   return true;
            case cmdZoomDownIn:     edition->zoomDown (1);      return true;
            case cmdZoomDownOut:    edition->zoomDown (-1);     return true;
            case cmdZoomToFit:      edition->zoomToFit();       return true;
            case cmdPlayStop:           edition->playOrStop();          commandManager.commandStatusChanged(); return true;
            case cmdPlayPrescheduled:   edition->playPrescheduled();    commandManager.commandStatusChanged(); return true;
            case cmdPlayOn:             edition->playOn();              commandManager.commandStatusChanged(); return true;
            case cmdToggleLoop:         edition->toggleLoop();          commandManager.commandStatusChanged(); return true;
            case cmdToStart:            edition->toStart();             return true;
            default: break;
        }

    const int tab = (int) info.commandID - cmdShowTab;
    if (tab < 0 || tab >= Tabs::count) return false;
    showTab (tab);
    return true;
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "View", "Transport" };
}

juce::PopupMenu MainComponent::getMenuForIndex (int menuIndex, const juce::String&)
{
    juce::PopupMenu m;
    if (menuIndex == 0)
    {
        m.addCommandItem (&commandManager, cmdOpen);
    }
    else if (menuIndex == 1)
    {
        for (auto id : { cmdZoomAcrossIn, cmdZoomAcrossOut, cmdZoomDownIn, cmdZoomDownOut, cmdZoomToFit })
            m.addCommandItem (&commandManager, id);
        m.addSeparator();
        for (int i = 0; i < Tabs::count; ++i)
            m.addCommandItem (&commandManager, cmdShowTab + i);
        m.addSeparator();
        m.addCommandItem (&commandManager, cmdToggleList);
        m.addCommandItem (&commandManager, cmdToggleInspector);
    }
    else if (menuIndex == 2)
    {
        for (auto id : { cmdPlayStop, cmdPlayPrescheduled, cmdPlayOn })
            m.addCommandItem (&commandManager, id);
        m.addSeparator();
        m.addCommandItem (&commandManager, cmdToggleLoop);
        m.addCommandItem (&commandManager, cmdToStart);
    }
    return m;
}
