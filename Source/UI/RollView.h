// RollView.h — a roll on paper: one set of staves per voice, the notes with
// their duration lines in proportional time, a time ruler, zoom and scroll.
//
// Stage 2a draws voices, clefs, note heads, stems, ledger lines and duration
// lines. Not yet: accidentals (2b), pitch breakpoints and markers (2c), the
// playhead (2d), the content of slots (step 5), selecting and editing
// (step 4).
//
// The layout is bach's (Notation/Staves, Notation/Spelling), so that a roll
// looks as it does in bach.roll. The roll area is light with dark notation,
// as Joost asked on 24 Nov 2026; the frame around it keeps Maket's theme.
//
// The clefs and voice names stay at the left while the notes scroll under
// them, and the ruler stays at the top.

#pragma once

#include <JuceHeader.h>
#include <functional>
#include <vector>

#include "../Model/Roll.h"
#include "../Notation/Staves.h"

class RollView : public juce::Component,
                 private juce::ScrollBar::Listener
{
public:
    RollView();
    ~RollView() override;

    /** Shows a roll (a copy is kept). */
    void setRoll (const maket::Roll&);
    void clear();
    bool hasRoll() const                { return ! voices.empty() || rollLength > 0.0; }

    // Zoom. Horizontal: 1 is 100 pixels a second. Vertical: 1 is bach's 100 % (a staff space of 6 pixels).
    double horizontalZoom() const       { return zoomX; }
    double verticalZoom() const         { return zoomY; }
    void setHorizontalZoom (double zoom, double anchorX = -1.0);    // anchorX: the pixel that keeps its time (default: the left edge)
    void setVerticalZoom (double zoom, double anchorY = -1.0);
    void zoomToFit();

    static constexpr double minZoomX = 0.02, maxZoomX = 40.0, minZoomY = 0.4, maxZoomY = 6.0;

    /** Called when zoom or position changed. */
    std::function<void()> onViewChanged;

    /** The time under a pixel, in ms; and what the view shows, for tests on screen. */
    double timeAt (double x) const;
    juce::String describeView() const;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scaleFactor) override;

private:
    struct NoteItem
    {
        int step = 0;               // diatonic steps above middle C
        double units = 0.0;         // height in the roll, bach units
        double end = 0.0;           // ms
        float lightness = 0.0f;     // 0 black (loud) to about 0.8 (softest)
        std::vector<int> ledgers;   // steps of its ledger lines
        bool otherSide = false;     // the head sits on the far side of the stem, as the upper of two notes a second apart does
    };
    struct ChordItem
    {
        double onset = 0.0;         // ms
        double end = 0.0;           // ms, of its longest note
        int direction = 1;          // stem: 1 up, -1 down
        double stemFromUnits = 0.0, stemToUnits = 0.0;
        std::vector<NoteItem> notes;
    };
    struct VoiceItem
    {
        maket::notation::VoicePlace place;
        juce::String name;
        std::vector<ChordItem> chords;
    };

    void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;
    void updateScrollBars();
    void viewChanged();

    juce::Rectangle<int> rollArea() const;      // where notes are drawn
    double pixelsPerMs() const          { return zoomX * 0.1; }
    double pixelsPerUnit() const        { return zoomY; }
    float spacePixels() const           { return (float) (zoomY * 2.0 * maket::notation::stepUnits); }
    double xOf (double ms) const;
    double yOf (double units) const;
    int headerWidth() const;

    void paintRuler (juce::Graphics&, juce::Rectangle<int> area);
    void paintStaves (juce::Graphics&, int fromX, int toX);
    void paintNotes (juce::Graphics&, juce::Rectangle<int> area);
    void paintHeader (juce::Graphics&, juce::Rectangle<int> area);

    std::vector<VoiceItem> voices;
    double rollLength = 0.0;            // ms
    double heightUnits = 0.0;

    double zoomX = 0.5, zoomY = 1.0;
    double scrollMs = 0.0;              // the time at the left edge of the roll area
    double scrollY = 0.0;               // pixels scrolled down
    float nameWidth = 0.0f;             // of the widest voice name, at the font used

    juce::ScrollBar horizontalBar { false }, verticalBar { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RollView)
};
