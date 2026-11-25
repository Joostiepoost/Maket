// RollView.cpp

#include "RollView.h"
#include "MusicFont.h"
#include "Theme.h"
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
    voices.clear();
    rollLength = heightUnits = 0.0;
    scrollMs = scrollY = 0.0;
    updateScrollBars();
    repaint();
}

void RollView::setRoll (const maket::Roll& roll)
{
    voices.clear();
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
        if (! place.hidden && place.ownsStaves)
            nameWidth = juce::jmax (nameWidth, juce::GlyphArrangement::getStringWidth (nameFont, item.name));

        for (const auto& chord : voice.chords)
        {
            if (chord.notes.empty()) continue;

            ChordItem c;
            c.onset = chord.onset;
            int lowest = 1 << 20, highest = -(1 << 20);
            double sum = 0.0;

            for (const auto& note : chord.notes)
            {
                NoteItem n;
                n.step = spell (note.cents, 8).steps;
                n.units = place.offset + place.clef.unitsOf (n.step);
                n.end = chord.onset + note.duration;
                n.lightness = 0.8f * (1.0f - (float) juce::jlimit (0, 126, note.velocity - 1) / 126.0f);   // softest: almost white, as bach
                n.ledgers = ledgerSteps (place.clef, n.step);
                c.end = juce::jmax (c.end, n.end);
                lowest = juce::jmin (lowest, n.step);
                highest = juce::jmax (highest, n.step);
                sum += n.step;
                c.notes.push_back (std::move (n));
            }

            // The stem: bach's rule for its direction (Notation/Staves); an octave long beyond the outer note.
            c.direction = stemDirection (place, sum / (double) c.notes.size());

            // Heads a second (or less) apart cannot sit above each other: every other one goes to the far side of the stem,
            // counted from the end of the stem that holds the heads.
            {
                std::vector<size_t> order (c.notes.size());
                for (size_t i = 0; i < order.size(); ++i) order[i] = i;
                std::stable_sort (order.begin(), order.end(), [&c] (size_t a, size_t b)
                                  { return c.direction == 1 ? c.notes[a].step < c.notes[b].step : c.notes[a].step > c.notes[b].step; });
                for (size_t i = 1; i < order.size(); ++i)
                {
                    const auto& before = c.notes[order[i - 1]];
                    auto& note = c.notes[order[i]];
                    note.otherSide = std::abs (note.step - before.step) <= 1 && ! before.otherSide;
                }
            }
            const double lowUnits = place.offset + place.clef.unitsOf (lowest), highUnits = place.offset + place.clef.unitsOf (highest);
            if (c.direction == 1)   { c.stemFromUnits = lowUnits;  c.stemToUnits = highUnits - 7.0 * stepUnits; }
            else                    { c.stemFromUnits = highUnits; c.stemToUnits = lowUnits + 7.0 * stepUnits; }

            item.chords.push_back (std::move (c));
        }
        voices.push_back (std::move (item));
    }

    scrollMs = scrollY = 0.0;
    updateScrollBars();
    repaint();
}

// --- Geometry ---------------------------------------------------------------------

int RollView::headerWidth() const
{
    const float space = spacePixels();
    return juce::roundToInt (juce::jmax (36.0f, nameWidth + 20.0f) + 4.8f * space + 6.0f);
}

juce::Rectangle<int> RollView::rollArea() const
{
    return getLocalBounds().withTrimmedTop (rulerHeight).withTrimmedLeft (headerWidth())
                           .withTrimmedRight (barThickness).withTrimmedBottom (barThickness);
}

double RollView::xOf (double ms) const
{
    return rollArea().getX() + 2.0 * spacePixels() + (ms - scrollMs) * pixelsPerMs();
}

double RollView::timeAt (double x) const
{
    return scrollMs + (x - rollArea().getX() - 2.0 * spacePixels()) / pixelsPerMs();
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
    if (anchorX < area.getX()) anchorX = area.getX() + 2.0 * spacePixels();
    const double time = timeAt (anchorX);
    zoomX = juce::jlimit (minZoomX, maxZoomX, zoom);
    scrollMs = time - (anchorX - area.getX() - 2.0 * spacePixels()) / pixelsPerMs();
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
    const auto area = rollArea();
    const double visibleMs = area.getWidth() / pixelsPerMs();
    scrollMs = juce::jlimit (0.0, juce::jmax (0.0, rollLength + tailMs - visibleMs), scrollMs);
    scrollY = juce::jlimit (0.0, juce::jmax (0.0, heightUnits * pixelsPerUnit() + 20.0 - area.getHeight()), scrollY);
    updateScrollBars();
    repaint();
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
    repaint();
    if (onViewChanged) onViewChanged();
}

void RollView::resized()
{
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

            paintStaves (g, area.getX(), area.getRight());
            paintNotes (g, area);
        }
        paintHeader (g, { 0, rulerHeight, headerWidth(), area.getHeight() });
        paintRuler (g, { 0, 0, getWidth() - barThickness, rulerHeight });
    }

    // The corner between the two scroll bars.
    g.setColour (Theme::panel);
    g.fillRect (getWidth() - barThickness, getHeight() - barThickness, barThickness, barThickness);
}

void RollView::paintStaves (juce::Graphics& g, int fromX, int toX)
{
    const float thickness = juce::jmax (1.0f, 0.11f * spacePixels());
    const auto clip = g.getClipBounds();
    g.setColour (staffLine);

    for (const auto& v : voices)
    {
        if (v.place.hidden || ! v.place.ownsStaves) continue;
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
    const float lineThickness = juce::jmax (1.5f, 0.36f * space);
    const float thin = juce::jmax (1.0f, 0.12f * space);
    const float ledgerThickness = juce::jmax (1.0f, 0.14f * space);

    const double fromMs = timeAt (area.getX() - 3.0 * space), toMs = timeAt (area.getRight() + 3.0 * space);
    const float top = (float) area.getY() - 8.0f * space, bottom = (float) area.getBottom() + 8.0f * space;

    for (const auto& v : voices)
    {
        if (v.place.hidden) continue;

        for (const auto& c : v.chords)
        {
            if (c.end < fromMs || c.onset > toMs) continue;

            const float stemX = (float) xOf (c.onset);
            const float nearLeft = c.direction == 1 ? stemX - headWidth : stemX;      // the side of the stem the heads sit on
            const float farLeft = c.direction == 1 ? stemX : stemX - headWidth;
            float darkest = 1.0f;

            for (const auto& n : c.notes)
            {
                const float y = (float) yOf (n.units);
                if (y < top || y > bottom) continue;
                darkest = juce::jmin (darkest, n.lightness);
                const float headLeft = n.otherSide ? farLeft : nearLeft;
                const float headCentre = headLeft + headWidth * 0.5f;

                // Ledger lines.
                g.setColour (staffLine);
                for (int step : n.ledgers)
                {
                    const float ly = (float) yOf (v.place.offset + v.place.clef.unitsOf (step));
                    g.fillRect (juce::Rectangle<float> (headLeft - 0.38f * space, ly - ledgerThickness * 0.5f, headWidth + 0.76f * space, ledgerThickness));
                }

                // The duration line and its tail.
                const auto colour = juce::Colour::greyLevel (n.lightness);
                const float endX = (float) xOf (n.end);
                g.setColour (colour);
                if (endX > headCentre)
                {
                    g.fillRect (juce::Rectangle<float> (headCentre, y - lineThickness * 0.5f, endX - headCentre, lineThickness));
                    g.fillRect (juce::Rectangle<float> (endX - thin * 0.5f, y - 0.5f * space, thin, space));
                }

                // The head.
                if (font.isLoaded())
                    font.draw (g, MusicFont::noteheadBlack, headLeft, y, space);
                else
                    g.fillEllipse (headLeft, y - 0.5f * space, headWidth, space);
            }

            // The stem.
            const float y1 = (float) yOf (c.stemFromUnits), y2 = (float) yOf (c.stemToUnits);
            if (juce::jmax (y1, y2) >= top && juce::jmin (y1, y2) <= bottom)
            {
                g.setColour (juce::Colour::greyLevel (darkest));
                const float x = c.direction == 1 ? stemX - thin : stemX;
                g.fillRect (juce::Rectangle<float> (x, juce::jmin (y1, y2), thin, std::abs (y2 - y1)));
            }
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
    const int staffStart = juce::roundToInt (juce::jmax (36.0f, nameWidth + 20.0f));

    paintStaves (g, staffStart, area.getRight());

    g.setFont (Theme::font (13.0f));
    for (const auto& v : voices)
    {
        if (v.place.hidden || ! v.place.ownsStaves) continue;

        const float topY = (float) yOf (v.place.offset + v.place.clef.topUnits());
        const float bottomY = (float) yOf (v.place.offset + v.place.clef.bottomUnits());
        if (bottomY < (float) (area.getY() - 40) || topY > (float) (area.getBottom() + 40)) continue;

        // The left end of the staves, and the name beside them.
        g.setColour (staffLine);
        g.fillRect (juce::Rectangle<float> ((float) staffStart, topY, juce::jmax (1.0f, 0.11f * space), bottomY - topY));
        g.setColour (ink);
        g.drawText (v.name, juce::Rectangle<float> (8.0f, (topY + bottomY) * 0.5f - 10.0f, (float) staffStart - 12.0f, 20.0f),
                    juce::Justification::centredLeft, false);

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

    const auto roll = rollArea();
    juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (area.withLeft (roll.getX() - 24).withRight (roll.getRight()));      // a little room for the label of 0'00"

    double major = 1.0, minor = 0.5;
    tickSteps (pixelsPerMs() * 1000.0, major, minor);
    const bool tenths = major < 1.0;
    g.setFont (Theme::font (11.0f));

    const double first = std::floor (timeAt (roll.getX() - 60.0) / 1000.0 / minor) * minor;
    for (double t = juce::jmax (0.0, first); xOf (t * 1000.0) < roll.getRight() + 60.0; t += minor)
    {
        const float x = (float) xOf (t * 1000.0);
        const bool isMajor = std::abs (t / major - std::round (t / major)) < 1.0e-6;
        g.setColour (rulerLine);
        g.fillRect (juce::Rectangle<float> (x - 0.5f, (float) area.getBottom() - (isMajor ? 9.0f : 5.0f), 1.0f, isMajor ? 9.0f : 5.0f));
        if (isMajor && x >= (float) roll.getX() - 1.0f)
        {
            g.setColour (rulerText);
            g.drawText (clockLabel (t, tenths), juce::Rectangle<float> (x - 40.0f, 3.0f, 80.0f, 14.0f), juce::Justification::centred, false);
        }
    }
}
