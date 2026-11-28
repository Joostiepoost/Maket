// SurfacePage.h — the page of the Build surface tab (stage 3d).
//
// The project's build surface: tracks under one another, each with its name,
// mute, solo, gain and pan on the left, and to the right its lane under a
// time ruler, with the blocks of sound placed on it. A block is dragged to
// another time or track, and by its edges to use less or more of its sound.
// Play, stop, loop and playhead work as in the roll and the sound's view.
// Every change is heard at once, kept in the project, and can be undone.
//
// The page does not own the project: that is the library's. It is told when
// the project or one of its snippets changes.

#pragma once

#include <JuceHeader.h>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "../Model/Project.h"
#include "../Model/Surface.h"
#include "../Sound/SurfaceEngine.h"

class SurfacePage : public juce::Component,
                    private juce::Timer,
                    private juce::ScrollBar::Listener
{
public:
    SurfacePage (maket::SurfaceEngine&, const maket::Project&);
    ~SurfacePage() override;

    /** The project was opened or closed, or its library changed. */
    void projectChanged();
    /** A snippet was updated from its file, renamed, or taken out of the library. */
    void snippetChanged (const juce::String& id);
    bool hasProject() const                 { return project.isOpen(); }

    bool isPlaying() const                  { return playing; }
    bool isLoopOn() const                   { return loopOn; }
    void playOrStop();
    void stopPlaying();
    void toggleLoop();
    void toStart();
    void zoomAcross (int direction);
    void zoomToFit();

    void addTrack();
    bool canUndo() const                    { return ! undoStack.empty(); }
    bool canRedo() const                    { return ! redoStack.empty(); }
    void undo();
    void redo();
    bool hasSelection() const               { return selected.isNotEmpty(); }
    void deleteSelected();
    void duplicateSelected();

    /** Set by the main window. */
    std::function<juce::String()> turnAudioOn;
    std::function<void()> onWillPlay;       // whatever else sounds is to stop
    std::function<void()> onChanged;        // something the menus show has changed
    std::function<void (const juce::String& kind, const juce::String& name, const juce::StringPairArray& lines)> showDetail;
    std::function<void()> clearDetail;
    void audioChanged (bool isOn);
    /** The tab was brought in front: the inspector shows the selected block again. */
    void tabShown()                         { showSelection(); }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scale) override;
    bool keyPressed (const juce::KeyPress&) override;

    static constexpr int headerWidth = 204, trackHeight = 92;

private:
    class Header;
    friend class Header;
    enum class Drag { none, move, trimStart, trimEnd, playhead, loop };

    void timerCallback() override;
    void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;

    // --- The surface and its sounds ---
    void load();
    void save();
    std::shared_ptr<const maket::Sound> soundOf (const juce::String& snippetId);
    double soundLength (const juce::String& snippetId) const;
    void rebuild();                         // the arrangement the engine plays, from the surface as it is
    void beginGesture();
    void touched();
    void endGesture();
    void edit (const std::function<void()>& change);
    void restore (const maket::Surface&);
    void syncHeaders();
    void select (const juce::String& placementId);
    void showSelection();
    void setNotice (const juce::String& text, bool isProblem = false);
    void updateBar();

    // --- Tracks (called by their headers) ---
    void trackChanged (const juce::String& trackId, const std::function<void (maket::SurfaceTrack&)>& change);
    void showTrackMenu (const juce::String& trackId);
    void moveTrack (const juce::String& trackId, int by);
    void setCurrentTrack (const juce::String& trackId);

    // --- Blocks ---
    void showLaneMenu (int trackIndex, double atMs);
    void showBlockMenu (const juce::String& placementId);
    void showSoundsMenu();
    juce::PopupMenu soundsMenu (int firstId, std::vector<juce::String>& ids) const;
    void addBlock (const juce::String& snippetId, const juce::String& trackId, double atMs);
    const maket::Placement* blockAt (juce::Point<int>) const;
    juce::Rectangle<int> blockBounds (const maket::Placement&) const;
    void paintBlock (juce::Graphics&, const maket::Placement&, juce::Rectangle<int> lanes, double crossingInMs) const;
    /** Which end of a block the mouse is on: -1 its start, 1 its end, 0 neither. */
    int edgeAt (const maket::Placement&, juce::Point<int>) const;

    // --- The view ---
    juce::Rectangle<int> rulerArea() const;
    juce::Rectangle<int> lanesArea() const;
    double msAt (int x) const;
    int xOf (double ms) const;
    int trackTop (int index) const;
    int trackIndexAt (int y) const;
    int rowsToShow() const;                 // the tracks, and one more while a block is dragged (the track it may make)
    void viewChanged();
    void zoomAround (double factor, double anchorMs);
    void repaintPlayhead (double before, double after);

    maket::SurfaceEngine& engine;
    const maket::Project& project;
    juce::File loadedFolder;
    maket::Surface surface;
    std::map<juce::String, std::shared_ptr<const maket::Sound>> sounds;     // by snippet; null: could not be read
    std::vector<maket::Surface> undoStack, redoStack;
    maket::Surface beforeGesture;
    bool gestureOpen = false, gestureChanged = false;

    juce::String selected, currentTrack;
    Drag drag = Drag::none;
    maket::Placement grabbed;               // the block as it was when the drag began
    double grabMs = 0.0, dragAnchor = 0.0;
    juce::Point<int> downAt;

    double pixelsPerMs = 0.05, scrollMs = 0.0;
    int scrollY = 0;
    double playhead = 0.0, playedFrom = 0.0;
    double loopStart = 0.0, loopEnd = 0.0;
    bool loopOn = false, playing = false, wasPlaying = false;
    bool newTrackBelow = false;             // a block is being dragged below the last track: it gets a new one when let go
    juce::String notice;
    bool noticeIsProblem = false;

    juce::Component headerHolder;
    juce::OwnedArray<Header> headers;
    juce::TextButton playButton { "Play" }, stopButton { "Stop" }, loopButton { "Loop" };
    juce::TextButton addTrackButton { "+ Track" }, addSoundButton { "+ Sound" };
    juce::TextButton zoomOutButton { juce::String::fromUTF8 ("\xe2\x88\x92") }, zoomInButton { "+" }, fitButton { "Fit" };
    juce::ScrollBar scrollBar { false }, downBar { true };

    static constexpr int rowHeight = 40, rulerHeight = 28, barThickness = 12, undoMost = 200;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SurfacePage)
};
