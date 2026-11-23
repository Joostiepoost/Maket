// BachTests.cpp — stage 1a: the list type, the reader of bach's text, and
// the reader of bach.roll files.
//
// Two kinds of check. First the text reader on small cases whose right
// answer is known from bach's own rules. Then Joost's two rolls, against
// what bach itself says about them: the files `01a` and `02a` in Temp hold
// bach's answers to getnumvoices, getnumchords, getnumnotes and getmarker,
// and its dump of every note (sel all, dumpselection). The files `01b` and
// `02b` are the same rolls saved again by bach.

#include <JuceHeader.h>
#include <cmath>
#include <vector>

#include "TestReport.h"
#include "../Source/Model/Llll.h"
#include "../Source/Model/Roll.h"
#include "../Source/Formats/LlllText.h"
#include "../Source/Formats/BachRoll.h"

using namespace testreport;
using maket::Llll;
using maket::Roll;
using maket::Note;
using maket::Chord;

namespace
{

// bach prints six decimals in its dump; the files carry at most six.
constexpr double tolerance = 1.0e-6;

bool about (double a, double b)      { return std::abs (a - b) <= tolerance; }

juce::String kindName (const Llll& a)
{
    switch (a.kind)
    {
        case Llll::Kind::integer:   return "whole number";
        case Llll::Kind::real:      return "real number";
        case Llll::Kind::rational:  return "fraction";
        case Llll::Kind::pitch:     return "pitch";
        case Llll::Kind::symbol:    return "symbol";
        case Llll::Kind::list:      return "list";
    }
    return {};
}

juce::String joined (const std::vector<int>& numbers)
{
    juce::StringArray s;
    for (int n : numbers) s.add (juce::String (n));
    return s.joinIntoString (" ");
}

// --- The text reader on small cases --------------------------------------------

void testWords()
{
    struct Case { const char* word; Llll::Kind kind; double value; const char* why; };
    const Case cases[] =
    {
        { "117",          Llll::Kind::integer,  117.0,    "" },
        { "-5",           Llll::Kind::integer,  -5.0,     "" },
        { "7100.0",       Llll::Kind::real,     7100.0,   "117 and 117.0 are kept apart" },
        { "-0.2875",      Llll::Kind::real,     -0.2875,  "" },
        { "0.",           Llll::Kind::real,     0.0,      "" },
        { ".5",           Llll::Kind::real,     0.5,      "" },
        { "1e-3",         Llll::Kind::real,     0.001,    "" },
        { "1/4",          Llll::Kind::rational, 0.25,     "" },
        { "-3/2",         Llll::Kind::rational, -1.5,     "" },
        { "C5",           Llll::Kind::pitch,    6000.0,   "middle C" },
        { "Eb4",          Llll::Kind::pitch,    5100.0,   "" },
        { "c#5",          Llll::Kind::pitch,    6100.0,   "" },
        { "Dq5",          Llll::Kind::pitch,    6250.0,   "quarter sharp" },
        { "Dd5",          Llll::Kind::pitch,    6150.0,   "quarter flat" },
        { "C^5",          Llll::Kind::pitch,    6025.0,   "eighth sharp" },
        { "Cv5",          Llll::Kind::pitch,    5975.0,   "eighth flat" },
        { "bb4",          Llll::Kind::pitch,    5800.0,   "B flat" },
        { "C5+1/4t",      Llll::Kind::pitch,    6050.0,   "a quarter of a tone up" },
        { "G8vb",         Llll::Kind::pitch,    10175.0,  "so bach writes this clef name with a backtick" },
        { "pizz",         Llll::Kind::symbol,   0.0,      "" },
        { "sul.pont",     Llll::Kind::symbol,   0.0,      "" },
        { "-eind",        Llll::Kind::symbol,   0.0,      "" },
        { "uitgegaan...", Llll::Kind::symbol,   0.0,      "" },
        { "s3z1+2b.wav",  Llll::Kind::symbol,   0.0,      "" },
        { "12abc",        Llll::Kind::symbol,   0.0,      "" },
        { "1.2.3",        Llll::Kind::symbol,   0.0,      "" },
        { "L1a",          Llll::Kind::symbol,   0.0,      "" },
        { "Fl",           Llll::Kind::symbol,   0.0,      "" }
    };

    line ("Single words, read by bach's order of trial (whole number, fraction, real number, pitch name, symbol):");
    line();
    line ("| Word | Read as | Value | Note |");
    line ("| --- | --- | --- | --- |");

    bool all = true;
    int count = 0;
    for (const auto& c : cases)
    {
        const auto atom = maket::readLlllWord (c.word);
        const bool ok = atom.kind == c.kind && (! atom.isNumber() || about (atom.value, c.value))
                        && (! atom.isSymbol() || atom.text == c.word);
        all = all && ok;
        ++count;
        line ("| `" + juce::String (c.word) + "` | " + kindName (atom) + (ok ? "" : " **WRONG**") + " | "
              + (atom.isNumber() ? juce::String (atom.value) : juce::String()) + " | " + c.why + " |");
    }
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");
    checkRow (all, "All " + juce::String (count) + " words above are read as bach reads them");
}

void testSmallTexts()
{
    auto read = [] (const char* text, Llll& out) { return maket::readLlllText (juce::String::fromUTF8 (text), out); };
    Llll a, b;

    {
        const bool ok = read ("[ 1 `1 \"a b\" `gain\\ (dB) a\\ b ]", a).wasOk()
                        && a.size() == 1 && a[0].size() == 5
                        && a[0][0].kind == Llll::Kind::integer
                        && a[0][1].isSymbol ("1") && a[0][2].isSymbol ("a b")
                        && a[0][3].isSymbol ("gain (dB)") && a[0][4].isSymbol ("a b");
        checkRow (ok, "A backtick makes a symbol of anything (`` `1 `` is the text 1, `` `gain\\ (dB) `` is \"gain (dB)\"); quotes and a backslash keep spaces");
    }
    {
        const bool ok = read ("\"D'aarde is niet uit haar \" \"say \\\"hi\\\"\"", a).wasOk()
                        && a.size() == 2 && a[0].isSymbol ("D'aarde is niet uit haar ") && a[1].isSymbol ("say \"hi\"");
        checkRow (ok, "A quoted text keeps its apostrophe and the space at its end; a quote inside it is kept by a backslash");
    }
    {
        const bool ok = read ("( 1 ( 2.5 x ) ) [ ]", a).wasOk() && read ("[ 1 [ 2.5 x ] ] [ ]", b).wasOk()
                        && Llll::sameContent (a, b) && a.size() == 2 && a[0].depth() == 2 && a[1].isList() && a[1].size() == 0;
        checkRow (ok, "Round and square brackets mean the same (bach's older files use round ones)");
    }
    {
        const bool ok = read ("[ 1 null nil 2 ]", a).wasOk() && a[0].size() == 3
                        && a[0][1].isList() && a[0][1].size() == 0 && a[0][2].kind == Llll::Kind::integer;
        checkRow (ok, "`nil` is an empty list and `null` is nothing");
    }
    {
        const auto open = read ("[ 1 [ 2 ]\n 3", a);
        const auto close = read ("1 2 ]", a);
        const auto quote = read ("[ \"never ends ]", a);
        checkRow (open.failed() && open.getErrorMessage().contains ("Line 1") && close.failed() && quote.failed(),
                  "A bracket that is never closed, one that closes nothing, and a quote that never ends are refused, with the line");
    }
    {
        const bool ok = read ("[ 117 117.0 ]", a).wasOk()
                        && a[0][0].kind == Llll::Kind::integer && a[0][1].kind == Llll::Kind::real
                        && a[0][0].spelling == "117" && a[0][1].spelling == "117.0" && a.toLine() == "[ [ 117 117.0 ] ]";
        checkRow (ok, "Every atom keeps the way it was written, so `117.0` is written back as `117.0`");
    }
}

// --- bach's own answers (the 01a and 02a files) ----------------------------------

struct BachAnswers
{
    int numVoices = -1;
    std::vector<int> numChords;                 // per voice
    std::vector<std::vector<int>> numNotes;     // per voice, per chord
    std::vector<Llll> markers;                  // getmarker: [ position names role content ]

    struct DumpedNote   { int voice = 0, channel = 0; Llll chord; };
    struct DumpedMarker { Llll names; double position = 0.0; juce::String role; };
    std::vector<DumpedNote> notes;
    std::vector<DumpedMarker> dumpedMarkers;
};

juce::Result readBachAnswers (const juce::File& file, BachAnswers& answers)
{
    const auto text = file.loadFileAsString();
    const int cut = text.indexOf ("***");
    if (cut < 0) return juce::Result::fail ("no *** line between the counts and the dump");

    Llll head, dump;
    auto r = maket::readLlllText (text.substring (0, cut), head);
    if (r.failed()) return r;
    r = maket::readLlllText (text.substring (cut + 3).fromFirstOccurrenceOf ("dumpselection:", false, true), dump);
    if (r.failed()) return r;

    juce::String key;
    for (const auto& item : head.items)
    {
        if (item.isSymbol ("numvoices") || item.isSymbol ("numchords") || item.isSymbol ("numnotes") || item.isSymbol ("markers"))
        {
            key = item.text;
            continue;
        }
        if (key == "numvoices" && item.isNumber())      answers.numVoices = (int) item.value;
        else if (key == "numchords" && item.isNumber()) answers.numChords.push_back ((int) item.value);
        else if (key == "numnotes" && item.isList())
        {
            std::vector<int> perChord;
            for (const auto& n : item.items) perChord.push_back ((int) n.value);
            answers.numNotes.push_back (perChord);
        }
        else if (key == "markers" && item.isList())     answers.markers.push_back (item);
    }

    const int n = dump.size();
    for (int i = 0; i < n; )
    {
        if (dump[i].isSymbol ("note") && i + 3 < n && dump[i + 3].isList())
        {
            answers.notes.push_back ({ (int) dump[i + 1].value, (int) dump[i + 2].value, dump[i + 3] });
            i += 4;
        }
        else if (dump[i].isSymbol ("marker") && i + 3 < n)
        {
            answers.dumpedMarkers.push_back ({ dump[i + 1], dump[i + 2].value, dump[i + 3].text });
            i += 4;
            if (i < n && dump[i].isList()) ++i;         // the content of the role, such as [ 3 4 ]
        }
        else
        {
            return juce::Result::fail ("unexpected `" + dump[i].toLine() + "` in the dump");
        }
    }
    return juce::Result::ok();
}

// --- Comparing notes --------------------------------------------------------------

/** bach expands a dynamics slot when it plays a note: `[ auto p < ]` in the
    file is `[ [ auto 0 0.5 ] p < ]` in the dump. This brings the dumped form
    back to the file's form. */
Llll dynamicsAsInFile (Llll content)
{
    for (auto& mark : content.items)
        if (mark.isList() && mark.size() > 0 && mark[0].isList() && mark[0].size() > 0)
        {
            const Llll first = mark[0][0];
            mark.items[0] = first;
        }
    return content;
}

bool sameNote (const Note& a, const Note& b, const Roll& roll, bool bIsDumped)
{
    if (! about (a.cents, b.cents) || ! about (a.duration, b.duration) || a.velocity != b.velocity || a.flags != b.flags)
        return false;

    if (a.breakpoints.size() != b.breakpoints.size()) return false;
    for (size_t i = 0; i < a.breakpoints.size(); ++i)
        if (! about (a.breakpoints[i].position, b.breakpoints[i].position)
            || ! about (a.breakpoints[i].cents, b.breakpoints[i].cents)
            || ! about (a.breakpoints[i].slope, b.breakpoints[i].slope))
            return false;

    if (a.slots.size() != b.slots.size()) return false;
    for (const auto& slot : a.slots)
    {
        const auto* other = b.slot (slot.number);
        if (other == nullptr) return false;

        const auto* info = roll.slot (slot.number);
        const bool dynamics = bIsDumped && info != nullptr && info->type == "dynamics";
        if (! Llll::sameContent (slot.content, dynamics ? dynamicsAsInFile (other->content) : other->content, tolerance))
            return false;
    }
    return true;
}

bool sameBody (const Roll& a, const Roll& b)
{
    if (a.voices.size() != b.voices.size() || a.markers.size() != b.markers.size()) return false;

    for (size_t v = 0; v < a.voices.size(); ++v)
    {
        const auto& va = a.voices[v];
        const auto& vb = b.voices[v];
        if (va.flags != vb.flags || va.chords.size() != vb.chords.size()) return false;

        for (size_t c = 0; c < va.chords.size(); ++c)
        {
            const auto& ca = va.chords[c];
            const auto& cb = vb.chords[c];
            if (! about (ca.onset, cb.onset) || ca.flags != cb.flags || ca.notes.size() != cb.notes.size()) return false;
            for (size_t n = 0; n < ca.notes.size(); ++n)
                if (! sameNote (ca.notes[n], cb.notes[n], a, false)) return false;
        }
    }

    for (size_t m = 0; m < a.markers.size(); ++m)
        if (! about (a.markers[m].position, b.markers[m].position) || a.markers[m].role != b.markers[m].role
            || ! Llll::sameContent (a.markers[m].names, b.markers[m].names)
            || ! Llll::sameContent (a.markers[m].content, b.markers[m].content))
            return false;

    return true;
}

// --- One roll against bach ----------------------------------------------------------

void row (const juce::String& what, const juce::String& maket, const juce::String& bach, bool same)
{
    check (same, what);
    line ("| " + what + " | " + maket + " | " + bach + " | " + (same ? "yes" : "**NO**") + " |");
}

void testRoll (const juce::String& title, const juce::File& rollFile, const juce::File& answersFile, const juce::File& resavedFile)
{
    line ("### " + title);
    line();

    if (! rollFile.existsAsFile() || ! answersFile.existsAsFile())
    {
        check (false, title + ": the roll or bach's answers are missing from Temp");
        line ("**Not tested:** `" + rollFile.getFileName() + "` or `" + answersFile.getFileName() + "` is not in `Temp`.");
        line();
        return;
    }

    // The text into a list.
    const auto text = rollFile.loadFileAsString();
    Llll content;
    const auto textRead = maket::readLlllText (text, content);

    // The list into a roll.
    Roll roll;
    juce::StringArray remarks;
    const auto rollRead = textRead.wasOk() ? maket::readBachRoll (content, roll, remarks) : textRead;

    BachAnswers bach;
    const auto answersRead = readBachAnswers (answersFile, bach);

    if (rollRead.failed() || answersRead.failed())
    {
        check (false, title + ": could not be read");
        line ("**Could not be read:** " + (rollRead.failed() ? rollRead.getErrorMessage()
                                                               : "bach's answers: " + answersRead.getErrorMessage()));
        line();
        return;
    }

    juce::StringArray headerNames;
    for (const auto& e : roll.header) headerNames.add (e.name);

    line ("`" + rollFile.getFileName() + "`: " + juce::String (text.length()) + " characters, "
          + juce::String (content.countAtoms()) + " numbers and words, lists " + juce::String (content.depth() - 1)
          + " deep. Length of the music: " + juce::String (roll.length() / 1000.0, 1) + " s. "
          + "Header entries: " + headerNames.joinIntoString (", ") + "."
          + (remarks.isEmpty() ? juce::String() : " Remarks of the reader: " + remarks.joinIntoString (" ")));
    line();
    line ("| | Maket's reader | bach | The same |");
    line ("| --- | --- | --- | --- |");

    // Nothing lost by the text reader: all characters that are not spaces come back in order.
    {
        const auto written = content.toLine().fromFirstOccurrenceOf ("[", false, false).upToLastOccurrenceOf ("]", false, false);
        const bool same = text.removeCharacters (" \t\r\n") == written.removeCharacters (" \t\r\n");
        row ("Every character of the file that is not a space comes back, in order, when the list is written out", same ? "yes" : "no", "", same);
    }

    // Counts.
    std::vector<int> chords;
    std::vector<std::vector<int>> notes;
    for (const auto& v : roll.voices)
    {
        chords.push_back ((int) v.chords.size());
        std::vector<int> perChord;
        for (const auto& c : v.chords) perChord.push_back ((int) c.notes.size());
        notes.push_back (perChord);
    }
    int bachNotes = 0;
    for (const auto& v : bach.numNotes) for (int n : v) bachNotes += n;

    row ("Voices", juce::String ((int) roll.voices.size()), juce::String (bach.numVoices), (int) roll.voices.size() == bach.numVoices);
    row ("Chords per voice", joined (chords), joined (bach.numChords), chords == bach.numChords);
    row ("Notes", juce::String (roll.numNotes()), juce::String (bachNotes), roll.numNotes() == bachNotes);
    row ("Notes in each chord of each voice", "compared one by one", "", notes == bach.numNotes);

    // Markers against getmarker.
    {
        bool same = roll.markers.size() == bach.markers.size();
        for (size_t i = 0; same && i < roll.markers.size(); ++i)
        {
            const auto& mine = roll.markers[i];
            const auto& theirs = bach.markers[i];
            same = theirs.size() >= 3 && about (mine.position, theirs[0].value)
                   && Llll::sameContent (mine.names, theirs[1], tolerance)
                   && mine.role == theirs[2].text
                   && (theirs.size() < 4 || Llll::sameContent (mine.content, theirs[3], tolerance));
        }
        row ("Markers, each with its time, name, role and content", juce::String ((int) roll.markers.size()),
             juce::String ((int) bach.markers.size()), same);
    }

    // The dump: every note bach would play.
    {
        const int muted = roll.numMutedNotes();
        row ("Notes bach sends when it plays everything (a muted note is not sent)",
             juce::String (roll.numNotes() - muted) + " = " + juce::String (roll.numNotes()) + " - " + juce::String (muted) + " muted",
             juce::String ((int) bach.notes.size()), roll.numNotes() - muted == (int) bach.notes.size());

        std::vector<std::vector<std::vector<bool>>> used;
        for (const auto& v : roll.voices)
        {
            std::vector<std::vector<bool>> perVoice;
            for (const auto& c : v.chords) perVoice.emplace_back (c.notes.size(), false);
            used.push_back (perVoice);
        }

        int found = 0, equal = 0, channelRight = 0;
        bool ordered = true;
        double lastOnset = -1.0e300;

        for (const auto& dumped : bach.notes)
        {
            // Read the dumped chord with the same reader, as a roll of one voice with one chord.
            Llll asVoice, asRoll;
            asVoice.items.push_back (dumped.chord);
            asRoll.items.push_back (asVoice);
            Roll one;
            juce::StringArray ignored;
            if (maket::readBachRoll (asRoll, one, ignored).failed() || one.voices.empty()
                || one.voices[0].chords.empty() || one.voices[0].chords[0].notes.empty())
                continue;

            const Chord& theirChord = one.voices[0].chords[0];
            const Note& theirs = theirChord.notes[0];
            ordered = ordered && theirChord.onset >= lastOnset;
            lastOnset = theirChord.onset;

            const int v = dumped.voice - 1;
            if (v < 0 || v >= (int) roll.voices.size()) continue;
            if (roll.voices[(size_t) v].midiChannel == dumped.channel) ++channelRight;

            bool matched = false;
            const auto& voice = roll.voices[(size_t) v];
            for (size_t c = 0; ! matched && c < voice.chords.size(); ++c)
            {
                if (! about (voice.chords[c].onset, theirChord.onset)) continue;
                for (size_t n = 0; ! matched && n < voice.chords[c].notes.size(); ++n)
                {
                    if (used[(size_t) v][c][n] || ! about (voice.chords[c].notes[n].cents, theirs.cents)) continue;
                    used[(size_t) v][c][n] = true;
                    matched = true;
                    ++found;
                    if (sameNote (voice.chords[c].notes[n], theirs, roll, true) && voice.chords[c].flags == theirChord.flags) ++equal;
                }
            }
        }

        const int total = (int) bach.notes.size();
        row ("Sent notes found in Maket's roll, in the same voice at the same onset and pitch",
             juce::String (found), juce::String (total), found == total);
        row ("Of those, with the same duration, velocity, breakpoints and slot contents",
             juce::String (equal), juce::String (total), equal == total);
        row ("The channel bach sends with each note is the voice's MIDI channel in the header",
             juce::String (channelRight), juce::String (total), channelRight == total && ordered);

        // Markers in the dump.
        bool same = bach.dumpedMarkers.size() == roll.markers.size();
        for (size_t i = 0; same && i < roll.markers.size(); ++i)
        {
            const auto& mine = roll.markers[i];
            const auto& theirs = bach.dumpedMarkers[i];
            const bool nameless = mine.names.isList() && mine.names.size() == 0;
            same = about (mine.position, theirs.position) && mine.role == theirs.role
                   && (nameless ? theirs.names.isSymbol ("none") : Llll::sameContent (mine.names, theirs.names, tolerance));
        }
        row ("Markers bach sends, each with its time, name and role", juce::String ((int) roll.markers.size()),
             juce::String ((int) bach.dumpedMarkers.size()), same);
    }

    // The roll as bach saved it again.
    if (resavedFile.existsAsFile())
    {
        Roll again;
        juce::StringArray ignored;
        const auto r = maket::readBachRollFile (resavedFile, again, ignored);
        const bool same = r.wasOk() && sameBody (roll, again);

        juce::StringArray added;
        for (const auto& e : again.header)
            if (roll.headerEntry (e.name.toRawUTF8()) == nullptr) added.add (e.name);

        row ("The roll as bach saved it again reads to the same voices, chords, notes and markers",
             same ? "yes" : "no",
             resavedFile.loadFileAsString() == text ? "saved byte for byte the same"
                                                    : (added.isEmpty() ? juce::String ("header differs")
                                                                       : "bach added to the header: " + added.joinIntoString (", ")),
             same);
    }
    line();

    // What the roll holds, for the eye.
    juce::StringArray slotLines;
    for (const auto& info : roll.slotInfo)
        if (const int n = roll.numFilledSlots (info.number); n > 0)
            slotLines.add ("slot " + juce::String (info.number) + " " + info.name + " (" + info.type + ") in " + juce::String (n) + (n == 1 ? " note" : " notes"));

    line ("Read from it: " + juce::String (roll.numNotesWithBreakpoints()) + " notes glide, with "
          + juce::String (roll.numBreakpoints()) + " breakpoints between them; "
          + juce::String (roll.numFilledSlots()) + " slots are filled: " + slotLines.joinIntoString ("; ") + ".");
    line();
}

} // namespace

void testBachReading (const juce::File& temp)
{
    line ("## 3. Reading bach rolls");
    line();
    line ("Stage 1a: the list type bach's data comes in, the reader of bach's text, and the reader that makes a roll "
          "of it. The rules of the text are bach's own, taken from its source. Joost's two rolls are then checked "
          "against what bach itself says about them: its counts, and its dump of every note it would play.");
    line();

    line ("### The text reader");
    line();
    testWords();
    testSmallTexts();
    line();

    testRoll ("doodsbed final v1c",
              material (temp, "doodsbed final v1c.txt"),
              material (temp, "01a doodsbed roll stats.txt"),
              material (temp, "01b doodsbed roll resave after dump.txt"));

    testRoll ("mixed_roll_example_01h",
              material (temp, "mixed_roll_example_01h.txt"),
              material (temp, "02a mixed_roll_example_01h roll stats"),
              material (temp, "02b mixed_roll_example_01h resave after dump.txt"));
}
