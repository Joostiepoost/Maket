// EditionPage.h — the page of the tab "Library and edition".
//
// Since stage 2a: a roll opened from a bach.roll text file is shown in the
// roll view, with a bar above it for the file's name and the zoom. Since 2d
// a second row holds the transport: the roll plays on the conTimbre ePlayer.
// The library on the left arrives at step 3, editing at step 4.

#pragma once

#include <JuceHeader.h>
#include <functional>
#include <memory>

#include "RollView.h"
#include "../Audio/AudioOutput.h"
#include "../Engine/Transport.h"
#include "../Model/Settings.h"

class EditionPage : public juce::Component,
                    public juce::FileDragAndDropTarget,
                    private juce::Timer
{
public:
    EditionPage (maket::Settings&, maket::OscScheduler&, maket::Synth&, AudioOutput&);
    ~EditionPage() override;

    /** Shows the file dialog and opens what is chosen. */
    void chooseFile();

    /** Opens a bach.roll text file; false, with the reason shown on the page, when it cannot be read. */
    bool openFile (const juce::File&);

    /** Stage 3b: shows a roll that comes from the library and not from a file. */
    void showRoll (const maket::Roll&, const juce::String& name);
    /** The file this page shows, when it was opened from one. */
    const juce::File& getFile() const   { return current; }
    const juce::String& getTitle() const { return title; }

    /** Stage 3b. The standard pitch to play at: the project's, or the preferences' when this is not set. */
    std::function<double()> standardPitchNow;
    /** Called just before this page begins to play: only one roll sounds at a time, the others are stopped first. */
    std::function<void()> onWillPlay;
    /** Called whenever play begins or ends here. */
    std::function<void()> onPlayStateChanged;

    bool hasRoll() const                { return view.hasRoll(); }
    RollView& rollView()                { return view; }

    /** Zoom by a step: +1 in, -1 out. */
    void zoomAcross (int direction);
    void zoomDown (int direction);
    void zoomToFit()                    { view.zoomToFit(); }
    /** Stage 3a. Set by the main window: turns Maket's audio on and returns what went wrong, or nothing. */
    std::function<juce::String()> turnAudioOn;
    /** The main window tells that audio went on or off (by the switch, or by a play). */
    void audioChanged (bool isOn, const juce::String& trouble);

    void toggleDrawingTimes()           { view.setShowTimes (! view.showsTimes()); }
    bool showsDrawingTimes() const      { return view.showsTimes(); }

    // --- Transport (stage 2d): the roll on the conTimbre ePlayer ---

    /** Plays from the playhead with the playhead following the sound, or stops when it plays. */
    void playOrStop();
    /** Plays from the playhead "prescheduled": nothing on screen follows the sound. */
    void playPrescheduled();
    /** Plays on from where the last play was stopped. */
    void playOn();
    void stopPlaying();
    /** Switches going round the loop on or off; it takes effect at the next play. */
    void toggleLoop();
    /** The playhead to the start of the roll. */
    void toStart();
    bool isPlaying() const              { return transport.isPlaying(); }
    bool isLoopOn() const               { return transport.isLoopOn(); }

    /** Called after a file was opened, with what to show about it. */
    std::function<void (const juce::File&, const maket::Roll&)> onRollOpened;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray&) override     { return true; }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void updateBar();
    void take (const maket::Roll&);
    double standardPitch() const;
    bool wasPlaying = false;
    void startPlaying (double from, bool follow);
    void timerCallback() override;
    void setNotice (const juce::String& text, bool isProblem = false);
    juce::String transportText() const;

    void showEngines();
    bool anyVoiceOnSynth() const;
    void openAudioIfNeeded();

    maket::Settings& settings;
    maket::OscScheduler& scheduler;
    maket::Synth& synth;
    AudioOutput& audio;
    maket::Transport transport;
    bool following = true;              // whether the playhead follows the sound of the play under way
    double cursor = 0.0;                // where play begins, ms
    juce::TextButton playButton, prescheduledButton, stopButton, loopButton;
    juce::String notice;
    bool noticeIsProblem = false;
    juce::uint32 noticeUntil = 0;
    juce::Rectangle<int> transportArea, transportTextArea;
    RollView view;
    juce::TextButton openButton, acrossOut, acrossIn, downOut, downIn, fitButton;
    juce::ComboBox gridBox;             // stage 2b: the grid notes are drawn on (bach's tone division)
    juce::ComboBox velocityBox;         // stage 2b.1: how a note shows its velocity
    juce::String title, detail, problem;
    juce::Rectangle<int> barArea, titleArea, zoomTextArea;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File current;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditionPage)
};
