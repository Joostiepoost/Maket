// Settings.cpp

#include "Settings.h"

namespace maket
{

namespace
{
    const char* const formatKey = "maket-settings";     // the file's own mark, with its format number
    const int formatNumber = 1;

    juce::File homeFolder()
    {
        return juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    }
}

// --- The one declaration of every setting --------------------------------------

const std::vector<SettingInfo>& Settings::all()
{
    static const std::vector<SettingInfo> table =
    {
        { "projectsFolder", "Folders", "Projects", SettingKind::folder, "~/Documents/Maket", false,
          "Where new projects are made and where the dialog for opening a project starts." },

        { "importsFolder", "Folders", "Imports", SettingKind::folder, "~/Documents/Maket/Imports", false,
          "Where the file dialog starts when something is imported: a bach roll, an OpenMusic instance, "
          "a SPEAR file, a MIDI file, a sound." },

        { "exportsFolder", "Folders", "Exports", SettingKind::folder, "~/Documents/Maket/Exports", false,
          "Where the file dialog starts when something is exported: a bach roll, a shelf, MusicXML, "
          "a MIDI file, a captured sound file." },

        { "contimbreDataFolder", "Folders", "conTimbre search data", SettingKind::folder, "", true,
          "The search_data folder inside conTimbre's \"algorithmic orchestration\" folder. The orchestra "
          "generator reads names, pitches and key maps from it; the samples are never opened. "
          "Needed from step 14." },

        { "ePlayerHost", "conTimbre ePlayer (OSC)", "Host", SettingKind::host, "127.0.0.1", false,
          "The computer on which Max runs with the conTimbre ePlayer. 127.0.0.1 is this Mac itself." },

        { "ePlayerPort", "conTimbre ePlayer (OSC)", "Port", SettingKind::port, "46001", false,
          "The port on which the Max patch listens for the ePlayer's messages (its udpreceive object)." },

        { "standardPitch", "Tuning", "Standard pitch (Hz)", SettingKind::frequency, "442", false,
          "The frequency of the A above middle C. Maket sends it to the ePlayer (as kammerton) at the start of every play; "
          "442 is the ePlayer's own default. For now one value for all of Maket; it moves into the project when projects come." },

        { "audioOutput", "Sound from Maket", "Audio output", SettingKind::audioDevice, "", true,
          "The audio device Maket's own sound (the synth) goes to. Not set: the Mac's default output. "
          "Choose lists the outputs this Mac has now. A change takes effect at the next play." },

        { "synthLevel", "Sound from Maket", "Synth level (dB)", SettingKind::level, "-18", false,
          "How loud one note of the synth is at full velocity, in dB under full scale. Eight such notes together reach full scale at -18." },

        { "synthOffset", "Sound from Maket", "Synth timing (ms)", SettingKind::milliseconds, "0", false,
          "Moves the synth in time against the ePlayer: a positive number makes the synth later, a negative one earlier. "
          "At 0 a synth note sounds at the very moment its command would leave for the ePlayer, which then still needs a moment of its own." },

        { "logPatchPort", "Tests", "Logging patch port", SettingKind::port, "46002", false,
          "The port of Maket's logging patch in Max (Max/Maket OSC log.maxpat), which notes when each message "
          "arrives. Used by the OSC test page. It must differ from the ePlayer's port." },

        { "timingFolder", "Tests", "Test results", SettingKind::folder, "~/Documents/Maket/Exports/Timing", false,
          "Where the test pages of step 1 keep their files: the logs the Max patch writes, Maket's own send logs and "
          "commands, the passages of the timing test as bach.roll files, and the results." }
    };
    return table;
}

const SettingInfo* Settings::find (const juce::String& key)
{
    for (const auto& s : all())
        if (key == s.key) return &s;
    return nullptr;
}

juce::File Settings::defaultFile()
{
    auto folder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    folder = folder.getChildFile ("Application Support");
   #endif
    return folder.getChildFile ("Maket").getChildFile ("settings.json");
}

// --- Home folder ----------------------------------------------------------------

juce::File Settings::expandHome (const juce::String& path)
{
    if (path == "~") return homeFolder();
    if (path.startsWith ("~/")) return homeFolder().getChildFile (path.substring (2));
    if (path.startsWithChar ('/')) return juce::File (path);
    return {};      // not a whole path (this includes ~name/..., another user's home)
}

juce::String Settings::withHome (const juce::File& folder)
{
    const auto home = homeFolder();
    if (folder == home) return "~";
    if (folder.isAChildOf (home))
        return "~/" + folder.getRelativePathFrom (home).replaceCharacter ('\\', '/');
    return folder.getFullPathName();
}

// --- Checking a value -----------------------------------------------------------

juce::Result Settings::validate (const SettingInfo& info, const juce::String& value, juce::String& tidied)
{
    const auto text = value.trim();

    if (text.isEmpty())
    {
        if (! info.mayBeEmpty)
            return juce::Result::fail (juce::String (info.label) + " cannot be left empty.");
        tidied.clear();
        return juce::Result::ok();
    }

    switch (info.kind)
    {
        case SettingKind::folder:
        {
            const auto folder = expandHome (text);
            if (folder == juce::File())
                return juce::Result::fail ("A folder is written as its whole path, starting with / or with ~ for the home folder.");
            tidied = withHome (folder);
            return juce::Result::ok();
        }

        case SettingKind::host:
        {
            if (text.length() > 253
                || ! text.containsOnly ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-:"))
                return juce::Result::fail ("A host is a name or an address without spaces, such as 127.0.0.1 or localhost.");
            tidied = text;
            return juce::Result::ok();
        }

        case SettingKind::port:
        {
            if (! text.containsOnly ("0123456789") || text.length() > 5
                || text.getIntValue() < 1 || text.getIntValue() > 65535)
                return juce::Result::fail ("A port is a whole number from 1 to 65535.");
            tidied = juce::String (text.getIntValue());
            return juce::Result::ok();
        }

        case SettingKind::frequency:
        {
            const auto number = text.replaceCharacter (',', '.');
            const double hz = number.getDoubleValue();
            if (! number.containsOnly ("0123456789.") || number.indexOfChar ('.') != number.lastIndexOfChar ('.')
                || ! number.containsAnyOf ("0123456789") || hz < 380.0 || hz > 480.0)
                return juce::Result::fail ("A standard pitch is a number of Hz from 380 to 480, such as 442 or 440.5.");
            // At most three decimals, and no zeros at the end: 442, 440.5.
            tidied = juce::String (hz, 3);
            while (tidied.endsWithChar ('0')) tidied = tidied.dropLastCharacters (1);
            if (tidied.endsWithChar ('.')) tidied = tidied.dropLastCharacters (1);
            return juce::Result::ok();
        }

        case SettingKind::audioDevice:
        case SettingKind::level:
        case SettingKind::milliseconds:
            break;
    }

    if (info.kind == SettingKind::audioDevice)
    {
        if (text.length() > 200) return juce::Result::fail ("The name of an audio device is shorter than that.");
        tidied = text;
        return juce::Result::ok();
    }

    if (info.kind == SettingKind::level || info.kind == SettingKind::milliseconds)
    {
        const bool isLevel = info.kind == SettingKind::level;
        const auto number = text.replaceCharacter (',', '.').replace (juce::String::fromUTF8 ("\xe2\x88\x92"), "-");
        const double amount = number.getDoubleValue();
        const double low = isLevel ? -60.0 : -200.0, high = isLevel ? 6.0 : 200.0;
        const auto digits = number.startsWithChar ('-') ? number.substring (1) : number;
        if (! digits.containsOnly ("0123456789.") || digits.indexOfChar ('.') != digits.lastIndexOfChar ('.')
            || ! digits.containsAnyOf ("0123456789") || amount < low || amount > high)
            return juce::Result::fail (isLevel ? "A level is a number of dB from -60 to 6, such as -18 or -12.5."
                                               : "A shift in time is a number of milliseconds from -200 to 200, such as 0, 15 or -7.5.");
        tidied = juce::String (amount, 1);
        if (tidied.endsWith (".0")) tidied = tidied.dropLastCharacters (2);
        if (tidied == "-0") tidied = "0";
        return juce::Result::ok();
    }

    return juce::Result::fail ("Unknown kind of setting.");
}

// --- Values -----------------------------------------------------------------------

Settings::Settings (const juce::File& fileToUse) : file (fileToUse)
{
    for (const auto& s : all())
        values.set (s.key, s.defaultValue);
}

juce::String Settings::get (const juce::String& key) const
{
    return values[key];
}

int Settings::getInt (const juce::String& key) const
{
    return values[key].getIntValue();
}

double Settings::getDouble (const juce::String& key) const
{
    return values[key].getDoubleValue();
}

juce::File Settings::getFolder (const juce::String& key) const
{
    return expandHome (values[key]);
}

juce::Result Settings::set (const juce::String& key, const juce::String& value)
{
    const auto* info = find (key);
    if (info == nullptr)
        return juce::Result::fail ("There is no setting named " + key + ".");

    juce::String tidied;
    const auto result = validate (*info, value, tidied);
    if (result.wasOk())
        values.set (key, tidied);
    return result;
}

void Settings::reset (const juce::String& key)
{
    if (const auto* info = find (key))
        values.set (key, info->defaultValue);
}

bool Settings::isDefault (const juce::String& key) const
{
    const auto* info = find (key);
    return info != nullptr && values[key] == info->defaultValue;
}

// --- The file ---------------------------------------------------------------------

juce::Result Settings::load()
{
    if (! file.existsAsFile())
        return juce::Result::ok();

    juce::var root;
    const auto parsed = juce::JSON::parse (file.loadFileAsString(), root);

    if (parsed.failed() || root.getDynamicObject() == nullptr)
    {
        // Not readable as settings: keep it for inspection, start from the defaults.
        const auto aside = file.getSiblingFile ("settings (unreadable).json").getNonexistentSibling();
        file.moveFileTo (aside);
        return juce::Result::fail ("The settings file could not be read. It was set aside as "
                                   + aside.getFileName() + ", and the defaults are used.");
    }

    juce::StringArray problems;
    auto* unknown = new juce::DynamicObject();
    others = juce::var (unknown);

    for (const auto& property : root.getDynamicObject()->getProperties())
    {
        const auto key = property.name.toString();
        if (key == formatKey) continue;

        if (find (key) == nullptr)
        {
            unknown->setProperty (property.name, property.value);       // from a newer Maket, perhaps: keep it
            continue;
        }

        const auto result = set (key, property.value.toString());
        if (result.failed())
            problems.add (key + ": " + result.getErrorMessage() + " The default is used.");
    }

    return problems.isEmpty() ? juce::Result::ok()
                              : juce::Result::fail (problems.joinIntoString (" "));
}

juce::Result Settings::save() const
{
    auto* object = new juce::DynamicObject();
    const juce::var root (object);

    object->setProperty (formatKey, formatNumber);
    for (const auto& s : all())
    {
        if (s.kind == SettingKind::port)            object->setProperty (s.key, getInt (s.key));
        else if (s.kind == SettingKind::frequency || s.kind == SettingKind::level || s.kind == SettingKind::milliseconds)
                                                    object->setProperty (s.key, getDouble (s.key));
        else                             object->setProperty (s.key, get (s.key));
    }
    if (auto* unknown = others.getDynamicObject())
        for (const auto& property : unknown->getProperties())
            object->setProperty (property.name, property.value);

    const auto text = juce::JSON::toString (root) + "\n";

    if (! file.getParentDirectory().createDirectory().wasOk())
        return juce::Result::fail ("The folder " + file.getParentDirectory().getFullPathName() + " could not be made.");

    // Written beside the file first and then put in its place, so that a
    // crash in between never leaves half a settings file.
    juce::TemporaryFile temp (file);
    if (! temp.getFile().replaceWithText (text, false, false, "\n") || ! temp.overwriteTargetFileWithTemporary())
        return juce::Result::fail ("The settings file " + file.getFullPathName() + " could not be written.");

    return juce::Result::ok();
}

} // namespace maket
