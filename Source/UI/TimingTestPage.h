// TimingTestPage.h — the "Timing test" page of the Utilities tab (stage 1d).
//
// It answers one question of the gate: do Maket's messages arrive in Max
// evenly enough? A fixed passage is played, the logging patch in Max notes
// when each message arrives, and Maket holds those times against the times
// the messages were due. The same passage can be played by bach.roll inside
// Max, as the reference.
//
// One take is one click: Maket tells the patch to start a fresh log, plays
// the passage, tells the patch where to write its log, reads that file and
// adds a line to the table of results. Between takes Joost changes one
// thing in Max and says in the Condition field what it is.
//
// A test bench, like the OSC test page: it goes when the gate is passed.

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

#include "../Model/Settings.h"
#include "../Engine/OscScheduler.h"
#include "../Engine/TimingTest.h"

class TimingTestPage : public juce::Component,
                       private juce::Timer
{
public:
    TimingTestPage (maket::Settings&, maket::OscScheduler&);
    ~TimingTestPage() override;

    int getHeightFor (int width) const;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum class State { idle, running, beforeWrite, waitingForLog, bachTake };
    enum class Kind { none, maketRun, bachTake };

    void timerCallback() override;

    bool prepare (const juce::String& playedBy);    // fields, folder, names of this take's files
    void runFromMaket();
    void startBachTake();
    void finishBachTake();
    void stop();
    void writePassages (bool tellInStatus);
    void askMaxToWrite();
    void readLog();                                 // reads the take's log and shows the result
    void setState (State);
    void setStatus (const juce::String& text, bool isProblem = false);
    void addToView (const juce::String& text);

    maket::timing::Passage chosenPassage() const;
    juce::File folder() const;

    maket::Settings& settings;
    maket::OscScheduler& scheduler;

    juce::TextEditor conditionField, voiceField, dynamicField;
    juce::ComboBox passageBox;
    juce::ToggleButton playerToggle;
    juce::TextButton runButton, bachStartButton, bachFinishButton, stopButton, passagesButton, readButton;
    juce::TextEditor view;

    // The take that is under way, or the last one.
    State state = State::idle;
    Kind kind = Kind::none;
    double stateSince = 0.0;
    maket::timing::Passage passage;
    juce::String condition, takeName, when;
    juce::File maxLogFile, sendLogFile;
    std::vector<maket::OscScheduler::Sent> sent;
    int expectedRecords = 0;
    double startTime = 0.0;
    bool onPlayer = false;
    juce::int64 lastLogSize = -1;

    juce::String status;
    bool statusIsProblem = false;

    juce::Rectangle<int> titleArea, subtitleArea, targetArea, labelArea[4], statusArea, viewHeadingArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimingTestPage)
};
