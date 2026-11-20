// MaketTests.cpp — Maket's console test program (./build.sh tests).
//
// It builds the sources that hold no interface code (model, file formats,
// engines, notation) without the window, runs its checks on them and writes
// Measurements/report.md. Every build step adds its own section here.
//
//   1. The test program itself (stage 0b): version and stage, a work folder,
//      a file that reads back byte for byte, the report folder.
//   2. Test material: what is in the repository's Temp folder, and whether
//      the files the coming step needs are there.
//
// Options:  --out <folder>   where report.md is written (default: Measurements)
//
// The program ends with status 0 when every check passed and 1 otherwise.

#include <JuceHeader.h>
#include "../Source/Model/AppInfo.h"

#include <cstring>
#include <iostream>

namespace
{

juce::String md;          // the report, in Markdown
int checks = 0;
int failures = 0;

void line (const juce::String& s = {})       { md << s << "\n"; }

/** Counts a check; a failed one is also printed in Terminal. */
bool check (bool ok, const juce::String& what)
{
    ++checks;
    if (! ok) { ++failures; std::cout << "  FAIL: " << what << std::endl; }
    return ok;
}

/** A check that is also a row of the table being written. */
void checkRow (bool ok, const juce::String& what)
{
    check (ok, what);
    line ("| " + what + " | " + (ok ? "yes" : "**NO**") + " |");
}

bool isNumber (const juce::String& s)
{
    return s.isNotEmpty() && s.containsOnly ("0123456789");
}

// --- 1. The test program itself ----------------------------------------------

void testProgram (const juce::File& work, const juce::File& outDir)
{
    const auto& app = maket::appInfo();
    const juce::String version (app.version), stage (app.stage);

    line ("## 1. The test program");
    line();
    line ("This program builds Maket's sources that hold no interface code (model, file formats, engines, "
          "notation) without the window, runs checks on them and writes this report. At stage 0b there is "
          "almost nothing to check yet: this section shows that the program builds, runs, can work with "
          "files and reports a result. From step 1a on, every step adds its own section.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    // The version: three numbers separated by points.
    const auto parts = juce::StringArray::fromTokens (version, ".", "");
    bool versionOk = parts.size() == 3;
    for (const auto& p : parts) versionOk = versionOk && isNumber (p);
    checkRow (versionOk, "The version reads as three numbers (`" + version + "`)");

    // The stage: a step number, then a letter for the sub-stage (0b, 1a, 12c ...).
    const auto digits = stage.initialSectionContainingOnly ("0123456789");
    const auto letters = stage.substring (digits.length());
    checkRow (digits.isNotEmpty() && letters.length() <= 1 && letters.containsOnly ("abcdefghijklmnopqrstuvwxyz"),
              "The build stage reads as a step number and a letter (`" + stage + "`)");

    // A work folder in the system's temporary folder, used by the file tests of later steps.
    {
        const auto folder = work.getChildFile ("a folder");
        const bool made = folder.createDirectory().wasOk() && folder.isDirectory();
        const bool gone = folder.deleteRecursively() && ! folder.exists();
        checkRow (work.isDirectory() && made && gone,
                  "A work folder can be made in the system's temporary folder and removed again");
    }

    // A file must come back exactly as written: accents, both kinds of line
    // ending, a tab, and no line break at the end. (bach files are written
    // back unchanged at step 1b; this is the ground under that.)
    {
        juce::MemoryOutputStream bytes;
        for (int i = 0; i < 100; ++i)
        {
            bytes << "voice " << i << "\t" << 6000.25 + i << "\r\n";
            bytes << juce::String::fromUTF8 ("doodsbed \xe2\x80\x94 \xc3\xa9t\xc3\xa9 \xe2\x99\xaf") << "\n";
        }
        bytes << "no line break at the end";

        const auto file = work.getChildFile ("byte for byte.txt");
        const bool written = file.replaceWithData (bytes.getData(), bytes.getDataSize());

        juce::MemoryBlock back;
        const bool read = file.loadFileAsData (back);
        const bool same = written && read
                          && back.getSize() == bytes.getDataSize()
                          && std::memcmp (back.getData(), bytes.getData(), bytes.getDataSize()) == 0;

        checkRow (same, "A text file of " + juce::String ((int) bytes.getDataSize())
                        + " bytes with accents and both kinds of line ending reads back byte for byte");
    }

    checkRow (outDir.isDirectory() && outDir.hasWriteAccess(), "The report folder can be written");
    line();
}

// --- 2. Test material ----------------------------------------------------------

/** A file name without extension, case, spaces, underscores and hyphens, so
    that "mixed_roll_example_01h.txt" and "Mixed roll example 01h" compare equal. */
juce::String plainName (const juce::String& name)
{
    return name.upToLastOccurrenceOf (".", false, false).toLowerCase().removeCharacters (" _-");
}

void testMaterial (const juce::File& temp)
{
    line ("## 2. Test material");
    line();
    line ("The files Joost puts in the repository's `Temp` folder are what the coming steps are tested on. "
          "This section lists what the program finds there; nothing is read yet. A missing file is not a "
          "failed check at this stage.");
    line();

    juce::Array<juce::File> files;
    if (temp.isDirectory())
        for (const auto& entry : juce::RangedDirectoryIterator (temp, true, "*", juce::File::findFiles))
        {
            const auto f = entry.getFile();
            if (f.getFileName().startsWithChar ('.')) continue;                               // .DS_Store and the like
            if (f.getParentDirectory() == temp && f.getFileName() == "README.md") continue;   // the folder's own note
            files.add (f);
        }
    files.sort();

    if (! temp.isDirectory())
        line ("The folder `Temp` was not found at `" + temp.getFullPathName() + "`.");
    else if (files.isEmpty())
        line ("`Temp` holds no files yet (apart from its README).");
    else
    {
        line ("| File in `Temp` | Size |");
        line ("| --- | --- |");
        for (const auto& f : files)
            line ("| `" + f.getRelativePathFrom (temp) + "` | " + juce::File::descriptionOfSizeInBytes (f.getSize()) + " |");
    }
    line();

    // What step 1a needs.
    const char* needed[] = { "doodsbed final v1c", "mixed_roll_example_01h" };

    line ("Needed for step 1a (the bach text reader):");
    line();
    line ("| File | Found |");
    line ("| --- | --- |");
    for (const auto* name : needed)
    {
        juce::String found;
        for (const auto& f : files)
            if (plainName (f.getFileName()) == plainName (name)) { found = f.getRelativePathFrom (temp); break; }

        line ("| " + juce::String (name) + " | " + (found.isNotEmpty() ? "yes, as `" + found + "`" : "not yet") + " |");
    }
    line();
}

} // namespace

int main (int argc, char** argv)
{
    juce::File outDir = juce::File::getCurrentWorkingDirectory().getChildFile ("Measurements");
    for (int i = 1; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        if (a == "--out" && i + 1 < argc)
            outDir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++i]));
    }
    outDir.createDirectory();

    auto work = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("MaketTests");
    work.deleteRecursively();
    work.createDirectory();

    const auto& app = maket::appInfo();
    const juce::File sourceDir (juce::String::fromUTF8 (MAKET_SOURCE_DIR));

    md << "# Maket: test report\n\n";
    md << app.name << " " << app.version << ", stage " << app.stage << ". "
       << "Machine: " << juce::SystemStats::getCpuModel() << ", " << juce::SystemStats::getOperatingSystemName()
       << ". Date: " << juce::Time::getCurrentTime().toString (true, true) << ".\n\n";

    std::cout << app.name << " " << app.version << ", stage " << app.stage << std::endl;
    std::cout << "The test program..." << std::endl;     testProgram (work, outDir);
    std::cout << "Test material..." << std::endl;        testMaterial (sourceDir.getChildFile ("Temp"));

    work.deleteRecursively();

    const juce::String verdict = failures == 0
        ? "All " + juce::String (checks) + " checks passed."
        : juce::String (failures) + " of " + juce::String (checks) + " checks FAILED.";

    md << verdict << "\n";
    const auto report = outDir.getChildFile ("report.md");
    if (! report.replaceWithData (md.toRawUTF8(), md.getNumBytesAsUTF8()))
    {
        std::cout << "The report could not be written to " << report.getFullPathName() << std::endl;
        return 1;
    }

    std::cout << verdict << std::endl;
    return failures == 0 ? 0 : 1;
}
