// PlayerStudyPage.h — the "ePlayer study" page of the Utilities tab (stage 1f.2).
//
// Plays the figure of Engine/PlayerStudy many times, each way in turn, and
// writes the plan and a send log; a recording of the run is then compared
// outside Maket. A test bench, like the other test pages.

#pragma once

#include <JuceHeader.h>
#include <vector>

#include "../Model/Settings.h"
#include "../Engine/OscScheduler.h"
#include "../Engine/PlayerStudy.h"

class PlayerStudyPage : public juce::Component,
                        private juce::Timer
{
public:
    PlayerStudyPage (maket::Settings&, maket::OscScheduler&);
    ~PlayerStudyPage() override;

    int getHeightFor (int) const    { return 720; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    bool readOptions (maket::study::Options&);
    void start (int onlyWay, int run = 1);  // onlyWay -1: the whole run; run 2: the second study (stage 1f.3)
    void stop();
    void finish (bool stopped);
    void setRunning (bool);
    void setStatus (const juce::String&, bool isProblem = false);
    void showWays();

    maket::Settings& settings;
    maket::OscScheduler& scheduler;

    juce::TextEditor voiceField, programField, repetitionsField;
    juce::ToggleButton numberToggle, correctionToggle;
    juce::TextButton tryPitchButton, tryNumberButton, runButton, secondRunButton, stopButton;
    juce::TextEditor view;

    maket::study::Plan plan;
    bool running = false, wholeRun = false;
    double runStart = 0.0;
    int numToSend = 0, numRefused = 0;
    double latest = 0.0;
    std::vector<maket::OscScheduler::Sent> sent;
    juce::String stamp;

    juce::String status;
    bool statusIsProblem = false;

    juce::Rectangle<int> titleArea, subtitleArea, targetArea, voiceLabel, programLabel, repetitionsLabel, statusArea, viewHeadingArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayerStudyPage)
};
