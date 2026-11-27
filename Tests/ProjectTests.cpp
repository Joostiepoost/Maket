// ProjectTests.cpp — stage 3b: a project on disk and its library.
//
// A project is made in the work folder, Joost's rolls are imported, the
// project is opened anew, and what it holds is held against the files the
// rolls came from, byte for byte. Then renaming, removing, updating from
// the file, a damaged list, the miniature, and the list of recent projects.

#include <JuceHeader.h>
#include <cmath>
#include <set>

#include "TestReport.h"
#include "../Source/Model/Project.h"
#include "../Source/Model/WindowState.h"
#include "../Source/Formats/BachRoll.h"
#include "../Source/Notation/Miniature.h"

using namespace testreport;
using maket::Project;

void testProject (const juce::File& work, const juce::File& temp)
{
    line ("## 13. Projects and the library (stage 3b)");
    line();
    line ("A project is a folder; a note snippet in it is kept as the bach.roll text bach itself would save, with a small file beside it for what bach has no place for.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    const auto folder = work.getChildFile ("project test");
    folder.deleteRecursively();

    // --- Making and opening ---
    Project project;
    const bool made = Project::create (folder, 48000.0, 442.0, project).wasOk();
    Project second;
    const bool refusedTwice = Project::create (folder, 48000.0, 442.0, second).failed();
    Project wrongRate;
    const bool refusedRate = Project::create (work.getChildFile ("project test 2"), 44100.0, 442.0, wrongRate).failed();
    const bool laidOut = folder.getChildFile ("project.json").existsAsFile() && folder.getChildFile ("Snippets").isDirectory()
                         && folder.getChildFile ("Sounds").isDirectory() && Project::isProject (folder) && ! Project::isProject (work);
    {
        juce::StringArray remarks;
        Project opened;
        const bool reopened = Project::open (folder, opened, remarks).wasOk() && juce::approximatelyEqual (opened.getSampleRate(), 48000.0)
                              && juce::approximatelyEqual (opened.getStandardPitch(), 442.0) && opened.getSnippets().empty() && remarks.isEmpty();
        Project none;
        const bool noProject = Project::open (work, none, remarks).failed();
        checkRow (made && refusedTwice && refusedRate && laidOut && reopened && noProject,
                  "A new project is a folder with `project.json`, `Snippets` and `Sounds`; a second project in the same folder, and a sample rate other than 48000 or 96000, "
                  "are refused; opened anew it has its rate and its standard pitch; a folder without `project.json` is not taken for a project");
    }

    // --- Importing Joost's rolls ---
    juce::Array<juce::File> sources;
    for (const char* sub : { "support materials", "Roll view 2/roll txt" })
        for (const auto& f : temp.getChildFile (sub).findChildFiles (juce::File::findFiles, false, "*.txt"))
            sources.add (f);
    sources.sort();

    int imported = 0, same = 0;
    juce::StringArray different, unreadable, ids;
    for (const auto& source : sources)
    {
        juce::String id;
        juce::StringArray remarks;
        if (project.importRoll (source, id, remarks).failed()) { unreadable.add (source.getFileName()); continue; }
        ++imported;
        ids.add (id);
        if (project.rollFile (id).loadFileAsString() == source.loadFileAsString()) ++same;
        else different.add (source.getFileName());
    }
    checkRow (imported >= 4 && different.isEmpty(),
              juce::String (imported) + " of the " + juce::String (sources.size()) + " bach files in `Temp/support materials` and `Temp/Roll view 2/roll txt` are imported; "
              + juce::String (same) + " of them are kept in the project byte for byte as the file they came from"
              + (different.isEmpty() ? juce::String() : " (not: " + different.joinIntoString (", ") + ")")
              + (unreadable.isEmpty() ? juce::String() : "; not a bach.roll text: " + unreadable.joinIntoString (", ")));

    // --- Opened anew ---
    {
        juce::StringArray remarks;
        Project opened;
        bool ok = Project::open (folder, opened, remarks).wasOk() && (int) opened.getSnippets().size() == imported && remarks.isEmpty();
        std::set<juce::String> unique;
        for (int i = 0; ok && i < imported; ++i)
        {
            const auto& s = opened.getSnippets()[(size_t) i];
            unique.insert (s.id);
            maket::Roll roll;
            ok = s.id == ids[i] && s.id.length() == 8 && s.kind == "notes" && s.name.isNotEmpty() && s.sourcePath.isNotEmpty()
                 && opened.loadRoll (s.id, roll, remarks).wasOk() && s.notes == roll.numNotes() && s.voices == (int) roll.voices.size()
                 && maket::writeBachRollText (roll) == opened.rollFile (s.id).loadFileAsString();
        }
        checkRow (ok && (int) unique.size() == imported,
                  "The project opened anew holds the same snippets in the same order, each with its own ID, its name, where it came from, and its counts of voices and notes; "
                  "each one's notes, read and written again, are the text that is kept");
    }

    // --- Renaming, order, removing ---
    if (imported >= 3)
    {
        const auto first = ids[0], secondId = ids[1], third = ids[2];
        const bool renamed = project.rename (first, "  a new name  ").wasOk() && project.find (first)->name == "a new name" && project.rename (first, "   ").failed();
        project.move (first, 2);
        const bool moved = project.getSnippets()[2].id == first && project.getSnippets()[0].id == secondId;
        const bool removed = project.remove (third).wasOk() && project.find (third) == nullptr && ! project.rollFile (third).exists()
                             && folder.getChildFile ("Trash").getChildFile (third + ".roll.txt").existsAsFile()
                             && folder.getChildFile ("Trash").getChildFile (third + ".json").existsAsFile();
        juce::StringArray remarks;
        Project opened;
        const bool kept = Project::open (folder, opened, remarks).wasOk() && (int) opened.getSnippets().size() == imported - 1
                          && opened.getSnippets()[1].id == first && opened.getSnippets()[1].name == "a new name" && opened.find (third) == nullptr;
        checkRow (renamed && moved && removed && kept, "A snippet is renamed (an empty name is refused), put at another place in the list, and removed: its two files are then in the "
                                                       "project's `Trash` folder, not deleted; all of it is there when the project is opened anew");
    }
    else checkRow (false, "At least three rolls are there to try renaming and removing with");

    // --- Update from the file ---
    {
        const auto copy = work.getChildFile ("update test roll.txt");
        maket::Roll roll;
        juce::StringArray remarks;
        bool ok = temp.getChildFile ("support materials").getChildFile ("mixed_roll_example_01h.txt").copyFileTo (copy)
                  && maket::readBachRollFile (copy, roll, remarks).wasOk();
        juce::String id;
        ok = ok && project.importRoll (copy, id, remarks).wasOk();
        const int before = ok ? project.find (id)->notes : 0;

        // The file changes outside Maket: one chord fewer.
        if (ok)
        {
            for (auto& voice : roll.voices) if (! voice.chords.empty()) { voice.chords.pop_back(); break; }
            ok = maket::writeBachRollFile (copy, roll).wasOk();
        }
        const bool unchangedYet = ok && project.find (id)->notes == before;
        const bool updated = ok && project.updateFromFile (id, remarks).wasOk() && project.find (id)->notes == roll.numNotes() && roll.numNotes() < before
                             && project.rollFile (id).loadFileAsString() == copy.loadFileAsString();

        // A file that is no roll any more, and a file that is gone: the snippet stays as it was.
        const auto kept = ok ? project.rollFile (id).loadFileAsString() : juce::String();
        copy.replaceWithText ("this is no bach roll");
        const bool refusedBad = ok && project.updateFromFile (id, remarks).failed() && project.rollFile (id).loadFileAsString() == kept;
        copy.deleteFile();
        const bool refusedGone = ok && project.updateFromFile (id, remarks).failed() && project.rollFile (id).loadFileAsString() == kept;
        checkRow (unchangedYet && updated && refusedBad && refusedGone,
                  "Update from file: a snippet does not change when the file it came from changes; on request it takes the file's notes (" + juce::String (before) + " notes become "
                  + juce::String (roll.numNotes()) + "); when the file is no roll any more, or is gone, the snippet stays as it was");
        if (ok) project.remove (id);
    }

    // --- A damaged list, the standard pitch ---
    {
        const bool pitch = project.setStandardPitch (440.0).wasOk() && project.setStandardPitch (500.0).failed();
        const auto file = folder.getChildFile ("project.json");
        auto parsed = juce::JSON::parse (file.loadFileAsString());
        const int listed = parsed["snippets"].size();
        if (auto* object = parsed.getDynamicObject()) object->setProperty ("snippets", juce::Array<juce::var>());
        file.replaceWithText (juce::JSON::toString (parsed));

        juce::StringArray remarks;
        Project opened;
        const bool recovered = Project::open (folder, opened, remarks).wasOk() && (int) opened.getSnippets().size() == listed && listed > 0 && remarks.size() == listed
                               && juce::approximatelyEqual (opened.getStandardPitch(), 440.0);
        checkRow (pitch && recovered, "The standard pitch belongs to the project (set to 440 and kept; 500 refused). A `project.json` that has lost its list of snippets: "
                                      "every snippet in the folder is put back, with a remark for each (" + juce::String (listed) + ")");
    }

    // --- The miniature ---
    {
        maket::Roll doodsbed;
        juce::StringArray remarks;
        bool ok = maket::readBachRollFile (material (temp, "doodsbed final v1c.txt"), doodsbed, remarks).wasOk();
        const auto lines = maket::notation::miniatureOf (doodsbed);
        int expected = 0;
        for (const auto& v : doodsbed.voices)
            for (const auto& c : v.chords)
                for (const auto& n : c.notes)
                    expected += n.breakpoints.size() >= 2 ? (int) n.breakpoints.size() - 1 : 1;
        // A muted note is in the picture, paler than the same note unmuted.
        maket::Roll two;
        two.voices.resize (1);
        for (int i = 0; i < 2; ++i)
        {
            maket::Chord chord;
            chord.onset = 1000.0 * i;
            maket::Note note;
            note.cents = 6000.0; note.duration = 500.0; note.velocity = 100;
            if (i == 1) note.flags = maket::flagMuted;
            chord.notes.push_back (note);
            two.voices[0].chords.push_back (chord);
        }
        const auto pair = maket::notation::miniatureOf (two);
        const bool paler = pair.size() == 2 && pair[1].weight > 0.0f && std::abs (pair[1].weight - 0.35f * pair[0].weight) < 1.0e-5f;
        bool inside = ! lines.empty();
        for (const auto& l : lines)
            inside = inside && l.x0 >= 0.0f && l.x1 <= 1.0001f && l.x0 <= l.x1 && l.y0 >= 0.0f && l.y0 <= 1.0f && l.y1 >= 0.0f && l.y1 <= 1.0f;

        maket::Roll large;
        const bool haveLarge = maket::readBachRollFile (temp.getChildFile ("written by Maket").getChildFile ("Large roll by Maket.txt"), large, remarks).wasOk();
        const auto many = maket::notation::miniatureOf (large);
        maket::Roll empty;
        checkRow (ok && paler && (int) lines.size() == expected && inside && maket::notation::miniatureOf (empty).empty() && haveLarge && many.size() > 1500 && many.size() < 4500,
                  "The miniature of doodsbed: " + juce::String ((int) lines.size()) + " lines, one for every note (a muted one paler) and one for every stretch of a glide, "
                  "all inside the picture; the large roll's is thinned out to " + juce::String ((int) many.size()) + " lines; an empty roll has none");
    }

    // --- The projects opened last ---
    {
        const auto file = work.getChildFile ("window state recent test.json");
        file.deleteFile();
        maket::WindowState state (file);
        for (int i = 1; i <= 12; ++i) state.addRecentProject ("/projects/p" + juce::String (i));
        state.addRecentProject ("/projects/p5");
        const bool order = state.recentProjects().size() == 10 && state.recentProjects()[0] == "/projects/p5" && state.recentProjects()[1] == "/projects/p12"
                           && ! state.recentProjects().contains ("/projects/p2");
        state.removeRecentProject ("/projects/p12");
        state.save();
        maket::WindowState again (file);
        again.load();
        file.deleteFile();
        checkRow (order && again.recentProjects().size() == 9 && again.recentProjects()[0] == "/projects/p5" && again.recentProjects()[1] == "/projects/p11",
                  "The projects opened last: the newest first, each once, ten at most, one that is gone taken off the list; kept with the window's state");
    }

    folder.deleteRecursively();
    work.getChildFile ("project test 2").deleteRecursively();
    line();
}
