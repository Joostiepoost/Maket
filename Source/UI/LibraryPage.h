// LibraryPage.h — the page of the Library and edition tab (stage 3b).
//
// On the left the library of the open project: a list of its snippets, each
// with a miniature. In the rest: a row of tabs, one for every open snippet,
// and under it the editor of the one in front (an EditionPage each, with its
// own view, playhead and loop). When nothing is open: the start panel, with
// the projects opened last.
//
// Only one roll sounds at a time: a play in one tab stops the others first.
// A roll that plays goes on when another tab is brought to the front.

#pragma once

#include <JuceHeader.h>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "EditionPage.h"
#include "SoundPage.h"
#include "../Model/Project.h"
#include "../Model/Settings.h"
#include "../Model/WindowState.h"
#include "../Notation/Miniature.h"

class LibraryPage : public juce::Component,
                    public juce::FileDragAndDropTarget
{
public:
    LibraryPage (maket::Settings&, maket::WindowState&, maket::OscScheduler&, maket::Synth&, AudioOutput&, maket::SoundPlayer&);
    ~LibraryPage() override;

    // --- The project ---
    bool hasProject() const                 { return project.isOpen(); }
    const maket::Project& getProject() const { return project; }
    void newProject();                      // asks for a name and a sample rate
    void chooseProject();                   // the folder dialog
    bool openProject (const juce::File& folder);
    void closeProject();
    void chooseRollsToImport();
    void importFiles (const juce::Array<juce::File>&);
    void editStandardPitch();

    // --- A single roll outside any project ---
    void chooseRoll();
    bool openLooseRoll (const juce::File&);

    // --- The editor in front: of a roll, or of a sound (stage 3c); null when it is of the other kind or nothing is open ---
    EditionPage* front() const;
    SoundPage* frontSound() const;
    bool hasRoll() const                    { return front() != nullptr && front()->hasRoll(); }
    bool isPlaying() const                  { return front() != nullptr && front()->isPlaying(); }
    bool anythingPlays() const;
    void closeFrontTab();

    // --- The list on the left ---
    bool hasList() const                    { return project.isOpen(); }
    void setListWidth (int);
    int getListWidth() const                { return listWidth; }
    void setListVisible (bool);

    /** Set by the main window; handed to every editor. */
    std::function<juce::String()> turnAudioOn;
    void audioChanged (bool isOn, const juce::String& trouble);
    /** Something changed that the menus or the window's title show. */
    std::function<void()> onChanged;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override     { return true; }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    struct Open
    {
        juce::String id;                    // of the snippet; empty for a roll outside the project
        juce::File file;                    // of a roll outside the project
        std::unique_ptr<EditionPage> page;  // a roll's editor,
        std::unique_ptr<SoundPage> sound;   // or a sound's: one of the two

        juce::Component* component() const  { return page != nullptr ? static_cast<juce::Component*> (page.get()) : sound.get(); }
        bool isPlaying() const              { return page != nullptr ? page->isPlaying() : sound != nullptr && sound->isPlaying(); }
        void stopPlaying() const            { if (page != nullptr) page->stopPlaying(); else if (sound != nullptr) sound->stopPlaying(); }
    };

    class List;
    class Tabs;
    class Start;

    void openSnippet (const juce::String& id);
    Open& addEditor();
    void bringToFront (int index);
    void closeTab (int index);
    void stopAllBut (const juce::Component* keep);
    void refreshMiniature (const juce::String& id);
    void showMenuFor (const juce::String& id);
    void renameSnippet (const juce::String& id);
    void updateSnippet (const juce::String& id);
    void removeSnippet (const juce::String& id);
    void setNotice (const juce::String& text, bool isProblem = false);
    void changed();
    juce::String tabTitle (int index) const;
    juce::String soundFacts (const maket::SnippetInfo&) const;
    int indexOfSnippet (const juce::String& id) const;

    maket::Settings& settings;
    maket::WindowState& windowState;
    maket::OscScheduler& scheduler;
    maket::Synth& synth;
    AudioOutput& audio;
    maket::SoundPlayer& soundPlayer;

    maket::Project project;
    std::map<juce::String, std::vector<maket::notation::MiniLine>> miniatures;
    std::vector<Open> open;
    int frontIndex = -1;
    EditionPage* asking = nullptr;          // the editor that is asking for audio right now

    std::unique_ptr<List> list;
    std::unique_ptr<Tabs> tabs;
    std::unique_ptr<Start> start;
    int listWidth = 200;
    bool listVisible = true;
    juce::String notice;
    bool noticeIsProblem = false;
    std::unique_ptr<juce::FileChooser> chooser;

    static constexpr int tabsHeight = 30;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LibraryPage)
};
