// WindowState.cpp — see WindowState.h.

#include "WindowState.h"

namespace maket
{

WindowState::WindowState (juce::File f) : file (std::move (f)) {}

juce::File WindowState::defaultFile()
{
    auto folder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    folder = folder.getChildFile ("Application Support");
   #endif
    return folder.getChildFile ("Maket").getChildFile ("window.json");
}

void WindowState::load()
{
    if (! file.existsAsFile()) return;
    const auto parsed = juce::JSON::parse (file.loadFileAsString());
    auto* object = parsed.getDynamicObject();
    if (object == nullptr || ! object->hasProperty ("maket-window")) return;

    auto number = [object] (const char* key, int fallback)
    {
        const auto v = object->getProperty (key);
        return (v.isInt() || v.isInt64() || v.isDouble()) ? (int) v : fallback;
    };
    auto flag = [object] (const char* key, bool fallback)
    {
        const auto v = object->getProperty (key);
        return v.isBool() ? (bool) v : fallback;
    };

    listW = juce::jlimit (listWidthMin, listWidthMax, number ("listWidth", listWidthDefault));
    inspectorW = juce::jlimit (inspectorWidthMin, inspectorWidthMax, number ("inspectorWidth", inspectorWidthDefault));
    listOn = flag ("listShown", true);
    inspectorOn = flag ("inspectorShown", true);
    tabIndex = juce::jmax (0, number ("tab", 0));
    const auto b = object->getProperty ("window");
    bounds = b.isString() ? b.toString() : juce::String();
    recent.clear();
    bool dropped = false;
    if (auto* list = object->getProperty ("recentProjects").getArray())
        for (const auto& v : *list)
        {
            // An entry that is not a path (the crash of 3b could leave one) is left out, and the file is written again.
            if (v.isString() && juce::File::isAbsolutePath (v.toString()) && recent.size() < 10) recent.addIfNotAlreadyThere (v.toString());
            else if (v.isString()) dropped = true;
        }
    changed = dropped;
}

bool WindowState::save()
{
    if (! changed) return true;

    auto* object = new juce::DynamicObject();
    const juce::var root (object);
    object->setProperty ("maket-window", 1);
    object->setProperty ("window", bounds);
    object->setProperty ("tab", tabIndex);
    object->setProperty ("listShown", listOn);
    object->setProperty ("listWidth", listW);
    object->setProperty ("inspectorShown", inspectorOn);
    object->setProperty ("inspectorWidth", inspectorW);
    juce::Array<juce::var> list;
    for (const auto& r : recent) list.add (r);
    object->setProperty ("recentProjects", list);

    if (! file.getParentDirectory().createDirectory().wasOk()) return false;
    if (! file.replaceWithText (juce::JSON::toString (root) + "\n", false, false, "\n")) return false;
    changed = false;
    return true;
}

void WindowState::setListWidth (int w)
{
    w = juce::jlimit (listWidthMin, listWidthMax, w);
    if (w != listW) { listW = w; changed = true; }
}

void WindowState::setInspectorWidth (int w)
{
    w = juce::jlimit (inspectorWidthMin, inspectorWidthMax, w);
    if (w != inspectorW) { inspectorW = w; changed = true; }
}

void WindowState::setListShown (bool on)            { if (on != listOn) { listOn = on; changed = true; } }
void WindowState::setInspectorShown (bool on)       { if (on != inspectorOn) { inspectorOn = on; changed = true; } }
void WindowState::setTab (int t)                    { t = juce::jmax (0, t); if (t != tabIndex) { tabIndex = t; changed = true; } }
void WindowState::addRecentProject (const juce::String& path)
{
    if (path.isEmpty() || (recent.size() > 0 && recent[0] == path)) return;
    recent.removeString (path);
    recent.insert (0, path);
    while (recent.size() > 10) recent.remove (recent.size() - 1);
    changed = true;
}

void WindowState::removeRecentProject (const juce::String& path)
{
    if (recent.contains (path)) { recent.removeString (path); changed = true; }
}

void WindowState::setWindowBounds (const juce::String& b)   { if (b != bounds) { bounds = b; changed = true; } }

} // namespace maket
