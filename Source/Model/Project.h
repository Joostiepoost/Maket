// Project.h — a Maket project on disk, and its library (stage 3b).
//
// A project is a folder:
//
//   project.json                  what the project is: its sample rate, its standard pitch, the order of the library
//   Snippets/<id>.json            one per snippet: its kind, ID, name, where it was imported from
//   Snippets/<id>.roll.txt        a note snippet's notes, as bach.roll text: the same text bach itself would save,
//                                 so every note snippet of a project opens in bach.roll as it is
//   Sounds/  Orchestra/           from stages 3c and 14
//   Trash/                        what was removed from the library; nothing is deleted by Maket
//
// The notes are kept as bach text and not as JSON of Maket's own: the reader and the writer of that text are proven
// byte for byte (stage 1b), and the library stays readable from Max. The JSON beside it holds what bach has no
// place for. No interface code.

#pragma once

#include <JuceHeader.h>
#include <vector>

#include "Roll.h"

namespace maket
{

struct SnippetInfo
{
    juce::String id;                // eight hex digits, never changed
    juce::String kind = "notes";    // notes; audio from 3c; curve from step 11
    juce::String name;
    juce::String sourcePath;        // the file it was imported from, for "update from file"; may be empty
    juce::int64 sourceModified = 0; // that file's time of change when it was read, ms
    juce::String added;             // ISO date and time

    // Read from the notes when the project is opened, not stored:
    int voices = 0, notes = 0;
    double length = 0.0;            // ms
};

class Project
{
public:
    /** Makes a new project in `folder` (made if it is not there; refused if it already holds a project). */
    static juce::Result create (const juce::File& folder, double sampleRate, double standardPitch, Project& result);
    /** Opens the project in `folder`. What is odd but readable is put in `remarks`. */
    static juce::Result open (const juce::File& folder, Project& result, juce::StringArray& remarks);
    static bool isProject (const juce::File& folder);

    bool isOpen() const                 { return folder != juce::File(); }
    const juce::File& getFolder() const { return folder; }
    juce::String getName() const        { return folder.getFileName(); }

    double getSampleRate() const        { return sampleRate; }
    double getStandardPitch() const     { return standardPitch; }
    juce::Result setStandardPitch (double hz);

    const std::vector<SnippetInfo>& getSnippets() const     { return snippets; }
    const SnippetInfo* find (const juce::String& id) const;

    /** Reads a bach.roll text file and puts it in the library as a note snippet; the file itself is not touched. */
    juce::Result importRoll (const juce::File& bachFile, juce::String& newId, juce::StringArray& remarks);
    /** Reads the snippet's source file again and puts its notes in the snippet's place. */
    juce::Result updateFromFile (const juce::String& id, juce::StringArray& remarks);
    juce::Result rename (const juce::String& id, const juce::String& newName);
    /** Takes the snippet out of the library; its files go to the project's Trash folder. */
    juce::Result remove (const juce::String& id);
    /** Puts the snippet at another place in the library's order. */
    void move (const juce::String& id, int newIndex);

    juce::Result loadRoll (const juce::String& id, Roll& roll, juce::StringArray& remarks) const;
    juce::Result saveRoll (const juce::String& id, const Roll& roll);

    juce::File rollFile (const juce::String& id) const      { return folder.getChildFile ("Snippets").getChildFile (id + ".roll.txt"); }
    juce::File infoFile (const juce::String& id) const      { return folder.getChildFile ("Snippets").getChildFile (id + ".json"); }

    static constexpr double defaultSampleRate = 48000.0;
    static bool sampleRateAllowed (double rate)             { return juce::approximatelyEqual (rate, 48000.0) || juce::approximatelyEqual (rate, 96000.0); }

private:
    juce::Result writeProjectFile() const;
    juce::Result writeInfo (const SnippetInfo&) const;
    SnippetInfo* findMutable (const juce::String& id);
    juce::String newId() const;
    void measure (SnippetInfo&, const Roll&) const;

    juce::File folder;
    double sampleRate = defaultSampleRate, standardPitch = 442.0;
    std::vector<SnippetInfo> snippets;
};

} // namespace maket
