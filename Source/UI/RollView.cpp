// RollView.cpp

#include "RollView.h"
#include "MusicFont.h"
#include "Theme.h"
#include "../Notation/Chords.h"
#include "../Notation/Glides.h"
#include "../Notation/Spelling.h"

#include <algorithm>
#include <cmath>

using namespace maket::notation;

namespace
{
    constexpr int rulerHeight = 28;
    constexpr int barThickness = 12;
    constexpr double tailMs = 2000.0;           // empty paper after the last note

    // The paper and what is drawn on it.
    const juce::Colour paper       { 0xfffdfdfb };
    const juce::Colour ink         { 0xff101010 };
    const juce::Colour staffLine   { 0xff3c3c3c };
    const juce::Colour gridMinor   { 0xffeeeeeb };
    const juce::Colour gridMajor   { 0xffdcdcd8 };
    const juce::Colour rulerText   { 0xff454545 };
    const juce::Colour rulerLine   { 0xffb9b9b4 };
    const juce::Colour loopColour  = juce::Colour::fromFloatRGBA (0.68f, 0.48f, 0.69f, 1.0f);      // bach's
    const juce::Colour playColour  = juce::Colour::fromFloatRGBA (0.34f, 0.87f, 0.20f, 1.0f);      // bach's

    juce::String clockLabel (double seconds, bool withTenths)
    {
        const int minutes = (int) std::floor (seconds / 60.0 + 1.0e-9);
        const double rest = seconds - 60.0 * minutes;
        return juce::String (minutes) + "'" + (withTenths ? juce::String (rest, 1).paddedLeft ('0', 4)
                                                          : juce::String (juce::roundToInt (rest)).paddedLeft ('0', 2)) + "\"";
    }

    /** The distance between labelled ticks and between small ticks, in seconds, for a zoom. */
    void tickSteps (double pixelsPerSecond, double& major, double& minor)
    {
        static const double majors[] = { 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 15.0, 30.0, 60.0, 120.0, 300.0, 600.0 };
        static const double minors[] = { 0.02, 0.05, 0.1, 0.25, 0.5, 1.0, 2.0, 5.0, 10.0, 15.0, 30.0, 60.0, 120.0 };
        size_t i = 0;
        while (i + 1 < std::size (majors) && majors[i] * pixelsPerSecond < 72.0) ++i;
        major = majors[i];
        minor = minors[i];
    }

    juce::juce_wchar clefSign (const Staff& s)
    {
        switch (s.sign)
        {
            case ClefSign::g:
                return s.octaves == 2 ? MusicFont::gClef15ma : s.octaves == 1 ? MusicFont::gClef8va
                     : s.octaves == -1 ? MusicFont::gClef8vb : s.octaves == -2 ? MusicFont::gClef15mb : MusicFont::gClef;
            case ClefSign::f:
                return s.octaves == 2 ? MusicFont::fClef15ma : s.octaves == 1 ? MusicFont::fClef8va
                     : s.octaves == -1 ? MusicFont::fClef8vb : s.octaves == -2 ? MusicFont::fClef15mb : MusicFont::fClef;
            case ClefSign::c:           return MusicFont::cClef;
            case ClefSign::percussion:  return MusicFont::percussionClef;
            case ClefSign::none:        break;
        }
        return 0;
    }
}

RollView::RollView()
{
    setOpaque (true);
    paperLayer.setOpaque (true);
    paperLayer.setBufferedToImage (true);
    for (auto* layer : { &paperLayer, &overlayLayer })
    {
        layer->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*layer);
    }
    for (auto* bar : { &horizontalBar, &verticalBar })
    {
        bar->setAutoHide (false);
        bar->addListener (this);
        addAndMakeVisible (*bar);
    }
}

RollView::~RollView() = default;

// --- The roll ---------------------------------------------------------------------

void RollView::clear()
{
    roll = {};
    voices.clear();
    rollLength = heightUnits = 0.0;
    scrollMs = scrollY = 0.0;
    updateScrollBars();
    paperLayer.repaint();
}

namespace
{
    juce::juce_wchar signOf (Accidental a)
    {
        switch (a)
        {
            case Accidental::naturalUp:     return MusicFont::naturalArrowUp;
            case Accidental::quarterSharp:  return MusicFont::quarterSharp;
            case Accidental::quarterSharpUp: return MusicFont::quarterSharpArrowUp;
            case Accidental::sharp:         return MusicFont::sharp;
            case Accidental::naturalDown:   return MusicFont::naturalArrowDown;
            case Accidental::quarterFlat:   return MusicFont::quarterFlat;
            case Accidental::quarterFlatDown: return MusicFont::quarterFlatArrowDown;
            case Accidental::flat:          return MusicFont::flat;
            case Accidental::none:          break;
        }
        return 0;
    }
}

void RollView::setRoll (const maket::Roll& newRoll)
{
    roll = newRoll;
    rebuild();
    scrollMs = scrollY = 0.0;
    updateScrollBars();
    paperLayer.repaint();
}

void RollView::setToneDivision (int newDivision)
{
    newDivision = newDivision == 2 || newDivision == 4 ? newDivision : 8;
    if (newDivision == division) return;
    division = newDivision;
    rebuild();
    paperLayer.repaint();
}

void RollView::setVelocityDisplay (VelocityDisplay newMode)
{
    if (newMode == velocityMode) return;
    velocityMode = newMode;
    paperLayer.repaint();
}

juce::Colour RollView::flagColour (int itemFlags, juce::Colour otherwise)
{
    // bach's colours for what is locked, muted or solo; together, their mean.
    float r = 0.0f, g = 0.0f, b = 0.0f;
    int count = 0;
    if (itemFlags & maket::flagLocked)  { r += 0.83f; g += 0.04f; b += 0.04f; ++count; }
    if (itemFlags & maket::flagMuted)   { r += 0.29f; g += 0.29f; b += 0.72f; ++count; }
    if (itemFlags & maket::flagSolo)    { r += 0.61f; g += 0.61f; b += 0.21f; ++count; }
    if (count == 0) return otherwise;
    return juce::Colour::fromFloatRGBA (r / (float) count, g / (float) count, b / (float) count, 1.0f);
}

juce::Colour RollView::colourOf (int velocity, int itemFlags) const
{
    // bach counts from velocity 1 to 127.
    const float loud = (float) juce::jlimit (0, 126, velocity - 1) / 126.0f;
    const auto base = flagColour (itemFlags, ink);
    switch (velocityMode)
    {
        case VelocityDisplay::spectrum:     return juce::Colour::fromHSL ((240.0f / 360.0f) * (1.0f - loud), 0.85f, 0.42f, 1.0f);
        case VelocityDisplay::alpha:        return base.withAlpha (0.3f + 0.7f * loud);
        case VelocityDisplay::greyScale:    break;
    }
    return base.interpolatedWith (juce::Colours::white, 0.8f * (1.0f - loud));
}

void RollView::rebuild()
{
    const double began = juce::Time::getMillisecondCounterHiRes();
    auto& font = MusicFont::get();
    voices.clear();
    startReach = 0.0;
    const auto layout = layoutVoices (roll);
    heightUnits = layout.height;
    rollLength = roll.length();

    const auto nameFont = Theme::font (13.0f);
    nameWidth = 0.0f;

    for (const auto& place : layout.voices)
    {
        VoiceItem item;
        item.place = place;
        const auto& voice = roll.voices[(size_t) place.voice];
        item.name = voice.name;
        item.flags = voice.flags;
        if (! place.hidden)
            nameWidth = juce::jmax (nameWidth, juce::GlyphArrangement::getStringWidth (nameFont, item.name));

        for (const auto& chord : voice.chords)
        {
            if (chord.notes.empty()) continue;

            ChordItem c;
            c.onset = chord.onset;
            c.flags = chord.flags | voice.flags;
            int lowest = 1 << 20, highest = -(1 << 20);
            double sum = 0.0;

            // bach's order within a chord: by pitch, lowest first.
            std::vector<const maket::Note*> byPitch;
            for (const auto& note : chord.notes) byPitch.push_back (&note);
            std::stable_sort (byPitch.begin(), byPitch.end(), [] (const maket::Note* a, const maket::Note* b) { return a->cents < b->cents; });

            c.maxVelocity = 1;
            for (const auto* notePointer : byPitch)
            {
                const auto& note = *notePointer;
                NoteItem n;
                const auto spelling = spellAs (note.pitchName, note.cents, division);
                n.step = spelling.steps;
                n.accidental = signOf (accidentalOf (spelling.alteration));
                n.units = place.offset + place.clef.unitsOf (n.step);
                n.end = chord.onset + note.duration;
                n.velocity = note.velocity;
                n.flags = note.flags | c.flags;
                n.glide = glidePoints (spelling, note.breakpoints);
                n.ledgers = ledgerSteps (place.clef, n.step);
                c.end = juce::jmax (c.end, n.end);
                c.maxVelocity = juce::jmax (c.maxVelocity, note.velocity);
                lowest = juce::jmin (lowest, n.step);
                highest = juce::jmax (highest, n.step);
                sum += n.step;
                c.notes.push_back (std::move (n));
            }

            // The stem: bach's rule for its direction (Notation/Staves); an octave long beyond the outer note.
            c.direction = stemDirection (place, sum / (double) c.notes.size());
            const double lowUnits = place.offset + place.clef.unitsOf (lowest), highUnits = place.offset + place.clef.unitsOf (highest);
            if (c.direction == 1)   { c.stemFromUnits = lowUnits;  c.stemToUnits = highUnits - 7.0 * stepUnits; }
            else                    { c.stemFromUnits = highUnits; c.stemToUnits = lowUnits + 7.0 * stepUnits; }

            // The heads against the stem, and the accidentals left of the chord: bach's methods (Notation/Chords),
            // with the sizes of this font's signs.
            {
                const double headWidth = font.noteheadWidth();
                std::vector<int> steps;
                for (const auto& n : c.notes) steps.push_back (n.step);
                const auto heads = placeHeads (steps, c.direction, headWidth);

                std::vector<ChordNote> placing;
                for (size_t i = 0; i < c.notes.size(); ++i)
                {
                    auto& n = c.notes[i];
                    n.headLeft = (float) heads[i].left;
                    n.auxiliaryStem = heads[i].auxiliaryStem;

                    ChordNote p;
                    p.step = n.step;
                    p.headLeft = heads[i].left;
                    const auto b = n.accidental != 0 ? font.bounds (n.accidental) : juce::Rectangle<float>();
                    if (b.isEmpty()) n.accidental = 0;
                    p.hasAccidental = n.accidental != 0;
                    p.above = -b.getY();
                    p.below = b.getBottom();
                    p.width = b.getRight();
                    placing.push_back (p);
                }
                const auto right = placeAccidentals (placing);
                double reach = 0.0;
                for (size_t i = 0; i < c.notes.size(); ++i)
                {
                    c.notes[i].accidentalLeft = (float) (right[i] - placing[i].width);
                    reach = juce::jmax (reach, -placing[i].headLeft, placing[i].hasAccidental ? -(double) c.notes[i].accidentalLeft : 0.0);
                }
                // The stem stands half a head beside the onset (see paintNotes).
                if (c.onset <= 250.0) startReach = juce::jmax (startReach, reach - c.direction * 0.5 * headWidth);
            }

            item.chords.push_back (std::move (c));
        }
        voices.push_back (std::move (item));
    }

    markers.clear();
    for (const auto& m : roll.markers)
    {
        MarkerItem item;
        item.ms = m.position;
        item.role = m.role == "none" ? juce::String() : m.role;
        // bach writes the name of a plain marker; for a tempo or a time signature what it holds; for other roles nothing.
        if (item.role.isEmpty())
            item.name = m.nameAsText();
        else if ((item.role == "tempo" || item.role == "timesig") && m.hasContent)
        {
            juce::StringArray words;
            if (m.content.isList()) for (const auto& w : m.content.items) words.add (w.toLine());
            else                    words.add (m.content.toLine());
            item.name = words.joinIntoString (" ");
        }
        markers.push_back (std::move (item));
    }
    std::stable_sort (markers.begin(), markers.end(), [] (const MarkerItem& a, const MarkerItem& b) { return a.ms < b.ms; });
    rebuildMs = juce::Time::getMillisecondCounterHiRes() - began;
}

// --- Geometry ---------------------------------------------------------------------

int RollView::headerWidth() const
{
    const float space = spacePixels();
    return juce::roundToInt (nameColumn() + 4.8f * space + 6.0f);
}

float RollView::nameColumn() const
{
    // Room for the names, and for the tags (2e) when there are tags: under the name, or small beside it when the
    // staves stand close (3a). The column is as wide either way, so that it does not jump with the zoom.
    return voiceTags.isEmpty() ? juce::jmax (36.0f, nameWidth + 20.0f) : juce::jmax (72.0f, nameWidth + 20.0f + smallTag + 2.0f);
}

void RollView::updateTagLayout()
{
    // Does a name with its tag under it fit between neighbouring staves everywhere? If not, every tag goes small.
    compactTags = false;
    if (voiceTags.isEmpty()) return;
    bool first = true;
    double reachOfPrevious = 0.0;
    for (const auto& v : voices)
    {
        if (v.place.hidden || ! v.place.ownsStaves) continue;
        const double middle = (v.place.offset + 0.5 * (v.place.clef.topUnits() + v.place.clef.bottomUnits())) * pixelsPerUnit();
        const double parts = v.place.numParts;
        const double above = parts <= 1 ? 10.0 : 14.5 * parts, below = parts <= 1 ? 27.0 : 14.5 * parts;
        if (! first && middle - above < reachOfPrevious + 1.0) { compactTags = true; return; }
        reachOfPrevious = middle + below;
        first = false;
    }
}

juce::Rectangle<float> RollView::nameBounds (const VoiceItem& v) const
{
    const float topY = (float) yOf (v.place.offset + v.place.clef.topUnits()), bottomY = (float) yOf (v.place.offset + v.place.clef.bottomUnits());
    const float middle = (topY + bottomY) * 0.5f;
    const float left = compactTags ? 4.0f + smallTag + 2.0f : 4.0f, width = nameColumn() - 4.0f - left;
    if (v.place.numParts <= 1) return { left, middle - 10.0f, width, 20.0f };

    // Voices that share their staves (bach's voice ensemble): their names one under the other beside the staves, as in bach.roll.
    const float row = voiceTags.isEmpty() || compactTags ? 15.0f : 29.0f;
    return { left, middle - 0.5f * row * (float) v.place.numParts + row * (float) v.place.partIndex, width, 15.0f };
}

double RollView::zeroPad() const
{
    // Between the clefs and time zero: room for the heads and the accidentals of the chords that begin the roll.
    return juce::jmax (4.0, startReach + 0.4) * spacePixels();
}

juce::Rectangle<int> RollView::rollArea() const
{
    return getLocalBounds().withTrimmedTop (rulerHeight).withTrimmedLeft (headerWidth())
                           .withTrimmedRight (barThickness).withTrimmedBottom (barThickness);
}

double RollView::xOf (double ms) const
{
    return rollArea().getX() + zeroPad() + (ms - scrollMs) * pixelsPerMs();
}

double RollView::timeAt (double x) const
{
    return scrollMs + (x - rollArea().getX() - zeroPad()) / pixelsPerMs();
}

double RollView::yOf (double units) const
{
    return rulerHeight + units * pixelsPerUnit() - scrollY;
}

juce::String RollView::describeView() const
{
    return "across " + juce::String (juce::roundToInt (zoomX * 100.0)) + " %   down " + juce::String (juce::roundToInt (zoomY * 100.0)) + " %";
}

// --- Zoom and scroll ----------------------------------------------------------------

void RollView::setHorizontalZoom (double zoom, double anchorX)
{
    const auto area = rollArea();
    if (anchorX < area.getX()) anchorX = area.getX() + zeroPad();
    const double time = timeAt (anchorX);
    zoomX = juce::jlimit (minZoomX, maxZoomX, zoom);
    scrollMs = time - (anchorX - area.getX() - zeroPad()) / pixelsPerMs();
    viewChanged();
}

void RollView::setVerticalZoom (double zoom, double anchorY)
{
    if (anchorY < rulerHeight) anchorY = rulerHeight;
    const double units = (anchorY - rulerHeight + scrollY) / pixelsPerUnit();
    const double timeLeft = scrollMs;                   // the header widens with the zoom; keep the time at the left edge
    zoomY = juce::jlimit (minZoomY, maxZoomY, zoom);
    scrollY = units * pixelsPerUnit() - (anchorY - rulerHeight);
    scrollMs = timeLeft;
    viewChanged();
}

void RollView::zoomToFit()
{
    if (! hasRoll()) return;
    const auto area = rollArea();
    zoomY = juce::jlimit (minZoomY, 1.5, (area.getHeight() - 10.0) / juce::jmax (1.0, heightUnits));
    const auto after = rollArea();
    zoomX = juce::jlimit (minZoomX, maxZoomX, (after.getWidth() - 5.0 * spacePixels()) / juce::jmax (1.0, rollLength * 0.1));
    scrollMs = scrollY = 0.0;
    viewChanged();
}

void RollView::viewChanged()
{
    updateTagLayout();
    const auto area = rollArea();
    const double visibleMs = area.getWidth() / pixelsPerMs();
    scrollMs = juce::jlimit (0.0, juce::jmax (0.0, rollLength + tailMs - visibleMs), scrollMs);
    scrollY = juce::jlimit (0.0, juce::jmax (0.0, heightUnits * pixelsPerUnit() + 20.0 - area.getHeight()), scrollY);
    updateScrollBars();
    paperLayer.repaint();
    overlayLayer.repaint();
    if (onViewChanged) onViewChanged();
}

void RollView::updateScrollBars()
{
    const auto area = rollArea();
    const double visibleMs = area.getWidth() / pixelsPerMs();
    horizontalBar.setRangeLimits (0.0, juce::jmax (visibleMs, rollLength + tailMs), juce::dontSendNotification);
    horizontalBar.setCurrentRange (scrollMs, visibleMs, juce::dontSendNotification);
    verticalBar.setRangeLimits (0.0, juce::jmax ((double) area.getHeight(), heightUnits * pixelsPerUnit() + 20.0), juce::dontSendNotification);
    verticalBar.setCurrentRange (scrollY, area.getHeight(), juce::dontSendNotification);
}

void RollView::scrollBarMoved (juce::ScrollBar* bar, double newRangeStart)
{
    if (bar == &horizontalBar) scrollMs = newRangeStart;
    else                       scrollY = newRangeStart;
    paperLayer.repaint();
    overlayLayer.repaint();
    if (onViewChanged) onViewChanged();
}

void RollView::resized()
{
    paperLayer.setBounds (getLocalBounds());
    overlayLayer.setBounds (getLocalBounds());
    auto area = getLocalBounds();
    verticalBar.setBounds (area.removeFromRight (barThickness).withTrimmedBottom (barThickness));
    horizontalBar.setBounds (area.removeFromBottom (barThickness));
    viewChanged();
}

void RollView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (! hasRoll()) return;

    if (e.mods.isCommandDown())
    {
        setHorizontalZoom (zoomX * std::pow (2.0, wheel.deltaY * 2.0), e.position.x);
        return;
    }
    if (e.mods.isAltDown())
    {
        setVerticalZoom (zoomY * std::pow (2.0, wheel.deltaY * 2.0), e.position.y);
        return;
    }

    double dx = wheel.deltaX, dy = wheel.deltaY;
    if (e.mods.isShiftDown() && std::abs (dx) < 1.0e-6) { dx = dy; dy = 0.0; }
    scrollMs -= dx * 400.0 / pixelsPerMs();
    scrollY -= dy * 400.0;
    viewChanged();
}

void RollView::mouseMagnify (const juce::MouseEvent& e, float scaleFactor)
{
    if (hasRoll()) setHorizontalZoom (zoomX * scaleFactor, e.position.x);
}

// --- Drawing ------------------------------------------------------------------------

void RollView::paint (juce::Graphics& g)
{
    g.fillAll (paper);      // the layers cover it
}

void RollView::paintPaper (juce::Graphics& g)
{
    const double began = juce::Time::getMillisecondCounterHiRes();
    notesDrawn = 0;
    g.fillAll (paper);

    const auto area = rollArea();
    if (hasRoll() && ! area.isEmpty())
    {
        {
            juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (area);

            // The time grid behind everything.
            double major = 1.0, minor = 0.5;
            tickSteps (pixelsPerMs() * 1000.0, major, minor);
            const double first = std::floor (timeAt (area.getX()) / 1000.0 / minor) * minor;
            for (double t = juce::jmax (0.0, first); xOf (t * 1000.0) < area.getRight(); t += minor)
            {
                const bool isMajor = std::abs (t / major - std::round (t / major)) < 1.0e-6;
                g.setColour (isMajor ? gridMajor : gridMinor);
                g.fillRect (juce::Rectangle<float> ((float) xOf (t * 1000.0) - 0.5f, (float) area.getY(), 1.0f, (float) area.getHeight()));
            }

            // The loop region over the staves, when play goes round it.
            if (loopShownOn && loopEndMs > loopStartMs)
            {
                const float x1 = (float) xOf (loopStartMs), x2 = (float) xOf (loopEndMs);
                g.setColour (loopColour.withAlpha (0.10f));
                g.fillRect (juce::Rectangle<float> (x1, (float) area.getY(), x2 - x1, (float) area.getHeight()));
                g.setColour (loopColour.withAlpha (0.8f));
                g.fillRect (juce::Rectangle<float> (x1 - 0.5f, (float) area.getY(), 1.0f, (float) area.getHeight()));
                g.fillRect (juce::Rectangle<float> (x2 - 0.5f, (float) area.getY(), 1.0f, (float) area.getHeight()));
            }

            paintStaves (g, area.getX(), area.getRight());
            paintMarkers (g, area, false);
            paintNotes (g, area);
            paintMarkers (g, area, true);
        }
        paintHeader (g, { 0, rulerHeight, headerWidth(), area.getHeight() });
        paintRuler (g, { 0, 0, getWidth() - barThickness, rulerHeight });

        // The loop region in the ruler: always shown when there is one, stronger when play goes round it.
        if (loopEndMs > loopStartMs)
        {
            juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (area.getX(), 0, area.getWidth(), rulerHeight);
            const float x1 = (float) xOf (loopStartMs), x2 = (float) xOf (loopEndMs);
            g.setColour (loopColour.withAlpha (loopShownOn ? 0.45f : 0.18f));
            g.fillRect (juce::Rectangle<float> (x1, 0.0f, x2 - x1, (float) rulerHeight));
            g.setColour (loopColour.withAlpha (loopShownOn ? 1.0f : 0.5f));
            g.fillRect (juce::Rectangle<float> (x1 - 1.0f, 0.0f, 2.0f, (float) rulerHeight));
            g.fillRect (juce::Rectangle<float> (x2 - 1.0f, 0.0f, 2.0f, (float) rulerHeight));
        }
    }

    // The corner between the two scroll bars.
    g.setColour (Theme::panel);
    g.fillRect (getWidth() - barThickness, getHeight() - barThickness, barThickness, barThickness);

    if (showTimes && hasRoll())
    {
        const double took = juce::Time::getMillisecondCounterHiRes() - began;
        const juce::String text = "drawn in " + juce::String (took, 1) + " ms (" + juce::String (notesDrawn) + " notes in view); notation worked out in "
                                  + juce::String (rebuildMs, 1) + " ms";
        const auto font = Theme::font (11.0f);
        const auto box = juce::Rectangle<int> (4, getHeight() - barThickness - 20, juce::roundToInt (juce::GlyphArrangement::getStringWidth (font, text)) + 12, 18);
        g.setColour (paper.withAlpha (0.9f));
        g.fillRect (box);
        g.setColour (juce::Colour (0xff555555));
        g.setFont (font);
        g.drawText (text, box.reduced (6, 0), juce::Justification::centredLeft, false);
    }
}

// --- The playhead and the loop (stage 2d) ------------------------------------------

juce::Rectangle<int> RollView::playheadStrip (double ms) const
{
    const int x = juce::roundToInt (xOf (ms));
    return { x - 8, 0, 16, getHeight() - barThickness };
}

void RollView::setPlayhead (double ms)
{
    if (std::abs (ms - playheadMs) < 1.0e-6) return;
    const auto before = playheadStrip (playheadMs);
    playheadMs = ms;
    overlayLayer.repaint (before);
    overlayLayer.repaint (playheadStrip (playheadMs));
}

void RollView::paintOverlay (juce::Graphics& g)
{
    if (! hasRoll()) return;
    const auto area = rollArea();
    const float x = (float) xOf (playheadMs);
    if (x < (float) area.getX() - 1.0f || x > (float) area.getRight() + 1.0f) return;

    // bach's play colour, a shade darker so that it reads on paper.
    g.setColour (playColour.darker (0.15f));
    g.fillRect (juce::Rectangle<float> (x - 0.75f, 0.0f, 1.5f, (float) area.getBottom()));
    juce::Path flag;
    flag.addTriangle (x - 6.0f, 0.0f, x + 6.0f, 0.0f, x, 9.0f);
    g.fillPath (flag);
}

void RollView::turnPageTo (double ms)
{
    const auto area = rollArea();
    const double x = xOf (ms);
    if (x >= area.getX() && x <= area.getRight() - 2) return;
    scrollMs = ms;
    viewChanged();
}

void RollView::setLoop (double startMs, double endMs, bool on)
{
    if (std::abs (startMs - loopStartMs) < 1.0e-6 && std::abs (endMs - loopEndMs) < 1.0e-6 && on == loopShownOn) return;
    loopStartMs = startMs;
    loopEndMs = endMs;
    loopShownOn = on;
    paperLayer.repaint();
}

void RollView::setVoiceTags (const juce::StringArray& tags)
{
    if (tags == voiceTags) return;
    const bool widthChanges = tags.isEmpty() != voiceTags.isEmpty();
    voiceTags = tags;
    updateTagLayout();
    if (widthChanges) viewChanged();        // the names' column gets room for the tags
    paperLayer.repaint();
}

juce::Rectangle<float> RollView::tagBounds (const VoiceItem& v) const
{
    const auto name = nameBounds (v);
    // Staves close together: a small tag with one letter, left of the name.
    if (compactTags) return { 4.0f, name.getCentreY() - 6.5f, smallTag, 13.0f };

    // Under the name, in the middle of the names' column.
    const float width = juce::jmin (64.0f, name.getWidth());
    const bool shared = v.place.numParts > 1;
    return { name.getCentreX() - 0.5f * width, name.getBottom() + (shared ? -1.0f : 1.0f), width, shared ? 13.0f : 15.0f };
}

void RollView::mouseDown (const juce::MouseEvent& e)
{
    // A click on a voice's tag.
    if (hasRoll() && e.x < rollArea().getX() && e.y > rulerHeight && onVoiceTagClicked)
        for (size_t i = 0; i < voices.size(); ++i)
            if (! voices[i].place.hidden && voices[i].place.voice < voiceTags.size()
                && tagBounds (voices[i]).expanded (2.0f).contains (e.position))
            {
                onVoiceTagClicked (voices[i].place.voice);
                pressInRuler = draggingLoop = false;
                return;
            }

    const auto area = rollArea();
    pressInRuler = hasRoll() && e.y < rulerHeight && e.x >= area.getX() && e.x < area.getRight();
    draggingLoop = loopDragged = false;
    if (! pressInRuler) return;

    const double ms = juce::jlimit (0.0, rollLength, timeAt (e.x));
    if (e.mods.isAltDown())
    {
        draggingLoop = true;
        loopAnchorMs = ms;
    }
    else if (onPlayheadSet) onPlayheadSet (ms, false);
}

void RollView::mouseDrag (const juce::MouseEvent& e)
{
    if (! pressInRuler) return;
    const double ms = juce::jlimit (0.0, rollLength, timeAt (e.x));
    if (draggingLoop)
    {
        if (std::abs (e.getDistanceFromDragStartX()) < 3) return;
        loopDragged = true;
        if (onLoopSet) onLoopSet (juce::jmin (loopAnchorMs, ms), juce::jmax (loopAnchorMs, ms));
    }
    else if (onPlayheadSet) onPlayheadSet (ms, true);
}

void RollView::mouseUp (const juce::MouseEvent&)
{
    // The option key and a click without a drag: no loop.
    if (pressInRuler && draggingLoop && ! loopDragged && onLoopSet) onLoopSet (0.0, 0.0);
    pressInRuler = draggingLoop = false;
}

void RollView::paintStaves (juce::Graphics& g, int fromX, int toX)
{
    const float thickness = juce::jmax (1.0f, 0.11f * spacePixels());
    const auto clip = g.getClipBounds();

    for (const auto& v : voices)
    {
        if (v.place.hidden || ! v.place.ownsStaves) continue;
        g.setColour (flagColour (v.flags, staffLine));
        for (const auto& staff : v.place.clef.staves)
            for (int k = 0; k < 5; ++k)
            {
                const float y = (float) yOf (v.place.offset + v.place.clef.unitsOf (staff.topLineStep - 2 * k));
                if (y < (float) (clip.getY() - 2) || y > (float) (clip.getBottom() + 2)) continue;
                g.fillRect (juce::Rectangle<float> ((float) fromX, y - thickness * 0.5f, (float) (toX - fromX), thickness));
            }
    }
}

void RollView::paintNotes (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto& font = MusicFont::get();
    const float space = spacePixels();
    const float headWidth = font.noteheadWidth() * space;
    const float lineThickness = juce::jmax (1.5f, space / 3.0f);       // bach: 2 of its units; a staff space is 6
    const float thin = juce::jmax (1.0f, 0.12f * space);
    const float tailWidth = juce::jmax (1.0f, space / 3.0f), tailHalf = space / 3.0f;       // bach: 2 units wide, 2 up and down
    const float ledgerThickness = juce::jmax (1.0f, 0.14f * space);

    const double fromMs = timeAt (area.getX() - 3.0 * space), toMs = timeAt (area.getRight() + 3.0 * space);
    const float top = (float) area.getY() - 8.0f * space, bottom = (float) area.getBottom() + 8.0f * space;

    for (const auto& v : voices)
    {
        if (v.place.hidden) continue;

        for (const auto& c : v.chords)
        {
            if (c.end < fromMs || c.onset > toMs) continue;

            // As in bach.roll, the onset is the middle of the head that sits at the stem; the stem half a head beside it.
            const float stemX = (float) xOf (c.onset) + (float) c.direction * 0.5f * headWidth;
            const float topY = (float) yOf (c.direction == 1 ? c.stemToUnits + 7.0 * stepUnits : c.stemFromUnits);         // the highest note
            const float bottomY = (float) yOf (c.direction == 1 ? c.stemFromUnits : c.stemToUnits - 7.0 * stepUnits);      // the lowest note
            const float stemLength = 3.5f * space;
            const auto stemColour = colourOf (c.maxVelocity, c.flags);

            for (const auto& n : c.notes)
            {
                const float y = (float) yOf (n.units);
                if (y < top || y > bottom) continue;
                ++notesDrawn;
                const float headLeft = stemX + n.headLeft * space;
                const float headCentre = headLeft + headWidth * 0.5f;

                // Ledger lines.
                g.setColour (staffLine);
                for (int step : n.ledgers)
                {
                    const float ly = (float) yOf (v.place.offset + v.place.clef.unitsOf (step));
                    g.fillRect (juce::Rectangle<float> (headLeft - 0.38f * space, ly - ledgerThickness * 0.5f, headWidth + 0.76f * space, ledgerThickness));
                }

                // The duration line and its tail: from the right edge of the head to the note's end.
                const auto colour = colourOf (n.velocity, n.flags);
                const float startX = headLeft + headWidth - 0.08f * space;
                const float endX = (float) xOf (n.end);
                g.setColour (colour);
                if (n.glide.empty())
                {
                    if (endX > startX)
                    {
                        g.fillRect (juce::Rectangle<float> (startX, y - lineThickness * 0.5f, endX - startX, lineThickness));
                        g.fillRect (juce::Rectangle<float> (endX - tailWidth * 0.5f, y - tailHalf, tailWidth, 2.0f * tailHalf));
                    }
                }
                else
                {
                    // A glide: from point to point along bach's curve, a diamond at each point between head and tail.
                    const double onsetX = xOf (c.onset), length = (double) endX - onsetX;
                    auto pointX = [&] (const GlidePoint& p)   { return p.position <= 0.0 ? startX : (float) (onsetX + p.position * length); };
                    auto pointY = [&] (double steps)          { return (float) yOf (v.place.offset + v.place.clef.unitsOf (steps)); };

                    juce::Path line;
                    line.startNewSubPath (startX, y);
                    for (size_t i = 1; i < n.glide.size(); ++i)
                    {
                        const auto& a = n.glide[i - 1];
                        const auto& b = n.glide[i];
                        const float x1 = pointX (a), x2 = pointX (b);
                        const float y1 = i == 1 ? y : pointY (a.steps), y2 = pointY (b.steps);
                        const int pieces = std::abs (b.slope) < 1.0e-9 || std::abs (y1 - y2) < 0.01f ? 1 : juce::jlimit (4, 200, (int) ((x2 - x1) / 3.0f));
                        for (int k = 1; k <= pieces; ++k)
                        {
                            const double t = (double) k / pieces;
                            line.lineTo (x1 + (x2 - x1) * (float) t, y1 + (y2 - y1) * (float) slopeCurve (t, b.slope));
                        }
                    }
                    g.strokePath (line, juce::PathStrokeType (lineThickness, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

                    for (size_t i = 1; i + 1 < n.glide.size(); ++i)
                    {
                        const float px = pointX (n.glide[i]), py = pointY (n.glide[i].steps);
                        juce::Path diamond;
                        diamond.addQuadrilateral (px - 0.3f * space, py, px, py - 0.5f * space, px + 0.3f * space, py, px, py + 0.5f * space);
                        g.setColour (colour);
                        g.fillPath (diamond);
                        g.setColour (paper);
                        g.strokePath (diamond, juce::PathStrokeType (0.9f));
                    }
                    g.setColour (colour);
                    const float tailY = pointY (n.glide.back().steps);
                    g.fillRect (juce::Rectangle<float> (endX - tailWidth * 0.5f, tailY - tailHalf, tailWidth, 2.0f * tailHalf));
                }

                // A head away from the stem: bach's branch, from the stem to above (or under) the head.
                if (n.auxiliaryStem)
                {
                    g.setColour (stemColour);
                    const float fromY = c.direction == 1 ? topY - 0.4f * stemLength : bottomY + 0.4f * stemLength;
                    const float bendY = c.direction == 1 ? topY - 0.2f * stemLength : bottomY + 0.2f * stemLength;
                    const float headY = c.direction == 1 ? y - 0.25f * space : y + 0.25f * space;
                    const float branch = juce::jmax (1.0f, 0.87f * thin);
                    g.drawLine (stemX - (float) c.direction * 0.5f * thin, fromY, headCentre, bendY, branch);
                    g.drawLine (headCentre, bendY, headCentre, headY, branch);
                    g.setColour (colour);
                }

                // The head.
                if (font.isLoaded())
                    font.draw (g, MusicFont::noteheadBlack, headLeft, y, space);
                else
                    g.fillEllipse (headLeft, y - 0.5f * space, headWidth, space);

                // The accidental.
                if (n.accidental != 0)
                    font.draw (g, n.accidental, stemX + n.accidentalLeft * space, y, space);
            }

            // The stem.
            const float y1 = (float) yOf (c.stemFromUnits), y2 = (float) yOf (c.stemToUnits);
            if (juce::jmax (y1, y2) >= top && juce::jmin (y1, y2) <= bottom)
            {
                g.setColour (stemColour);
                const float x = c.direction == 1 ? stemX - thin : stemX;
                g.fillRect (juce::Rectangle<float> (x, juce::jmin (y1, y2), thin, std::abs (y2 - y1)));
            }
        }
    }
}

void RollView::paintMarkers (juce::Graphics& g, juce::Rectangle<int> area, bool names)
{
    if (markers.empty()) return;

    // bach's marker: a green line over the whole height, its name in bold beside the top of it. A marker with a
    // role (bar line, time signature, tempo ...) is dashed in that role's colour, with the role written along it.
    const juce::Colour green = juce::Colour::fromFloatRGBA (0.24f, 0.70f, 0.19f, 0.9f);
    auto roleColour = [] (const juce::String& role)
    {
        if (role == "tempo")    return juce::Colour::fromFloatRGBA (0.6f, 0.2f, 0.2f, 1.0f);
        if (role == "timesig")  return juce::Colour::fromFloatRGBA (0.2f, 0.2f, 0.6f, 1.0f);
        if (role == "barline")  return juce::Colour::fromFloatRGBA (0.2f, 0.6f, 0.6f, 1.0f);
        if (role == "division") return juce::Colour::fromFloatRGBA (0.6f, 0.6f, 0.2f, 1.0f);
        return juce::Colour::fromFloatRGBA (0.6f, 0.2f, 0.6f, 1.0f);
    };
    auto roleWord = [] (const juce::String& role)
    {
        if (role == "tempo")    return juce::String ("TEMPO");
        if (role == "timesig")  return juce::String ("TIME SIG");
        if (role == "barline")  return juce::String ("BARLINE");
        if (role == "division") return juce::String ("DIVISION");
        return role.toUpperCase();
    };

    const float space = spacePixels();
    const float top = (float) area.getY(), bottom = (float) area.getBottom();
    const auto nameFont = Theme::font (juce::jlimit (10.0f, 18.0f, 1.5f * space)).boldened();        // bach: 9 units, bold

    if (! names)
    {
        for (const auto& m : markers)
        {
            const float x = (float) xOf (m.ms);
            if (x < (float) area.getX() - 2.0f || x > (float) area.getRight() + 2.0f) continue;
            g.setColour (green);
            g.fillRect (juce::Rectangle<float> (x - 0.75f, top, 1.5f, bottom - top));
            if (m.role.isNotEmpty())
            {
                g.setColour (roleColour (m.role));
                const float dash[2] = { 0.5f * space, 0.5f * space };
                g.drawDashedLine (juce::Line<float> (x, top, x, bottom), dash, 2, 1.5f);
            }
        }
        return;
    }

    // The names: bach's "smart" placing over all markers, so that a name keeps its line while the roll scrolls.
    std::vector<double> xs, widths;
    for (const auto& m : markers)
    {
        xs.push_back (xOf (m.ms));
        widths.push_back (m.name.isEmpty() ? 0.0 : (double) juce::GlyphArrangement::getStringWidth (nameFont, m.name) + 0.5 * space);
    }
    const auto lines = markerNameLines (xs, widths, space);
    const float lineHeight = nameFont.getHeight();

    for (size_t i = 0; i < markers.size(); ++i)
    {
        const auto& m = markers[i];
        const float x = (float) xs[i];
        if (x + (float) widths[i] < (float) area.getX() || x > (float) area.getRight()) continue;

        if (m.role.isNotEmpty())
        {
            // The role, as bach writes it: small capitals one under the other, left of the line (a tempo at its foot).
            const auto small = Theme::font (juce::jlimit (8.0f, 13.0f, space)).boldened();
            const auto word = roleWord (m.role);
            const float letter = small.getHeight() * 0.86f;
            const float wordTop = m.role == "tempo" ? bottom - 0.5f * space - letter * (float) word.length() : top + 0.5f * space;
            g.setColour (roleColour (m.role));
            g.setFont (small);
            for (int k = 0; k < word.length(); ++k)
                g.drawText (word.substring (k, k + 1), juce::Rectangle<float> (x - 1.7f * space, wordTop + letter * (float) k, 1.7f * space, letter),
                            juce::Justification::centred, false);
        }

        if (m.name.isNotEmpty())
        {
            const juce::Rectangle<float> box (x + 0.5f * space, top + 2.0f + (float) lines[i] * lineHeight, (float) widths[i], lineHeight);
            g.setColour (paper.withAlpha (0.8f));
            g.fillRect (box.withTrimmedRight (0.4f * space));
            g.setColour (green.withAlpha (1.0f).darker (0.25f));
            g.setFont (nameFont);
            g.drawText (m.name, box, juce::Justification::centredLeft, false);
        }
    }
}

void RollView::paintHeader (juce::Graphics& g, juce::Rectangle<int> area)
{
    juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (area);
    g.setColour (paper);
    g.fillRect (area);

    auto& font = MusicFont::get();
    const float space = spacePixels();
    const int staffStart = juce::roundToInt (nameColumn());

    paintStaves (g, staffStart, area.getRight());

    for (const auto& v : voices)
    {
        if (v.place.hidden) continue;

        const float topY = (float) yOf (v.place.offset + v.place.clef.topUnits());
        const float bottomY = (float) yOf (v.place.offset + v.place.clef.bottomUnits());
        if (bottomY < (float) (area.getY() - 40) || topY > (float) (area.getBottom() + 40)) continue;

        // What sounds the voice: a small tag under the name (a click changes it).
        if (v.place.voice < voiceTags.size())
        {
            const auto tag = voiceTags[v.place.voice];
            const auto box = tagBounds (v);
            const bool silent = tag == "silent";
            const auto tint = tag == "synth" ? juce::Colour (0xff5b4fa8) : silent ? juce::Colour (0xff8a8a86) : juce::Colour (0xff1f7a6d);
            g.setColour (tint.withAlpha (silent ? 0.0f : 0.14f));
            g.fillRoundedRectangle (box, 4.0f);
            g.setColour (tint.withAlpha (silent ? 0.6f : 0.85f));
            g.drawRoundedRectangle (box.reduced (0.5f), 4.0f, 1.0f);
            g.setFont (Theme::font (10.5f));
            g.drawText (compactTags ? (silent ? juce::String::fromUTF8 ("\xe2\x80\x93") : tag.substring (0, 1)) : tag, box, juce::Justification::centred, false);
        }

        // The name: in the middle of its column, in the colour of a locked, muted or solo voice, as in ScAst's roll.
        g.setFont (Theme::font (13.0f));
        g.setColour (flagColour (v.flags, ink));
        g.drawText (v.name, nameBounds (v), juce::Justification::centred, false);

        // The left end of the staves and their clefs: once for voices that share them.
        if (! v.place.ownsStaves) continue;
        g.setColour (flagColour (v.flags, staffLine));
        g.fillRect (juce::Rectangle<float> ((float) staffStart, topY, juce::jmax (1.0f, 0.11f * space), bottomY - topY));
        g.setColour (flagColour (v.flags, ink));
        for (const auto& staff : v.place.clef.staves)
            if (const auto sign = clefSign (staff))
                font.draw (g, sign, (float) staffStart + 0.7f * space,
                           (float) yOf (v.place.offset + v.place.clef.unitsOf (staff.clefLineStep)), space);
    }
}

void RollView::paintRuler (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (paper);
    g.fillRect (area);
    g.setColour (rulerLine);
    g.fillRect (area.getX(), area.getBottom() - 1, area.getWidth(), 1);

    const auto across = rollArea();
    juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (area.withLeft (across.getX() - 24).withRight (across.getRight()));      // a little room for the label of 0'00"

    double major = 1.0, minor = 0.5;
    tickSteps (pixelsPerMs() * 1000.0, major, minor);
    const bool tenths = major < 1.0;
    g.setFont (Theme::font (11.0f));

    const double first = std::floor (timeAt (across.getX() - 60.0) / 1000.0 / minor) * minor;
    for (double t = juce::jmax (0.0, first); xOf (t * 1000.0) < across.getRight() + 60.0; t += minor)
    {
        const float x = (float) xOf (t * 1000.0);
        const bool isMajor = std::abs (t / major - std::round (t / major)) < 1.0e-6;
        g.setColour (rulerLine);
        g.fillRect (juce::Rectangle<float> (x - 0.5f, (float) area.getBottom() - (isMajor ? 9.0f : 5.0f), 1.0f, isMajor ? 9.0f : 5.0f));
        if (isMajor && x >= (float) across.getX() - 1.0f)
        {
            g.setColour (rulerText);
            g.drawText (clockLabel (t, tenths), juce::Rectangle<float> (x - 40.0f, 3.0f, 80.0f, 14.0f), juce::Justification::centred, false);
        }
    }
}
