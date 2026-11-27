// LibraryPage.cpp — see LibraryPage.h.

#include "LibraryPage.h"
#include "Theme.h"
#include "../Formats/BachRoll.h"
#include "../Model/AppInfo.h"

namespace
{
    juce::String lengthText (double ms)
    {
        const int seconds = juce::roundToInt (ms / 1000.0);
        return juce::String (seconds / 60) + "'" + juce::String (seconds % 60).paddedLeft ('0', 2) + "\"";
    }

    const juce::Colour miniPaper (0xfffaf9f5), miniInk (0xff26262b);

    void styleButton (juce::TextButton& b)
    {
        b.setWantsKeyboardFocus (false);
        b.setMouseClickGrabsKeyboardFocus (false);
    }
}

//==============================================================================
// The library's list.

class LibraryPage::List : public juce::Component,
                          private juce::ListBoxModel
{
public:
    explicit List (LibraryPage& o) : owner (o)
    {
        importButton.setButtonText (juce::String::fromUTF8 ("Import\xe2\x80\xa6"));
        importButton.setTooltip (juce::String::fromUTF8 ("Bring bach.roll text files into the library  (\xe2\x8c\x98I). Files can also be dropped here."));
        importButton.onClick = [this] { owner.chooseRollsToImport(); };
        styleButton (importButton);
        addAndMakeVisible (importButton);

        box.setModel (this);
        box.setColour (juce::ListBox::backgroundColourId, Theme::panel);
        box.setWantsKeyboardFocus (false);
        addAndMakeVisible (box);
    }

    void refresh()
    {
        box.updateContent();
        box.setVisible (! owner.project.getSnippets().empty());
        box.repaint();
        repaint();
    }

    void showSelected (const juce::String& id)
    {
        const auto& all = owner.project.getSnippets();
        int row = -1;
        for (size_t i = 0; i < all.size(); ++i) if (all[i].id == id) row = (int) i;
        if (row >= 0) box.selectRow (row, false, true); else box.deselectAllRows();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Theme::panel);
        g.setColour (Theme::line);
        g.fillRect (getWidth() - 1, 0, 1, getHeight());

        auto head = getLocalBounds().removeFromTop (headHeight).reduced (12, 0);
        g.setColour (Theme::text);
        g.setFont (Theme::font (14.0f, true));
        g.drawText (owner.project.getName(), head.removeFromTop (34).withTrimmedTop (10).withTrimmedRight (importButton.getWidth() + 4), juce::Justification::centredLeft, true);
        g.setColour (Theme::textMuted);
        g.setFont (Theme::font (11.5f));
        g.drawText (juce::String (juce::roundToInt (owner.project.getSampleRate() / 1000.0)) + juce::String::fromUTF8 (" kHz  \xc2\xb7  A ")
                    + juce::String (owner.project.getStandardPitch(), 1).trimCharactersAtEnd ("0").trimCharactersAtEnd (".") + " Hz"
                    + juce::String::fromUTF8 ("  \xc2\xb7  ") + juce::String ((int) owner.project.getSnippets().size()) + " snippets",
                    head.removeFromTop (18), juce::Justification::centredLeft, true);

        if (owner.project.getSnippets().empty())
        {
            g.setFont (Theme::font (12.5f));
            g.drawFittedText (juce::String::fromUTF8 ("The library is empty. Import\xe2\x80\xa6 brings bach.roll text files in; they can also be dropped here."),
                              getLocalBounds().withTrimmedTop (headHeight + 14).reduced (14, 0).removeFromTop (70), juce::Justification::topLeft, 4);
        }

        if (owner.notice.isNotEmpty())
        {
            g.setColour (owner.noticeIsProblem ? Theme::warning : Theme::textMuted);
            g.setFont (Theme::font (11.5f));
            g.drawFittedText (owner.notice, getLocalBounds().removeFromBottom (noticeHeight).reduced (12, 4), juce::Justification::centredLeft, 3);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds();
        auto head = area.removeFromTop (headHeight);
        importButton.setBounds (head.removeFromRight (84).withTrimmedRight (10).withSizeKeepingCentre (74, 26).translated (0, -8));
        area.removeFromRight (1);
        if (owner.notice.isNotEmpty()) area.removeFromBottom (noticeHeight);
        box.setBounds (area);
        // The rows follow the width: a miniature a third as high as it is wide, and the line with the name under it.
        box.setRowHeight (juce::jlimit (70, 300, juce::roundToInt ((float) (area.getWidth() - 20) * 0.34f)) + captionHeight + 10);
    }

    static constexpr int captionHeight = 20, headHeight = 62, noticeHeight = 52;

private:
    int getNumRows() override       { return (int) owner.project.getSnippets().size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        const auto& all = owner.project.getSnippets();
        if (row < 0 || row >= (int) all.size()) return;
        const auto& s = all[(size_t) row];
        const int openAt = owner.indexOfSnippet (s.id);

        if (selected || openAt >= 0)
        {
            g.setColour (selected ? Theme::accent.withAlpha (0.16f) : Theme::header.withAlpha (0.6f));
            g.fillRect (0, 0, width, height);
        }
        if (selected) { g.setColour (Theme::accent); g.fillRect (0, 0, 3, height); }

        // The miniature: the notes as lines, across by time, up and down by pitch. It takes the width of the list,
        // so that a wider list gives larger miniatures (stage 3b.1).
        const juce::Rectangle<float> mini (10.0f, 7.0f, (float) width - 20.0f, (float) (height - captionHeight - 10));
        g.setColour (miniPaper);
        g.fillRoundedRectangle (mini, 3.0f);
        const auto found = owner.miniatures.find (s.id);
        if (found != owner.miniatures.end())
        {
            const auto inner = mini.reduced (4.0f, 4.0f);
            const float thickness = inner.getHeight() > 90.0f ? 1.5f : 1.0f;
            for (const auto& l : found->second)
            {
                g.setColour (miniInk.withAlpha (0.3f + 0.65f * l.weight));
                const float x0 = inner.getX() + l.x0 * inner.getWidth(), x1 = juce::jmax (x0 + 1.0f, inner.getX() + l.x1 * inner.getWidth());
                g.drawLine (x0, inner.getY() + l.y0 * inner.getHeight(), x1, inner.getY() + l.y1 * inner.getHeight(), thickness);
            }
        }

        // Under it one line: the name, and as much of the counts as there is room for.
        auto caption = juce::Rectangle<int> (12, height - captionHeight - 3, width - 24, captionHeight);
        const auto nameFont = Theme::font (13.0f, openAt >= 0), factsFont = Theme::font (11.0f);
        const int nameWidth = juce::roundToInt (juce::GlyphArrangement::getStringWidth (nameFont, s.name)) + 10;
        const juce::String length = lengthText (s.length);
        const juce::String notes = juce::String (s.notes) + " notes, " + length;
        const juce::String all3 = juce::String (s.voices) + (s.voices == 1 ? " voice, " : " voices, ") + notes;
        juce::String facts;
        for (const auto& candidate : { all3, notes, length })
            if (facts.isEmpty() && nameWidth + juce::roundToInt (juce::GlyphArrangement::getStringWidth (factsFont, candidate)) <= caption.getWidth())
                facts = candidate;
        g.setColour (Theme::textMuted);
        g.setFont (factsFont);
        g.drawText (facts, caption, juce::Justification::centredRight, false);
        g.setColour (Theme::text);
        g.setFont (nameFont);
        g.drawText (s.name, facts.isEmpty() ? caption : caption.withTrimmedRight (juce::roundToInt (juce::GlyphArrangement::getStringWidth (factsFont, facts)) + 8),
                    juce::Justification::centredLeft, true);

        g.setColour (Theme::line.withAlpha (0.6f));
        g.fillRect (0, height - 1, width, 1);
    }

    void listBoxItemClicked (int row, const juce::MouseEvent& e) override
    {
        const auto& all = owner.project.getSnippets();
        if (row < 0 || row >= (int) all.size()) return;
        const auto id = all[(size_t) row].id;
        if (e.mods.isPopupMenu()) owner.showMenuFor (id);
        else                      owner.openSnippet (id);
    }

    LibraryPage& owner;
    juce::ListBox box;
    juce::TextButton importButton;
};

//==============================================================================
// The row of tabs, one for every open snippet.

class LibraryPage::Tabs : public juce::Component
{
public:
    explicit Tabs (LibraryPage& o) : owner (o) {}

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Theme::panel);
        g.setColour (Theme::line);
        g.fillRect (0, getHeight() - 1, getWidth(), 1);

        for (int i = 0; i < (int) owner.open.size(); ++i)
        {
            const auto r = tabBounds (i);
            const bool isFront = i == owner.frontIndex;
            const bool plays = owner.open[(size_t) i].page->isPlaying();
            if (isFront)
            {
                g.setColour (Theme::header);
                g.fillRect (r);
                g.setColour (Theme::accent);
                g.fillRect (r.getX(), r.getBottom() - 2, r.getWidth(), 2);
            }
            g.setColour (Theme::line);
            g.fillRect (r.getRight() - 1, r.getY() + 6, 1, r.getHeight() - 12);

            auto text = r.reduced (10, 0).withTrimmedRight (16);
            if (plays)
            {
                // A small triangle: this one sounds.
                juce::Path triangle;
                const float cx = (float) text.getX() + 4.0f, cy = (float) r.getCentreY();
                triangle.addTriangle (cx - 3.5f, cy - 5.0f, cx - 3.5f, cy + 5.0f, cx + 5.0f, cy);
                g.setColour (juce::Colour (0xff57de33));
                g.fillPath (triangle);
                text.removeFromLeft (14);
            }
            g.setColour (isFront ? Theme::text : Theme::textMuted);
            g.setFont (Theme::font (12.5f, isFront));
            g.drawText (owner.tabTitle (i), text, juce::Justification::centredLeft, true);

            // The cross that closes the tab.
            const auto cross = crossBounds (i).toFloat().reduced (5.0f);
            g.setColour (Theme::textMuted.withAlpha (i == hoverCross ? 1.0f : 0.6f));
            g.drawLine (cross.getX(), cross.getY(), cross.getRight(), cross.getBottom(), 1.3f);
            g.drawLine (cross.getRight(), cross.getY(), cross.getX(), cross.getBottom(), 1.3f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (int i = 0; i < (int) owner.open.size(); ++i)
        {
            if (crossBounds (i).contains (e.getPosition())) { owner.closeTab (i); return; }
            if (tabBounds (i).contains (e.getPosition()))   { owner.bringToFront (i); return; }
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        int over = -1;
        for (int i = 0; i < (int) owner.open.size(); ++i)
            if (crossBounds (i).contains (e.getPosition())) over = i;
        if (over != hoverCross) { hoverCross = over; repaint(); }
    }

    void mouseExit (const juce::MouseEvent&) override       { if (hoverCross != -1) { hoverCross = -1; repaint(); } }

private:
    juce::Rectangle<int> tabBounds (int index) const
    {
        const int count = juce::jmax (1, (int) owner.open.size());
        const int width = juce::jlimit (90, 220, getWidth() / count);
        return { index * width, 0, width, getHeight() - 1 };
    }

    juce::Rectangle<int> crossBounds (int index) const
    {
        const auto r = tabBounds (index);
        return { r.getRight() - 24, r.getCentreY() - 9, 18, 18 };
    }

    LibraryPage& owner;
    int hoverCross = -1;
};

//==============================================================================
// The start panel: shown when no snippet is open.

class LibraryPage::Start : public juce::Component
{
public:
    explicit Start (LibraryPage& o) : owner (o)
    {
        newButton.setButtonText (juce::String::fromUTF8 ("New project\xe2\x80\xa6"));
        openButton.setButtonText (juce::String::fromUTF8 ("Open project\xe2\x80\xa6"));
        rollButton.setButtonText (juce::String::fromUTF8 ("Open a bach roll\xe2\x80\xa6"));
        importButton.setButtonText (juce::String::fromUTF8 ("Import bach rolls\xe2\x80\xa6"));
        newButton.onClick    = [this] { owner.newProject(); };
        openButton.onClick   = [this] { owner.chooseProject(); };
        rollButton.onClick   = [this] { owner.chooseRoll(); };
        importButton.onClick = [this] { owner.chooseRollsToImport(); };
        for (auto* b : { &newButton, &openButton, &rollButton, &importButton }) { styleButton (*b); addAndMakeVisible (*b); }
        refresh();
    }

    void refresh()
    {
        recent.clear();
        if (! owner.project.isOpen())
            for (const auto& path : owner.windowState.recentProjects())
            {
                auto* b = recent.add (new juce::TextButton());
                const juce::File folder (path);
                const bool there = maket::Project::isProject (folder);
                b->setButtonText (folder.getFileName() + (there ? juce::String() : juce::String ("  (not found)")));
                b->setTooltip (path);
                b->setEnabled (there);
                // Opening the project rebuilds this very list of buttons. That must not happen while the click on one
                // of them is still being handled (it crashed on the Mac at 3b), so the opening waits for the click to end.
                b->onClick = [safe = juce::Component::SafePointer<LibraryPage> (&owner), folder]
                {
                    juce::MessageManager::callAsync ([safe, folder] { if (safe != nullptr) safe->openProject (folder); });
                };
                styleButton (*b);
                addAndMakeVisible (b);
            }
        const bool inProject = owner.project.isOpen();
        newButton.setVisible (! inProject);
        openButton.setVisible (! inProject);
        importButton.setVisible (inProject);
        resized();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Theme::background);
        auto column = columnArea();
        if (column.getWidth() < 160) return;

        g.setColour (Theme::text);
        g.setFont (Theme::font (26.0f, true));
        g.drawText (owner.project.isOpen() ? owner.project.getName() : juce::String ("Maket"), column.removeFromTop (38), juce::Justification::centredLeft, true);
        g.setColour (Theme::textMuted);
        g.setFont (Theme::font (13.5f));
        g.drawFittedText (owner.project.isOpen()
                              ? juce::String ("Click a snippet in the library to open it; several can be open at once, each in a tab. "
                                              "Import brings bach.roll text files into the library; the files themselves are not touched.")
                              : juce::String ("A project is a folder that holds a library of snippets. Make a new one, open one, "
                                              "or open a single bach.roll text file for a look outside any project."),
                          column.removeFromTop (44), juce::Justification::topLeft, 3);

        if (! owner.project.isOpen() && ! recent.isEmpty())
        {
            g.setColour (Theme::text);
            g.setFont (Theme::font (13.5f, true));
            g.drawText ("Projects opened last", recentHeading, juce::Justification::centredLeft, true);
        }
        if (owner.notice.isNotEmpty() && ! owner.project.isOpen())
        {
            g.setColour (owner.noticeIsProblem ? Theme::warning : Theme::textMuted);
            g.setFont (Theme::font (12.5f));
            g.drawFittedText (owner.notice, noticeArea, juce::Justification::topLeft, 3);
        }
    }

    void resized() override
    {
        auto column = columnArea();
        column.removeFromTop (38 + 44 + 12);
        auto row = column.removeFromTop (30);
        for (auto* b : { &newButton, &openButton, &importButton, &rollButton })
            if (b->isVisible()) { b->setBounds (row.removeFromLeft (b == &rollButton ? 150 : 140)); row.removeFromLeft (8); }
        column.removeFromTop (10);
        noticeArea = column.removeFromTop (owner.notice.isNotEmpty() ? 44 : 0);
        column.removeFromTop (14);
        recentHeading = column.removeFromTop (24);
        column.removeFromTop (4);
        for (auto* b : recent) { b->setBounds (column.removeFromTop (28).withWidth (juce::jmin (360, column.getWidth()))); column.removeFromTop (4); }
    }

private:
    juce::Rectangle<int> columnArea() const
    {
        const int width = juce::jmin (620, getWidth() - 80);
        return juce::Rectangle<int> (juce::jmax (40, (getWidth() - width) / 2), juce::jmax (30, getHeight() / 7), juce::jmax (0, width), juce::jmax (0, getHeight() - 60));
    }

    LibraryPage& owner;
    juce::TextButton newButton, openButton, rollButton, importButton;
    juce::OwnedArray<juce::TextButton> recent;
    juce::Rectangle<int> recentHeading, noticeArea;
};

//==============================================================================

LibraryPage::LibraryPage (maket::Settings& s, maket::WindowState& w, maket::OscScheduler& o, maket::Synth& sy, AudioOutput& a)
    : settings (s), windowState (w), scheduler (o), synth (sy), audio (a)
{
    list = std::make_unique<List> (*this);
    tabs = std::make_unique<Tabs> (*this);
    start = std::make_unique<Start> (*this);
    addChildComponent (*list);
    addChildComponent (*tabs);
    addAndMakeVisible (*start);
}

LibraryPage::~LibraryPage() = default;

EditionPage* LibraryPage::front() const
{
    return frontIndex >= 0 && frontIndex < (int) open.size() ? open[(size_t) frontIndex].page.get() : nullptr;
}

bool LibraryPage::anythingPlays() const
{
    for (const auto& o : open) if (o.page->isPlaying()) return true;
    return false;
}

int LibraryPage::indexOfSnippet (const juce::String& id) const
{
    for (size_t i = 0; i < open.size(); ++i) if (open[i].id == id && id.isNotEmpty()) return (int) i;
    return -1;
}

juce::String LibraryPage::tabTitle (int index) const
{
    const auto& o = open[(size_t) index];
    if (o.id.isNotEmpty())
        if (const auto* s = project.find (o.id)) return s->name;
    return o.page->getTitle() + (project.isOpen() ? juce::String::fromUTF8 ("  \xc2\xb7  not in the library") : juce::String());
}

void LibraryPage::changed()
{
    if (list != nullptr)
    {
        list->setVisible (project.isOpen() && listVisible);
        list->refresh();
        const auto* f = frontIndex >= 0 ? &open[(size_t) frontIndex] : nullptr;
        list->showSelected (f != nullptr ? f->id : juce::String());
    }
    tabs->setVisible (! open.empty());
    tabs->repaint();
    start->setVisible (open.empty());
    start->refresh();
    resized();
    repaint();
    if (onChanged) onChanged();
}

void LibraryPage::setNotice (const juce::String& text, bool isProblem)
{
    notice = text;
    noticeIsProblem = isProblem;
    if (list != nullptr) { list->resized(); list->repaint(); }
    start->resized();
    start->repaint();
}

//==============================================================================
// The project.

void LibraryPage::newProject()
{
    auto* w = new juce::AlertWindow ("New project", "A project is a folder. It is made in the projects folder of the preferences:\n"
                                                    + settings.getFolder ("projectsFolder").getFullPathName(), juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", "", "Name");
    w->addComboBox ("rate", { "48 kHz", "96 kHz" }, "Sample rate");
    w->addButton ("Make", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int result)
    {
        if (result != 1) return;
        const auto name = juce::File::createLegalFileName (w->getTextEditorContents ("name").trim());
        const double rate = w->getComboBoxComponent ("rate")->getSelectedItemIndex() == 1 ? 96000.0 : 48000.0;
        if (name.isEmpty()) { setNotice ("A project needs a name.", true); return; }

        maket::Project made;
        const auto folder = settings.getFolder ("projectsFolder").getChildFile (name);
        const auto result2 = maket::Project::create (folder, rate, settings.getDouble ("standardPitch"), made);
        if (result2.failed()) { setNotice (result2.getErrorMessage(), true); return; }
        openProject (folder);
    }), true);
    if (auto* field = w->getTextEditor ("name")) field->grabKeyboardFocus();
}

void LibraryPage::chooseProject()
{
    chooser = std::make_unique<juce::FileChooser> ("Open a project: choose its folder", settings.getFolder ("projectsFolder"));
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                          [this] (const juce::FileChooser& fc)
                          {
                              if (fc.getResult() != juce::File()) openProject (fc.getResult());
                          });
}

bool LibraryPage::openProject (const juce::File& folder)
{
    maket::Project opened;
    juce::StringArray remarks;
    const auto result = maket::Project::open (folder, opened, remarks);
    if (result.failed())
    {
        windowState.removeRecentProject (folder.getFullPathName());
        windowState.save();
        setNotice (result.getErrorMessage(), true);
        changed();
        return false;
    }

    closeProject();
    project = std::move (opened);
    miniatures.clear();
    for (const auto& s : project.getSnippets()) refreshMiniature (s.id);
    windowState.addRecentProject (folder.getFullPathName());
    windowState.save();
    setNotice (remarks.joinIntoString (" "), ! remarks.isEmpty());
    changed();
    return true;
}

void LibraryPage::closeProject()
{
    // The tabs of the project's snippets close with it; a roll from outside stays.
    for (int i = (int) open.size(); --i >= 0;)
        if (open[(size_t) i].id.isNotEmpty()) closeTab (i);
    project = maket::Project();
    miniatures.clear();
    notice.clear();
    changed();
}

void LibraryPage::refreshMiniature (const juce::String& id)
{
    maket::Roll roll;
    juce::StringArray remarks;
    if (project.loadRoll (id, roll, remarks).wasOk()) miniatures[id] = maket::notation::miniatureOf (roll);
    else                                              miniatures.erase (id);
}

void LibraryPage::chooseRollsToImport()
{
    if (! project.isOpen()) { setNotice ("Open or make a project first: a library belongs to a project.", true); return; }
    chooser = std::make_unique<juce::FileChooser> ("Import bach.roll text files into the library", settings.getFolder ("importsFolder"), "*.txt;*.llll");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems,
                          [this] (const juce::FileChooser& fc) { importFiles (fc.getResults()); });
}

void LibraryPage::importFiles (const juce::Array<juce::File>& files)
{
    if (files.isEmpty() || ! project.isOpen()) return;
    int done = 0;
    juce::StringArray refused;
    juce::String last;
    for (const auto& file : files)
    {
        juce::String id;
        juce::StringArray remarks;
        const auto result = project.importRoll (file, id, remarks);
        if (result.failed()) { refused.add (file.getFileName()); continue; }
        refreshMiniature (id);
        last = id;
        ++done;
    }
    setNotice (juce::String (done) + (done == 1 ? " roll imported." : " rolls imported.")
               + (refused.isEmpty() ? juce::String() : " Not a bach.roll text: " + refused.joinIntoString (", ") + "."), ! refused.isEmpty());
    changed();
    if (done == 1) openSnippet (last);
}

void LibraryPage::editStandardPitch()
{
    if (! project.isOpen()) return;
    auto* w = new juce::AlertWindow ("The project's standard pitch", "The frequency of the A above middle C, in Hz, for everything in this project.", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("pitch", juce::String (project.getStandardPitch()), "Standard pitch (Hz)");
    w->addButton ("Set", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int result)
    {
        if (result != 1) return;
        const auto typed = w->getTextEditorContents ("pitch").trim().replaceCharacter (',', '.');
        const auto set = typed.containsOnly ("0123456789.") ? project.setStandardPitch (typed.getDoubleValue()) : juce::Result::fail ("That is not a number.");
        setNotice (set.wasOk() ? juce::String ("The standard pitch is ") + typed + " Hz; it holds from the next play." : set.getErrorMessage(), set.failed());
        changed();
    }), true);
    if (auto* field = w->getTextEditor ("pitch")) field->grabKeyboardFocus();
}

//==============================================================================
// Editors and their tabs.

LibraryPage::Open& LibraryPage::addEditor()
{
    Open o;
    o.page = std::make_unique<EditionPage> (settings, scheduler, synth, audio);
    auto* page = o.page.get();
    page->turnAudioOn = [this, page]
    {
        asking = page;                      // what goes wrong is told on the page that asked, not on every open one
        const auto trouble = turnAudioOn ? turnAudioOn() : juce::String();
        asking = nullptr;
        return trouble;
    };
    page->onWillPlay = [this, page] { stopAllBut (page); };
    page->onPlayStateChanged = [this] { tabs->repaint(); if (onChanged) onChanged(); };
    addChildComponent (*page);
    open.push_back (std::move (o));
    return open.back();
}

void LibraryPage::openSnippet (const juce::String& id)
{
    const int already = indexOfSnippet (id);
    if (already >= 0) { bringToFront (already); return; }
    const auto* info = project.find (id);
    if (info == nullptr) return;

    maket::Roll roll;
    juce::StringArray remarks;
    const auto result = project.loadRoll (id, roll, remarks);
    if (result.failed()) { setNotice ("\"" + info->name + "\": " + result.getErrorMessage(), true); return; }

    auto& o = addEditor();
    o.id = id;
    o.page->standardPitchNow = [this] { return project.isOpen() ? project.getStandardPitch() : settings.getDouble ("standardPitch"); };
    o.page->showRoll (roll, info->name);
    bringToFront ((int) open.size() - 1);
}

void LibraryPage::chooseRoll()
{
    chooser = std::make_unique<juce::FileChooser> ("Open a bach.roll text file", settings.getFolder ("importsFolder"), "*.txt;*.llll");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              if (fc.getResult() != juce::File()) openLooseRoll (fc.getResult());
                          });
}

bool LibraryPage::openLooseRoll (const juce::File& file)
{
    for (size_t i = 0; i < open.size(); ++i)
        if (open[i].id.isEmpty() && open[i].file == file) { bringToFront ((int) i); return true; }

    auto& o = addEditor();
    o.file = file;
    if (! o.page->openFile (file))
    {
        removeChildComponent (o.page.get());
        open.pop_back();
        setNotice (file.getFileName() + " is not a bach.roll text file.", true);
        changed();
        return false;
    }
    bringToFront ((int) open.size() - 1);
    return true;
}

void LibraryPage::bringToFront (int index)
{
    if (index < 0 || index >= (int) open.size()) return;
    frontIndex = index;
    for (size_t i = 0; i < open.size(); ++i) open[i].page->setVisible ((int) i == index);
    changed();
}

void LibraryPage::closeFrontTab()       { closeTab (frontIndex); }

void LibraryPage::closeTab (int index)
{
    if (index < 0 || index >= (int) open.size()) return;
    open[(size_t) index].page->stopPlaying();
    removeChildComponent (open[(size_t) index].page.get());
    open.erase (open.begin() + index);
    if (open.empty())               frontIndex = -1;
    else if (index < frontIndex)    --frontIndex;
    else if (index == frontIndex)   frontIndex = juce::jmin (index, (int) open.size() - 1);
    for (size_t i = 0; i < open.size(); ++i) open[i].page->setVisible ((int) i == frontIndex);
    changed();
}

void LibraryPage::stopAllBut (const EditionPage* keep)
{
    for (auto& o : open)
        if (o.page.get() != keep && o.page->isPlaying()) o.page->stopPlaying();
}

void LibraryPage::audioChanged (bool isOn, const juce::String& trouble)
{
    for (auto& o : open)
        o.page->audioChanged (isOn, asking == nullptr || asking == o.page.get() ? trouble : juce::String());
}

//==============================================================================
// What can be done with a snippet of the library.

void LibraryPage::showMenuFor (const juce::String& id)
{
    const auto* info = project.find (id);
    if (info == nullptr) return;
    juce::PopupMenu m;
    m.addItem (1, "Open");
    m.addItem (2, juce::String::fromUTF8 ("Rename\xe2\x80\xa6"));
    m.addSeparator();
    m.addItem (3, "Update from its file", info->sourcePath.isNotEmpty());
    m.addItem (4, "Show its file in Finder", juce::File (info->sourcePath).existsAsFile());
    m.addItem (5, "Show the snippet's own files in Finder");
    m.addSeparator();
    m.addItem (6, "Remove from the library");
    m.showMenuAsync (juce::PopupMenu::Options(), [this, id] (int chosen)
    {
        const auto* s = project.find (id);
        if (s == nullptr) return;
        if (chosen == 1) openSnippet (id);
        if (chosen == 2) renameSnippet (id);
        if (chosen == 3) updateSnippet (id);
        if (chosen == 4) juce::File (s->sourcePath).revealToUser();
        if (chosen == 5) project.rollFile (id).revealToUser();
        if (chosen == 6) removeSnippet (id);
    });
}

void LibraryPage::renameSnippet (const juce::String& id)
{
    const auto* info = project.find (id);
    if (info == nullptr) return;
    auto* w = new juce::AlertWindow ("Rename the snippet", "", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", info->name, "Name");
    w->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w, id] (int result)
    {
        if (result != 1) return;
        const auto renamed = project.rename (id, w->getTextEditorContents ("name"));
        setNotice (renamed.wasOk() ? juce::String() : renamed.getErrorMessage(), renamed.failed());
        changed();
    }), true);
    if (auto* field = w->getTextEditor ("name")) field->grabKeyboardFocus();
}

void LibraryPage::updateSnippet (const juce::String& id)
{
    juce::StringArray remarks;
    const auto updated = project.updateFromFile (id, remarks);
    const auto* info = project.find (id);
    if (updated.failed() || info == nullptr) { setNotice (updated.getErrorMessage(), true); return; }

    refreshMiniature (id);
    const int at = indexOfSnippet (id);
    if (at >= 0)
    {
        // The open tab takes the new notes (a play of it is stopped).
        maket::Roll roll;
        if (project.loadRoll (id, roll, remarks).wasOk()) open[(size_t) at].page->showRoll (roll, info->name);
    }
    setNotice ("\"" + info->name + "\" now holds what its file holds: " + juce::String (info->notes) + " notes.");
    changed();
}

void LibraryPage::removeSnippet (const juce::String& id)
{
    const auto* info = project.find (id);
    if (info == nullptr) return;
    const auto name = info->name;
    const int at = indexOfSnippet (id);
    if (at >= 0) closeTab (at);
    const auto removed = project.remove (id);
    miniatures.erase (id);
    setNotice (removed.wasOk() ? "\"" + name + "\" is out of the library; its files are in the project's Trash folder." : removed.getErrorMessage(), removed.failed());
    changed();
}

//==============================================================================

void LibraryPage::setListWidth (int width)
{
    if (width == listWidth) return;
    listWidth = width;
    resized();
}

void LibraryPage::setListVisible (bool shouldShow)
{
    listVisible = shouldShow;
    changed();
}

void LibraryPage::filesDropped (const juce::StringArray& files, int, int)
{
    if (files.isEmpty()) return;
    const juce::File first (files[0]);
    if (first.isDirectory())        { openProject (first); return; }
    if (project.isOpen())
    {
        juce::Array<juce::File> all;
        for (const auto& f : files) all.add (juce::File (f));
        importFiles (all);
    }
    else openLooseRoll (first);
}

void LibraryPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);
}

void LibraryPage::resized()
{
    auto area = getLocalBounds();
    if (list->isVisible()) list->setBounds (area.removeFromLeft (juce::jmin (listWidth, juce::jmax (120, getWidth() / 2))));
    if (open.empty())
    {
        start->setBounds (area);
        return;
    }
    tabs->setBounds (area.removeFromTop (tabsHeight));
    for (auto& o : open) o.page->setBounds (area);
}
