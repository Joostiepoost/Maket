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
