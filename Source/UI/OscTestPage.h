// OscTestPage.h — the "OSC test" page of the Utilities tab.
//
// For the tests of step 1, the gate: it sends single notes and a short run
// of notes to the conTimbre ePlayer in Max, and shows for every message
// when it was due and when it left Maket. A copy of everything can go to
// Maket's logging patch in Max (Max/Maket OSC log.maxpat), which notes when
// each message arrives.
//
// The page is a test bench and goes when the instruments page (step 7) and
// the roll (step 2) have taken over its work.

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

#include "../Model/Settings.h"
#include "../Engine/OscScheduler.h"
#include "../Engine/ConTimbre.h"

class OscTestPage : public juce::Component,
                    private juce::Timer
{
public:
    OscTestPage (maket::Settings&, maket::OscScheduler&);
    ~OscTestPage() override;

    /** The height the page needs at a given width. */
    int getHeightFor (int width) const;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Entry { double due = 0.0, sent = 0.0; juce::String text; int targets = 0; bool ok = true; };

    void timerCallback() override;

    /** Reads the fields; false (with `problem` set) when one cannot be used. */
    bool readFields();
    int targets() const;
    void applyTargets();                        // host and ports from the settings, as they are now

    void playNote();
    void playRun();
    void allNotesOff();
    void clearLog();
    void saveLog();
    void appendToLog (const std::vector<maket::OscScheduler::Sent>&);
    juce::String logAsText() const;
    juce::String whereTo() const;

    maket::Settings& settings;
    maket::OscScheduler& scheduler;

    juce::TextEditor voiceField, programField, pitchField, dynamicField, durationField;
    juce::ToggleButton copyToggle;
    juce::TextButton noteButton, runButton, offButton, clearButton, saveButton;
    juce::TextEditor logView;
    std::unique_ptr<juce::FileChooser> chooser;

    // What the fields held when they were last read.
    juce::String voice, programName;
    double pitch = 60.0, duration = 1000.0;
    int dynamic = 100;

    juce::String problem, saved;
    std::vector<Entry> entries;
    double firstDue = 0.0;

    juce::Rectangle<int> titleArea, subtitleArea, targetArea, labelArea[5], problemArea, logHeadingArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscTestPage)
};
