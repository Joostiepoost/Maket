// Settings.h — Maket's settings: what they are, their defaults, and the file
// they are kept in.
//
// Nothing in Maket points at a place or a port that exists on one Mac only.
// Whatever differs from one Mac to another (folders, the address of the
// ePlayer) is a setting with a default, kept in one readable JSON file:
//
//     ~/Library/Application Support/Maket/settings.json
//
// Every setting is declared once, in Settings::all(). The preferences page,
// the settings file, the test report and the inspector all read that one
// table. A new setting is one more entry there.
//
// No interface code here: the console test program uses the same settings.

#pragma once

#include <JuceHeader.h>
#include <vector>

namespace maket
{

enum class SettingKind
{
    folder,     // a folder on disk; a path inside the home folder is kept as ~/...
    host,       // the name or address of a computer
    port,       // a network port, 1 to 65535
    frequency,  // a standard pitch in Hz, 380 to 480, with at most three decimals
    audioDevice,    // the name of an audio output device; empty for the computer's default output
    level,          // a level in dB, -60 to 6, with at most one decimal
    milliseconds    // a shift in time in ms, -200 to 200, with at most one decimal
};

struct SettingInfo
{
    const char* key;            // its name in the settings file
    const char* group;          // the heading it stands under on the preferences page
    const char* label;          // its name on the preferences page
    SettingKind kind;
    const char* defaultValue;
    bool mayBeEmpty;            // true when "not set" is allowed
    const char* help;           // one or two sentences on what it is for
};

class Settings
{
public:
    /** Every setting Maket has, in the order of the preferences page. */
    static const std::vector<SettingInfo>& all();

    /** The description of one setting, or nullptr when the key is unknown. */
    static const SettingInfo* find (const juce::String& key);

    /** Where the settings file lives on this computer. */
    static juce::File defaultFile();

    /** Starts with every setting at its default; nothing is read yet. */
    explicit Settings (const juce::File& fileToUse);

    const juce::File& getFile() const               { return file; }

    /** Reads the file. A missing file is not an error: the defaults stay.
        A value in the file that is not valid is replaced by its default, and
        a file that cannot be read at all is set aside under another name; in
        both cases the result says what happened. */
    juce::Result load();

    /** Writes the file (and makes its folder if needed). */
    juce::Result save() const;

    /** The value as it is kept: text, with ~ for the home folder. */
    juce::String get (const juce::String& key) const;

    /** A port (or any whole number) as a number. */
    int getInt (const juce::String& key) const;

    /** A frequency (or any number) as a number. */
    double getDouble (const juce::String& key) const;

    /** A folder setting as a folder, with ~ expanded. An empty setting gives
        an invalid File. */
    juce::File getFolder (const juce::String& key) const;

    /** Changes a setting if the value is valid for it; otherwise nothing
        changes and the result says why. The value is tidied first (spaces at
        the ends removed, a folder inside the home folder written with ~). */
    juce::Result set (const juce::String& key, const juce::String& value);

    /** Puts a setting back to its default. */
    void reset (const juce::String& key);

    bool isDefault (const juce::String& key) const;

    /** Checks a value for a setting; on success `tidied` holds the form in
        which it would be kept. */
    static juce::Result validate (const SettingInfo&, const juce::String& value, juce::String& tidied);

    /** "~/Documents/Maket" to the full path on this computer. */
    static juce::File expandHome (const juce::String& path);

    /** A full path to its short form with ~ when it lies in the home folder. */
    static juce::String withHome (const juce::File& folder);

private:
    juce::File file;
    juce::StringPairArray values;       // key -> value, for every known setting
    juce::var others;                   // entries of the file this version does not know; kept and written back
};

} // namespace maket
