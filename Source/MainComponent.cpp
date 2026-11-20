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
    addAndMakeVisible (tabBar);

    for (int i = 0; i < Tabs::count; ++i)
    {
        pages[(size_t) i] = std::make_unique<EmptyPage> (i);
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

    setWantsKeyboardFocus (true);
    setSize (1400, 860);
}

MainComponent::~MainComponent()
{
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
    commandManager.commandStatusChanged();        // the View menu ticks the tab in front
}

// --- Commands and the menu bar -----------------------------------------------

void MainComponent::getAllCommands (juce::Array<juce::CommandID>& c)
{
    for (int i = 0; i < Tabs::count; ++i)
        c.add (cmdShowTab + i);
}

void MainComponent::getCommandInfo (juce::CommandID id, juce::ApplicationCommandInfo& info)
{
    const int tab = (int) id - cmdShowTab;
    if (tab < 0 || tab >= Tabs::count) return;

    const juce::String name (Tabs::info (tab).name);
    info.setInfo (name, "Bring the tab " + name + " in front", "View", 0);
    info.setTicked (tab == tabBar.getCurrent());
    info.addDefaultKeypress ('1' + tab, juce::ModifierKeys::commandModifier);
}

bool MainComponent::perform (const InvocationInfo& info)
{
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
        for (int i = 0; i < Tabs::count; ++i)
            m.addCommandItem (&commandManager, cmdShowTab + i);
    return m;
}
