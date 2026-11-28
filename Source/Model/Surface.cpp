// Surface.cpp — see Surface.h.

#include "Surface.h"

namespace maket
{

double Surface::length() const
{
    double end = 0.0;
    for (const auto& p : placements) end = juce::jmax (end, p.end());
    return end;
}

int Surface::trackIndex (const juce::String& trackId) const
{
    for (size_t i = 0; i < tracks.size(); ++i) if (tracks[i].id == trackId) return (int) i;
    return -1;
}

Placement* Surface::find (const juce::String& placementId)
{
    for (auto& p : placements) if (p.id == placementId) return &p;
    return nullptr;
}

juce::String Surface::newId (const char* prefix) const
{
    juce::Random random;
    for (;;)
    {
        const auto candidate = juce::String (prefix) + juce::String::toHexString (random.nextInt()).paddedLeft ('0', 8).substring (0, 6);
        bool taken = false;
        for (const auto& t : tracks) taken = taken || t.id == candidate;
        for (const auto& p : placements) taken = taken || p.id == candidate;
        if (! taken) return candidate;
    }
}

const Placement* Surface::find (const juce::String& placementId) const
{
    for (const auto& p : placements) if (p.id == placementId) return &p;
    return nullptr;
}

SurfaceTrack& Surface::addTrack()
{
    SurfaceTrack t;
    t.id = newId ("t");
    t.name = "Track " + juce::String ((int) tracks.size() + 1);
    tracks.push_back (t);
    return tracks.back();
}

void Surface::removeTrack (const juce::String& trackId)
{
    placements.erase (std::remove_if (placements.begin(), placements.end(), [&] (const Placement& p) { return p.trackId == trackId; }), placements.end());
    tracks.erase (std::remove_if (tracks.begin(), tracks.end(), [&] (const SurfaceTrack& t) { return t.id == trackId; }), tracks.end());
}

Placement& Surface::addPlacement (const juce::String& snippetId, const juce::String& trackId, double start, double soundLength)
{
    Placement p;
    p.id = newId ("p");
    p.snippetId = snippetId;
    p.trackId = trackId;
    p.start = juce::jmax (0.0, start);
    p.length = juce::jmax (shortestBlock, soundLength);
    placements.push_back (p);
    return placements.back();
}

void Surface::removePlacement (const juce::String& placementId)
{
    placements.erase (std::remove_if (placements.begin(), placements.end(), [&] (const Placement& p) { return p.id == placementId; }), placements.end());
}

juce::String Surface::toJson() const
{
    auto* object = new juce::DynamicObject();
    const juce::var root (object);
    object->setProperty ("maket-surface", 1);
    object->setProperty ("id", id);
    object->setProperty ("name", name);
    juce::Array<juce::var> trackList, blockList;
    for (const auto& t : tracks)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("id", t.id); o->setProperty ("name", t.name); o->setProperty ("gainDb", t.gainDb);
        o->setProperty ("pan", t.pan); o->setProperty ("mute", t.mute); o->setProperty ("solo", t.solo);
        trackList.add (juce::var (o));
    }
    for (const auto& p : placements)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("id", p.id); o->setProperty ("snippet", p.snippetId); o->setProperty ("track", p.trackId);
        o->setProperty ("start", p.start); o->setProperty ("offset", p.offset); o->setProperty ("length", p.length);
        o->setProperty ("gainDb", p.gainDb); o->setProperty ("fadeIn", p.fadeIn); o->setProperty ("fadeOut", p.fadeOut);
        blockList.add (juce::var (o));
    }
    object->setProperty ("tracks", trackList);
    object->setProperty ("placements", blockList);
    return juce::JSON::toString (root) + "\n";
}

juce::Result Surface::fromJson (const juce::String& text, Surface& result)
{
    const auto parsed = juce::JSON::parse (text);
    auto* object = parsed.getDynamicObject();
    if (object == nullptr || ! object->hasProperty ("maket-surface")) return juce::Result::fail ("This is not a Maket surface file.");

    result = Surface();
    result.id = object->getProperty ("id").toString();
    result.name = object->getProperty ("name").toString();
    if (result.id.isEmpty()) result.id = "main";
    if (auto* list = object->getProperty ("tracks").getArray())
        for (const auto& v : *list)
        {
            SurfaceTrack t;
            t.id = v["id"].toString(); t.name = v["name"].toString();
            t.gainDb = juce::jlimit (-60.0, 12.0, (double) v["gainDb"]); t.pan = juce::jlimit (-1.0, 1.0, (double) v["pan"]);
            t.mute = (bool) v["mute"]; t.solo = (bool) v["solo"];
            if (t.id.isNotEmpty()) result.tracks.push_back (t);
        }
    if (auto* list = object->getProperty ("placements").getArray())
        for (const auto& v : *list)
        {
            Placement p;
            p.id = v["id"].toString(); p.snippetId = v["snippet"].toString(); p.trackId = v["track"].toString();
            p.start = juce::jmax (0.0, (double) v["start"]); p.offset = juce::jmax (0.0, (double) v["offset"]);
            p.length = juce::jmax (shortestBlock, (double) v["length"]);
            p.gainDb = juce::jlimit (-60.0, 24.0, (double) v["gainDb"]);
            p.fadeIn = juce::jmax (0.0, (double) v["fadeIn"]); p.fadeOut = juce::jmax (0.0, (double) v["fadeOut"]);
            if (p.id.isNotEmpty() && result.trackIndex (p.trackId) >= 0) result.placements.push_back (p);      // a block without its track is left out
        }
    return juce::Result::ok();
}

} // namespace maket
