// WindowState.h — what Maket remembers of its window between starts.
//
// The window's place and size, the tab in front, whether the list and the
// inspector show and how wide they are. Kept in a small JSON file next to
// the settings file; it is not a setting (nothing on the preferences page),
// only the state the window was left in. No interface code: the main window
// reads it at the start and writes it when something changes.

#pragma once

#include <JuceHeader.h>

namespace maket
{

class WindowState
{
public:
    explicit WindowState (juce::File file);

    /** `~/Library/Application Support/Maket/window.json` on a Mac. */
    static juce::File defaultFile();

    /** Reads the file. A file that is missing or makes no sense leaves the first values. */
    void load();
    /** Writes the file if something changed since the last write. */
    bool save();

    // Widths in pixels, kept within their limits.
    static constexpr int listWidthDefault = 200, listWidthMin = 140, listWidthMax = 520;
    static constexpr int inspectorWidthDefault = 280, inspectorWidthMin = 200, inspectorWidthMax = 640;

    int listWidth() const               { return listW; }
    int inspectorWidth() const          { return inspectorW; }
    bool listShown() const              { return listOn; }
    bool inspectorShown() const         { return inspectorOn; }
    int tab() const                     { return tabIndex; }
    /** The window's place and size, in the form JUCE gives and takes; empty when never stored. */
    juce::String windowBounds() const   { return bounds; }

    void setListWidth (int);
    void setInspectorWidth (int);
    void setListShown (bool);
    void setInspectorShown (bool);
    void setTab (int);
    void setWindowBounds (const juce::String&);

private:
    juce::File file;
    int listW = listWidthDefault, inspectorW = inspectorWidthDefault, tabIndex = 0;
    bool listOn = true, inspectorOn = true, changed = false;
    juce::String bounds;
};

} // namespace maket
