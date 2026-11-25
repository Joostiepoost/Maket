// MusicFont.h — the signs of music notation, from the Bravura font.
//
// Bravura (Steinberg, SIL Open Font License) is built into Maket. A sign is
// asked for by its SMuFL code and comes as a path measured in staff spaces:
// 1 is the distance between two lines of a staff, the origin is the sign's
// own (for a clef: the line it sits on), y grows downwards. So the view
// only has to multiply by the size of a staff space on screen.

#pragma once

#include <JuceHeader.h>
#include <map>

class MusicFont
{
public:
    // SMuFL codes of the signs used so far.
    enum Sign : juce::juce_wchar
    {
        gClef = 0xE050, gClef15mb = 0xE051, gClef8vb = 0xE052, gClef8va = 0xE053, gClef15ma = 0xE054,
        cClef = 0xE05C,
        fClef = 0xE062, fClef15mb = 0xE063, fClef8vb = 0xE064, fClef8va = 0xE065, fClef15ma = 0xE066,
        percussionClef = 0xE06A,      // the open box, as bach's (0xE069 is the pair of bars)
        noteheadBlack = 0xE0A4,
        flat = 0xE260, sharp = 0xE262,
        naturalArrowUp = 0xE272, naturalArrowDown = 0xE273,
        quarterFlat = 0xE280, quarterSharp = 0xE282,            // the reversed flat and the half sharp
        quarterFlatArrowDown = 0xE291, quarterSharpArrowUp = 0xE299
    };

    static MusicFont& get();

    /** False when the font could not be loaded; the view then draws plain shapes. */
    bool isLoaded() const       { return loaded; }

    /** The sign as a path in staff spaces (empty when the font lacks it). */
    const juce::Path& path (juce::juce_wchar sign);

    /** Draws a sign with its origin at (x, y), for a staff space of `space` pixels. */
    void draw (juce::Graphics&, juce::juce_wchar sign, float x, float y, float space);

    /** The width of a black note head in staff spaces. */
    float noteheadWidth();

    /** A sign's bounds in staff spaces, around its origin (empty when the font lacks it). */
    juce::Rectangle<float> bounds (juce::juce_wchar sign)      { return path (sign).getBounds(); }

private:
    MusicFont();

    juce::Typeface::Ptr typeface;
    bool loaded = false;
    float unitsPerSpace = 1.0f;         // path units of the reference font per staff space
    std::map<juce::juce_wchar, juce::Path> cache;

    juce::Path rawPath (juce::juce_wchar sign) const;
};
