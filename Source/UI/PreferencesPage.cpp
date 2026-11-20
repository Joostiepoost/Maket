// PreferencesPage.cpp

#include "PreferencesPage.h"
#include "Theme.h"

namespace
{
    constexpr int margin        = 32;       // around the page
    constexpr int maxWidth      = 820;      // of the column of rows
    constexpr int labelWidth    = 180;
    constexpr int narrowLabelWidth = 150;   // in a narrow window
    constexpr int rowHeight     = 58;       // field line + status line
    constexpr int fieldHeight   = 28;
    constexpr int chooseWidth   = 86;
    constexpr int defaultWidth  = 74;
    constexpr int gap           = 8;

    /** A text field that says when it gets the keyboard. */
    struct Field : public juce::TextEditor
    {
        std::function<void()> onFocus;

        void focusGained (FocusChangeType cause) override
        {
            juce::TextEditor::focusGained (cause);
            if (onFocus) onFocus();
        }
    };
}

// --- One setting ------------------------------------------------------------------

class PreferencesPage::Row : public juce::Component
{
public:
    Row (PreferencesPage& ownerPage, const maket::SettingInfo& setting)
        : owner (ownerPage), info (setting)
    {
        field.setFont (Theme::font (14.0f));
        field.setIndents (8, 6);
        field.setSelectAllWhenFocused (true);
        field.setTextToShowWhenEmpty (info.mayBeEmpty ? "not set" : "", Theme::textMuted);
        field.onReturnKey   = [this] { commit(); };
        field.onFocusLost   = [this] { commit(); };
        field.onEscapeKey   = [this] { problem.clear(); refresh(); };
        field.onTextChange  = [this] { if (problem.isNotEmpty()) { problem.clear(); repaint(); } };
        field.onFocus       = [this] { if (owner.onSettingSelected) owner.onSettingSelected (info); };
        addAndMakeVisible (field);

        if (info.kind == maket::SettingKind::folder)
        {
            chooseButton.setButtonText (juce::String::fromUTF8 ("Choose\xe2\x80\xa6"));
            chooseButton.onClick = [this] { chooseFolder(); };
            addAndMakeVisible (chooseButton);
        }

        defaultButton.setButtonText ("Default");
        defaultButton.setTooltip (juce::String ("Back to the default: ")
                                  + (juce::String (info.defaultValue).isEmpty() ? "not set" : info.defaultValue));
        defaultButton.onClick = [this]
        {
            owner.settings.reset (info.key);
            problem = owner.settings.save().getErrorMessage();
            refresh();
        };
        addAndMakeVisible (defaultButton);

        refresh();
    }

    /** Shows the value as it is kept. */
    void refresh()
    {
        field.setText (owner.settings.get (info.key), false);
        defaultButton.setEnabled (! owner.settings.isDefault (info.key));
        repaint();
        owner.repaint();        // the line about the settings file may have changed too
    }

    /** Takes over what stands in the field, if it is valid. */
    void commit()
    {
        if (field.getText() == owner.settings.get (info.key)) return;
        apply (field.getText());
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Theme::text);
        g.setFont (Theme::font (14.0f));
        const int label = labelWidthNow();
        g.drawText (info.label, 0, 0, label - gap, fieldHeight, juce::Justification::centredLeft, true);

        const auto status = problem.isNotEmpty() ? problem : statusText();
        g.setColour (problem.isNotEmpty() ? Theme::warning : Theme::textMuted);
        g.setFont (Theme::font (12.0f));
        g.drawText (status, label, fieldHeight + 4, getWidth() - label, 18,
                    juce::Justification::centredLeft, true);
    }

    void resized() override
    {
        auto line = getLocalBounds().removeFromTop (fieldHeight).withTrimmedLeft (labelWidthNow());
        defaultButton.setBounds (line.removeFromRight (defaultWidth));
        line.removeFromRight (gap);
        if (info.kind == maket::SettingKind::folder)
        {
            chooseButton.setBounds (line.removeFromRight (chooseWidth));
            line.removeFromRight (gap);
        }
        field.setBounds (line);
    }

private:
    /** The labels take less room when the window is narrow. */
    int labelWidthNow() const       { return getWidth() < 600 ? narrowLabelWidth : labelWidth; }

    void apply (const juce::String& value)
    {
        const auto result = owner.settings.set (info.key, value);
        problem = result.wasOk() ? owner.settings.save().getErrorMessage()
                                 : "Not changed. " + result.getErrorMessage();
        refresh();
    }

    /** The line under the field. For a folder: where it is on this Mac and
        whether it is there. For the rest: the first sentence of its help. */
    juce::String statusText() const
    {
        if (info.kind != maket::SettingKind::folder)
        {
            const juce::String help (info.help);
            return help.contains (". ") ? help.upToFirstOccurrenceOf (". ", false, false) + "." : help;
        }

        const auto folder = owner.settings.getFolder (info.key);
        if (folder == juce::File()) return "Not set.";

        const juce::String there (folder.isDirectory() ? "This folder exists." : "This folder does not exist yet.");
        if (folder.getFullPathName() == owner.settings.get (info.key)) return there;

        // Kept with ~: show the whole path on this Mac as well.
        return folder.getFullPathName() + juce::String::fromUTF8 ("  \xc2\xb7  ") + there;
    }

    void chooseFolder()
    {
        // Start at the folder itself, or at the nearest folder above it that exists.
        auto start = owner.settings.getFolder (info.key);
        if (start == juce::File()) start = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
        while (! start.isDirectory() && start != start.getParentDirectory())
            start = start.getParentDirectory();

        chooser = std::make_unique<juce::FileChooser> (juce::String ("Choose the folder: ") + info.label, start);
        juce::Component::SafePointer<Row> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [safe] (const juce::FileChooser& fc)
                              {
                                  if (safe == nullptr) return;
                                  const auto chosen = fc.getResult();
                                  if (chosen != juce::File()) safe->apply (chosen.getFullPathName());
                              });
    }

    PreferencesPage& owner;
    const maket::SettingInfo& info;
    Field field;
    juce::TextButton chooseButton, defaultButton;
    juce::String problem;                       // why the last change was refused, or empty
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Row)
};

// --- The page -----------------------------------------------------------------------

PreferencesPage::PreferencesPage (maket::Settings& settingsToEdit) : settings (settingsToEdit)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);       // a click beside the fields ends an edit

    for (const auto& s : maket::Settings::all())
    {
        groups.addIfNotAlreadyThere (s.group);
        addAndMakeVisible (rows.add (new Row (*this, s)));
    }

   #if JUCE_MAC
    showFileButton.setButtonText ("Show in Finder");
   #else
    showFileButton.setButtonText ("Show the file");
   #endif
    showFileButton.onClick = [this]
    {
        // The file exists only once something was saved; show its folder until then.
        const auto file = settings.getFile();
        if (file.existsAsFile())                          file.revealToUser();
        else if (file.getParentDirectory().isDirectory()) file.getParentDirectory().revealToUser();
        else                                              file.getParentDirectory().getParentDirectory().revealToUser();
    };
    addAndMakeVisible (showFileButton);
}

PreferencesPage::~PreferencesPage() = default;

void PreferencesPage::setNotice (const juce::String& text)
{
    notice = text;
    resized();
    repaint();
}

void PreferencesPage::commitPendingEdits()
{
    for (auto* row : rows) row->commit();
}

void PreferencesPage::mouseDown (const juce::MouseEvent&)
{
    grabKeyboardFocus();
}

PreferencesPage::Layout PreferencesPage::computeLayout (int width) const
{
    Layout l;
    const int w = juce::jmax (360, juce::jmin (maxWidth, width - 2 * margin));
    int y = margin;

    l.title    = { margin, y, w, 30 };     y += 32;
    l.subtitle = { margin, y, w, 20 };     y += 28;

    if (notice.isNotEmpty())
    {
        l.notice = { margin, y, w, 38 };   y += 46;
    }

    const auto& all = maket::Settings::all();
    for (const auto& group : groups)
    {
        y += 14;
        l.groupHeadings.add ({ margin, y, w, 22 });
        y += 38;

        for (size_t i = 0; i < all.size(); ++i)
            if (group == all[i].group)
            {
                l.rowBounds.set ((int) i, { margin, y, w, rowHeight });
                y += rowHeight + 6;
            }
    }

    y += 14;
    l.fileLabel  = { margin, y, w, 22 };                       y += 30;
    l.fileButton = { margin + w - 130, y, 130, fieldHeight };
    l.filePath   = { margin, y, w - 130 - gap, fieldHeight };  y += fieldHeight;

    l.height = y + margin;
    return l;
}

int PreferencesPage::getHeightFor (int width) const
{
    return computeLayout (width).height;
}

void PreferencesPage::resized()
{
    layout = computeLayout (getWidth());
    for (int i = 0; i < rows.size(); ++i)
        rows[i]->setBounds (layout.rowBounds[i]);
    showFileButton.setBounds (layout.fileButton);
}

void PreferencesPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    g.setColour (Theme::text);
    g.setFont (Theme::font (22.0f, true));
    g.drawText ("Preferences", layout.title, juce::Justification::centredLeft, true);

    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (13.0f));
    g.drawText ("What differs from one Mac to another. A change is saved at once.",
                layout.subtitle, juce::Justification::centredLeft, true);

    if (notice.isNotEmpty())
    {
        g.setColour (Theme::warning);
        g.setFont (Theme::font (13.0f));
        g.drawFittedText (notice, layout.notice, juce::Justification::topLeft, 2, 1.0f);
    }

    for (int i = 0; i < groups.size(); ++i)
    {
        const auto r = layout.groupHeadings[i];
        g.setColour (Theme::text);
        g.setFont (Theme::font (15.0f, true));
        g.drawText (groups[i], r, juce::Justification::centredLeft, true);
        g.setColour (Theme::line);
        g.fillRect (r.getX(), r.getBottom() + 2, r.getWidth(), 1);
    }

    g.setColour (Theme::text);
    g.setFont (Theme::font (15.0f, true));
    g.drawText ("Settings file", layout.fileLabel, juce::Justification::centredLeft, true);
    g.setColour (Theme::line);
    g.fillRect (layout.fileLabel.getX(), layout.fileLabel.getBottom() + 2, layout.fileLabel.getWidth(), 1);

    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (13.0f));
    g.drawText (settings.getFile().getFullPathName()
                    + (settings.getFile().existsAsFile() ? "" : "   (made at the first change)"),
                layout.filePath, juce::Justification::centredLeft, true);
}
