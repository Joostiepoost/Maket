// TestReport.h — the report the test program writes, shared by its files.
//
// A test section writes lines of Markdown with line(), and counts its checks
// with check() or, for a check that is also a row of a table, checkRow().

#pragma once

#include <JuceHeader.h>
#include <iostream>

namespace testreport
{

inline juce::String md;          // the report, in Markdown
inline int checks = 0;
inline int failures = 0;

inline void line (const juce::String& s = {})       { md << s << "\n"; }

/** Counts a check; a failed one is also printed in Terminal. */
inline bool check (bool ok, const juce::String& what)
{
    ++checks;
    if (! ok) { ++failures; std::cout << "  FAIL: " << what << std::endl; }
    return ok;
}

/** A check that is also a row of the table being written. */
inline void checkRow (bool ok, const juce::String& what)
{
    check (ok, what);
    line ("| " + what + " | " + (ok ? "yes" : "**NO**") + " |");
}

} // namespace testreport

/** Stage 1a: the bach text reader (Tests/BachTests.cpp). `temp` is the
    repository's Temp folder with Joost's rolls and bach's own answers. */
void testBachReading (const juce::File& temp);

/** Stage 1b: the bach text writer (Tests/BachWriteTests.cpp). Writes the two
    rolls to `Temp/written by Maket`, for Joost to open in bach.roll. */
void testBachWriting (const juce::File& temp);

/** Stage 1c: OSC messages, the sender and the send thread (Tests/OscTests.cpp). */
void testOsc();

/** Stage 1d: the passages, the log reader and the comparison of the timing
    tests (Tests/TimingTests.cpp). `temp` holds the two logs of stage 1c. */
void testTiming (const juce::File& temp);

/** A file of Joost's test material: in `Temp/support materials`, where he keeps
    it since 23 Nov, or straight in `Temp`, where it was before. */
inline juce::File material (const juce::File& temp, const juce::String& name)
{
    const auto moved = temp.getChildFile ("support materials").getChildFile (name);
    return moved.exists() ? moved : temp.getChildFile (name);
}

/** Stage 1e: a roll's notes as commands for the conTimbre ePlayer, and the
    comparison with a log from Max (Tests/MappingTests.cpp). */
void testConTimbreMapping (const juce::File& temp);

/** Stage 1f.2: the plan of the ePlayer study (Tests/StudyTests.cpp). */
void testPlayerStudy();

// Section 8: where notes sit on the staves (stage 2a), NotationTests.cpp.
void testNotation (const juce::File& temp);
void testPlaying (const juce::File& temp);
void testSynth (const juce::File& temp);
void testSpeed (const juce::File& temp);
void testState (const juce::File& workFolder);
void testProject (const juce::File& workFolder, const juce::File& temp);
void testSound (const juce::File& workFolder);
void testSurface (const juce::File& workFolder);
