// ConTimbreTestPage.h — the "conTimbre test" page of the Utilities tab (stage 1e).
//
// Two things, for a roll file chosen here:
//
//   Play     Maket turns the notes of the chosen voices into commands for
//            the conTimbre ePlayer and sends them at their times, so that
//            the roll can be heard as from the Max patch.
//
//   Compare  The logging patch in Max notes every command ScAst sends to
//            the ePlayer while it plays the same roll; Maket holds its own
//            commands against that log, command by command.
//
// A test bench, like the OSC test and the timing test: it goes when the
// arranger (step 4) plays rolls itself.

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

#include "../Model/Settings.h"
#include "../Model/Roll.h"
#include "../Engine/OscScheduler.h"
#include "../Engine/ConTimbreMapping.h"

class ConTimbreTestPage : public juce::Component,
                          public juce::FileDragAndDropTarget,
                          private juce::Timer
{
public:
    ConTimbreTestPage (maket::Settings&, maket::OscScheduler&);
    ~ConTimbreTestPage() override;

    int getHeightFor (int width) const;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override    { return files.size() == 1; }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    enum class State { idle, playing, take, waitingForLog };

    void timerCallback() override;

    void chooseRoll();
    void loadRoll (const juce::File&);
    void play();
    void stop();
    void startTake();
    void finishTake();
    void chooseLog();
    void compareWith (const juce::File& logFile);

    std::vector<int> chosenVoices() const;
    maket::contimbre::MappingOptions options() const;   // the ways of the Max patch that are switched on
    juce::String optionsInWords() const;
    bool readyToSend();                     // a roll, a voice, the addresses from the preferences
    void setState (State);
    void setStatus (const juce::String& text, bool isProblem = false);
    void addToView (const juce::String& text);
    juce::File folder() const;
    void writePlayLog();                    // when every command of a run left Maket, against when it was due

    maket::Settings& settings;
    maket::OscScheduler& scheduler;

    juce::TextEditor rollField, fromField;
    juce::TextButton chooseButton, playButton, stopButton, takeButton, finishButton, logButton;
    std::vector<std::unique_ptr<juce::ToggleButton>> voiceToggles;
    std::vector<int> voiceOfToggle;         // which voice of the roll each toggle stands for
    juce::ToggleButton noteOnFirstToggle, glissandoToZeroToggle, spacingToggle;     // stage 1f.1: the ways of the Max patch
    juce::TextEditor view;
    std::unique_ptr<juce::FileChooser> chooser;

    maket::Roll roll;
    juce::File rollFile;
    bool hasRoll = false;

    State state = State::idle;
    double stateSince = 0.0;
    double playStart = 0.0, playFrom = 0.0, playEnd = 0.0;      // playStart on the send thread's clock; the others in the roll
    int numToSend = 0, numSent = 0, numRefused = 0;
    double latest = 0.0;                                        // how late the latest command left Maket, ms
    std::vector<maket::OscScheduler::Sent> played;              // the records of the run, for its send log
    juce::String playedHow;
    juce::File maxLogFile;
    juce::int64 lastLogSize = -1;

    juce::String status;
    bool statusIsProblem = false;

    juce::Rectangle<int> titleArea, subtitleArea, targetArea, rollLabelArea, voicesLabelArea, waysLabelArea, fromLabelArea, statusArea, viewHeadingArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConTimbreTestPage)
};
