// EditionPage.h — the page of the tab "Library and edition".
//
// Since stage 2a: a roll opened from a bach.roll text file is shown in the
// roll view, with a bar above it for the file's name and the zoom. The
// library on the left arrives at step 3, editing at step 4.

#pragma once

#include <JuceHeader.h>
#include <functional>
#include <memory>

#include "RollView.h"
#include "../Model/Settings.h"

class EditionPage : public juce::Component,
                    public juce::FileDragAndDropTarget
{
public:
    explicit EditionPage (maket::Settings&);
    ~EditionPage() override;

    /** Shows the file dialog and opens what is chosen. */
    void chooseFile();

    /** Opens a bach.roll text file; false, with the reason shown on the page, when it cannot be read. */
    bool openFile (const juce::File&);

    bool hasRoll() const                { return view.hasRoll(); }
    RollView& rollView()                { return view; }

    /** Zoom by a step: +1 in, -1 out. */
    void zoomAcross (int direction);
    void zoomDown (int direction);
    void zoomToFit()                    { view.zoomToFit(); }

    /** Called after a file was opened, with what to show about it. */
    std::function<void (const juce::File&, const maket::Roll&)> onRollOpened;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray&) override     { return true; }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void updateBar();

    maket::Settings& settings;
    RollView view;
    juce::TextButton openButton, acrossOut, acrossIn, downOut, downIn, fitButton;
    juce::String title, detail, problem;
    juce::Rectangle<int> barArea, titleArea, zoomTextArea;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File current;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditionPage)
};
