// BachWriteTests.cpp — stage 1b: the writer of bach's text and of bach.roll
// files.
//
// The test agreed for this stage: Joost's two rolls, read and written back,
// come out byte for byte as they were, each with the header it came with.
// It is done twice per file: through the list alone, and through the roll
// (where every onset, pitch, duration, velocity, breakpoint and marker is
// written from its value, not from the text it was read from).

#include <JuceHeader.h>
#include <cmath>
#include <string>
#include <vector>

#include "TestReport.h"
#include "../Source/Model/Llll.h"
#include "../Source/Model/Roll.h"
#include "../Source/Formats/LlllText.h"
#include "../Source/Formats/BachRoll.h"

using namespace testreport;
using maket::Llll;
using maket::Roll;

namespace
{

std::string bytesOf (const juce::String& text)
{
    return std::string (text.toRawUTF8(), text.getNumBytesAsUTF8());
}

std::string bytesOf (const juce::File& file)
{
    juce::MemoryBlock data;
    file.loadFileAsData (data);
    return std::string ((const char*) data.getData(), data.getSize());
}

std::vector<std::string> linesOf (const std::string& s)
{
    std::vector<std::string> lines;
    size_t start = 0;
    for (;;)
    {
        const auto end = s.find ('\n', start);
        if (end == std::string::npos) { lines.push_back (s.substr (start)); break; }
        lines.push_back (s.substr (start, end - start));
        start = end + 1;
    }
    return lines;
}

juce::String shown (const std::string& s)
{
    auto t = juce::String::fromUTF8 (s.data(), (int) s.size()).trim();
    return t.length() > 70 ? t.substring (0, 70) + "..." : t;
}

/** Where two texts first differ, in words; empty when they are the same. */
juce::String firstDifference (const std::string& original, const std::string& written)
{
    if (original == written) return {};

    const auto a = linesOf (original);
    const auto b = linesOf (written);
    for (size_t i = 0; i < a.size() && i < b.size(); ++i)
        if (a[i] != b[i])
            return "line " + juce::String ((int) i + 1) + ": the file has `" + shown (a[i]) + "`, Maket wrote `" + shown (b[i]) + "`";

    return "the file has " + juce::String ((int) a.size()) + " lines, Maket wrote " + juce::String ((int) b.size());
}

/** How many lines differ between two texts of as many lines; -1 when the number of lines differs. */
int numLinesThatDiffer (const std::string& a, const std::string& b)
{
    const auto la = linesOf (a);
    const auto lb = linesOf (b);
    if (la.size() != lb.size()) return -1;

    int n = 0;
    for (size_t i = 0; i < la.size(); ++i)
        if (la[i] != lb[i]) ++n;
    return n;
}

void countAtoms (const Llll& item, int& all, int& made)
{
    if (! item.isList())
    {
        ++all;
        if (item.spelling.isEmpty()) ++made;
        return;
    }
    for (const auto& sub : item.items) countAtoms (sub, all, made);
}

/** Writes every atom from its value alone, without its spelling, and counts
    those that do not come out as the file spelled them. */
void respell (const Llll& item, int& all, int& different, juce::String& firstDifferent)
{
    if (item.isList())
    {
        for (const auto& sub : item.items) respell (sub, all, different, firstDifferent);
        return;
    }
    if (item.kind == Llll::Kind::pitch || item.kind == Llll::Kind::rational) return;    // Maket makes neither

    Llll bare = item;
    bare.spelling.clear();
    const auto written = maket::writeLlllAtom (bare);
    ++all;
    if (written != item.spelling)
    {
        if (++different == 1) firstDifferent = "the file has `" + item.spelling + "`, Maket would write `" + written + "`";
    }
}

void row (bool ok, const juce::String& what, const juce::String& result)
{
    check (ok, what);
    line ("| " + what + " | " + result + " | " + (ok ? "yes" : "**NO**") + " |");
}

// --- The text writer on small cases ----------------------------------------------

void testNumbers()
{
    struct Case { double value; const char* written; };
    const Case cases[] =
    {
        { 7100.0,        "7100.0" },
        { 0.5,           "0.5" },
        { 35407.0514,    "35407.0514" },
        { 40452.095651,  "40452.095651" },
        { 179244.0,      "179244.0" },
        { -0.2875,       "-0.2875" },
        { 1.0 / 3.0,     "0.333333" },
        { 2.0 / 3.0,     "0.666667" },
        { 0.0000001,     "0.0" }
    };

    juce::StringArray shownCases;
    bool all = true;
    for (const auto& c : cases)
    {
        const auto written = maket::writeLlllAtom (Llll::makeReal (c.value));
        all = all && written == c.written;
        shownCases.add ("`" + written + "`");
    }
    all = all && maket::writeLlllAtom (Llll::makeInteger (117)) == "117"
              && maket::writeLlllAtom (Llll::makeInteger (-5)) == "-5";

    checkRow (all, "A real number is written with at most six decimals and always a point, a whole number without one ("
                   + shownCases.joinIntoString (", ") + ", `117`)");
}

void testSymbols()
{
    struct Case { const char* name; const char* written; const char* why; };
    const Case cases[] =
    {
        { "pizz",                       "pizz",                             "" },
        { "-eind",                      "-eind",                            "" },
        { "uitgegaan...",               "uitgegaan...",                     "" },
        { "s3z1+2b.wav",                "s3z1+2b.wav",                      "" },
        { "1",                          "`1",                               "would be read as a number" },
        { "7100.0",                     "`7100.0",                          "would be read as a number" },
        { "G8vb",                       "`G8vb",                            "would be read as a pitch" },
        { "nil",                        "`nil",                             "would be read as an empty list" },
        { "[x]",                        "`[x]",                             "brackets" },
        { "gain (dB)",                  "`gain\\ (dB)",                     "brackets and a space" },
        { "a b",                        "\"a b\"",                          "a space" },
        { "x,y",                        "\"x,y\"",                          "a comma" },
        { "D'aarde is niet uit haar ",  "\"D'aarde is niet uit haar \"",    "spaces, one at the end" },
        { "say \"hi\"",                 "\"say \\\"hi\\\"\"",               "a space and quotes" },
        { "Fl\xc3\xb6te 1",             "\"Fl\xc3\xb6te 1\"",               "an accent is nothing special" },
        { "a\\b",                       "\"a\\\\b\"",                       "a backslash" },
        { "",                           "\"\"",                             "an empty text" }
    };

    line();
    line ("Texts, and how each is written so that bach and Maket read it back as the same text:");
    line();
    line ("| Text | Written as | Why |");
    line ("| --- | --- | --- |");

    bool all = true;
    int count = 0;
    for (const auto& c : cases)
    {
        const auto name = juce::String::fromUTF8 (c.name);
        const auto expected = juce::String::fromUTF8 (c.written);
        const auto written = maket::writeLlllAtom (Llll::makeSymbol (name));

        Llll back;
        const bool readsBack = maket::readLlllText (written, back).wasOk() && back.size() == 1 && back[0].isSymbol() && back[0].text == name;
        const bool ok = written == expected && readsBack;
        all = all && ok;
        ++count;

        line ("| `" + (name.isEmpty() ? juce::String ("(empty)") : name) + "` | `` " + written + " ``" + (ok ? "" : " **WRONG**")
              + " | " + c.why + " |");
    }

    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");
    checkRow (all, "All " + juce::String (count) + " texts above are written as expected and read back as themselves");
}

void testLayout()
{
    // One line in, bach's layout out: worked out by hand from bach's rules.
    const char* oneLine = "roll ( a 1 ( b 2.5 ) c ( ) ) ( 0 )";
    const char* expected = "roll\n"
                           "\t[ a 1\n"
                           "\t\t[ b 2.5 ]\n"
                           "\tc\n"
                           "\t\t[ ]\n"
                           "\t]\n"
                           "\t[ 0 ]";

    Llll list;
    const bool ok = maket::readLlllText (oneLine, list).wasOk() && maket::writeLlllText (list) == expected;
    checkRow (ok, "bach's layout: a tab per level, every list on its own line, a closing bracket on the last line of its "
                  "list or alone after an inner list, no line end at the end; round brackets come out square");

    Llll again;
    const bool stable = maket::readLlllText (maket::writeLlllText (list), again).wasOk()
                        && maket::writeLlllText (again) == maket::writeLlllText (list)
                        && Llll::sameContent (list, again);
    checkRow (stable, "What was written reads back to the same list, and is written the same again");
}

// --- One roll -----------------------------------------------------------------------

void testRollWriting (const juce::String& title, const juce::File& file, const juce::File& copyFolder)
{
    line ("### " + title);
    line();

    if (! file.existsAsFile())
    {
        check (false, title + ": `" + file.getFileName() + "` is not in Temp");
        line ("**Not tested:** `" + file.getFileName() + "` is not in `Temp`.");
        line();
        return;
    }

    const auto original = bytesOf (file);
    const auto size = juce::String ((int) original.size()) + " bytes, " + juce::String ((int) linesOf (original).size()) + " lines";

    Llll content;
    Roll roll;
    juce::StringArray remarks;
    auto r = maket::readLlllText (file.loadFileAsString(), content);
    if (r.wasOk()) r = maket::readBachRoll (content, roll, remarks);

    if (r.failed())
    {
        check (false, title + ": could not be read");
        line ("**Could not be read:** " + r.getErrorMessage());
        line();
        return;
    }

    line ("`" + file.getFileName() + "`: " + size + ".");
    line();
    line ("| Check | Result | Passed |");
    line ("| --- | --- | --- |");

    // 1. Through the list alone: only the layout is Maket's.
    {
        const auto written = bytesOf (maket::writeLlllText (content));
        const auto difference = firstDifference (original, written);
        row (difference.isEmpty(), "Read into a list and written out: byte for byte the file",
             difference.isEmpty() ? "the same" : difference);
    }

    // 2. Through the roll: the notes, chords, voices and markers are written from their values.
    const auto asList = maket::bachRollAsList (roll);
    const auto fromRoll = bytesOf (maket::writeBachRollText (roll));
    {
        const auto difference = firstDifference (original, fromRoll);
        row (difference.isEmpty(), "Read into a roll and written out from the roll: byte for byte the file, with the header it came with",
             difference.isEmpty() ? "the same" : difference);

    }

    int numAtoms = 0, numMade = 0;
    countAtoms (asList, numAtoms, numMade);

    // 3. bach's rules for numbers and texts, on everything the file holds.
    {
        int all = 0, different = 0;
        juce::String first;
        respell (content, all, different, first);
        row (different == 0, "Every number and word of the file, written from its value alone by bach's rules, comes out as the file spelled it",
             different == 0 ? juce::String (all) + " of " + juce::String (all)
                            : juce::String (all - different) + " of " + juce::String (all) + "; " + first);
    }

    // 4. The writer writes the roll, not a copy of the file: a change in the roll shows.
    {
        Roll changed = roll;
        bool done = false;
        for (auto& voice : changed.voices)
        {
            for (auto& chord : voice.chords)
                if (! chord.notes.empty()) { chord.notes[0].velocity += 1; done = true; break; }
            if (done) break;
        }

        const int differing = done ? numLinesThatDiffer (fromRoll, bytesOf (maket::writeBachRollText (changed))) : -1;
        row (differing == 1, "With the velocity of the first note raised by 1 in the roll, the written text differs in exactly one line",
             differing < 0 ? juce::String ("not tested") : juce::String (differing) + (differing == 1 ? " line" : " lines"));
    }

    // 5. As a file on disk, for Joost to open in bach.roll.
    if (copyFolder != juce::File())
    {
        const auto copy = copyFolder.getChildFile (file.getFileName());
        const auto written = maket::writeBachRollFile (copy, roll);
        const bool same = written.wasOk() && bytesOf (copy) == original;
        row (same, "Written as a file, `Temp/" + copyFolder.getFileName() + "/" + copy.getFileName() + "`: byte for byte the original",
             written.failed() ? written.getErrorMessage() : (same ? "the same" : "differs"));
    }

    line();
    line ("Of the " + juce::String (numAtoms) + " numbers and words written from the roll, " + juce::String (numMade)
          + " come from the roll's values (onsets, pitches, durations, velocities, breakpoints, marker times, flags); "
            "the others are kept as the file spelled them (slot contents, header).");
    line();
}

/** A file that is not a copy: the mixed example with two changes made in
    the roll, for Joost to open in bach.roll. It is the first text bach reads
    that bach did not write itself. */
void testChangedRoll (const juce::File& original, const juce::File& copyFolder)
{
    line ("### A roll changed by Maket");
    line();

    Roll roll;
    juce::StringArray remarks;
    const bool wasRead = maket::readBachRollFile (original, roll, remarks).wasOk();

    // The first voice on a staff with pitches (not a percussion staff) that has a note.
    size_t v = 0;
    while (wasRead && v < roll.voices.size()
           && (roll.voices[v].clef == "Percussion" || roll.voices[v].chords.empty() || roll.voices[v].chords[0].notes.empty()))
        ++v;

    if (! wasRead || v >= roll.voices.size())
    {
        check (false, "A roll changed by Maket: `" + original.getFileName() + "` could not be read");
        line ("**Not tested:** `" + original.getFileName() + "` could not be read.");
        line();
        return;
    }

    const juce::String markerName ("Maket test (1b)");
    const double markerTime = 1000.0;
    const double before = roll.voices[v].chords[0].notes[0].cents;
    const double onset = roll.voices[v].chords[0].onset;

    Roll changed = roll;
    changed.voices[v].chords[0].notes[0].cents = before + 50.0;

    maket::Marker marker;
    marker.position = markerTime;
    marker.names = Llll::makeSymbol (markerName);
    marker.role = "none";
    changed.markers.insert (changed.markers.begin(), marker);

    const auto file = copyFolder.getChildFile (original.getFileNameWithoutExtension() + " changed by Maket.txt");
    const auto written = maket::writeBachRollFile (file, changed);

    line ("`Temp/" + copyFolder.getFileName() + "/" + file.getFileName() + "` is `" + original.getFileName() + "` with two changes made in the roll:");
    line();
    line ("- the first note of voice " + juce::String ((int) v + 1) + " (`" + roll.voices[v].name + "`, at " + juce::String (onset / 1000.0, 2)
          + " s) is 50 cents, a quarter tone, higher: " + juce::String (before, 1) + " cents became " + juce::String (before + 50.0, 1) + ";");
    line ("- a marker named `" + markerName + "` stands at " + juce::String (markerTime / 1000.0, 1)
          + " s. In the file its name is written `` " + maket::writeLlllAtom (marker.names) + " ``.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    auto same = [] (double a, double b) { return std::abs (a - b) < 1.0e-9; };

    Roll back;
    juce::StringArray ignored;
    const bool readsBack = written.wasOk() && maket::readBachRollFile (file, back, ignored).wasOk()
                           && back.markers.size() == roll.markers.size() + 1
                           && back.markers[0].names.isSymbol() && back.markers[0].names.text == markerName
                           && same (back.markers[0].position, markerTime) && back.markers[0].role == "none"
                           && back.voices.size() == roll.voices.size()
                           && ! back.voices[v].chords.empty() && ! back.voices[v].chords[0].notes.empty()
                           && same (back.voices[v].chords[0].notes[0].cents, before + 50.0);
    checkRow (readsBack, "The file is written, and read back it holds the higher note and the marker");

    bool restOfIt = false;
    if (readsBack)
    {
        back.markers.erase (back.markers.begin());
        back.voices[v].chords[0].notes[0].cents = before;
        restOfIt = bytesOf (maket::writeBachRollText (back)) == bytesOf (original);
    }
    checkRow (restOfIt, "With the two changes taken out again, it is byte for byte the original: nothing else was touched");
    line();
}

} // namespace

void testBachWriting (const juce::File& temp)
{
    line ("## 4. Writing bach rolls");
    line();
    line ("Stage 1b: the writer of bach's text, and the writer that makes a bach.roll file of a roll. The layout and the "
          "rules for numbers and texts are bach's own, taken from its source. The test agreed for this stage: Joost's "
          "two rolls, read and written back, come out byte for byte as they were, each with the header it came with.");
    line();

    line ("### The text writer");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");
    testNumbers();
    testLayout();
    testSymbols();
    line();

    const auto copies = temp.getChildFile ("written by Maket");

    testRollWriting ("doodsbed final v1c", temp.getChildFile ("doodsbed final v1c.txt"), copies);
    testRollWriting ("mixed_roll_example_01h", temp.getChildFile ("mixed_roll_example_01h.txt"), copies);

    testChangedRoll (temp.getChildFile ("mixed_roll_example_01h.txt"), copies);

    line ("The same for the two files as bach saved them again (for doodsbed that is the file with the longer header of the newer bach):");
    line();
    testRollWriting ("doodsbed, re-saved by bach", temp.getChildFile ("01b doodsbed roll resave after dump.txt"), {});
    testRollWriting ("mixed example, re-saved by bach", temp.getChildFile ("02b mixed_roll_example_01h resave after dump.txt"), {});
}
