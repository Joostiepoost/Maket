// SoundPage.h — the editor of an audio snippet in a tab of the library (stage 3c).
//
// For now a plain view: the waveform of each channel over a time ruler, with
// zoom across and scrolling, a playhead and a loop region, and play, stop and
// loop through Maket's audio output. A click in the ruler or the waveform
// sets the playhead; an ⌥-drag in the ruler sets the loop, as in the roll.
// What part of a sound a block uses, its gain and its fades belong to the
// block on the build surface (3d), not to this view.

#pragma once

#include <JuceHeader.h>
#include <functional>
#include <memory>

#include "../Sound/Sound.h"
#include "../Sound/SoundPlayer.h"

class SoundPage : public juce::Component,
                  private juce::Timer,
                  private juce::ScrollBar::Listener
{
public:
    explicit SoundPage (maket::SoundPlayer&);
    ~SoundPage() override;

    void showSound (std::shared_ptr<const maket::Sound>, const juce::String& name, const juce::String& facts);
    const juce::String& getTitle() const    { return title; }

    bool isPlaying() const                  { return mine && player.isPlaying(); }
    void playOrStop();
    void stopPlaying();
    void toggleLoop();
    void toStart();
    void zoomAcross (int direction);
    void zoomToFit();

    /** Set by the library's page, as for a roll's editor. */
    std::function<juce::String()> turnAudioOn;
    std::function<void()> onWillPlay;
    std::function<void()> onPlayStateChanged;
    void audioChanged (bool isOn, const juce::String& trouble);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scale) override;

private:
    void timerCallback() override;
    void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;
    void viewChanged();
    void updateBar();
    juce::Rectangle<int> waveArea() const;
    juce::Rectangle<int> rulerArea() const;
    double msAt (int x) const;
    int xOf (double ms) const;
    void zoomAround (double factor, double anchorMs);
    void paintChannel (juce::Graphics&, juce::Rectangle<int> area, int channel) const;

    maket::SoundPlayer& player;
    std::shared_ptr<const maket::Sound> sound;
    juce::String title, facts, notice;
    bool noticeIsProblem = false;

    double pixelsPerMs = 0.1, scrollMs = 0.0;
    double playhead = 0.0, playedFrom = 0.0;
    double loopStart = 0.0, loopEnd = 0.0, dragAnchor = 0.0;
    bool loopOn = false, mine = false, wasPlaying = false, draggingLoop = false;

    juce::TextButton playButton { "Play" }, stopButton { "Stop" }, loopButton { "Loop" };
    juce::TextButton zoomOutButton { juce::String::fromUTF8 ("\xe2\x88\x92") }, zoomInButton { "+" }, fitButton { "Fit" };
    juce::ScrollBar scrollBar { false };

    static constexpr int rowHeight = 40, rulerHeight = 28, barThickness = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundPage)
};
