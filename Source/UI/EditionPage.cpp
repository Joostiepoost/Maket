// EditionPage.cpp

#include "EditionPage.h"
#include "Theme.h"
#include "../Formats/BachRoll.h"

namespace
{
    constexpr int rowHeight = 40;
    constexpr int barHeight = 2 * rowHeight;        // the file and the zoom; under it the transport and the two menus
    constexpr double zoomStep = 1.25;
    constexpr int transportTag = 4;                 // the mark on the transport's commands in the send thread

    juce::String timeText (double ms)
    {
        const double seconds = juce::jmax (0.0, ms) / 1000.0;
        const int minutes = (int) std::floor (seconds / 60.0);
        return juce::String (minutes) + "'" + juce::String (seconds - 60.0 * minutes, 1).paddedLeft ('0', 4) + "\"";
    }
}

EditionPage::EditionPage (maket::Settings& s, maket::OscScheduler& sched, maket::Synth& syn, AudioOutput& out)
    : settings (s), scheduler (sched), synth (syn), audio (out), transport (sched, transportTag)
{
    setOpaque (true);
    transport.setSynth (&synth);

    // A click on a voice's tag: conTimbre, synth, silent, and round again.
    view.onVoiceTagClicked = [this] (int voice)
    {
        auto engines = transport.getEngines();
        if (voice < 0 || voice >= (int) engines.size()) return;
        using maket::playing::VoiceEngine;
        const bool named = view.voiceName (voice).isNotEmpty();     // a voice without a name is never sent to the ePlayer
        engines[(size_t) voice] = engines[(size_t) voice] == VoiceEngine::conTimbre ? VoiceEngine::synth
                                : engines[(size_t) voice] == VoiceEngine::synth ? VoiceEngine::silent
                                : named ? VoiceEngine::conTimbre : VoiceEngine::synth;
        transport.setEngines (engines);
        showEngines();
        if (transport.isPlaying()) setNotice ("The change takes effect at the next play.");
        else if (! named)          setNotice ("A voice without a name cannot play on conTimbre: it goes between synth and silent.");
    };

    playButton.setButtonText ("Play");
    prescheduledButton.setButtonText ("Play ps");
    stopButton.setButtonText ("Stop");
    loopButton.setButtonText ("Loop");
    loopButton.setClickingTogglesState (false);
    playButton.setTooltip ("Play from the playhead, on the conTimbre ePlayer and on Maket's synth; the playhead follows the sound  (space)");
    prescheduledButton.setTooltip (juce::String::fromUTF8 ("Play prescheduled: the same, but nothing on screen follows the sound  (\xe2\x87\xa7space)"));
    stopButton.setTooltip (juce::String::fromUTF8 ("Stop; the playhead goes back to where play began  (space). \xe2\x8c\xa5space plays on from where it stopped"));
    loopButton.setTooltip (juce::String::fromUTF8 ("Go round the loop region  (L). Mark the region by dragging in the ruler with \xe2\x8c\xa5 held; \xe2\x8c\xa5-click removes it"));
    playButton.onClick = [this] { if (! transport.isPlaying()) startPlaying (cursor, true); };
    prescheduledButton.onClick = [this] { playPrescheduled(); };
    stopButton.onClick = [this] { stopPlaying(); };
    loopButton.onClick = [this] { toggleLoop(); };
    for (auto* b : { &playButton, &prescheduledButton, &stopButton, &loopButton })
    {
        b->setWantsKeyboardFocus (false);       // the space bar is the transport's, not the last button's
        addAndMakeVisible (*b);
    }

    view.onPlayheadSet = [this] (double ms, bool dragging)
    {
        cursor = ms;
        if (! transport.isPlaying())    view.setPlayhead (ms);
        else if (! dragging)            startPlaying (ms, following);       // a click while it plays: on from there
        repaint (transportArea);
    };
    view.onLoopSet = [this] (double start, double end)
    {
        transport.setLoop (start, end);
        transport.setLoopOn (transport.hasLoop());          // a region just marked is gone round; without one there is no loop
        view.setLoop (transport.loopStart(), transport.loopEnd(), transport.isLoopOn());
        if (transport.isPlaying()) setNotice ("The loop takes effect at the next play.");
        updateBar();
    };

    openButton.setButtonText (juce::String::fromUTF8 ("Open\xe2\x80\xa6"));
    openButton.setTooltip (juce::String::fromUTF8 ("Open a bach.roll text file  (\xe2\x8c\x98O)"));
    openButton.onClick = [this] { chooseFile(); };

    acrossOut.setButtonText (juce::String::fromUTF8 ("\xe2\x88\x92"));
    acrossIn.setButtonText ("+");
    downOut.setButtonText (juce::String::fromUTF8 ("\xe2\x88\x92"));
    downIn.setButtonText ("+");
    fitButton.setButtonText ("Fit");
    acrossOut.setTooltip (juce::String::fromUTF8 ("Less time detail  (\xe2\x8c\x98\xe2\x88\x92)"));
    acrossIn.setTooltip (juce::String::fromUTF8 ("More time detail  (\xe2\x8c\x98+)"));
    downOut.setTooltip (juce::String::fromUTF8 ("Smaller staves  (\xe2\x8c\xa5\xe2\x8c\x98\xe2\x88\x92)"));
    downIn.setTooltip (juce::String::fromUTF8 ("Larger staves  (\xe2\x8c\xa5\xe2\x8c\x98+)"));
    fitButton.setTooltip (juce::String::fromUTF8 ("The whole roll in the window  (\xe2\x8c\x98" "0)"));
    acrossOut.onClick = [this] { zoomAcross (-1); };
    acrossIn.onClick  = [this] { zoomAcross (1); };
    downOut.onClick   = [this] { zoomDown (-1); };
    downIn.onClick    = [this] { zoomDown (1); };
    fitButton.onClick = [this] { zoomToFit(); };

    for (auto* b : { &openButton, &acrossOut, &acrossIn, &downOut, &downIn, &fitButton })
        addAndMakeVisible (*b);

    gridBox.addItem ("Eighth tones", 8);
    gridBox.addItem ("Semitones", 2);
    gridBox.setSelectedId (8, juce::dontSendNotification);
    gridBox.setTooltip ("The grid notes are drawn on: each note gets the accidental of the nearest step");
    gridBox.onChange = [this] { view.setToneDivision (gridBox.getSelectedId()); };
    addAndMakeVisible (gridBox);

    velocityBox.addItem ("Velocity: grey", (int) RollView::VelocityDisplay::greyScale);
    velocityBox.addItem ("Velocity: colour", (int) RollView::VelocityDisplay::spectrum);
    velocityBox.addItem ("Velocity: alpha", (int) RollView::VelocityDisplay::alpha);
    velocityBox.setSelectedId ((int) RollView::VelocityDisplay::greyScale, juce::dontSendNotification);
    velocityBox.setTooltip ("How a note shows its velocity: grey (soft is light, as ScAst's roll), colour (blue is soft, red is loud) or alpha (soft is see-through)");
    velocityBox.onChange = [this] { view.setVelocityDisplay ((RollView::VelocityDisplay) velocityBox.getSelectedId()); };
    addAndMakeVisible (velocityBox);

    // Keys are the commands' (space, return, L): no button or menu of the bar keeps the keyboard after a click.
    for (auto* c : std::initializer_list<juce::Component*> { &openButton, &acrossOut, &acrossIn, &downOut, &downIn, &fitButton, &gridBox, &velocityBox })
        c->setWantsKeyboardFocus (false);

    view.onViewChanged = [this] { repaint (barArea); };
    addChildComponent (view);
    updateBar();
}

EditionPage::~EditionPage()
{
    stopTimer();
    transport.stop();
}

void EditionPage::chooseFile()
{
    const auto start = current.existsAsFile() ? current.getParentDirectory() : settings.getFolder ("importsFolder");
    chooser = std::make_unique<juce::FileChooser> ("Open a bach.roll text file", start, "*.txt;*.llll");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (file != juce::File()) openFile (file);
                          });
}

bool EditionPage::openFile (const juce::File& file)
{
    maket::Roll roll;
    juce::StringArray remarks;
    const auto result = maket::readBachRollFile (file, roll, remarks);
    if (result.failed())
    {
        problem = file.getFileName() + ": " + result.getErrorMessage();
        repaint();
        return false;
    }

    problem.clear();
    current = file;
    title = file.getFileNameWithoutExtension();
    detail = juce::String ((int) roll.voices.size()) + " voices, " + juce::String (roll.numNotes()) + " notes, "
             + juce::String (juce::roundToInt (std::floor (roll.length() / 60000.0))) + "'"
             + juce::String (juce::roundToInt (std::fmod (roll.length() / 1000.0, 60.0))).paddedLeft ('0', 2) + "\"";
    stopTimer();
    transport.setRoll (roll);       // every voice gets its default engine
    transport.setLoop (0.0, 0.0);
    transport.setLoopOn (false);
    cursor = 0.0;
    notice.clear();
    view.setRoll (roll);
    view.setLoop (0.0, 0.0, false);
    view.setPlayhead (0.0);
    view.setVisible (true);
    showEngines();
    updateBar();
    resized();
    repaint();
    if (onRollOpened) onRollOpened (file, roll);
    return true;
}

void EditionPage::filesDropped (const juce::StringArray& files, int, int)
{
    if (! files.isEmpty()) openFile (juce::File (files[0]));
}

void EditionPage::zoomAcross (int direction)
{
    view.setHorizontalZoom (view.horizontalZoom() * (direction > 0 ? zoomStep : 1.0 / zoomStep));
}

void EditionPage::zoomDown (int direction)
{
    view.setVerticalZoom (view.verticalZoom() * (direction > 0 ? zoomStep : 1.0 / zoomStep));
}

void EditionPage::updateBar()
{
    const bool has = view.hasRoll();
    for (auto* b : { &acrossOut, &acrossIn, &downOut, &downIn, &fitButton })
        b->setEnabled (has);
    gridBox.setEnabled (has);
    velocityBox.setEnabled (has);

    const bool playing = transport.isPlaying();
    playButton.setEnabled (has && ! playing);
    prescheduledButton.setEnabled (has && ! playing);
    stopButton.setEnabled (has && playing);
    loopButton.setEnabled (has);
    loopButton.setToggleState (transport.isLoopOn(), juce::dontSendNotification);
    repaint (transportArea);
}

// --- Transport ---------------------------------------------------------------------

void EditionPage::setNotice (const juce::String& text, bool isProblem)
{
    notice = text;
    noticeIsProblem = isProblem;
    noticeUntil = juce::Time::getMillisecondCounter() + 6000;
    repaint (transportArea);
    if (! isTimerRunning()) startTimer (500);       // to take the notice away again
}

void EditionPage::showEngines()
{
    juce::StringArray tags;
    for (auto e : transport.getEngines())
        tags.add (e == maket::playing::VoiceEngine::conTimbre ? "conTimbre" : e == maket::playing::VoiceEngine::synth ? "synth" : "silent");
    view.setVoiceTags (tags);
}

bool EditionPage::anyVoiceOnSynth() const
{
    for (auto e : transport.getEngines())
        if (e == maket::playing::VoiceEngine::synth) return true;
    return false;
}

void EditionPage::openAudioIfNeeded()
{
    // Stage 3a: audio goes on at a play that needs Maket's own sound, and not before; it is off until then, and
    // again after the switch in the tab row. With audio on already, this follows a change of the output in the preferences.
    if (! anyVoiceOnSynth()) return;
    const auto trouble = turnAudioOn ? turnAudioOn() : audio.open (settings.get ("audioOutput"));
    if (trouble.isNotEmpty()) setNotice ("Synth: " + trouble, true);
}

void EditionPage::audioChanged (bool isOn, const juce::String& trouble)
{
    if (trouble.isNotEmpty())                                   setNotice ("Synth: " + trouble, true);
    else if (! isOn && transport.isPlaying() && anyVoiceOnSynth()) setNotice ("Audio is off: the voices on the synth are silent until the next play.");
    else if (notice.startsWith ("Synth: ") || notice.startsWith ("Audio is off")) { notice.clear(); noticeIsProblem = false; }
    updateBar();
}

void EditionPage::startPlaying (double from, bool follow)
{
    if (! view.hasRoll()) return;

    scheduler.setPlayer (settings.get ("ePlayerHost"), settings.getInt ("ePlayerPort"));
    synth.setStandardPitch (settings.getDouble ("standardPitch"));
    synth.setLevel (settings.getDouble ("synthLevel"));
    transport.setSynthOffset (settings.getDouble ("synthOffset"));
    openAudioIfNeeded();
    const auto audioProblem = notice;

    juce::String why;
    if (! transport.play (from, settings.getDouble ("standardPitch"), why))
    {
        setNotice (why, true);
        updateBar();
        return;
    }

    following = follow;
    if (! (noticeIsProblem && audioProblem.isNotEmpty())) notice.clear();     // a problem with the audio output stays in sight
    if (! follow) view.setPlayhead (cursor);        // prescheduled: the playhead stays where play began
    startTimer (follow ? 33 : 200);                 // prescheduled: only to keep a loop supplied and to notice the end
    updateBar();
}

void EditionPage::playOrStop()
{
    if (transport.isPlaying())  stopPlaying();
    else                        startPlaying (cursor, true);
}

void EditionPage::playPrescheduled()
{
    if (transport.isPlaying()) stopPlaying();
    startPlaying (cursor, false);
}

void EditionPage::playOn()
{
    if (transport.isPlaying() || ! view.hasRoll()) return;
    const double from = transport.stoppedAt();
    startPlaying (from > 0.0 && from < transport.rollLength() ? from : cursor, true);
}

void EditionPage::stopPlaying()
{
    transport.stop();
    stopTimer();
    view.setPlayhead (cursor);
    updateBar();
}

void EditionPage::toggleLoop()
{
    if (! view.hasRoll()) return;
    if (! transport.hasLoop())
    {
        setNotice (juce::String::fromUTF8 ("No loop region yet: drag in the ruler with \xe2\x8c\xa5 held."), true);
        return;
    }
    transport.setLoopOn (! transport.isLoopOn());
    view.setLoop (transport.loopStart(), transport.loopEnd(), transport.isLoopOn());
    if (transport.isPlaying()) setNotice ("The loop takes effect at the next play.");
    updateBar();
}

void EditionPage::toStart()
{
    if (! view.hasRoll()) return;
    cursor = 0.0;
    if (transport.isPlaying())  startPlaying (0.0, following);
    else                        view.setPlayhead (0.0);
    view.turnPageTo (0.0);
    repaint (transportArea);
}

void EditionPage::timerCallback()
{
    if (notice.isNotEmpty() && juce::Time::getMillisecondCounter() > noticeUntil)
    {
        notice.clear();
        repaint (transportArea);
    }

    const bool wasPlaying = transport.isPlaying();
    transport.service();

    if (! transport.isPlaying())
    {
        if (wasPlaying)
        {
            // The roll has run to its end.
            view.setPlayhead (cursor);
            updateBar();
        }
        if (notice.isEmpty()) stopTimer();
        else                  startTimer (500);
        return;
    }

    if (following)
    {
        const double position = transport.position();
        view.turnPageTo (position);
        view.setPlayhead (position);
        repaint (transportTextArea);
    }
}

juce::String EditionPage::transportText() const
{
    if (notice.isNotEmpty()) return notice;
    if (! transport.isPlaying()) return "from " + timeText (cursor);
    const juce::String round = transport.isLoopOn() && transport.hasLoop() && transport.startedFrom() < transport.loopEnd() ? ", round the loop" : "";
    return following ? timeText (transport.position()) + round
                     : "playing prescheduled from " + timeText (transport.startedFrom()) + round;
}

void EditionPage::resized()
{
    auto area = getLocalBounds();
    barArea = area.removeFromTop (barHeight);
    view.setBounds (area);

    auto bar = barArea.withHeight (rowHeight).reduced (10, 7);
    openButton.setBounds (bar.removeFromLeft (76));
    bar.removeFromLeft (14);

    // The second row: the transport at the left, the two menus at the right.
    transportArea = barArea.withTrimmedTop (rowHeight);
    auto row = transportArea.reduced (10, 7).withTrimmedTop (-3).withTrimmedBottom (3);
    playButton.setBounds (row.removeFromLeft (56));
    row.removeFromLeft (4);
    prescheduledButton.setBounds (row.removeFromLeft (72));
    row.removeFromLeft (4);
    stopButton.setBounds (row.removeFromLeft (56));
    row.removeFromLeft (12);
    loopButton.setBounds (row.removeFromLeft (56));
    row.removeFromLeft (14);
    velocityBox.setBounds (row.removeFromRight (140));
    row.removeFromRight (8);
    gridBox.setBounds (row.removeFromRight (128));
    row.removeFromRight (12);
    transportTextArea = row;

    fitButton.setBounds (bar.removeFromRight (44));
    bar.removeFromRight (14);
    downIn.setBounds (bar.removeFromRight (28));
    bar.removeFromRight (4);
    downOut.setBounds (bar.removeFromRight (28));
    bar.removeFromRight (10);
    acrossIn.setBounds (bar.removeFromRight (28));
    bar.removeFromRight (4);
    acrossOut.setBounds (bar.removeFromRight (28));
    bar.removeFromRight (10);
    zoomTextArea = bar.removeFromRight (170);
    bar.removeFromRight (12);
    titleArea = bar;
}

void EditionPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    g.setColour (Theme::header);
    g.fillRect (barArea);
    g.setColour (Theme::line);
    g.fillRect (barArea.getX(), barArea.getBottom() - 1, barArea.getWidth(), 1);

    if (view.hasRoll())
    {
        g.setColour (Theme::text);
        g.setFont (Theme::font (14.0f, true));
        const int titleWidth = juce::jmin (titleArea.getWidth(), juce::roundToInt (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title)) + 14);
        g.drawText (title, titleArea.withWidth (titleWidth), juce::Justification::centredLeft, true);
        g.setColour (Theme::textMuted);
        g.setFont (Theme::font (12.5f));
        g.drawText (detail, titleArea.withTrimmedLeft (titleWidth), juce::Justification::centredLeft, true);
        g.drawText (view.describeView(), zoomTextArea, juce::Justification::centredRight, true);

        g.setColour (notice.isNotEmpty() && noticeIsProblem ? Theme::warning : transport.isPlaying() ? Theme::text : Theme::textMuted);
        g.setFont (Theme::font (13.0f));
        g.drawText (transportText(), transportTextArea, juce::Justification::centredLeft, true);
        return;
    }

    // Nothing open yet.
    const int width = juce::jmin (560, getWidth() - 80);
    if (width < 120) return;
    auto column = getLocalBounds().withTrimmedTop (barHeight).withSizeKeepingCentre (width, 160);

    g.setColour (Theme::text);
    g.setFont (Theme::font (22.0f, true));
    g.drawText ("Library and edition", column.removeFromTop (32), juce::Justification::centredLeft, true);
    g.setColour (problem.isNotEmpty() ? Theme::warning : Theme::textMuted);
    g.setFont (Theme::font (14.0f));
    g.drawFittedText (problem.isNotEmpty() ? problem
                                           : juce::String::fromUTF8 ("Open a bach.roll text file: the Open\xe2\x80\xa6 button above, File > Open (\xe2\x8c\x98O), "
                                                                     "or drop the file on this window."),
                      column.removeFromTop (60), juce::Justification::topLeft, 3);
    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (12.5f));
    g.drawFittedText ("The roll is shown as bach.roll shows it and plays on the conTimbre ePlayer. "
                      "The library arrives at step 3, editing at step 4.",
                      column, juce::Justification::topLeft, 4);
}
