// Surface.h — a build surface: tracks, and blocks of sound placed on them (stage 3d).
//
// A block (a placement) is one appearance of a snippet on the surface: it
// says which snippet, on which track, where it begins, which part of the
// sound it uses, and its own gain and fades. The snippet itself is not
// copied: every block of one snippet uses the same sound. Kept as one JSON
// file per surface in the project's Surfaces folder. No interface code.

#pragma once

#include <JuceHeader.h>
#include <vector>

namespace maket
{

struct SurfaceTrack
{
    juce::String id, name;
    double gainDb = 0.0;        // -60 (taken as silence) to +12
    double pan = 0.0;           // -1 left, 0 middle, 1 right
    bool mute = false, solo = false;
};

struct Placement
{
    juce::String id, snippetId, trackId;
    double start = 0.0;         // ms on the surface
    double offset = 0.0;        // ms into the sound: where this block's part begins
    double length = 0.0;        // ms
    double gainDb = 0.0;
    double fadeIn = 0.0, fadeOut = 0.0;     // ms

    double end() const          { return start + length; }
};

struct Surface
{
    juce::String id = "main", name = "Surface";
    std::vector<SurfaceTrack> tracks;
    std::vector<Placement> placements;

    double length() const;
    int trackIndex (const juce::String& trackId) const;
    Placement* find (const juce::String& placementId);
    const Placement* find (const juce::String& placementId) const;
    SurfaceTrack& addTrack();
    /** Takes the track away, with the blocks on it. */
    void removeTrack (const juce::String& trackId);
    Placement& addPlacement (const juce::String& snippetId, const juce::String& trackId, double start, double soundLength);
    void removePlacement (const juce::String& placementId);

    juce::String toJson() const;
    static juce::Result fromJson (const juce::String& text, Surface& result);

    static constexpr double shortestBlock = 10.0;      // ms
private:
    juce::String newId (const char* prefix) const;
};

} // namespace maket
