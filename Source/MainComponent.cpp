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
    tabBar.audioButton.onClick     = [this] { setAudioOn (! isAudioOn()); };
    addAndMakeVisible (tabBar);

    // How the window was left: panels, widths, the tab in front.
    windowState.load();
    listShown = windowState.listShown();
    inspectorShown = windowState.inspectorShown();

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
            audioOutput.setSoundPlayer (&soundPlayer);
            auto page = std::make_unique<LibraryPage> (settings, windowState, oscScheduler, synth, audioOutput, soundPlayer);
            PreferencesPage::audioOutputNames = [this] { return audioOutput.outputNames(); };
            PreferencesPage::audioOutputState = [this] { return audioOutput.describe(); };
            library = page.get();
            library->turnAudioOn = [this] { return setAudioOn (true); };
            library->onChanged = [this] { resized(); updatePanelButtons(); commandManager.commandStatusChanged(); };
            pages[(size_t) i] = std::move (page);
        }
        else
        {
            pages[(size_t) i] = std::make_unique<EmptyPage> (i);
        }
        addChildComponent (*pages[(size_t) i]);
    }
    addAndMakeVisible (inspector);

    // The edges of the two panels: drag to change the width, double click for the first width.
    listEdge.onDragStart = [this] { widthAtDragStart = windowState.listWidth(); };
    listEdge.onDrag      = [this] (int moved) { windowState.setListWidth (widthAtDragStart + moved); applyPanelWidths(); };
    listEdge.onReset     = [this] { windowState.setListWidth (maket::WindowState::listWidthDefault); applyPanelWidths(); };
    listEdge.onDragEnd   = [this] { windowState.save(); };
    inspectorEdge.onDragStart = [this] { widthAtDragStart = windowState.inspectorWidth(); };
    inspectorEdge.onDrag      = [this] (int moved) { windowState.setInspectorWidth (widthAtDragStart - moved); applyPanelWidths(); };
    inspectorEdge.onReset     = [this] { windowState.setInspectorWidth (maket::WindowState::inspectorWidthDefault); applyPanelWidths(); };
    inspectorEdge.onDragEnd   = [this] { windowState.save(); };
    addChildComponent (listEdge);
    addChildComponent (inspectorEdge);
    if (utilities != nullptr)
    {
        utilities->setListWidth (windowState.listWidth());
        utilities->setListVisible (listShown);
    }
    if (library != nullptr)
    {
        library->setListWidth (windowState.listWidth());
        library->setListVisible (listShown);
    }
    inspector.setVisible (inspectorShown);

    commandManager.registerAllCommandsForTarget (this);
    commandManager.setFirstCommandTarget (this);
    addKeyListener (commandManager.getKeyMappings());
    setApplicationCommandManagerToWatch (&commandManager);
   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (this);
   #endif

    tabBar.setCurrent (Tabs::library);          // stage 3b: Maket opens on the start panel, in the first tab
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
    windowState.save();
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
    {
        // The inspector never takes more than the window can spare.
        const int width = juce::jmin (windowState.inspectorWidth(), juce::jmax (maket::WindowState::inspectorWidthMin, area.getWidth() - 480));
        inspector.setBounds (area.removeFromRight (width));
    }
    for (auto& page : pages)
        page->setBounds (area);

    // The edges lie over the borders, in front of everything.
    inspectorEdge.setVisible (inspectorShown);
    inspectorEdge.setBounds (area.getRight() - PanelEdge::thickness / 2, area.getY(), PanelEdge::thickness, area.getHeight());
    const bool list = listShown && currentTabHasList() && utilities != nullptr && library != nullptr;
    listEdge.setVisible (list);
    if (list)
    {
        const int width = tabBar.getCurrent() == Tabs::library ? library->getListWidth() : utilities->getListWidth();
        listEdge.setBounds (area.getX() + width - PanelEdge::thickness / 2 - 1, area.getY(), PanelEdge::thickness, area.getHeight());
    }
    listEdge.toFront (false);
    inspectorEdge.toFront (false);
}

void MainComponent::applyPanelWidths()
{
    const int listWidth = juce::jmin (windowState.listWidth(), juce::jmax (maket::WindowState::listWidthMin, getWidth() / 3));
    if (utilities != nullptr) utilities->setListWidth (listWidth);
    if (library != nullptr)   library->setListWidth (listWidth);
    resized();
}

juce::String MainComponent::setAudioOn (bool shouldBeOn)
{
    juce::String problem;
    if (shouldBeOn)
    {
        // Opens the output of the preferences; when it is open already with that output, nothing happens.
        problem = audioOutput.open (settings.get ("audioOutput"));
    }
    else
    {
        synth.allNotesOff();
        soundPlayer.stop();
        audioOutput.close();
    }

    tabBar.setAudioOn (isAudioOn());
    if (library != nullptr) library->audioChanged (isAudioOn(), problem);
    commandManager.commandStatusChanged();
    return problem;
}

void MainComponent::showTab (int index)
{
    index = juce::jlimit (0, (int) Tabs::count - 1, index);
    if (index == tabBar.getCurrent()) return;

    for (int i = 0; i < Tabs::count; ++i)
        pages[(size_t) i]->setVisible (i == index);

    tabBar.setCurrent (index);
    inspector.setTab (index);
    windowState.setTab (index);
    resized();                  // the list's edge shows only in a tab with a list
    updatePanelButtons();
    commandManager.commandStatusChanged();        // the View menu ticks the tab in front
}

void MainComponent::setListShown (bool shouldShow)
{
    listShown = shouldShow;
    windowState.setListShown (listShown);
    if (utilities != nullptr) utilities->setListVisible (listShown);
    if (library != nullptr)   library->setListVisible (listShown);
    resized();
    updatePanelButtons();
    commandManager.commandStatusChanged();
}

void MainComponent::setInspectorShown (bool shouldShow)
{
    inspectorShown = shouldShow;
    windowState.setInspectorShown (inspectorShown);
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
    for (auto id : { cmdOpen, cmdZoomAcrossIn, cmdZoomAcrossOut, cmdZoomDownIn, cmdZoomDownOut, cmdZoomToFit, cmdDrawingTimes,
                     cmdPlayStop, cmdPlayPrescheduled, cmdPlayOn, cmdToggleLoop, cmdToStart, cmdToggleAudio,
                     cmdNewProject, cmdOpenProject, cmdCloseProject, cmdImportRolls, cmdProjectPitch, cmdCloseTab })
        c.add (id);
}

bool MainComponent::openRollFile (const juce::File& file)
{
    showTab (Tabs::library);
    // A folder that holds a project is opened as a project; a file as a single roll, outside any project.
    const bool opened = library != nullptr && (file.isDirectory() ? library->openProject (file) : library->openLooseRoll (file));
    commandManager.commandStatusChanged();
    return opened;
}

void MainComponent::openFiles (const juce::Array<juce::File>& files)
{
    if (library == nullptr || files.isEmpty()) return;
    showTab (Tabs::library);
    juce::Array<juce::File> rolls;
    for (const auto& f : files)
    {
        if (f.isDirectory()) library->openProject (f);
        else                 rolls.add (f);
    }
    if (library->hasProject()) library->importFiles (rolls);
    else for (const auto& f : rolls) library->openLooseRoll (f);
    commandManager.commandStatusChanged();
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
    const auto cmdShift = juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier;
    const bool rollInFront = editor() != nullptr && editor()->hasRoll() && tabBar.getCurrent() == Tabs::library;
    const bool project = library != nullptr && library->hasProject();
    const bool soundInFront = library != nullptr && library->frontSound() != nullptr && tabBar.getCurrent() == Tabs::library;
    const bool anyInFront = rollInFront || soundInFront;
    switch (id)
    {
        case cmdOpen:
            info.setInfo (juce::String::fromUTF8 ("Open a bach roll\xe2\x80\xa6"), "Open a single bach.roll text file, outside any project", "File", 0);
            info.addDefaultKeypress ('o', cmd);
            return;
        case cmdNewProject:
            info.setInfo (juce::String::fromUTF8 ("New project\xe2\x80\xa6"), "Make a new project: a folder with a library", "File", 0);
            info.addDefaultKeypress ('n', cmdShift);
            return;
        case cmdOpenProject:
            info.setInfo (juce::String::fromUTF8 ("Open project\xe2\x80\xa6"), "Open a project by its folder", "File", 0);
            info.addDefaultKeypress ('o', cmdShift);
            return;
        case cmdCloseProject:
            info.setInfo ("Close the project", "Close the project; Maket shows the start panel again", "File", 0);
            info.setActive (project);
            return;
        case cmdImportRolls:
            info.setInfo (juce::String::fromUTF8 ("Import\xe2\x80\xa6"), "Bring bach.roll text files and sound files into the project's library", "File", 0);
            info.addDefaultKeypress ('i', cmd);
            info.setActive (project);
            return;
        case cmdProjectPitch:
            info.setInfo (juce::String::fromUTF8 ("The project's standard pitch\xe2\x80\xa6"), "Set the frequency of the A for everything in this project", "File", 0);
            info.setActive (project);
            return;
        case cmdCloseTab:
            info.setInfo ("Close the snippet's tab", "Close the tab in front; the snippet stays in the library", "File", 0);
            info.addDefaultKeypress ('w', cmd);
            info.setActive (editor() != nullptr && tabBar.getCurrent() == Tabs::library);
            return;
        case cmdZoomAcrossIn:
            info.setInfo ("More time detail", "Zoom in along the time axis", "View", 0);
            info.addDefaultKeypress ('=', cmd);
            info.addDefaultKeypress ('+', cmd);
            info.setActive (anyInFront);
            return;
        case cmdZoomAcrossOut:
            info.setInfo ("Less time detail", "Zoom out along the time axis", "View", 0);
            info.addDefaultKeypress ('-', cmd);
            info.setActive (anyInFront);
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
            info.setActive (anyInFront);
            return;
        case cmdPlayStop:
            info.setInfo ((editor() != nullptr && editor()->isPlaying()) || (library != nullptr && library->frontSound() != nullptr && library->frontSound()->isPlaying()) ? "Stop" : "Play", "Play the roll from the playhead on the conTimbre ePlayer, or stop", "Transport", 0);
            info.addDefaultKeypress (juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers);
            info.setActive (anyInFront);
            return;
        case cmdPlayPrescheduled:
            info.setInfo ("Play prescheduled", "Play from the playhead without anything on screen following the sound", "Transport", 0);
            info.addDefaultKeypress (juce::KeyPress::spaceKey, juce::ModifierKeys::shiftModifier);
            info.setActive (rollInFront);
            return;
        case cmdDrawingTimes:
            info.setInfo ("Show drawing times", "Write in the corner of the roll how long its last drawing took", "View", 0);
            info.addDefaultKeypress ('t', cmdAlt);
            info.setTicked (editor() != nullptr && editor()->showsDrawingTimes());
            info.setActive (rollInFront);
            return;
        case cmdPlayOn:
            info.setInfo ("Play on from where it stopped", "Play from where the last play was stopped", "Transport", 0);
            info.addDefaultKeypress (juce::KeyPress::spaceKey, juce::ModifierKeys::altModifier);
            info.setActive (rollInFront && ! editor()->isPlaying());
            return;
        case cmdToggleLoop:
            info.setInfo ("Go round the loop", "Play goes round the loop region marked in the ruler", "Transport", 0);
            info.addDefaultKeypress ('l', juce::ModifierKeys::noModifiers);
            info.setTicked (editor() != nullptr && editor()->isLoopOn());
            info.setActive (anyInFront);
            return;
        case cmdToggleAudio:
            info.setInfo ("Audio on", "Open or close Maket's audio output. Off: no sound from Maket itself and no processor time for it; conTimbre plays on", "Transport", 0);
            info.addDefaultKeypress ('a', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            info.setTicked (isAudioOn());
            return;
        case cmdToStart:
            info.setInfo ("Playhead to the start", "Put the playhead at the start of the roll", "Transport", 0);
            info.addDefaultKeypress (juce::KeyPress::returnKey, juce::ModifierKeys::noModifiers);
            info.setActive (anyInFront);
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
    if (info.commandID == cmdToggleAudio)     { setAudioOn (! isAudioOn()); return true; }

    if (library != nullptr)
    {
        switch (info.commandID)
        {
            case cmdOpen:           showTab (Tabs::library); library->chooseRoll();             return true;
            case cmdNewProject:     showTab (Tabs::library); library->newProject();             return true;
            case cmdOpenProject:    showTab (Tabs::library); library->chooseProject();          return true;
            case cmdCloseProject:   library->closeProject();                                    return true;
            case cmdImportRolls:    showTab (Tabs::library); library->chooseRollsToImport();    return true;
            case cmdProjectPitch:   library->editStandardPitch();                               return true;
            case cmdCloseTab:       library->closeFrontTab();                                   return true;
            default: break;
        }
    }
    if (auto* sound = library != nullptr ? library->frontSound() : nullptr)
        switch (info.commandID)
        {
            case cmdZoomAcrossIn:   sound->zoomAcross (1);      return true;
            case cmdZoomAcrossOut:  sound->zoomAcross (-1);     return true;
            case cmdZoomToFit:      sound->zoomToFit();         return true;
            case cmdPlayStop:       sound->playOrStop();        commandManager.commandStatusChanged(); return true;
            case cmdToggleLoop:     sound->toggleLoop();        commandManager.commandStatusChanged(); return true;
            case cmdToStart:        sound->toStart();           return true;
            default: break;
        }
    if (auto* edition = editor())
        switch (info.commandID)
        {
            case cmdZoomAcrossIn:   edition->zoomAcross (1);    return true;
            case cmdZoomAcrossOut:  edition->zoomAcross (-1);   return true;
            case cmdZoomDownIn:     edition->zoomDown (1);      return true;
            case cmdZoomDownOut:    edition->zoomDown (-1);     return true;
            case cmdZoomToFit:      edition->zoomToFit();       return true;
            case cmdDrawingTimes:   edition->toggleDrawingTimes(); commandManager.commandStatusChanged(); return true;
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
        for (auto id : { cmdNewProject, cmdOpenProject, cmdCloseProject })
            m.addCommandItem (&commandManager, id);
        m.addSeparator();
        m.addCommandItem (&commandManager, cmdImportRolls);
        m.addCommandItem (&commandManager, cmdProjectPitch);
        m.addSeparator();
        m.addCommandItem (&commandManager, cmdOpen);
        m.addCommandItem (&commandManager, cmdCloseTab);
    }
    else if (menuIndex == 1)
    {
        for (auto id : { cmdZoomAcrossIn, cmdZoomAcrossOut, cmdZoomDownIn, cmdZoomDownOut, cmdZoomToFit })
            m.addCommandItem (&commandManager, id);
        m.addCommandItem (&commandManager, cmdDrawingTimes);
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
        m.addSeparator();
        m.addCommandItem (&commandManager, cmdToggleAudio);
    }
    return m;
}
