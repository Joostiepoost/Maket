// Project.cpp — see Project.h.

#include "Project.h"
#include "../Formats/BachRoll.h"

namespace maket
{

namespace
{
    bool writeJson (const juce::File& file, const juce::var& root)
    {
        return file.getParentDirectory().createDirectory().wasOk()
               && file.replaceWithText (juce::JSON::toString (root) + "\n", false, false, "\n");
    }

    juce::String cleanName (juce::String name)
    {
        name = name.trim();
        return name.isEmpty() ? juce::String ("untitled") : name;
    }
}

bool Project::isProject (const juce::File& f)
{
    const auto file = f.getChildFile ("project.json");
    if (! file.existsAsFile()) return false;
    const auto parsed = juce::JSON::parse (file.loadFileAsString());
    auto* object = parsed.getDynamicObject();
    return object != nullptr && object->hasProperty ("maket-project");
}

juce::Result Project::create (const juce::File& f, double rate, double pitch, Project& result)
{
    if (f == juce::File())                  return juce::Result::fail ("No folder was given for the project.");
    if (isProject (f))                      return juce::Result::fail ("That folder already holds a project: " + f.getFullPathName());
    if (f.existsAsFile())                   return juce::Result::fail ("There is a file of that name: " + f.getFullPathName());
    if (! sampleRateAllowed (rate))         return juce::Result::fail ("A project's sample rate is 48000 or 96000.");
    if (! f.createDirectory().wasOk())      return juce::Result::fail ("The folder could not be made: " + f.getFullPathName());

    result = Project();
    result.folder = f;
    result.sampleRate = rate;
    result.standardPitch = pitch;
    for (const char* sub : { "Snippets", "Sounds", "Orchestra" })
        if (! f.getChildFile (sub).createDirectory().wasOk())
            return juce::Result::fail ("A folder inside the project could not be made: " + juce::String (sub));
    return result.writeProjectFile();
}

juce::Result Project::open (const juce::File& f, Project& result, juce::StringArray& remarks)
{
    const auto file = f.getChildFile ("project.json");
    if (! file.existsAsFile()) return juce::Result::fail ("This folder holds no Maket project (no project.json): " + f.getFullPathName());
    const auto parsed = juce::JSON::parse (file.loadFileAsString());
    auto* object = parsed.getDynamicObject();
    if (object == nullptr || ! object->hasProperty ("maket-project")) return juce::Result::fail ("project.json is not a Maket project file.");

    result = Project();
    result.folder = f;
    const double rate = (double) object->getProperty ("sampleRate");
    result.sampleRate = sampleRateAllowed (rate) ? rate : defaultSampleRate;
    if (! sampleRateAllowed (rate)) remarks.add ("The project's sample rate was not 48000 or 96000; 48000 is used.");
    const double pitch = (double) object->getProperty ("standardPitch");
    result.standardPitch = pitch >= 380.0 && pitch <= 480.0 ? pitch : 442.0;

    // The library in its order; then whatever lies in Snippets without being listed (nothing gets lost by a damaged list).
    juce::StringArray ids;
    if (auto* listed = object->getProperty ("snippets").getArray())
        for (const auto& v : *listed) ids.addIfNotAlreadyThere (v.toString());
    for (const auto& found : f.getChildFile ("Snippets").findChildFiles (juce::File::findFiles, false, "*.json"))
        if (! ids.contains (found.getFileNameWithoutExtension()))
        {
            ids.add (found.getFileNameWithoutExtension());
            remarks.add ("A snippet that was not in the project's list was put back at the end: " + found.getFileName());
        }

    for (const auto& id : ids)
    {
        const auto parsedInfo = juce::JSON::parse (result.infoFile (id).loadFileAsString());
        auto* info = parsedInfo.getDynamicObject();
        if (info == nullptr || ! info->hasProperty ("maket-snippet"))
        {
            remarks.add ("A snippet of the project's list has no readable file and is left out: " + id);
            continue;
        }
        SnippetInfo s;
        s.id = id;
        s.kind = info->getProperty ("kind").toString();
        s.name = cleanName (info->getProperty ("name").toString());
        s.added = info->getProperty ("added").toString();
        if (auto* source = info->getProperty ("source").getDynamicObject())
        {
            s.sourcePath = source->getProperty ("path").toString();
            s.sourceModified = (juce::int64) source->getProperty ("modified");
        }
        if (s.kind == "notes")
        {
            Roll roll;
            juce::StringArray rollRemarks;
            if (readBachRollFile (result.rollFile (id), roll, rollRemarks).wasOk()) result.measure (s, roll);
            else remarks.add ("The notes of \"" + s.name + "\" could not be read (" + result.rollFile (id).getFileName() + ").");
        }
        result.snippets.push_back (std::move (s));
    }
    return juce::Result::ok();
}

juce::Result Project::writeProjectFile() const
{
    auto* object = new juce::DynamicObject();
    const juce::var root (object);
    object->setProperty ("maket-project", 1);
    object->setProperty ("sampleRate", sampleRate);
    object->setProperty ("standardPitch", standardPitch);
    juce::Array<juce::var> ids;
    for (const auto& s : snippets) ids.add (s.id);
    object->setProperty ("snippets", ids);
    return writeJson (folder.getChildFile ("project.json"), root) ? juce::Result::ok()
                                                                    : juce::Result::fail ("project.json could not be written in " + folder.getFullPathName());
}

juce::Result Project::writeInfo (const SnippetInfo& s) const
{
    auto* object = new juce::DynamicObject();
    const juce::var root (object);
    object->setProperty ("maket-snippet", 1);
    object->setProperty ("kind", s.kind);
    object->setProperty ("id", s.id);
    object->setProperty ("name", s.name);
    object->setProperty ("added", s.added);
    auto* source = new juce::DynamicObject();
    source->setProperty ("path", s.sourcePath);
    source->setProperty ("modified", s.sourceModified);
    object->setProperty ("source", juce::var (source));
    return writeJson (infoFile (s.id), root) ? juce::Result::ok() : juce::Result::fail ("The snippet's file could not be written: " + infoFile (s.id).getFileName());
}

juce::Result Project::setStandardPitch (double hz)
{
    if (hz < 380.0 || hz > 480.0) return juce::Result::fail ("The standard pitch lies between 380 and 480 Hz.");
    standardPitch = hz;
    return writeProjectFile();
}

const SnippetInfo* Project::find (const juce::String& id) const
{
    for (const auto& s : snippets) if (s.id == id) return &s;
    return nullptr;
}

SnippetInfo* Project::findMutable (const juce::String& id)
{
    for (auto& s : snippets) if (s.id == id) return &s;
    return nullptr;
}

juce::String Project::newId() const
{
    juce::Random random;
    for (;;)
    {
        const auto id = juce::String::toHexString (random.nextInt()).paddedLeft ('0', 8).substring (0, 8);
        if (find (id) == nullptr && ! infoFile (id).exists() && ! rollFile (id).exists()) return id;
    }
}

void Project::measure (SnippetInfo& s, const Roll& roll) const
{
    s.voices = (int) roll.voices.size();
    s.notes = roll.numNotes();
    s.length = roll.length();
}

juce::Result Project::importRoll (const juce::File& bachFile, juce::String& idOut, juce::StringArray& remarks)
{
    if (! isOpen()) return juce::Result::fail ("No project is open.");
    Roll roll;
    const auto read = readBachRollFile (bachFile, roll, remarks);
    if (read.failed()) return read;

    SnippetInfo s;
    s.id = newId();
    s.name = cleanName (bachFile.getFileNameWithoutExtension());
    s.sourcePath = bachFile.getFullPathName();
    s.sourceModified = bachFile.getLastModificationTime().toMilliseconds();
    s.added = juce::Time::getCurrentTime().toISO8601 (true);
    measure (s, roll);

    if (! rollFile (s.id).getParentDirectory().createDirectory().wasOk()) return juce::Result::fail ("The project's Snippets folder could not be made.");
    const auto wrote = writeBachRollFile (rollFile (s.id), roll);
    if (wrote.failed()) return wrote;
    const auto info = writeInfo (s);
    if (info.failed()) return info;

    idOut = s.id;
    snippets.push_back (std::move (s));
    return writeProjectFile();
}

juce::Result Project::updateFromFile (const juce::String& id, juce::StringArray& remarks)
{
    auto* s = findMutable (id);
    if (s == nullptr) return juce::Result::fail ("That snippet is not in the library.");
    if (s->sourcePath.isEmpty()) return juce::Result::fail ("\"" + s->name + "\" was not imported from a file.");
    const juce::File source (s->sourcePath);
    if (! source.existsAsFile()) return juce::Result::fail ("The file \"" + s->name + "\" came from is no longer there: " + s->sourcePath);

    Roll roll;
    const auto read = readBachRollFile (source, roll, remarks);
    if (read.failed()) return read;             // the snippet stays as it was
    const auto wrote = writeBachRollFile (rollFile (id), roll);
    if (wrote.failed()) return wrote;
    s->sourceModified = source.getLastModificationTime().toMilliseconds();
    measure (*s, roll);
    return writeInfo (*s);
}

juce::Result Project::rename (const juce::String& id, const juce::String& newName)
{
    auto* s = findMutable (id);
    if (s == nullptr) return juce::Result::fail ("That snippet is not in the library.");
    if (newName.trim().isEmpty()) return juce::Result::fail ("A snippet needs a name.");
    s->name = newName.trim();
    return writeInfo (*s);
}

juce::Result Project::remove (const juce::String& id)
{
    const auto* s = find (id);
    if (s == nullptr) return juce::Result::fail ("That snippet is not in the library.");

    const auto trash = folder.getChildFile ("Trash");
    if (! trash.createDirectory().wasOk()) return juce::Result::fail ("The project's Trash folder could not be made.");
    for (const auto& file : { infoFile (id), rollFile (id) })
        if (file.existsAsFile() && ! file.moveFileTo (trash.getNonexistentChildFile (file.getFileNameWithoutExtension(), file.getFileExtension(), false)))
            return juce::Result::fail ("A file of the snippet could not be moved to the Trash folder: " + file.getFileName());

    snippets.erase (std::remove_if (snippets.begin(), snippets.end(), [&id] (const SnippetInfo& x) { return x.id == id; }), snippets.end());
    return writeProjectFile();
}

void Project::move (const juce::String& id, int newIndex)
{
    for (size_t i = 0; i < snippets.size(); ++i)
        if (snippets[i].id == id)
        {
            auto s = std::move (snippets[i]);
            snippets.erase (snippets.begin() + (std::ptrdiff_t) i);
            snippets.insert (snippets.begin() + juce::jlimit (0, (int) snippets.size(), newIndex), std::move (s));
            writeProjectFile();
            return;
        }
}

juce::Result Project::loadRoll (const juce::String& id, Roll& roll, juce::StringArray& remarks) const
{
    if (find (id) == nullptr) return juce::Result::fail ("That snippet is not in the library.");
    return readBachRollFile (rollFile (id), roll, remarks);
}

juce::Result Project::saveRoll (const juce::String& id, const Roll& roll)
{
    auto* s = findMutable (id);
    if (s == nullptr) return juce::Result::fail ("That snippet is not in the library.");
    const auto wrote = writeBachRollFile (rollFile (id), roll);
    if (wrote.wasOk()) measure (*s, roll);
    return wrote;
}

} // namespace maket
