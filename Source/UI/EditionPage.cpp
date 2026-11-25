// EditionPage.cpp

#include "EditionPage.h"
#include "Theme.h"
#include "../Formats/BachRoll.h"

namespace
{
    constexpr int barHeight = 40;
    constexpr double zoomStep = 1.25;
}

EditionPage::EditionPage (maket::Settings& s) : settings (s)
{
    setOpaque (true);

    openButton.setButtonText (juce::String::fromUTF8 ("Open\xe2\x80\xa6"));
    openButton.setTooltip (juce::String::fromUTF8 ("Open a bach.roll text file  (\xe2\x8c\x98O)"));
    openButton.onClick = [this] { chooseFile(); };

    acrossOut.setButtonText (juce::String::fromUTF8 ("\xe2\x88\x92"));
    acrossIn.setButtonText ("+");
    downOut.setButtonText (juce::String::fromUTF8 ("\xe2\x88\x92"));
    downIn.setButtonText ("+");
    fitButton.setButtonText ("Fit");
    acrossOut.setTooltip (juce::String::fromUTF8 ("Less time detail  (\xe2\x8c\x98\xe2\x88\x92)"));
    acrossIn.setTooltip (juce::String::fromUTF8 ("More time detail  (\xe2\x8c\x98+)"));
    downOut.setTooltip (juce::String::fromUTF8 ("Smaller staves  (\xe2\x8c\xa5\xe2\x8c\x98\xe2\x88\x92)"));
    downIn.setTooltip (juce::String::fromUTF8 ("Larger staves  (\xe2\x8c\xa5\xe2\x8c\x98+)"));
    fitButton.setTooltip (juce::String::fromUTF8 ("The whole roll in the window  (\xe2\x8c\x98" "0)"));
    acrossOut.onClick = [this] { zoomAcross (-1); };
    acrossIn.onClick  = [this] { zoomAcross (1); };
    downOut.onClick   = [this] { zoomDown (-1); };
    downIn.onClick    = [this] { zoomDown (1); };
    fitButton.onClick = [this] { zoomToFit(); };

    for (auto* b : { &openButton, &acrossOut, &acrossIn, &downOut, &downIn, &fitButton })
        addAndMakeVisible (*b);

    gridBox.addItem ("Eighth tones", 8);
    gridBox.addItem ("Semitones", 2);
    gridBox.setSelectedId (8, juce::dontSendNotification);
    gridBox.setTooltip ("The grid notes are drawn on: each note gets the accidental of the nearest step");
    gridBox.onChange = [this] { view.setToneDivision (gridBox.getSelectedId()); };
    addAndMakeVisible (gridBox);

    view.onViewChanged = [this] { repaint (barArea); };
    addChildComponent (view);
    updateBar();
}

EditionPage::~EditionPage() = default;

void EditionPage::chooseFile()
{
    const auto start = current.existsAsFile() ? current.getParentDirectory() : settings.getFolder ("importsFolder");
    chooser = std::make_unique<juce::FileChooser> ("Open a bach.roll text file", start, "*.txt;*.llll");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (file != juce::File()) openFile (file);
                          });
}

bool EditionPage::openFile (const juce::File& file)
{
    maket::Roll roll;
    juce::StringArray remarks;
    const auto result = maket::readBachRollFile (file, roll, remarks);
    if (result.failed())
    {
        problem = file.getFileName() + ": " + result.getErrorMessage();
        repaint();
        return false;
    }

    problem.clear();
    current = file;
    title = file.getFileNameWithoutExtension();
    detail = juce::String ((int) roll.voices.size()) + " voices, " + juce::String (roll.numNotes()) + " notes, "
             + juce::String (juce::roundToInt (std::floor (roll.length() / 60000.0))) + "'"
             + juce::String (juce::roundToInt (std::fmod (roll.length() / 1000.0, 60.0))).paddedLeft ('0', 2) + "\"";
    view.setRoll (roll);
    view.setVisible (true);
    updateBar();
    resized();
    repaint();
    if (onRollOpened) onRollOpened (file, roll);
    return true;
}

void EditionPage::filesDropped (const juce::StringArray& files, int, int)
{
    if (! files.isEmpty()) openFile (juce::File (files[0]));
}

void EditionPage::zoomAcross (int direction)
{
    view.setHorizontalZoom (view.horizontalZoom() * (direction > 0 ? zoomStep : 1.0 / zoomStep));
}

void EditionPage::zoomDown (int direction)
{
    view.setVerticalZoom (view.verticalZoom() * (direction > 0 ? zoomStep : 1.0 / zoomStep));
}

void EditionPage::updateBar()
{
    const bool has = view.hasRoll();
    for (auto* b : { &acrossOut, &acrossIn, &downOut, &downIn, &fitButton })
        b->setEnabled (has);
    gridBox.setEnabled (has);
}

void EditionPage::resized()
{
    auto area = getLocalBounds();
    barArea = area.removeFromTop (barHeight);
    view.setBounds (area);

    auto bar = barArea.reduced (10, 7);
    openButton.setBounds (bar.removeFromLeft (76));
    bar.removeFromLeft (14);

    fitButton.setBounds (bar.removeFromRight (44));
    bar.removeFromRight (14);
    downIn.setBounds (bar.removeFromRight (28));
    bar.removeFromRight (4);
    downOut.setBounds (bar.removeFromRight (28));
    bar.removeFromRight (10);
    acrossIn.setBounds (bar.removeFromRight (28));
    bar.removeFromRight (4);
    acrossOut.setBounds (bar.removeFromRight (28));
    bar.removeFromRight (10);
    zoomTextArea = bar.removeFromRight (170);
    bar.removeFromRight (12);
    gridBox.setBounds (bar.removeFromRight (128));
    bar.removeFromRight (12);
    titleArea = bar;
}

void EditionPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    g.setColour (Theme::header);
    g.fillRect (barArea);
    g.setColour (Theme::line);
    g.fillRect (barArea.getX(), barArea.getBottom() - 1, barArea.getWidth(), 1);

    if (view.hasRoll())
    {
        g.setColour (Theme::text);
        g.setFont (Theme::font (14.0f, true));
        const int titleWidth = juce::jmin (titleArea.getWidth(), juce::roundToInt (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title)) + 14);
        g.drawText (title, titleArea.withWidth (titleWidth), juce::Justification::centredLeft, true);
        g.setColour (Theme::textMuted);
        g.setFont (Theme::font (12.5f));
        g.drawText (detail, titleArea.withTrimmedLeft (titleWidth), juce::Justification::centredLeft, true);
        g.drawText (view.describeView(), zoomTextArea, juce::Justification::centredRight, true);
        return;
    }

    // Nothing open yet.
    const int width = juce::jmin (560, getWidth() - 80);
    if (width < 120) return;
    auto column = getLocalBounds().withTrimmedTop (barHeight).withSizeKeepingCentre (width, 160);

    g.setColour (Theme::text);
    g.setFont (Theme::font (22.0f, true));
    g.drawText ("Library and edition", column.removeFromTop (32), juce::Justification::centredLeft, true);
    g.setColour (problem.isNotEmpty() ? Theme::warning : Theme::textMuted);
    g.setFont (Theme::font (14.0f));
    g.drawFittedText (problem.isNotEmpty() ? problem
                                           : juce::String::fromUTF8 ("Open a bach.roll text file: the Open\xe2\x80\xa6 button above, File > Open (\xe2\x8c\x98O), "
                                                                     "or drop the file on this window."),
                      column.removeFromTop (60), juce::Justification::topLeft, 3);
    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (12.5f));
    g.drawFittedText ("The roll is shown as bach.roll shows it. Stage 2a: staves, clefs, notes and duration lines; "
                      "accidentals, glides and markers follow in 2b and 2c. The library arrives at step 3.",
                      column, juce::Justification::topLeft, 4);
}
