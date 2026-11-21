// MainComponent.cpp

#include "MainComponent.h"
#include "Model/AppInfo.h"

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
            auto page = std::make_unique<UtilitiesPage> (settings);
            utilities = page.get();
            utilities->setSettingsNotice (settingsRead.getErrorMessage());
            utilities->onSettingSelected = [this] (const maket::SettingInfo& info) { showSetting (info); };
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

    const int tab = (int) info.commandID - cmdShowTab;
    if (tab < 0 || tab >= Tabs::count) return false;
    showTab (tab);
    return true;
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "View" };
}

juce::PopupMenu MainComponent::getMenuForIndex (int menuIndex, const juce::String&)
{
    juce::PopupMenu m;
    if (menuIndex == 0)
    {
        for (int i = 0; i < Tabs::count; ++i)
            m.addCommandItem (&commandManager, cmdShowTab + i);
        m.addSeparator();
        m.addCommandItem (&commandManager, cmdToggleList);
        m.addCommandItem (&commandManager, cmdToggleInspector);
    }
    return m;
}
