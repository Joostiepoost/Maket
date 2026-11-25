// RollView.h — a roll on paper: one set of staves per voice, the notes with
// their duration lines in proportional time, a time ruler, zoom and scroll.
//
// Stage 2a draws voices, clefs, note heads, stems, ledger lines and duration
// lines; 2b the accidentals; 2c glides (pitch breakpoints), markers and the
// colours of what is locked, muted or solo. Not yet: the
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
#include "../Notation/Glides.h"
#include "../Notation/Staves.h"

class RollView : public juce::Component,
                 private juce::ScrollBar::Listener
{
public:
    RollView();
    ~RollView() override;

    /** Shows a roll (a copy is kept). */
    void setRoll (const maket::Roll&);

    /** How a note shows its velocity: bach's grey scale (softest almost white), a colour from blue (softest)
        to red (loudest), or bach's alpha (softest almost clear). */
    enum class VelocityDisplay { greyScale = 1, spectrum, alpha };
    VelocityDisplay velocityDisplay() const     { return velocityMode; }
    void setVelocityDisplay (VelocityDisplay);

    /** The grid notes are drawn on: 8 eighth tones, 4 quarter tones, 2 semitones (bach's tone division). */
    int toneDivision() const            { return division; }
    void setToneDivision (int);

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
        int velocity = 100;
        std::vector<int> ledgers;   // steps of its ledger lines
        float headLeft = 0.0f;      // the left edge of the head, staff spaces from the stem (negative: left of it)
        bool auxiliaryStem = false; // the head does not touch the stem: a branch leads to it
        int flags = 0;              // locked, muted, solo: its own, its chord's and its voice's together
        std::vector<maket::notation::GlidePoint> glide;     // the duration line of a note that glides; empty otherwise
        juce::juce_wchar accidental = 0;    // the sign left of the chord, 0 for none
        float accidentalLeft = 0.0f;        // the sign's left edge, staff spaces from the stem (negative: left of it)
    };
    struct ChordItem
    {
        double onset = 0.0;         // ms
        double end = 0.0;           // ms, of its longest note
        int direction = 1;          // stem: 1 up, -1 down
        int maxVelocity = 100;      // of its loudest note: the stem's
        int flags = 0;              // the chord's and its voice's
        double stemFromUnits = 0.0, stemToUnits = 0.0;
        std::vector<NoteItem> notes;
    };
    struct VoiceItem
    {
        maket::notation::VoicePlace place;
        juce::String name;
        int flags = 0;
        std::vector<ChordItem> chords;
    };
    struct MarkerItem
    {
        double ms = 0.0;
        juce::String name;          // its name, or for a tempo or time signature what it holds
        juce::String role;          // empty for a plain marker
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
    double zeroPad() const;

    void paintRuler (juce::Graphics&, juce::Rectangle<int> area);
    void paintMarkers (juce::Graphics&, juce::Rectangle<int> area, bool names);
    void paintStaves (juce::Graphics&, int fromX, int toX);
    void paintNotes (juce::Graphics&, juce::Rectangle<int> area);
    void paintHeader (juce::Graphics&, juce::Rectangle<int> area);

    void rebuild();

    maket::Roll roll;
    int division = 8;
    VelocityDisplay velocityMode = VelocityDisplay::greyScale;
    juce::Colour colourOf (int velocity, int itemFlags) const;
    static juce::Colour flagColour (int itemFlags, juce::Colour otherwise);
    std::vector<MarkerItem> markers;
    double startReach = 0.0;            // staff spaces the chords of the first quarter second reach left of their onset
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
