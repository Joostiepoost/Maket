// SoundPage.cpp — see SoundPage.h.

#include "SoundPage.h"
#include "Theme.h"

namespace
{
    const juce::Colour paper (0xfffaf9f5), ink (0xff26262b), gridLine (0xffe1dfd8), playColour (0xff57de33), loopColour (0xffad7ab0);

    juce::String timeText (double ms, bool tenths)
    {
        const double seconds = ms / 1000.0;
        const int minutes = (int) (seconds / 60.0);
        const double rest = seconds - minutes * 60.0;
        return juce::String (minutes) + "'" + (tenths ? juce::String (rest, 1).paddedLeft ('0', 4) : juce::String ((int) rest).paddedLeft ('0', 2)) + "\"";
    }
}

SoundPage::SoundPage (maket::SoundPlayer& p) : player (p)
{
    playButton.setTooltip ("Play from the playhead through Maket's audio output  (space)");
    stopButton.setTooltip ("Stop  (space)");
    loopButton.setTooltip (juce::String::fromUTF8 ("Go round the loop  (L). Set the loop by dragging in the ruler with \xe2\x8c\xa5 held; \xe2\x8c\xa5-click takes it away."));
    loopButton.setClickingTogglesState (false);
    playButton.onClick = [this] { if (! isPlaying()) playOrStop(); };
    stopButton.onClick = [this] { stopPlaying(); };
    loopButton.onClick = [this] { toggleLoop(); };
    zoomOutButton.onClick = [this] { zoomAcross (-1); };
    zoomInButton.onClick  = [this] { zoomAcross (1); };
    fitButton.onClick     = [this] { zoomToFit(); };
    for (auto* b : { &playButton, &stopButton, &loopButton, &zoomOutButton, &zoomInButton, &fitButton })
    {
        b->setWantsKeyboardFocus (false);
        b->setMouseClickGrabsKeyboardFocus (false);
        addAndMakeVisible (*b);
    }
    scrollBar.addListener (this);
    scrollBar.setAutoHide (false);
    addAndMakeVisible (scrollBar);
    updateBar();
}

SoundPage::~SoundPage()
{
    if (isPlaying()) player.stop();
}

void SoundPage::showSound (std::shared_ptr<const maket::Sound> s, const juce::String& name, const juce::String& factsLine)
{
    stopPlaying();
    sound = std::move (s);
    title = name;
    facts = factsLine;
    playhead = 0.0;
    loopStart = loopEnd = 0.0;
    loopOn = false;
    zoomToFit();
    updateBar();
    repaint();
}

//==============================================================================

void SoundPage::playOrStop()
{
    if (isPlaying()) { stopPlaying(); return; }
    if (sound == nullptr) return;
    if (onWillPlay) onWillPlay();                       // whatever else plays is stopped first

    const auto trouble = turnAudioOn ? turnAudioOn() : juce::String();
    if (trouble.isNotEmpty()) { notice = trouble; noticeIsProblem = true; updateBar(); return; }
    notice.clear();
    noticeIsProblem = false;

    if (playhead >= sound->lengthMs() - 1.0) playhead = 0.0;
    playedFrom = playhead;
    const bool loop = loopOn && loopEnd > loopStart;
    player.play (sound, playhead, loop, loopStart, loopEnd);
    mine = true;
    startTimerHz (30);
    updateBar();
}

void SoundPage::stopPlaying()
{
    if (mine && player.isPlaying()) player.stop();
    if (mine) playhead = playedFrom;                    // back to where the play began, as in the roll
    mine = false;
    stopTimer();
    updateBar();
    repaint();
}

void SoundPage::toggleLoop()
{
    loopOn = ! loopOn;
    if (loopOn && loopEnd <= loopStart) { notice = juce::String::fromUTF8 ("No loop is set: drag in the ruler with \xe2\x8c\xa5 held."); noticeIsProblem = false; }
    else if (isPlaying())               { notice = "The loop takes effect at the next play."; noticeIsProblem = false; }
    updateBar();
    repaint();
}

void SoundPage::toStart()
{
    if (isPlaying()) return;
    playhead = 0.0;
    scrollMs = 0.0;
    viewChanged();
}

void SoundPage::audioChanged (bool isOn, const juce::String&)
{
    if (! isOn && mine) { mine = false; stopTimer(); playhead = playedFrom; updateBar(); repaint(); }
}

void SoundPage::timerCallback()
{
    if (! mine) { stopTimer(); return; }
    if (! player.isPlaying())
    {
        // The sound came to its end.
        mine = false;
        stopTimer();
        playhead = playedFrom;
        updateBar();
        repaint();
        return;
    }
    playhead = player.positionMs();
    // Turn the page when the playhead runs out of view, as the roll does.
    const auto area = waveArea();
    const double visible = area.getWidth() / pixelsPerMs;
    if (playhead < scrollMs || playhead > scrollMs + visible) { scrollMs = juce::jmax (0.0, playhead - visible * 0.05); viewChanged(); }
    repaint();
}

void SoundPage::updateBar()
{
    const bool playing = isPlaying();
    playButton.setEnabled (sound != nullptr && ! playing);
    stopButton.setEnabled (playing);
    loopButton.setToggleState (loopOn, juce::dontSendNotification);
    repaint();
    if (playing != wasPlaying)
    {
        wasPlaying = playing;
        if (onPlayStateChanged) onPlayStateChanged();
    }
}

//==============================================================================

juce::Rectangle<int> SoundPage::rulerArea() const
{
    return getLocalBounds().withTrimmedTop (2 * rowHeight).removeFromTop (rulerHeight);
}

juce::Rectangle<int> SoundPage::waveArea() const
{
    return getLocalBounds().withTrimmedTop (2 * rowHeight + rulerHeight).withTrimmedBottom (barThickness);
}

double SoundPage::msAt (int x) const      { return scrollMs + (x - waveArea().getX()) / pixelsPerMs; }
int SoundPage::xOf (double ms) const      { return waveArea().getX() + juce::roundToInt ((ms - scrollMs) * pixelsPerMs); }

void SoundPage::viewChanged()
{
    const double length = sound != nullptr ? sound->lengthMs() : 0.0;
    const double visible = waveArea().getWidth() / pixelsPerMs;
    scrollMs = juce::jlimit (0.0, juce::jmax (0.0, length - visible * 0.9), scrollMs);
    scrollBar.setRangeLimits (0.0, juce::jmax (length, scrollMs + visible), juce::dontSendNotification);
    scrollBar.setCurrentRange (scrollMs, visible, juce::dontSendNotification);
    repaint();
}

void SoundPage::scrollBarMoved (juce::ScrollBar*, double newRangeStart)
{
    scrollMs = newRangeStart;
    repaint();
}

void SoundPage::zoomAround (double factor, double anchorMs)
{
    if (sound == nullptr) return;
    const double before = (anchorMs - scrollMs) * pixelsPerMs;
    // From the whole sound in a third of the view down to a sample every 8 pixels.
    const double least = waveArea().getWidth() / juce::jmax (1.0, sound->lengthMs()) / 3.0, most = 8.0 * sound->sampleRate / 1000.0;
    pixelsPerMs = juce::jlimit (least, most, pixelsPerMs * factor);
    scrollMs = anchorMs - before / pixelsPerMs;
    viewChanged();
}

void SoundPage::zoomAcross (int direction)
{
    const double middle = scrollMs + waveArea().getWidth() / pixelsPerMs * 0.5;
    zoomAround (direction > 0 ? 1.5 : 1.0 / 1.5, isPlaying() ? playhead : middle);
}

void SoundPage::zoomToFit()
{
    if (sound == nullptr || waveArea().getWidth() <= 0) return;
    pixelsPerMs = (waveArea().getWidth() - 24) / juce::jmax (1.0, sound->lengthMs());
    scrollMs = 0.0;
    viewChanged();
}

void SoundPage::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown()) { zoomAround (std::pow (1.5, wheel.deltaY * 4.0), msAt (e.x)); return; }
    const double delta = std::abs (wheel.deltaX) > std::abs (wheel.deltaY) ? wheel.deltaX : wheel.deltaY;
    scrollMs -= delta * 400.0 / pixelsPerMs;
    viewChanged();
}

void SoundPage::mouseMagnify (const juce::MouseEvent& e, float scale)     { zoomAround (scale, msAt (e.x)); }

void SoundPage::mouseDown (const juce::MouseEvent& e)
{
    if (sound == nullptr || e.y < 2 * rowHeight || e.y >= getHeight() - barThickness) return;
    const double ms = juce::jlimit (0.0, sound->lengthMs(), msAt (e.x));
    draggingLoop = e.mods.isAltDown() && rulerArea().contains (e.getPosition());
    if (draggingLoop)
    {
        dragAnchor = ms;
        loopStart = loopEnd = 0.0;          // a click with the key held, without a drag, takes the loop away
        repaint();
        return;
    }
    if (isPlaying()) return;                // the playhead belongs to the sound while it plays
    playhead = ms;
    repaint();
}

void SoundPage::mouseDrag (const juce::MouseEvent& e)
{
    if (sound == nullptr) return;
    const double ms = juce::jlimit (0.0, sound->lengthMs(), msAt (e.x));
    if (draggingLoop)
    {
        loopStart = juce::jmin (dragAnchor, ms);
        loopEnd = juce::jmax (dragAnchor, ms);
        if (loopEnd - loopStart >= 20.0) loopOn = true;
        updateBar();
    }
    else if (! isPlaying() && e.y >= 2 * rowHeight) playhead = ms;
    repaint();
}

//==============================================================================

void SoundPage::resized()
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

    scrollBar.setBounds (getLocalBounds().removeFromBottom (barThickness));
    viewChanged();
}

void SoundPage::paintChannel (juce::Graphics& g, juce::Rectangle<int> area, int channel) const
{
    const float middle = (float) area.getCentreY(), half = (float) area.getHeight() * 0.46f;
    g.setColour (gridLine);
    g.fillRect ((float) area.getX(), middle, (float) area.getWidth(), 1.0f);

    const double samplesPerPixel = sound->sampleRate / 1000.0 / pixelsPerMs;
    const float* data = sound->samples.getReadPointer (channel);
    const int length = sound->numSamples();
    g.setColour (ink);

    if (samplesPerPixel <= 2.0)
    {
        // Close up: the samples themselves, joined by a line.
        juce::Path path;
        const int first = juce::jmax (0, (int) (scrollMs * sound->sampleRate / 1000.0) - 1);
        const int last = juce::jmin (length - 1, first + (int) (area.getWidth() * samplesPerPixel) + 3);
        for (int i = first; i <= last; ++i)
        {
            const float x = (float) area.getX() + (float) ((i * 1000.0 / sound->sampleRate - scrollMs) * pixelsPerMs);
            const float y = middle - juce::jlimit (-1.0f, 1.0f, data[i]) * half;
            if (i == first) path.startNewSubPath (x, y); else path.lineTo (x, y);
            if (samplesPerPixel < 0.25) g.fillEllipse (x - 2.0f, y - 2.0f, 4.0f, 4.0f);
        }
        g.strokePath (path, juce::PathStrokeType (1.2f));
        return;
    }

    // Further off: for every pixel across, from the lowest to the highest value it covers.
    const auto& ranges = sound->overview[(size_t) channel];
    const bool fromOverview = samplesPerPixel >= maket::Sound::overviewStep;
    for (int px = 0; px < area.getWidth(); ++px)
    {
        const double from = (scrollMs + px / pixelsPerMs) * sound->sampleRate / 1000.0;
        const int a = (int) from, b = juce::jmin (length, (int) (from + samplesPerPixel) + 1);
        if (a >= length || b <= 0) continue;
        float low = 1.0f, high = -1.0f;
        if (fromOverview)
        {
            const int ra = juce::jmax (0, a / maket::Sound::overviewStep), rb = juce::jmin ((int) ranges.size(), b / maket::Sound::overviewStep + 1);
            for (int i = ra; i < rb; ++i) { low = juce::jmin (low, ranges[(size_t) i].low); high = juce::jmax (high, ranges[(size_t) i].high); }
        }
        else
        {
            const auto r = juce::FloatVectorOperations::findMinAndMax (data + juce::jmax (0, a), b - juce::jmax (0, a));
            low = r.getStart(); high = r.getEnd();
        }
        if (low > high) continue;
        const float top = middle - juce::jlimit (-1.0f, 1.0f, high) * half, bottom = middle - juce::jlimit (-1.0f, 1.0f, low) * half;
        g.fillRect ((float) (area.getX() + px), top, 1.0f, juce::jmax (1.0f, bottom - top));
    }
}

void SoundPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    // The two rows of the bar: what it is, and the transport.
    auto top = getLocalBounds().removeFromTop (rowHeight).reduced (14, 0).withTrimmedRight (150);
    g.setColour (Theme::text);
    g.setFont (Theme::font (15.0f, true));
    const int titleWidth = juce::roundToInt (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title)) + 16;
    g.drawText (title, top.removeFromLeft (juce::jmin (titleWidth, top.getWidth() / 2)), juce::Justification::centredLeft, true);
    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (12.5f));
    g.drawText (facts, top, juce::Justification::centredLeft, true);

    auto second = getLocalBounds().withTrimmedTop (rowHeight).removeFromTop (rowHeight).reduced (14, 0).withTrimmedLeft (200);
    g.setColour (noticeIsProblem ? Theme::warning : Theme::textMuted);
    g.setFont (Theme::font (12.5f));
    juce::String text = notice;
    if (text.isEmpty() && sound != nullptr)
        text = (isPlaying() ? juce::String() : juce::String ("from ")) + timeText (playhead, true)
               + (loopEnd > loopStart ? juce::String::fromUTF8 ("   \xc2\xb7   loop ") + timeText (loopStart, true) + juce::String::fromUTF8 (" \xe2\x80\x93 ") + timeText (loopEnd, true) : juce::String());
    g.drawText (text, second, juce::Justification::centredLeft, true);

    // The paper: ruler and waveforms.
    const auto ruler = rulerArea(), wave = waveArea();
    g.setColour (paper);
    g.fillRect (ruler.getUnion (wave));
    if (sound == nullptr) return;

    g.saveState();
    g.reduceClipRegion (ruler.getUnion (wave));

    // Ruler: a mark at a step that leaves the labels room.
    const double steps[] = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 30000, 60000 };
    double step = 60000.0;
    for (double s : steps) if (s * pixelsPerMs >= 70.0) { step = s; break; }
    g.setFont (Theme::font (11.0f));
    for (double t = std::floor (scrollMs / step) * step; t < scrollMs + wave.getWidth() / pixelsPerMs; t += step)
    {
        if (t < 0.0) continue;
        const int x = xOf (t);
        g.setColour (gridLine);
        g.fillRect (x, ruler.getY() + 18, 1, wave.getBottom() - ruler.getY() - 18);
        g.setColour (ink.withAlpha (0.8f));
        g.drawText (step < 1000.0 ? juce::String (t / 1000.0, step < 10.0 ? 3 : step < 100.0 ? 2 : 1) + "\"" : timeText (t, false),
                    x + 4, ruler.getY() + 2, 80, 16, juce::Justification::centredLeft, false);
    }
    g.setColour (ink.withAlpha (0.35f));
    g.fillRect (ruler.getX(), ruler.getBottom() - 1, ruler.getWidth(), 1);

    // Where the sound ends.
    const int endX = xOf (sound->lengthMs());
    if (endX < wave.getRight()) { g.setColour (juce::Colour (0xffeceae3)); g.fillRect (endX, wave.getY(), wave.getRight() - endX, wave.getHeight()); }

    // The loop behind the waveform.
    if (loopEnd > loopStart)
    {
        const int a = xOf (loopStart), b = xOf (loopEnd);
        g.setColour (loopColour.withAlpha (loopOn ? 0.22f : 0.10f));
        g.fillRect (a, ruler.getY(), b - a, wave.getBottom() - ruler.getY());
        g.setColour (loopColour.withAlpha (loopOn ? 0.9f : 0.45f));
        g.fillRect (a, ruler.getY(), 1, wave.getBottom() - ruler.getY());
        g.fillRect (b, ruler.getY(), 1, wave.getBottom() - ruler.getY());
    }

    const int channels = sound->numChannels();
    const int each = wave.getHeight() / juce::jmax (1, channels);
    for (int c = 0; c < channels; ++c)
        paintChannel (g, { wave.getX(), wave.getY() + c * each, wave.getWidth(), each }, c);
    if (channels == 2)
    {
        g.setColour (ink.withAlpha (0.25f));
        g.fillRect (wave.getX(), wave.getY() + each, wave.getWidth(), 1);
    }

    // The playhead.
    const int x = xOf (playhead);
    g.setColour (playColour);
    g.fillRect (x, ruler.getY(), 2, wave.getBottom() - ruler.getY());
    juce::Path head;
    head.addTriangle ((float) x - 5.0f, (float) ruler.getY(), (float) x + 7.0f, (float) ruler.getY(), (float) x + 1.0f, (float) ruler.getY() + 9.0f);
    g.fillPath (head);
    g.restoreState();
}
