// SurfacePage.cpp — see SurfacePage.h.

#include "SurfacePage.h"
#include "Theme.h"
#include <algorithm>
#include <cmath>

namespace
{
    const juce::Colour paper (0xfffaf9f5), paperAlt (0xfff4f2ec), ink (0xff26262b), gridLine (0xffe1dfd8), playColour (0xff57de33), loopColour (0xffad7ab0);
    const juce::Colour card (0xffe6e2d5), cardSelected (0xfff2e4bd), chosen (0xff2f6fb7), crossColour (0xff2f6fb7);

    juce::String timeText (double ms, bool tenths)
    {
        const double seconds = ms / 1000.0;
        const int minutes = (int) (seconds / 60.0);
        const double rest = seconds - minutes * 60.0;
        return juce::String (minutes) + "'" + (tenths ? juce::String (rest, 1).paddedLeft ('0', 4) : juce::String ((int) rest).paddedLeft ('0', 2)) + "\"";
    }

    /** To the millisecond: 1'02.345". */
    juce::String preciseTime (double ms)
    {
        const auto whole = (juce::int64) std::llround (juce::jmax (0.0, ms));
        return juce::String (whole / 60000) + "'" + juce::String ((whole / 1000) % 60).paddedLeft ('0', 2) + "." + juce::String (whole % 1000).paddedLeft ('0', 3) + "\"";
    }

    /** How long each block fades in and out, in ms: its own fades, and the crossing over with its neighbour on the track
        (the rule of buildArrangement). */
    std::map<juce::String, std::pair<double, double>> fadesOf (const maket::Surface& surface, const std::function<bool (const juce::String& snippetId)>& sounds)
    {
        std::map<juce::String, std::pair<double, double>> fades;
        for (const auto& track : surface.tracks)
        {
            std::vector<const maket::Placement*> blocks;
            for (const auto& p : surface.placements) if (p.trackId == track.id && sounds (p.snippetId)) blocks.push_back (&p);      // a block without its sound takes no part
            std::stable_sort (blocks.begin(), blocks.end(), [] (const maket::Placement* a, const maket::Placement* b) { return a->start < b->start; });
            for (const auto* p : blocks) fades[p->id] = { juce::jmin (p->fadeIn, p->length), juce::jmin (p->fadeOut, p->length) };
            for (size_t i = 1; i < blocks.size(); ++i)
            {
                const auto& a = *blocks[i - 1];
                const auto& b = *blocks[i];
                const double overlap = a.end() - b.start;
                if (overlap <= 0.0 || b.end() <= a.end()) continue;
                const double over = juce::jmin (overlap, a.length, b.length);
                fades[a.id].second = juce::jmax (fades[a.id].second, over);
                fades[b.id].first = juce::jmax (fades[b.id].first, over);
            }
        }
        return fades;
    }
}

//==============================================================================
// The left end of a track: its name, mute and solo, its gain and its pan.

class SurfacePage::Header : public juce::Component
{
public:
    Header (SurfacePage& p, const juce::String& id) : trackId (id), page (p)
    {
        name.setEditable (false, true, false);
        name.setFont (Theme::font (13.5f, true));
        name.setMinimumHorizontalScale (1.0f);
        name.setTooltip ("The track's name; double click to change it");
        name.onTextChange = [this]
        {
            const auto text = name.getText().trim();
            page.trackChanged (trackId, [text] (maket::SurfaceTrack& t) { if (text.isNotEmpty()) t.name = text; });
        };
        name.addMouseListener (this, false);
        addAndMakeVisible (name);

        mute.setTooltip ("Mute: the track is silent");
        solo.setTooltip ("Solo: only the tracks on solo sound");
        mute.setColour (juce::TextButton::buttonOnColourId, Theme::warning);
        for (auto* b : { &mute, &solo })
        {
            b->setClickingTogglesState (true);
            b->setWantsKeyboardFocus (false);
            b->setMouseClickGrabsKeyboardFocus (false);
            addAndMakeVisible (*b);
        }
        mute.onClick = [this] { const bool on = mute.getToggleState(); page.trackChanged (trackId, [on] (maket::SurfaceTrack& t) { t.mute = on; }); };
        solo.onClick = [this] { const bool on = solo.getToggleState(); page.trackChanged (trackId, [on] (maket::SurfaceTrack& t) { t.solo = on; }); };

        gain.setRange (-60.0, 12.0, 0.1);
        gain.setSkewFactorFromMidPoint (-12.0);
        gain.setDoubleClickReturnValue (true, 0.0);
        gain.textFromValueFunction = [] (double v) { return v <= -60.0 ? juce::String ("silent") : (v > 0.0 ? "+" : "") + juce::String (v, 1) + " dB"; };
        gain.setTooltip ("The track's gain; drag, double click for 0 dB");
        pan.setRange (-100.0, 100.0, 1.0);
        pan.setDoubleClickReturnValue (true, 0.0);
        pan.textFromValueFunction = [] (double v) { const int n = juce::roundToInt (v); return n == 0 ? juce::String ("middle") : juce::String (n < 0 ? "left " : "right ") + juce::String (std::abs (n)); };
        pan.setTooltip ("The track's place between left and right; drag, double click for the middle");
        for (auto* s : { &gain, &pan })
        {
            s->setSliderStyle (juce::Slider::LinearBar);
            s->setScrollWheelEnabled (false);           // scrolling the page over a header changes nothing
            s->setWantsKeyboardFocus (false);
            s->setColour (juce::Slider::trackColourId, Theme::lineStrong);
            s->setColour (juce::Slider::backgroundColourId, Theme::panel2);
            s->setColour (juce::Slider::textBoxTextColourId, Theme::text);
            s->setColour (juce::Slider::textBoxOutlineColourId, Theme::line);
            s->onDragStart = [this] { page.beginGesture(); };
            s->onDragEnd = [this] { page.endGesture(); };
            s->updateText();
            addAndMakeVisible (*s);
        }
        gain.onValueChange = [this] { const double v = gain.getValue(); page.trackChanged (trackId, [v] (maket::SurfaceTrack& t) { t.gainDb = v; }); };
        pan.onValueChange  = [this] { const double v = pan.getValue() / 100.0; page.trackChanged (trackId, [v] (maket::SurfaceTrack& t) { t.pan = v; }); };
    }

    void set (const maket::SurfaceTrack& t, bool isCurrent)
    {
        if (! name.isBeingEdited()) name.setText (t.name, juce::dontSendNotification);
        mute.setToggleState (t.mute, juce::dontSendNotification);
        solo.setToggleState (t.solo, juce::dontSendNotification);
        gain.setValue (t.gainDb, juce::dontSendNotification);
        pan.setValue (t.pan * 100.0, juce::dontSendNotification);
        if (current != isCurrent) { current = isCurrent; repaint(); }
    }

    void rename()       { name.showEditor(); }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10, 8);
        auto top = area.removeFromTop (24);
        solo.setBounds (top.removeFromRight (26));
        top.removeFromRight (4);
        mute.setBounds (top.removeFromRight (26));
        top.removeFromRight (6);
        name.setBounds (top);
        area.removeFromTop (6);
        gain.setBounds (area.removeFromTop (20));
        area.removeFromTop (5);
        pan.setBounds (area.removeFromTop (20));
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (current ? Theme::panel2 : Theme::panel);
        if (current) { g.setColour (Theme::accent); g.fillRect (0, 0, 3, getHeight()); }
        g.setColour (Theme::line);
        g.fillRect (0, getHeight() - 1, getWidth(), 1);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        page.setCurrentTrack (trackId);
        if (e.mods.isPopupMenu()) page.showTrackMenu (trackId);
    }

    const juce::String trackId;

private:
    SurfacePage& page;
    juce::Label name;
    juce::TextButton mute { "M" }, solo { "S" };
    juce::Slider gain, pan;
    bool current = false;
};

//==============================================================================

SurfacePage::SurfacePage (maket::SurfaceEngine& e, const maket::Project& p) : engine (e), project (p)
{
    playButton.setTooltip ("Play the surface from the playhead through Maket's audio output  (space)");
    stopButton.setTooltip ("Stop  (space)");
    loopButton.setTooltip (juce::String::fromUTF8 ("Go round the loop  (L). Set the loop by dragging in the ruler with \xe2\x8c\xa5 held; \xe2\x8c\xa5-click takes it away."));
    addTrackButton.setTooltip ("Add a track at the bottom");
    addSoundButton.setTooltip ("Put a sound of the library at the playhead, on the track last clicked");
    playButton.onClick = [this] { if (! isPlaying()) playOrStop(); };
    stopButton.onClick = [this] { stopPlaying(); };
    loopButton.onClick = [this] { toggleLoop(); };
    addTrackButton.onClick = [this] { addTrack(); };
    addSoundButton.onClick = [this] { showSoundsMenu(); };
    zoomOutButton.onClick = [this] { zoomAcross (-1); };
    zoomInButton.onClick  = [this] { zoomAcross (1); };
    fitButton.onClick     = [this] { zoomToFit(); };
    for (auto* b : { &playButton, &stopButton, &loopButton, &addTrackButton, &addSoundButton, &zoomOutButton, &zoomInButton, &fitButton })
    {
        b->setWantsKeyboardFocus (false);
        b->setMouseClickGrabsKeyboardFocus (false);
        addAndMakeVisible (*b);
    }
    headerHolder.setInterceptsMouseClicks (false, true);
    addAndMakeVisible (headerHolder);
    for (auto* bar : { &scrollBar, &downBar })
    {
        bar->addListener (this);
        bar->setAutoHide (false);
        addAndMakeVisible (*bar);
    }
    setWantsKeyboardFocus (true);
    updateBar();
}

SurfacePage::~SurfacePage()
{
    engine.halt();
    engine.setArrangement (nullptr);
}

//==============================================================================
// The surface and its sounds

void SurfacePage::projectChanged()
{
    if (! project.isOpen())
    {
        if (loadedFolder == juce::File()) return;
        engine.halt();
        engine.setArrangement (nullptr);
        playing = false;
        stopTimer();
        loadedFolder = juce::File();
        surface = maket::Surface();
        sounds.clear();
        undoStack.clear();
        redoStack.clear();
        selected.clear();
        currentTrack.clear();
        notice.clear();
        syncHeaders();
        if (clearDetail) clearDetail();
        updateBar();
        viewChanged();
        return;
    }
    if (project.getFolder() != loadedFolder) load();
    else repaint();                     // a name in the library may have changed
}

void SurfacePage::load()
{
    engine.halt();
    playing = false;
    stopTimer();
    loadedFolder = project.getFolder();
    sounds.clear();
    undoStack.clear();
    redoStack.clear();
    selected.clear();
    notice.clear();
    noticeIsProblem = false;
    playhead = playedFrom = 0.0;
    loopStart = loopEnd = 0.0;
    loopOn = false;
    scrollY = 0;

    const auto read = project.loadSurface (surface);
    if (read.failed()) setNotice ("The project's surface could not be read: " + read.getErrorMessage(), true);
    currentTrack = surface.tracks.empty() ? juce::String() : surface.tracks.front().id;

    juce::MouseCursor::showWaitCursor();
    rebuild();                          // reads the sounds its blocks use
    juce::MouseCursor::hideWaitCursor();
    syncHeaders();
    if (clearDetail) clearDetail();
    zoomToFit();
    updateBar();
}

void SurfacePage::save()
{
    if (! project.isOpen()) return;
    const auto written = project.saveSurface (surface);
    if (written.failed()) setNotice (written.getErrorMessage(), true);
}

std::shared_ptr<const maket::Sound> SurfacePage::soundOf (const juce::String& snippetId)
{
    const auto found = sounds.find (snippetId);
    if (found != sounds.end()) return found->second;

    std::shared_ptr<const maket::Sound> result;
    const auto* info = project.find (snippetId);
    if (info != nullptr && info->isAudio())
    {
        auto sound = std::make_shared<maket::Sound>();
        const auto read = maket::readSoundFile (project.soundFile (snippetId), *sound);
        if (read.wasOk()) result = sound;
        else setNotice ("\"" + info->name + "\": " + read.getErrorMessage(), true);
    }
    sounds[snippetId] = result;
    return result;
}

double SurfacePage::soundLength (const juce::String& snippetId) const
{
    const auto found = sounds.find (snippetId);
    if (found != sounds.end() && found->second != nullptr) return found->second->lengthMs();
    const auto* info = project.find (snippetId);
    return info != nullptr && info->isAudio() ? info->length : 0.0;
}

void SurfacePage::rebuild()
{
    if (! project.isOpen()) { engine.setArrangement (nullptr); return; }
    for (const auto& p : surface.placements) soundOf (p.snippetId);        // also of a muted track: its blocks are drawn all the same
    engine.setArrangement (maket::buildArrangement (surface, project.getSampleRate(), [this] (const juce::String& id) { return soundOf (id); }));
}

void SurfacePage::snippetChanged (const juce::String& id)
{
    if (! project.isOpen()) return;
    sounds.erase (id);
    // A sound that became shorter: its blocks keep within it.
    bool fitted = false;
    if (project.find (id) != nullptr)
    {
        const double length = soundOf (id) != nullptr ? soundOf (id)->lengthMs() : 0.0;
        if (length > 0.0)
            for (auto& p : surface.placements)
                if (p.snippetId == id && p.offset + p.length > length + 0.5)
                {
                    p.offset = juce::jmin (p.offset, juce::jmax (0.0, length - maket::Surface::shortestBlock));
                    p.length = juce::jmax (maket::Surface::shortestBlock, length - p.offset);
                    fitted = true;
                }
    }
    if (fitted) save();
    rebuild();
    showSelection();
    viewChanged();
}

void SurfacePage::beginGesture()
{
    if (gestureOpen) return;
    beforeGesture = surface;
    gestureOpen = true;
    gestureChanged = false;
}

void SurfacePage::touched()
{
    gestureChanged = true;
    rebuild();
    repaint();
}

void SurfacePage::endGesture()
{
    if (! gestureOpen) return;
    gestureOpen = false;
    if (gestureChanged)
    {
        undoStack.push_back (std::move (beforeGesture));
        if ((int) undoStack.size() > undoMost) undoStack.erase (undoStack.begin());
        redoStack.clear();
        save();
    }
    syncHeaders();
    showSelection();
    viewChanged();
    updateBar();
    if (onChanged) onChanged();
}

void SurfacePage::edit (const std::function<void()>& change)
{
    endGesture();
    beginGesture();
    change();
    touched();
    endGesture();
}

void SurfacePage::restore (const maket::Surface& s)
{
    surface = s;
    if (surface.find (selected) == nullptr) selected.clear();
    if (surface.trackIndex (currentTrack) < 0) currentTrack = surface.tracks.empty() ? juce::String() : surface.tracks.front().id;
    rebuild();
    save();
    syncHeaders();
    showSelection();
    viewChanged();
    updateBar();
    if (onChanged) onChanged();
}

void SurfacePage::undo()
{
    endGesture();
    if (undoStack.empty()) return;
    redoStack.push_back (surface);
    const auto back = std::move (undoStack.back());
    undoStack.pop_back();
    restore (back);
}

void SurfacePage::redo()
{
    endGesture();
    if (redoStack.empty()) return;
    undoStack.push_back (surface);
    const auto again = std::move (redoStack.back());
    redoStack.pop_back();
    restore (again);
}

void SurfacePage::syncHeaders()
{
    bool same = headers.size() == (int) surface.tracks.size();
    for (int i = 0; same && i < headers.size(); ++i) same = headers[i]->trackId == surface.tracks[(size_t) i].id;
    if (! same)
    {
        headers.clear();
        for (const auto& t : surface.tracks) headerHolder.addAndMakeVisible (headers.add (new Header (*this, t.id)));
    }
    for (int i = 0; i < headers.size(); ++i)
    {
        headers[i]->set (surface.tracks[(size_t) i], surface.tracks[(size_t) i].id == currentTrack);
        headers[i]->setBounds (0, i * trackHeight - scrollY, headerWidth, trackHeight);
    }
}

void SurfacePage::setNotice (const juce::String& text, bool isProblem)
{
    notice = text;
    noticeIsProblem = isProblem;
    repaint();
}

void SurfacePage::select (const juce::String& placementId)
{
    if (selected == placementId) return;
    selected = placementId;
    showSelection();
    repaint();
    if (onChanged) onChanged();
}

void SurfacePage::showSelection()
{
    const auto* p = surface.find (selected);
    if (p == nullptr) { if (clearDetail) clearDetail(); return; }
    if (! showDetail) return;
    const auto* info = project.find (p->snippetId);
    const int track = surface.trackIndex (p->trackId);
    const double whole = soundLength (p->snippetId);
    juce::StringPairArray lines;
    lines.set ("Starts at", preciseTime (p->start));
    lines.set ("Ends at", preciseTime (p->end()));
    lines.set ("Length", preciseTime (p->length));
    lines.set ("Part of its sound", whole <= 0.0 ? juce::String ("its sound is not in the library: the block is silent")
                                    : p->offset < 0.5 && p->length > whole - 0.5 ? "the whole sound (" + preciseTime (whole) + ")"
                                                                                : "from " + preciseTime (p->offset) + " to " + preciseTime (p->offset + p->length) + " of " + preciseTime (whole));
    lines.set ("Gain", juce::String (p->gainDb, 1) + " dB");
    lines.set ("Fades", p->fadeIn > 0.0 || p->fadeOut > 0.0 ? "in " + preciseTime (p->fadeIn) + ", out " + preciseTime (p->fadeOut) : juce::String ("none of its own"));
    showDetail (juce::String::fromUTF8 ("Block  \xc2\xb7  ") + (track >= 0 ? surface.tracks[(size_t) track].name : juce::String ("no track")),
                info != nullptr ? info->name : juce::String ("a sound that is not in the library"), lines);
}

//==============================================================================
// Tracks

void SurfacePage::addTrack()
{
    if (! project.isOpen()) return;
    edit ([this] { currentTrack = surface.addTrack().id; });
    // Bring the new track in view.
    scrollY = juce::jmax (0, (int) surface.tracks.size() * trackHeight - lanesArea().getHeight());
    viewChanged();
}

void SurfacePage::trackChanged (const juce::String& trackId, const std::function<void (maket::SurfaceTrack&)>& change)
{
    const int index = surface.trackIndex (trackId);
    if (index < 0) return;
    if (gestureOpen) { change (surface.tracks[(size_t) index]); touched(); }
    else edit ([&] { change (surface.tracks[(size_t) index]); });
}

void SurfacePage::setCurrentTrack (const juce::String& trackId)
{
    if (currentTrack == trackId) return;
    currentTrack = trackId;
    syncHeaders();
    repaint();
}

void SurfacePage::moveTrack (const juce::String& trackId, int by)
{
    const int from = surface.trackIndex (trackId), to = from + by;
    if (from < 0 || to < 0 || to >= (int) surface.tracks.size()) return;
    edit ([&] { std::swap (surface.tracks[(size_t) from], surface.tracks[(size_t) to]); });
}

void SurfacePage::showTrackMenu (const juce::String& trackId)
{
    const int index = surface.trackIndex (trackId);
    if (index < 0) return;
    int blocks = 0;
    for (const auto& p : surface.placements) blocks += p.trackId == trackId ? 1 : 0;

    juce::PopupMenu m;
    m.addItem (1, juce::String::fromUTF8 ("Rename\xe2\x80\xa6"));
    m.addItem (2, "Move up", index > 0);
    m.addItem (3, "Move down", index + 1 < (int) surface.tracks.size());
    m.addSeparator();
    m.addItem (4, blocks == 0 ? juce::String ("Remove the track") : "Remove the track and its " + juce::String (blocks) + (blocks == 1 ? " block" : " blocks"));
    m.showMenuAsync (juce::PopupMenu::Options(), [safe = juce::Component::SafePointer<SurfacePage> (this), trackId] (int picked)
    {
        if (safe == nullptr || picked == 0) return;
        auto& page = *safe;
        if (picked == 1) { for (auto* h : page.headers) if (h->trackId == trackId) h->rename(); }
        else if (picked == 2) page.moveTrack (trackId, -1);
        else if (picked == 3) page.moveTrack (trackId, 1);
        else if (picked == 4) page.edit ([&page, trackId]
        {
            page.surface.removeTrack (trackId);
            if (page.surface.find (page.selected) == nullptr) page.selected.clear();
            if (page.currentTrack == trackId) page.currentTrack = page.surface.tracks.empty() ? juce::String() : page.surface.tracks.front().id;
        });
    });
}

//==============================================================================
// Blocks

juce::PopupMenu SurfacePage::soundsMenu (int firstId, std::vector<juce::String>& ids) const
{
    juce::PopupMenu m;
    for (const auto& s : project.getSnippets())
    {
        if (! s.isAudio()) continue;
        m.addItem (firstId + (int) ids.size(), s.name + "     " + timeText (s.length, true) + (s.channels == 2 ? "  stereo" : "  mono"));
        ids.push_back (s.id);
    }
    if (ids.empty()) m.addItem (firstId, juce::String::fromUTF8 ("The library has no sounds yet: File > Import\xe2\x80\xa6"), false);
    return m;
}

void SurfacePage::addBlock (const juce::String& snippetId, const juce::String& trackId, double atMs)
{
    juce::MouseCursor::showWaitCursor();
    const auto sound = soundOf (snippetId);
    juce::MouseCursor::hideWaitCursor();
    if (sound == nullptr) return;       // (the notice says why)
    edit ([&]
    {
        auto track = trackId;
        if (surface.trackIndex (track) < 0) track = surface.addTrack().id;
        currentTrack = track;
        selected = surface.addPlacement (snippetId, track, atMs, sound->lengthMs()).id;
    });
}

void SurfacePage::showSoundsMenu()
{
    if (! project.isOpen()) return;
    auto ids = std::make_shared<std::vector<juce::String>>();
    auto m = soundsMenu (100, *ids);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (addSoundButton), [safe = juce::Component::SafePointer<SurfacePage> (this), ids] (int picked)
    {
        if (safe == nullptr || picked < 100 || picked - 100 >= (int) ids->size()) return;
        safe->addBlock ((*ids)[(size_t) (picked - 100)], safe->currentTrack, safe->playhead);
    });
}

void SurfacePage::showLaneMenu (int trackIndex, double atMs)
{
    auto ids = std::make_shared<std::vector<juce::String>>();
    juce::PopupMenu m;
    const juce::String trackId = trackIndex >= 0 ? surface.tracks[(size_t) trackIndex].id : juce::String();
    m.addSubMenu (trackIndex >= 0 ? "Add a sound here, at " + timeText (atMs, true) : juce::String ("Add a sound on a new track"), soundsMenu (100, *ids));
    m.addSeparator();
    m.addItem (1, "Add a track");
    m.showMenuAsync (juce::PopupMenu::Options(), [safe = juce::Component::SafePointer<SurfacePage> (this), ids, trackId, atMs] (int picked)
    {
        if (safe == nullptr || picked == 0) return;
        if (picked == 1) safe->addTrack();
        else if (picked >= 100 && picked - 100 < (int) ids->size()) safe->addBlock ((*ids)[(size_t) (picked - 100)], trackId, atMs);
    });
}

void SurfacePage::showBlockMenu (const juce::String& placementId)
{
    const auto* p = surface.find (placementId);
    if (p == nullptr) return;
    const double whole = soundLength (p->snippetId);
    juce::PopupMenu m;
    m.addItem (1, juce::String::fromUTF8 ("Duplicate, right after it     \xe2\x8c\x98" "D"));
    m.addItem (2, "Use the whole sound again", whole > 0.0 && (p->offset > 0.5 || p->length < whole - 0.5));
    m.addSeparator();
    m.addItem (3, juce::String::fromUTF8 ("Remove the block     \xe2\x8c\xab"));
    m.showMenuAsync (juce::PopupMenu::Options(), [safe = juce::Component::SafePointer<SurfacePage> (this), placementId, whole] (int picked)
    {
        if (safe == nullptr || picked == 0) return;
        auto& page = *safe;
        page.selected = placementId;
        if (picked == 1) page.duplicateSelected();
        else if (picked == 3) page.deleteSelected();
        else if (picked == 2) page.edit ([&page, placementId, whole]
        {
            if (auto* block = page.surface.find (placementId))
            {
                // The sound stays where it sounds: the block grows around it.
                const double startOfSound = block->start - block->offset;
                block->start = juce::jmax (0.0, startOfSound);
                block->offset = block->start - startOfSound;
                block->length = whole - block->offset;
            }
        });
    });
}

void SurfacePage::deleteSelected()
{
    if (surface.find (selected) == nullptr) return;
    edit ([this] { surface.removePlacement (selected); selected.clear(); });
}

void SurfacePage::duplicateSelected()
{
    const auto* p = surface.find (selected);
    if (p == nullptr) return;
    const auto original = *p;
    edit ([&]
    {
        auto& copy = surface.addPlacement (original.snippetId, original.trackId, original.end(), original.length);
        const auto id = copy.id;
        copy = original;
        copy.id = id;
        copy.start = original.end();
        selected = id;
    });
}

juce::Rectangle<int> SurfacePage::blockBounds (const maket::Placement& p) const
{
    const int index = surface.trackIndex (p.trackId);
    if (index < 0) return {};
    const int x0 = xOf (p.start), x1 = xOf (p.end());
    return { x0, trackTop (index) + 5, juce::jmax (3, x1 - x0), trackHeight - 11 };
}

const maket::Placement* SurfacePage::blockAt (juce::Point<int> at) const
{
    if (! lanesArea().contains (at)) return nullptr;
    // The edges of the selected block can always be taken, also where another block lies over them.
    if (const auto* s = surface.find (selected))
        if (blockBounds (*s).contains (at) && edgeAt (*s, at) != 0) return s;
    // Otherwise: a block that begins later lies over one that begins earlier.
    const maket::Placement* found = nullptr;
    for (const auto& p : surface.placements)
        if (blockBounds (p).contains (at) && (found == nullptr || p.start >= found->start)) found = &p;
    return found;
}

int SurfacePage::edgeAt (const maket::Placement& p, juce::Point<int> at) const
{
    const auto b = blockBounds (p);
    const int edge = juce::jmin (7, b.getWidth() / 3);
    return at.x < b.getX() + edge ? -1 : at.x >= b.getRight() - edge ? 1 : 0;
}

//==============================================================================
// Play

void SurfacePage::playOrStop()
{
    if (isPlaying()) { stopPlaying(); return; }
    if (! project.isOpen()) return;
    endGesture();
    if (onWillPlay) onWillPlay();

    const auto trouble = turnAudioOn ? turnAudioOn() : juce::String();
    if (trouble.isNotEmpty()) { setNotice (trouble, true); return; }
    notice.clear();
    noticeIsProblem = false;

    rebuild();
    if (! (loopOn && loopEnd > loopStart) && playhead >= surface.length() - 1.0) playhead = 0.0;
    playedFrom = playhead;
    engine.play (playhead, loopOn && loopEnd > loopStart, loopStart, loopEnd);
    playing = true;
    startTimerHz (30);
    updateBar();
}

void SurfacePage::stopPlaying()
{
    if (! playing) return;
    engine.stop();
    playing = false;
    playhead = playedFrom;              // back to where the play began, as in the roll
    stopTimer();
    updateBar();
    repaint();
}

void SurfacePage::toggleLoop()
{
    loopOn = ! loopOn;
    if (loopOn && loopEnd <= loopStart) setNotice (juce::String::fromUTF8 ("No loop is set: drag in the ruler with \xe2\x8c\xa5 held."));
    else if (isPlaying())               setNotice ("The loop takes effect at the next play.");
    updateBar();
}

void SurfacePage::toStart()
{
    if (isPlaying()) return;
    playhead = 0.0;
    scrollMs = 0.0;
    viewChanged();
}

void SurfacePage::audioChanged (bool isOn)
{
    if (isOn || ! playing) return;
    engine.halt();
    playing = false;
    playhead = playedFrom;
    stopTimer();
    updateBar();
}

void SurfacePage::timerCallback()
{
    if (! playing) { stopTimer(); return; }
    if (! engine.isPlaying())
    {
        // It came to the end of the last block.
        playing = false;
        stopTimer();
        playhead = playedFrom;
        updateBar();
        return;
    }
    const double before = playhead;
    playhead = engine.positionMs();
    const double visible = lanesArea().getWidth() / pixelsPerMs;
    if (playhead < scrollMs || playhead > scrollMs + visible)
    {
        scrollMs = juce::jmax (0.0, playhead - visible * 0.05);        // turn the page
        viewChanged();
    }
    else repaintPlayhead (before, playhead);
}

void SurfacePage::repaintPlayhead (double before, double after)
{
    const auto ruler = rulerArea(), lanes = lanesArea();
    for (double ms : { before, after })
        repaint (xOf (ms) - 9, ruler.getY(), 20, lanes.getBottom() - ruler.getY());
    repaint (0, rowHeight, getWidth(), rowHeight);      // the time in the bar
}

void SurfacePage::updateBar()
{
    const bool open = project.isOpen();
    playButton.setEnabled (open && ! playing);
    stopButton.setEnabled (playing);
    loopButton.setEnabled (open);
    loopButton.setToggleState (loopOn, juce::dontSendNotification);
    addTrackButton.setEnabled (open);
    addSoundButton.setEnabled (open);
    for (auto* b : { &zoomOutButton, &zoomInButton, &fitButton }) b->setEnabled (open);
    repaint();
    if (playing != wasPlaying)
    {
        wasPlaying = playing;
        if (onChanged) onChanged();
    }
}

//==============================================================================
// The view

juce::Rectangle<int> SurfacePage::rulerArea() const
{
    return getLocalBounds().withTrimmedTop (2 * rowHeight).removeFromTop (rulerHeight).withTrimmedLeft (headerWidth).withTrimmedRight (barThickness);
}

juce::Rectangle<int> SurfacePage::lanesArea() const
{
    return getLocalBounds().withTrimmedTop (2 * rowHeight + rulerHeight).withTrimmedBottom (barThickness).withTrimmedLeft (headerWidth).withTrimmedRight (barThickness);
}

double SurfacePage::msAt (int x) const        { return scrollMs + (x - lanesArea().getX()) / pixelsPerMs; }
int SurfacePage::xOf (double ms) const        { return lanesArea().getX() + juce::roundToInt ((ms - scrollMs) * pixelsPerMs); }
int SurfacePage::trackTop (int index) const   { return lanesArea().getY() + index * trackHeight - scrollY; }

int SurfacePage::trackIndexAt (int y) const
{
    const int index = (int) std::floor ((y - lanesArea().getY() + scrollY) / (double) trackHeight);
    return index >= 0 && index < (int) surface.tracks.size() ? index : -1;
}

void SurfacePage::viewChanged()
{
    const auto lanes = lanesArea();
    const double visible = lanes.getWidth() / pixelsPerMs;
    const double length = surface.length();
    scrollMs = juce::jlimit (0.0, juce::jmax (0.0, length - visible * 0.5), scrollMs);
    scrollBar.setRangeLimits (0.0, juce::jmax (length + visible * 0.5, scrollMs + visible), juce::dontSendNotification);
    scrollBar.setCurrentRange (scrollMs, visible, juce::dontSendNotification);

    const int all = (int) surface.tracks.size() * trackHeight;
    scrollY = juce::jlimit (0, juce::jmax (0, all - lanes.getHeight()), scrollY);
    downBar.setVisible (all > lanes.getHeight());
    downBar.setRangeLimits (0.0, (double) juce::jmax (all, lanes.getHeight()), juce::dontSendNotification);
    downBar.setCurrentRange ((double) scrollY, (double) lanes.getHeight(), juce::dontSendNotification);

    for (int i = 0; i < headers.size(); ++i) headers[i]->setBounds (0, i * trackHeight - scrollY, headerWidth, trackHeight);
    repaint();
}

void SurfacePage::scrollBarMoved (juce::ScrollBar* bar, double newRangeStart)
{
    if (bar == &scrollBar) scrollMs = newRangeStart;
    else                   scrollY = juce::roundToInt (newRangeStart);
    viewChanged();
}

void SurfacePage::zoomAround (double factor, double anchorMs)
{
    const double before = (anchorMs - scrollMs) * pixelsPerMs;
    // From half an hour across the view down to a pixel for every sample at 48 kHz.
    const double least = juce::jmax (1, lanesArea().getWidth()) / 1800000.0, most = 48.0;
    pixelsPerMs = juce::jlimit (least, most, pixelsPerMs * factor);
    scrollMs = anchorMs - before / pixelsPerMs;
    viewChanged();
}

void SurfacePage::zoomAcross (int direction)
{
    const double middle = scrollMs + lanesArea().getWidth() / pixelsPerMs * 0.5;
    const double playheadX = (playhead - scrollMs) * pixelsPerMs;
    const bool playheadShows = playheadX >= 0.0 && playheadX <= lanesArea().getWidth();
    zoomAround (direction > 0 ? 1.5 : 1.0 / 1.5, playheadShows ? playhead : middle);
}

void SurfacePage::zoomToFit()
{
    const int width = lanesArea().getWidth();
    if (width <= 0) return;
    // The whole surface with some air after it; an empty surface shows half a minute.
    pixelsPerMs = (width - 24) / juce::jmax (30000.0, surface.length() * 1.08);
    scrollMs = 0.0;
    viewChanged();
}

void SurfacePage::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown()) { zoomAround (std::pow (1.5, wheel.deltaY * 4.0), msAt (e.x)); return; }
    const bool tracksOverflow = (int) surface.tracks.size() * trackHeight > lanesArea().getHeight();
    if (std::abs (wheel.deltaX) > std::abs (wheel.deltaY) || e.mods.isShiftDown() || ! tracksOverflow)
    {
        const double delta = std::abs (wheel.deltaX) > std::abs (wheel.deltaY) ? wheel.deltaX : wheel.deltaY;
        scrollMs -= delta * 400.0 / pixelsPerMs;
    }
    else scrollY -= juce::roundToInt (wheel.deltaY * 300.0f);
    viewChanged();
}

void SurfacePage::mouseMagnify (const juce::MouseEvent& e, float scale)     { zoomAround (scale, msAt (e.x)); }

//==============================================================================
// The mouse and the keys

void SurfacePage::mouseMove (const juce::MouseEvent& e)
{
    auto shape = juce::MouseCursor::NormalCursor;
    if (const auto* p = blockAt (e.getPosition()))
        if (edgeAt (*p, e.getPosition()) != 0) shape = juce::MouseCursor::LeftRightResizeCursor;
    setMouseCursor (shape);
}

void SurfacePage::mouseDown (const juce::MouseEvent& e)
{
    if (! project.isOpen()) return;
    grabKeyboardFocus();
    drag = Drag::none;
    downAt = e.getPosition();
    const auto ruler = rulerArea(), lanes = lanesArea();
    const double ms = juce::jmax (0.0, msAt (e.x));

    if (ruler.contains (e.getPosition()))
    {
        if (e.mods.isAltDown())
        {
            drag = Drag::loop;
            dragAnchor = ms;
            loopStart = loopEnd = 0.0;      // a click with the key held, without a drag, takes the loop away
            updateBar();
        }
        else if (! isPlaying()) { drag = Drag::playhead; playhead = ms; repaint(); }
        return;
    }
    if (! lanes.contains (e.getPosition())) return;

    const int track = trackIndexAt (e.y);
    if (track >= 0) setCurrentTrack (surface.tracks[(size_t) track].id);

    if (const auto* p = blockAt (e.getPosition()))
    {
        const int end = edgeAt (*p, e.getPosition());
        grabbed = *p;
        select (grabbed.id);
        if (e.mods.isPopupMenu()) { showBlockMenu (grabbed.id); return; }
        drag = end < 0 ? Drag::trimStart : end > 0 ? Drag::trimEnd : Drag::move;
        grabMs = msAt (e.x);
        return;
    }

    select ({});
    if (e.mods.isPopupMenu()) { showLaneMenu (track, ms); return; }
    if (! isPlaying()) { drag = Drag::playhead; playhead = ms; repaint(); }
}

void SurfacePage::mouseDrag (const juce::MouseEvent& e)
{
    if (drag == Drag::none) return;
    const double ms = juce::jmax (0.0, msAt (e.x));

    if (drag == Drag::loop)
    {
        loopStart = juce::jmin (dragAnchor, ms);
        loopEnd = juce::jmax (dragAnchor, ms);
        if (loopEnd - loopStart >= 20.0) loopOn = true;
        updateBar();
        return;
    }
    if (drag == Drag::playhead)
    {
        if (! isPlaying()) { playhead = ms; repaint(); }
        return;
    }

    // A block: nothing moves until the mouse has gone a few pixels, so that a click is only a click.
    if (! gestureOpen && e.getPosition().getDistanceFrom (downAt) < 3) return;
    auto* p = surface.find (selected);
    if (p == nullptr) return;
    beginGesture();
    const double moved = msAt (e.x) - grabMs;
    const double whole = soundLength (grabbed.snippetId);

    if (drag == Drag::move)
    {
        p->start = juce::jmax (0.0, grabbed.start + moved);
        const int track = trackIndexAt (e.y);
        if (track >= 0) p->trackId = surface.tracks[(size_t) track].id;
        currentTrack = p->trackId;
    }
    else if (drag == Drag::trimStart)
    {
        // The sound stays where it sounds; the block begins later or earlier in it.
        const double by = juce::jlimit (-juce::jmin (grabbed.offset, grabbed.start), grabbed.length - maket::Surface::shortestBlock, moved);
        p->start = grabbed.start + by;
        p->offset = grabbed.offset + by;
        p->length = grabbed.length - by;
    }
    else if (drag == Drag::trimEnd)
    {
        const double most = whole > 0.0 ? whole - grabbed.offset : grabbed.length;
        p->length = juce::jlimit (maket::Surface::shortestBlock, juce::jmax (maket::Surface::shortestBlock, most), grabbed.length + moved);
    }
    touched();
    syncHeaders();
    showSelection();
}

void SurfacePage::mouseUp (const juce::MouseEvent&)
{
    if (drag == Drag::move || drag == Drag::trimStart || drag == Drag::trimEnd) endGesture();
    drag = Drag::none;
}

bool SurfacePage::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        if (! hasSelection()) return false;
        deleteSelected();
        return true;
    }
    if (key == juce::KeyPress::escapeKey && hasSelection()) { select ({}); return true; }
    return false;
}

//==============================================================================

void SurfacePage::resized()
{
    auto top = getLocalBounds().removeFromTop (rowHeight).reduced (10, 6);
    fitButton.setBounds (top.removeFromRight (54));
    top.removeFromRight (14);
    zoomInButton.setBounds (top.removeFromRight (30));
    top.removeFromRight (4);
    zoomOutButton.setBounds (top.removeFromRight (30));

    auto second = getLocalBounds().withTrimmedTop (rowHeight).removeFromTop (rowHeight).reduced (10, 6);
    playButton.setBounds (second.removeFromLeft (56));
    second.removeFromLeft (4);
    stopButton.setBounds (second.removeFromLeft (56));
    second.removeFromLeft (12);
    loopButton.setBounds (second.removeFromLeft (56));
    second.removeFromLeft (20);
    addTrackButton.setBounds (second.removeFromLeft (72));
    second.removeFromLeft (4);
    addSoundButton.setBounds (second.removeFromLeft (76));

    const auto lanes = lanesArea();
    headerHolder.setBounds (0, lanes.getY(), headerWidth, lanes.getHeight());
    scrollBar.setBounds (lanes.getX(), lanes.getBottom(), lanes.getWidth(), barThickness);
    downBar.setBounds (lanes.getRight(), lanes.getY(), barThickness, lanes.getHeight());
    viewChanged();
}

void SurfacePage::paintBlock (juce::Graphics& g, const maket::Placement& p, juce::Rectangle<int> lanes, double crossingInMs) const
{
    const auto b = blockBounds (p);
    const auto shown = b.getIntersection (lanes);
    if (shown.isEmpty()) return;
    const bool isSelected = p.id == selected;
    const auto found = sounds.find (p.snippetId);
    const auto* sound = found != sounds.end() ? found->second.get() : nullptr;
    const auto* info = project.find (p.snippetId);

    g.saveState();
    g.reduceClipRegion (shown);
    // Where it comes in over the end of the block before it, that block shows through.
    const int crossing = juce::jlimit (0, b.getWidth(), juce::roundToInt (crossingInMs * pixelsPerMs));
    const auto colour = isSelected ? cardSelected : card;
    g.setColour (colour.withAlpha (0.55f));
    g.fillRect (b.withWidth (crossing));
    g.setColour (colour);
    g.fillRect (b.withTrimmedLeft (crossing));

    if (sound == nullptr)
    {
        // Its sound is not there: a hatched block, silent.
        g.setColour (Theme::warning.withAlpha (0.55f));
        for (int x = shown.getX() - b.getHeight(); x < shown.getRight(); x += 9)
            g.drawLine ((float) x, (float) b.getBottom(), (float) (x + b.getHeight()), (float) b.getY(), 1.0f);
    }
    else
    {
        // The waveform: for every pixel across, from the lowest to the highest value it covers, of both channels.
        const float middle = (float) b.getCentreY() + 5.0f, half = (float) (b.getHeight() - 16) * 0.48f;
        const double perPixel = sound->sampleRate / 1000.0 / pixelsPerMs;
        const int length = sound->numSamples(), channels = sound->numChannels();
        const bool fromOverview = perPixel >= maket::Sound::overviewStep && ! sound->overview.empty();
        juce::RectangleList<float> columns;
        for (int x = shown.getX(); x < shown.getRight(); ++x)
        {
            const double from = (p.offset + (x - b.getX()) / pixelsPerMs) * sound->sampleRate / 1000.0;
            const int first = juce::jmax (0, (int) from), last = juce::jmin (length, juce::jmax ((int) from + 1, (int) (from + perPixel) + 1));
            if (first >= length || last <= first) continue;
            float low = 1.0f, high = -1.0f;
            for (int c = 0; c < channels; ++c)
            {
                if (fromOverview)
                {
                    const auto& ranges = sound->overview[(size_t) c];
                    const int ra = first / maket::Sound::overviewStep, rb = juce::jmin ((int) ranges.size(), last / maket::Sound::overviewStep + 1);
                    for (int i = ra; i < rb; ++i) { low = juce::jmin (low, ranges[(size_t) i].low); high = juce::jmax (high, ranges[(size_t) i].high); }
                }
                else
                {
                    const auto r = juce::FloatVectorOperations::findMinAndMax (sound->samples.getReadPointer (c) + first, last - first);
                    low = juce::jmin (low, r.getStart()); high = juce::jmax (high, r.getEnd());
                }
            }
            if (low > high) continue;
            const float top = middle - juce::jlimit (-1.0f, 1.0f, high) * half, bottom = middle - juce::jlimit (-1.0f, 1.0f, low) * half;
            columns.addWithoutMerging ({ (float) x, top, 1.0f, juce::jmax (1.0f, bottom - top) });
        }
        g.setColour (ink.withAlpha (0.82f));
        g.fillRectList (columns);
        if (crossing > 0)
        {
            // In the crossing its own waveform is drawn lighter, so that the two can be told apart.
            g.setColour (colour.withAlpha (0.45f));
            g.fillRect (b.withWidth (crossing).withTrimmedTop (16));
        }
    }

    g.setColour (ink);
    g.setFont (Theme::font (11.5f, true));
    g.drawText (info != nullptr ? info->name : juce::String ("sound not in the library"), b.getX() + 6, b.getY() + 2, juce::jmax (0, b.getWidth() - 10), 15,
                juce::Justification::centredLeft, false);
    g.restoreState();

    g.setColour (ink.withAlpha (0.5f));
    g.drawRect (b, 1);
}

void SurfacePage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    // The two rows of the bar.
    auto top = getLocalBounds().removeFromTop (rowHeight).reduced (14, 0).withTrimmedRight (150);
    g.setColour (Theme::text);
    g.setFont (Theme::font (15.0f, true));
    const juce::String title ("Build surface");
    const int titleWidth = juce::roundToInt (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title)) + 16;
    g.drawText (title, top.removeFromLeft (titleWidth), juce::Justification::centredLeft, true);
    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (12.5f));
    if (project.isOpen())
    {
        const int tracks = (int) surface.tracks.size(), blocks = (int) surface.placements.size();
        g.drawText (project.getName() + juce::String::fromUTF8 ("   \xc2\xb7   ") + juce::String (tracks) + (tracks == 1 ? " track, " : " tracks, ")
                    + juce::String (blocks) + (blocks == 1 ? " block" : " blocks") + juce::String::fromUTF8 ("   \xc2\xb7   ") + timeText (surface.length(), true)
                    + juce::String::fromUTF8 ("   \xc2\xb7   ") + juce::String (project.getSampleRate() / 1000.0, 0) + " kHz",
                    top, juce::Justification::centredLeft, true);
    }

    auto second = getLocalBounds().withTrimmedTop (rowHeight).removeFromTop (rowHeight).reduced (14, 0).withTrimmedLeft (380);
    g.setColour (noticeIsProblem ? Theme::warning : Theme::textMuted);
    juce::String text = notice;
    if (text.isEmpty() && project.isOpen())
        text = (isPlaying() ? juce::String() : juce::String ("from ")) + timeText (playhead, true)
               + (loopEnd > loopStart ? juce::String::fromUTF8 ("   \xc2\xb7   loop ") + timeText (loopStart, true) + juce::String::fromUTF8 (" \xe2\x80\x93 ") + timeText (loopEnd, true) : juce::String());
    g.drawText (text, second, juce::Justification::centredLeft, true);

    const auto ruler = rulerArea(), lanes = lanesArea();
    if (! project.isOpen())
    {
        g.setColour (Theme::textMuted);
        g.setFont (Theme::font (14.0f));
        g.drawText ("No project is open. Open or make one in the tab Library and edition; its build surface shows here.",
                    getLocalBounds().withTrimmedTop (2 * rowHeight), juce::Justification::centred, true);
        return;
    }

    // The corner over the track headers, and what lies under the last header.
    g.setColour (Theme::panel);
    g.fillRect (0, ruler.getY(), headerWidth, getHeight() - ruler.getY());
    g.setColour (Theme::line);
    g.fillRect (headerWidth - 1, ruler.getY(), 1, getHeight() - ruler.getY());
    g.fillRect (0, ruler.getBottom() - 1, headerWidth, 1);

    // The paper: ruler and lanes.
    g.setColour (paper);
    g.fillRect (ruler.getUnion (lanes));
    g.saveState();
    g.reduceClipRegion (ruler.getUnion (lanes));

    for (int i = 0; i < (int) surface.tracks.size(); ++i)
    {
        const int y = trackTop (i);
        if (y >= lanes.getBottom() || y + trackHeight <= lanes.getY()) continue;
        if (i % 2 == 1) { g.setColour (paperAlt); g.fillRect (lanes.getX(), juce::jmax (lanes.getY(), y), lanes.getWidth(), trackHeight - juce::jmax (0, lanes.getY() - y)); }
    }
    const int tracksEnd = juce::jlimit (lanes.getY(), lanes.getBottom(), trackTop ((int) surface.tracks.size()));
    if (tracksEnd < lanes.getBottom()) { g.setColour (juce::Colour (0xffeceae3)); g.fillRect (lanes.getX(), tracksEnd, lanes.getWidth(), lanes.getBottom() - tracksEnd); }

    // Ruler: a mark at a step that leaves the labels room.
    const double steps[] = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 30000, 60000, 120000, 300000, 600000 };
    double step = 600000.0;
    for (double s : steps) if (s * pixelsPerMs >= 70.0) { step = s; break; }
    g.setFont (Theme::font (11.0f));
    for (double t = std::floor (scrollMs / step) * step; t < scrollMs + lanes.getWidth() / pixelsPerMs; t += step)
    {
        if (t < 0.0) continue;
        const int x = xOf (t);
        g.setColour (gridLine);
        g.fillRect (x, ruler.getY() + 18, 1, tracksEnd - ruler.getY() - 18);
        g.setColour (ink.withAlpha (0.8f));
        g.drawText (step < 1000.0 ? juce::String (t / 1000.0, step < 10.0 ? 3 : step < 100.0 ? 2 : 1) + "\"" : timeText (t, false),
                    x + 4, ruler.getY() + 2, 80, 16, juce::Justification::centredLeft, false);
    }
    g.setColour (ink.withAlpha (0.35f));
    g.fillRect (ruler.getX(), ruler.getBottom() - 1, ruler.getWidth(), 1);
    g.setColour (ink.withAlpha (0.14f));
    for (int i = 1; i <= (int) surface.tracks.size(); ++i)
    {
        const int y = trackTop (i) - 1;
        if (y > lanes.getY() && y < lanes.getBottom()) g.fillRect (lanes.getX(), y, lanes.getWidth(), 1);
    }

    // The loop, behind the blocks.
    if (loopEnd > loopStart)
    {
        const int a = xOf (loopStart), b = xOf (loopEnd);
        g.setColour (loopColour.withAlpha (loopOn ? 0.22f : 0.10f));
        g.fillRect (a, ruler.getY(), b - a, lanes.getBottom() - ruler.getY());
        g.setColour (loopColour.withAlpha (loopOn ? 0.9f : 0.45f));
        g.fillRect (a, ruler.getY(), 1, lanes.getBottom() - ruler.getY());
        g.fillRect (b, ruler.getY(), 1, lanes.getBottom() - ruler.getY());
    }

    if (surface.tracks.empty())
    {
        g.setColour (ink.withAlpha (0.6f));
        g.setFont (Theme::font (14.0f));
        g.drawText ("No tracks yet. \"+ Track\" adds one; \"+ Sound\" puts a sound of the library on a track.", lanes, juce::Justification::centred, true);
    }

    // The blocks, the earlier first so that a later one lies over it; the selected one last.
    g.saveState();
    g.reduceClipRegion (lanes);
    const auto fades = fadesOf (surface, [this] (const juce::String& id) { const auto f = sounds.find (id); return f != sounds.end() && f->second != nullptr; });
    std::vector<const maket::Placement*> order;
    for (const auto& p : surface.placements) order.push_back (&p);
    std::stable_sort (order.begin(), order.end(), [] (const maket::Placement* a, const maket::Placement* b) { return a->start < b->start; });
    for (const auto* p : order)
    {
        const auto f = fades.find (p->id);
        const double in = f != fades.end() ? f->second.first : 0.0;
        paintBlock (g, *p, lanes, in > p->fadeIn + 0.01 ? in : 0.0);
    }

    // Fades and crossings: the line of each block's level at its ends.
    g.setColour (crossColour);
    for (const auto& p : surface.placements)
    {
        const auto f = fades.find (p.id);
        if (f == fades.end()) continue;
        const auto b = blockBounds (p);
        if (b.isEmpty() || ! b.intersects (lanes)) continue;
        const bool crossedIn = f->second.first > p.fadeIn + 0.01, crossedOut = f->second.second > p.fadeOut + 0.01;
        for (int end = 0; end < 2; ++end)
        {
            const double fade = end == 0 ? f->second.first : f->second.second;
            const float width = (float) (fade * pixelsPerMs);
            if (width < 2.0f) continue;
            const bool equalPower = end == 0 ? crossedIn : crossedOut;
            juce::Path line;
            for (int i = 0; i <= 16; ++i)
            {
                const float t = (float) i / 16.0f;
                const float level = equalPower ? std::sin (t * juce::MathConstants<float>::halfPi) : t;
                const float x = end == 0 ? (float) b.getX() + t * width : (float) b.getRight() - t * width;
                const float y = (float) b.getBottom() - level * (float) b.getHeight();
                if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
            }
            g.strokePath (line, juce::PathStrokeType (1.6f));
        }
    }
    // A track that does not sound now (muted, or others are on solo) lies under a veil.
    bool anySolo = false;
    for (const auto& t : surface.tracks) anySolo = anySolo || t.solo;
    for (int i = 0; i < (int) surface.tracks.size(); ++i)
    {
        const auto& t = surface.tracks[(size_t) i];
        if (! (t.mute || (anySolo && ! t.solo))) continue;
        g.setColour (paper.withAlpha (0.6f));
        g.fillRect (lanes.getX(), trackTop (i), lanes.getWidth(), trackHeight - 1);
    }

    // The selected block's outline over everything, so that it shows also where another block lies over it.
    if (const auto* s = surface.find (selected))
    {
        g.setColour (chosen);
        g.drawRect (blockBounds (*s), 2);
    }
    g.restoreState();

    // The playhead.
    const int x = xOf (playhead);
    g.setColour (playColour);
    g.fillRect (x, ruler.getY(), 2, lanes.getBottom() - ruler.getY());
    juce::Path head;
    head.addTriangle ((float) x - 5.0f, (float) ruler.getY(), (float) x + 7.0f, (float) ruler.getY(), (float) x + 1.0f, (float) ruler.getY() + 9.0f);
    g.fillPath (head);
    g.restoreState();
}
