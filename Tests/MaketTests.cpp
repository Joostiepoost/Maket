// MaketTests.cpp — Maket's console test program (./build.sh tests).
//
// It builds the sources that hold no interface code (model, file formats,
// engines, notation) without the window, runs its checks on them and writes
// Measurements/report.md. Every build step adds its own section here.
//
//   1. The test program itself (stage 0b): version and stage, a work folder,
//      a file that reads back byte for byte, the report folder.
//   2. Settings (stage 0c): defaults, the settings file written and read
//      back, values that must be refused, a damaged file.
//   3. Reading bach rolls (stage 1a, in BachTests.cpp): the text reader,
//      and Joost's two rolls against bach's own counts and its dump.
//   4. Writing bach rolls (stage 1b, in BachWriteTests.cpp): the text
//      writer, and Joost's rolls read and written back byte for byte.
//   5. OSC (stage 1c, in OscTests.cpp): the bytes of a message, sending to
//      a listener inside this program, the punctuality of the send thread.
//   6. Timing tests (stage 1d, in TimingTests.cpp): the passages, the
//      reader of the Max patch's log, the comparison with known answers.
//   7. The conTimbre mapping (stages 1e and 1f.1, in MappingTests.cpp): a roll's
//      notes as commands for the ePlayer; the comparison with a Max log.
//   8. Test material: what is in the repository's Temp folder, and whether
//      the files the coming steps need are there.
//
// Options:  --out <folder>   where report.md is written (default: Measurements)
//
// The program ends with status 0 when every check passed and 1 otherwise.

#include <JuceHeader.h>
#include <cmath>
#include "../Source/Model/AppInfo.h"
#include "../Source/Model/Settings.h"
#include "../Source/Engine/ConTimbreMapping.h"
#include "../Source/Engine/TimingTest.h"
#include "../Source/Formats/BachRoll.h"
#include "TestReport.h"

#include <cstring>
#include <iostream>

using namespace testreport;

namespace
{

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
          "notation) without the window, runs checks on them and writes this report. This first section "
          "shows that the program builds, runs, can work with files and reports a result. Every stage "
          "after it adds its own section.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    // The version: three numbers separated by points.
    const auto parts = juce::StringArray::fromTokens (version, ".", "");
    bool versionOk = parts.size() == 3;
    for (const auto& p : parts) versionOk = versionOk && isNumber (p);
    checkRow (versionOk, "The version reads as three numbers (`" + version + "`)");

    // The stage: a step number, then a letter for the sub-stage (0b, 1a, 12c ...), and where a
    // sub-stage is built in parts a point and the number of the part (1f.1).
    const auto digits = stage.initialSectionContainingOnly ("0123456789");
    const auto letters = stage.substring (digits.length()).upToFirstOccurrenceOf (".", false, false);
    const auto part = stage.fromFirstOccurrenceOf (".", false, false);
    checkRow (digits.isNotEmpty() && letters.length() <= 1 && letters.containsOnly ("abcdefghijklmnopqrstuvwxyz")
              && (! stage.containsChar ('.') || (letters.length() == 1 && isNumber (part))),
              "The build stage reads as a step number and a letter, with the number of a part where there is one (`" + stage + "`)");

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

// --- 2. Settings ----------------------------------------------------------------

void testSettings (const juce::File& work)
{
    using maket::Settings;
    using maket::SettingKind;

    line ("## 2. Settings");
    line();
    line ("Whatever differs from one Mac to another is a setting with a default, kept in one JSON file. "
          "The checks below work on a settings file in the test's own work folder; the real settings file "
          "of this Mac is never touched by this program.");
    line();
    line ("The settings and their defaults:");
    line();
    line ("| Setting | Kind | Default | Name in the file |");
    line ("| --- | --- | --- | --- |");
    for (const auto& s : Settings::all())
    {
        const juce::String kind = s.kind == SettingKind::folder ? "folder" : s.kind == SettingKind::host ? "host"
                                : s.kind == SettingKind::frequency ? "frequency" : "port";
        const juce::String def (s.defaultValue);
        line ("| " + juce::String (s.group) + ": " + s.label + " | " + kind + " | "
              + (def.isEmpty() ? "not set" : "`" + def + "`") + " | `" + s.key + "` |");
    }
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    // The table itself.
    {
        juce::StringArray keys;
        bool sound = true;
        for (const auto& s : Settings::all())
        {
            juce::String tidied;
            sound = sound && juce::String (s.key).isNotEmpty() && juce::String (s.label).isNotEmpty()
                          && juce::String (s.help).isNotEmpty() && ! keys.contains (s.key)
                          && Settings::validate (s, s.defaultValue, tidied).wasOk() && tidied == s.defaultValue;
            keys.add (s.key);
        }
        checkRow (sound, "Every setting has its own name, a label, a help text and a default that is valid");
    }

    const auto file = work.getChildFile ("settings test").getChildFile ("settings.json");

    // Starting without a file.
    {
        Settings fresh (file);
        bool defaults = fresh.load().wasOk();
        for (const auto& s : Settings::all())
            defaults = defaults && fresh.get (s.key) == s.defaultValue && fresh.isDefault (s.key);
        checkRow (defaults && ! file.exists(), "Without a settings file every setting has its default, and no file is made");
    }

    // Changed, saved, read back by a new object: the restart in miniature.
    const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    const juce::String outside ("/Volumes/Sounds/Maket exports");      // need not exist
    {
        Settings a (file);
        bool ok = a.set ("projectsFolder", home.getChildFile ("Music/Maket projects").getFullPathName()).wasOk()
               && a.set ("exportsFolder", outside + "/").wasOk()
               && a.set ("contimbreDataFolder", "  ~/conTimbre/search_data  ").wasOk()
               && a.set ("ePlayerHost", "studio-mac.local").wasOk()
               && a.set ("ePlayerPort", "07400").wasOk()
               && a.save().wasOk();

        Settings b (file);
        ok = ok && b.load().wasOk();
        for (const auto& s : Settings::all())
            ok = ok && a.get (s.key) == b.get (s.key);

        checkRow (ok && b.getInt ("ePlayerPort") == 7400 && b.get ("ePlayerHost") == "studio-mac.local"
                     && b.isDefault ("importsFolder") && ! b.isDefault ("projectsFolder"),
                  "Five changed settings are written to the file and read back the same by a fresh start");

        checkRow (b.get ("projectsFolder") == "~/Music/Maket projects"
                     && b.getFolder ("projectsFolder") == home.getChildFile ("Music/Maket projects")
                     && b.get ("contimbreDataFolder") == "~/conTimbre/search_data"
                     && b.get ("exportsFolder") == outside && b.getFolder ("exportsFolder") == juce::File (outside),
                  "A folder inside the home folder is kept as `~/...` and leads back to the same folder; one outside it keeps its whole path");
    }

    // The file is plain JSON that a person can read.
    {
        juce::var root;
        const auto text = file.loadFileAsString();
        const bool parsed = juce::JSON::parse (text, root).wasOk() && root.getDynamicObject() != nullptr;
        bool complete = parsed && (int) root["maket-settings"] == 1 && root["ePlayerPort"].isInt();
        for (const auto& s : Settings::all())
            complete = complete && root.hasProperty (s.key);
        checkRow (complete && text.contains ("\n") && ! text.contains ("\r"),
                  "The file is JSON on several lines and holds every setting by name");
    }

    // What must be refused.
    {
        Settings c (file);
        c.load();
        const auto port = c.get ("ePlayerPort"), host = c.get ("ePlayerHost"), projects = c.get ("projectsFolder");

        bool refused = true;
        for (const auto* bad : { "0", "65536", "-5", "46001.5", "port", "" })
            refused = refused && c.set ("ePlayerPort", bad).failed();
        for (const auto* bad : { "", "my mac", "127.0.0.1/osc" })
            refused = refused && c.set ("ePlayerHost", bad).failed();
        for (const auto* bad : { "", "Documents/Maket", "~joost/Maket" })
            refused = refused && c.set ("projectsFolder", bad).failed();
        refused = refused && c.set ("noSuchSetting", "1").failed();

        checkRow (refused && c.get ("ePlayerPort") == port && c.get ("ePlayerHost") == host
                     && c.get ("projectsFolder") == projects,
                  "Thirteen values that make no sense (port 0 or 65536, a host with a space, a folder that is not a whole path ...) are refused and change nothing");

        checkRow (c.set ("contimbreDataFolder", "").wasOk() && c.set ("ePlayerPort", "1").wasOk()
                     && c.set ("ePlayerPort", "65535").wasOk() && c.set ("ePlayerHost", "localhost").wasOk(),
                  "The edges are accepted: ports 1 and 65535, the host `localhost`, conTimbre's folder left unset");
    }

    // The standard pitch (stage 1f.8).
    {
        Settings t (file);
        t.load();
        bool ok = std::abs (t.getDouble ("standardPitch") - 442.0) < 1.0e-9 && t.isDefault ("standardPitch");
        ok = ok && t.set ("standardPitch", " 440,50 ").wasOk() && t.get ("standardPitch") == "440.5" && t.save().wasOk();
        Settings u (file);
        juce::var root;
        ok = ok && u.load().wasOk() && std::abs (u.getDouble ("standardPitch") - 440.5) < 1.0e-9
                && juce::JSON::parse (file.loadFileAsString(), root).wasOk() && root["standardPitch"].isDouble();
        for (const auto* bad : { "", "379.9", "480.1", "44o", "4.4.2", "-442", "A" })
            ok = ok && u.set ("standardPitch", bad).failed();
        ok = ok && u.get ("standardPitch") == "440.5" && u.set ("standardPitch", "380").wasOk() && u.set ("standardPitch", "480.000").wasOk()
                && u.get ("standardPitch") == "480";
        u.reset ("standardPitch");
        ok = ok && u.save().wasOk();
        checkRow (ok, "The standard pitch is 442 Hz by default, takes 380 to 480 with up to three decimals (a comma is read as a point), "
                      "is kept in the file as a number, and refuses seven values that make no sense");
    }

    // Back to the default.
    {
        Settings d (file);
        d.load();
        d.reset ("ePlayerPort");
        d.reset ("projectsFolder");
        checkRow (d.getInt ("ePlayerPort") == 46001 && d.isDefault ("projectsFolder") && ! d.isDefault ("ePlayerHost"),
                  "A setting put back to its default has it again, and the others keep their values");
    }

    // A file from a newer Maket: what this version does not know is kept.
    {
        file.replaceWithText ("{ \"maket-settings\": 1, \"ePlayerPort\": 7500, \"somethingNewer\": { \"a\": [1, 2, 3] } }\n",
                              false, false, "\n");
        Settings e (file);
        bool ok = e.load().wasOk() && e.getInt ("ePlayerPort") == 7500 && e.save().wasOk();
        juce::var root;
        ok = ok && juce::JSON::parse (file.loadFileAsString(), root).wasOk()
                && root["somethingNewer"]["a"].size() == 3 && (int) root["ePlayerPort"] == 7500;
        checkRow (ok, "An entry this version does not know is left in the file when it is written again");
    }

    // A wrong value in the file, and a file that is not JSON at all.
    {
        file.replaceWithText ("{ \"ePlayerPort\": 99999, \"ePlayerHost\": \"10.0.0.7\" }\n", false, false, "\n");
        Settings f (file);
        const auto result = f.load();
        checkRow (result.failed() && f.getInt ("ePlayerPort") == 46001 && f.get ("ePlayerHost") == "10.0.0.7",
                  "A value in the file that makes no sense is replaced by its default and reported; the rest is read");

        file.replaceWithText ("this is not a settings file {", false, false, "\n");
        Settings g (file);
        const auto damaged = g.load();
        bool defaults = true;
        for (const auto& s : Settings::all()) defaults = defaults && g.isDefault (s.key);
        checkRow (damaged.failed() && defaults && ! file.exists()
                     && file.getSiblingFile ("settings (unreadable).json").existsAsFile(),
                  "A file that cannot be read is set aside under another name, reported, and the defaults are used");
    }

    line();
    const auto real = Settings::defaultFile();
    line ("On this machine the app keeps its settings in `" + real.getFullPathName() + "` ("
          + (real.existsAsFile() ? "the file exists" : "no file yet: nothing was changed in the app so far") + ").");
    line();
}

// --- 8. Test material ----------------------------------------------------------

/** A file name without extension, case, spaces, underscores and hyphens, so
    that "mixed_roll_example_01h.txt" and "Mixed roll example 01h" compare equal. */
juce::String plainName (const juce::String& name)
{
    return name.upToLastOccurrenceOf (".", false, false).toLowerCase().removeCharacters (" _-");
}

void testMaterial (const juce::File& temp)
{
    line ("## 10. Test material");
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

    // What the coming stages need.
    struct Needed { const char* name; const char* what; };
    const Needed needed[] =
    {
        { "doodsbed final v1c",                          "1a and 1b: a roll to read and to write back" },
        { "mixed_roll_example_01h",                      "1a and 1b: a roll to read and to write back" },
        { "01a doodsbed roll stats",                     "1a: bach's own counts and its dump of every note" },
        { "02a mixed_roll_example_01h roll stats",       "1a: bach's own counts and its dump of every note" },
        { "01b doodsbed roll resave after dump",         "1b: the roll as bach saves it again" },
        { "02b mixed_roll_example_01h resave after dump", "1b: the roll as bach saves it again" },
        { "1c maket log",                                "1d: Maket's send log of 22 Nov, to check the measuring" },
        { "1c max log",                                  "1d: the Max patch's log of 22 Nov, to check the measuring" }
    };

    line ("Needed for the coming stages:");
    line();
    line ("| File | Needed for | Found |");
    line ("| --- | --- | --- |");
    for (const auto& n : needed)
    {
        juce::String found;
        for (const auto& f : files)
            if (plainName (f.getFileName()) == plainName (n.name)) { found = f.getRelativePathFrom (temp); break; }

        line ("| " + juce::String (n.name) + " | " + n.what + " | "
              + (found.isNotEmpty() ? "yes, as `" + found + "`" : "not yet") + " |");
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

        // MaketTests --compare <roll file> <log of the logging patch>: no tests, only the
        // comparison of the conTimbre test page, for every voice of the roll that has a name.
        if (a == "--compare" && i + 2 < argc)
        {
            const auto cwd = juce::File::getCurrentWorkingDirectory();
            maket::Roll roll;
            juce::StringArray remarks;
            const auto read = maket::readBachRollFile (cwd.getChildFile (juce::String::fromUTF8 (argv[i + 1])), roll, remarks);
            if (read.failed()) { std::cout << read.getErrorMessage() << std::endl; return 1; }

            const auto log = maket::timing::readCollLog (cwd.getChildFile (juce::String::fromUTF8 (argv[i + 2])).loadFileAsString());
            const auto comparison = maket::contimbre::compareWithLog (maket::contimbre::messagesForRoll (roll), log);
            std::cout << maket::contimbre::describe (comparison) << std::endl;
            return comparison.usable && comparison.numOnlyInLog == 0 && comparison.numOnlyInMaket == 0 ? 0 : 1;
        }

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
    std::cout << "Settings..." << std::endl;             testSettings (work);
    std::cout << "Reading bach rolls..." << std::endl;   testBachReading (sourceDir.getChildFile ("Temp"));
    std::cout << "Writing bach rolls..." << std::endl;   testBachWriting (sourceDir.getChildFile ("Temp"));
    std::cout << "OSC..." << std::endl;                  testOsc();
    std::cout << "Timing tests..." << std::endl;         testTiming (sourceDir.getChildFile ("Temp"));
    std::cout << "The conTimbre mapping..." << std::endl; testConTimbreMapping (sourceDir.getChildFile ("Temp"));
    std::cout << "Notation..." << std::endl;             testNotation (sourceDir.getChildFile ("Temp"));
    std::cout << "Playing..." << std::endl;              testPlaying (sourceDir.getChildFile ("Temp"));
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
