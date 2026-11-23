// PlayerStudyPage.cpp

#include "PlayerStudyPage.h"
#include "Theme.h"
#include "../Engine/ConTimbre.h"

namespace
{
    constexpr int margin = 28;
    constexpr int sendTag = 4;              // the mark on this page's messages in the send thread's records
    constexpr double leadTime = 300.0;      // from the click to the first command, ms

    juce::String asClock (double ms)
    {
        const int seconds = juce::jmax (0, juce::roundToInt (ms / 1000.0));
        return juce::String (seconds / 60) + ":" + juce::String (seconds % 60).paddedLeft ('0', 2);
    }
}

PlayerStudyPage::PlayerStudyPage (maket::Settings& s, maket::OscScheduler& sch) : settings (s), scheduler (sch)
{
    setOpaque (true);

    for (auto* field : { &voiceField, &programField, &repetitionsField })
    {
        field->setFont (Theme::font (14.0f));
        field->setSelectAllWhenFocused (true);
        field->onTextChange = [this] { if (! running) showWays(); };
        addAndMakeVisible (*field);
    }
    voiceField.setText ("1", false);
    programField.setText ("sul tasto", false);
    repetitionsField.setText ("20", false);

    numberToggle.setButtonText ("with the ways that address a note by number");
    correctionToggle.setButtonText ("with the ways that switch the dynamic correction off");
    for (auto* toggle : { &numberToggle, &correctionToggle })
    {
        toggle->setToggleState (true, juce::dontSendNotification);
        toggle->onClick = [this] { showWays(); };
        addAndMakeVisible (*toggle);
    }

    tryPitchButton.setButtonText ("Try one figure by pitch");
    tryNumberButton.setButtonText ("Try one figure by number");
    runButton.setButtonText ("Run the study");
    secondRunButton.setButtonText ("Run the second study");
    secondRunButton.onClick = [this] { start (-1, 2); };
    stopButton.setButtonText ("Stop");

    tryPitchButton.onClick  = [this] { start (0); };
    tryNumberButton.onClick = [this] { start (6); };
    runButton.onClick       = [this] { start (-1); };
    stopButton.onClick      = [this] { stop(); };

    for (auto* button : { &tryPitchButton, &tryNumberButton, &runButton, &secondRunButton, &stopButton })
        addAndMakeVisible (*button);

    view.setMultiLine (true);
    view.setReadOnly (true);
    view.setScrollbarsShown (true);
    view.setCaretVisible (false);
    view.setFont (Theme::mono (12.5f));
    addAndMakeVisible (view);

    setRunning (false);
    showWays();
    setStatus ("Load an orchestra with the voice in the ePlayer, try one figure, then run the study while recording.");
    startTimerHz (10);
}

PlayerStudyPage::~PlayerStudyPage()
{
    stopTimer();
}

// --- Layout ---------------------------------------------------------------------------

void PlayerStudyPage::resized()
{
    auto area = getLocalBounds().reduced (margin, 0);
    area.removeFromTop (24);

    titleArea    = area.removeFromTop (30);
    subtitleArea = area.removeFromTop (40);
    area.removeFromTop (10);
    targetArea   = area.removeFromTop (22);
    area.removeFromTop (14);

    auto labels = area.removeFromTop (18);
    auto fields = area.removeFromTop (28);
    voiceLabel = labels.removeFromLeft (150);           voiceField.setBounds (fields.removeFromLeft (150).withTrimmedRight (90));
    programLabel = labels.removeFromLeft (260);         programField.setBounds (fields.removeFromLeft (260).withTrimmedRight (20));
    repetitionsLabel = labels.removeFromLeft (220);     repetitionsField.setBounds (fields.removeFromLeft (220).withTrimmedRight (150));
    area.removeFromTop (10);

    numberToggle.setBounds (area.removeFromTop (24).withWidth (juce::jmin (area.getWidth(), 520)));
    correctionToggle.setBounds (area.removeFromTop (24).withWidth (juce::jmin (area.getWidth(), 520)));
    area.removeFromTop (12);

    auto row = area.removeFromTop (30);
    tryPitchButton.setBounds (row.removeFromLeft (180));
    row.removeFromLeft (8);
    tryNumberButton.setBounds (row.removeFromLeft (190));
    row.removeFromLeft (24);
    runButton.setBounds (row.removeFromLeft (120));
    row.removeFromLeft (8);
    secondRunButton.setBounds (row.removeFromLeft (170));
    row.removeFromLeft (8);
    stopButton.setBounds (row.removeFromLeft (70));

    area.removeFromTop (8);
    statusArea = area.removeFromTop (38);
    area.removeFromTop (6);
    viewHeadingArea = area.removeFromTop (22);
    area.removeFromTop (4);
    view.setBounds (area.withTrimmedBottom (20));
}

void PlayerStudyPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    g.setColour (Theme::text);
    g.setFont (Theme::font (22.0f, true));
    g.drawText ("ePlayer study", titleArea, juce::Justification::centredLeft, true);

    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (13.0f));
    g.drawFittedText ("Plays one figure of two notes with crescendi many times, each way of sending it in turn, to find out whether anything "
                      "on Maket's side makes the ePlayer follow its dynamic ramps more surely. One recording of the whole run is enough.",
                      subtitleArea, juce::Justification::topLeft, 2, 1.0f);

    g.drawText ("ePlayer: " + settings.get ("ePlayerHost") + ", port " + settings.get ("ePlayerPort")
                + juce::String::fromUTF8 ("  \xc2\xb7  files in ") + settings.get ("timingFolder"),
                targetArea, juce::Justification::centredLeft, true);

    g.setFont (Theme::font (12.0f));
    g.drawText ("Voice: \"voice\" + number", voiceLabel, juce::Justification::centredLeft, true);
    g.drawText ("Program of that voice", programLabel, juce::Justification::centredLeft, true);
    g.drawText ("Figures of each way", repetitionsLabel, juce::Justification::centredLeft, true);

    g.setColour (statusIsProblem ? Theme::warning : Theme::text);
    g.setFont (Theme::font (13.5f));
    g.drawFittedText (status, statusArea, juce::Justification::topLeft, 2, 1.0f);

    g.setColour (Theme::text);
    g.setFont (Theme::font (15.0f, true));
    g.drawText ("The run", viewHeadingArea, juce::Justification::centredLeft, true);
}

// --- State ------------------------------------------------------------------------------

void PlayerStudyPage::setRunning (bool isRunning)
{
    running = isRunning;
    for (auto* c : std::initializer_list<juce::Component*> { &voiceField, &programField, &repetitionsField, &numberToggle, &correctionToggle,
                                                              &tryPitchButton, &runButton, &secondRunButton })
        c->setEnabled (! running);
    tryNumberButton.setEnabled (! running && numberToggle.getToggleState());
    stopButton.setEnabled (running);
}

void PlayerStudyPage::setStatus (const juce::String& text, bool isProblem)
{
    if (text == status && isProblem == statusIsProblem) return;
    status = text;
    statusIsProblem = isProblem;
    repaint (statusArea);
}

bool PlayerStudyPage::readOptions (maket::study::Options& o)
{
    const auto voice = voiceField.getText().trim();
    const auto repetitions = repetitionsField.getText().trim();
    const auto program = programField.getText().trim();

    if (voice.isEmpty() || ! voice.containsOnly ("0123456789") || voice.getIntValue() < 1 || voice.getIntValue() > 128)
        { setStatus ("Voice: a whole number from 1 to 128; the figure is then played on the voice named voice + that number.", true); return false; }
    if (program.isEmpty())
        { setStatus ("Program: the name of a program of that voice, as the ePlayer shows it.", true); return false; }
    if (repetitions.isEmpty() || ! repetitions.containsOnly ("0123456789") || repetitions.getIntValue() < 1 || repetitions.getIntValue() > 100)
        { setStatus ("Figures of each way: a whole number from 1 to 100.", true); return false; }

    o.voiceNumber = voice.getIntValue();
    o.program = program;
    o.repetitions = repetitions.getIntValue();
    o.withNumber = numberToggle.getToggleState();
    o.withCorrectionOff = correctionToggle.getToggleState();
    return true;
}

void PlayerStudyPage::showWays()
{
    tryNumberButton.setEnabled (! running && numberToggle.getToggleState());

    maket::study::Options o;
    if (! readOptions (o)) return;
    setStatus ("Load an orchestra with the voice in the ePlayer, try one figure, then run the study while recording.");

    const auto p = maket::study::makePlan (o);
    juce::String text;
    text << "The figure, on " << o.voiceName() << " with the program " << o.program << ":\n"
         << "  note 1: 67.4, velocity 15, 1.7 s long, crescendo to 60 over 1.5 s\n"
         << "  note 2: 67.68, velocity 36, 1.9 s long, begins 1.5 s after note 1; crescendo to 70 over 0.7 s, to 100 over 0.6 s, to 127 over 0.4 s\n\n"
         << "The ways, played in turns, " << o.repetitions << " figures of each, one every 4 seconds:\n";
    for (size_t i = 0; i < p.ways.size(); ++i)
        text << "  " << juce::String ((int) i + 1).paddedLeft (' ', 2) << "  " << p.ways[i].name << "\n";
    text << "\nThe whole run: " << (int) p.repetitions.size() << " figures, " << asClock (p.length) << " minutes, "
         << (int) p.messages.size() << " commands.\n";

    const auto second = maket::study::makePlan (o, -1, 2);
    text << "\nThe second study (stage 1f.3), played in turns:\n"
         << "  single notes of 1.5 s without any dynamic, at the velocities 15, 36, 60, 100, 127, by pitch and by number\n";
    for (size_t i = 10; i < second.ways.size(); ++i)
        text << "  " << second.ways[i].name << "\n";
    text << "The whole second run: " << (int) second.repetitions.size() << " notes and figures, " << asClock (second.length) << " minutes, "
         << (int) second.messages.size() << " commands.\n";
    view.setText (text, false);
}

// --- Running ----------------------------------------------------------------------------

void PlayerStudyPage::start (int onlyWay, int run)
{
    maket::study::Options o;
    if (running || ! readOptions (o)) return;

    plan = maket::study::makePlan (o, onlyWay, run);
    wholeRun = onlyWay < 0;

    const auto host = settings.get ("ePlayerHost");
    scheduler.setPlayer (host, settings.getInt ("ePlayerPort"));
    scheduler.takeSent (sendTag);

    stamp = juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");
    if (wholeRun)
    {
        const auto dir = settings.getFolder ("timingFolder");
        if (dir == juce::File() || ! dir.createDirectory())
        {
            setStatus ("The folder for the test files could not be made: " + settings.get ("timingFolder")
                       + ". Set another one on the Preferences page (Test results).", true);
            return;
        }
        const auto text = maket::study::planAsText (plan);
        dir.getChildFile (stamp + (plan.run == 2 ? " player study 2 plan.txt" : " player study plan.txt")).replaceWithData (text.toRawUTF8(), text.getNumBytesAsUTF8());
    }

    runStart = maket::OscScheduler::now() + leadTime;
    for (const auto& m : plan.messages)
        scheduler.sendAt (runStart + m.time, m.message, maket::OscScheduler::toPlayer, sendTag);

    numToSend = (int) plan.messages.size();
    numRefused = 0;
    latest = 0.0;
    sent.clear();
    setRunning (true);
}

void PlayerStudyPage::stop()
{
    if (! running) return;

    scheduler.clear();
    for (const auto& s : scheduler.takeSent (sendTag)) sent.push_back (s);
    scheduler.sendImmediately (maket::contimbre::allNotesOff(), maket::OscScheduler::toPlayer, sendTag);
    scheduler.sendImmediately (maket::contimbre::dynamicCorrection (true), maket::OscScheduler::toPlayer, sendTag);
    finish (true);
}

void PlayerStudyPage::finish (bool stopped)
{
    setRunning (false);

    bool filed = false;
    juce::File dir;
    if (wholeRun && ! sent.empty())
    {
        dir = settings.getFolder ("timingFolder");
        juce::String text;
        text << "Maket, study of the ePlayer, run " << plan.run << ": when every command left.\n"
             << "Columns, separated by a tab: time in the run (ms), left Maket this much after it (ms), command\n";
        for (const auto& s : sent)
            text << juce::String (s.due - runStart, 3) << "\t" << juce::String (s.sent - s.due, 3) << "\t" << s.text << "\n";
        filed = dir.getChildFile (stamp + (plan.run == 2 ? " player study 2 send log.txt" : " player study send log.txt")).replaceWithData (text.toRawUTF8(), text.getNumBytesAsUTF8());
    }

    if (numRefused > 0)
        setStatus (juce::String (numRefused) + " commands could not be sent: is the ePlayer's address right on the Preferences page?", true);
    else if (stopped)
        setStatus ("Stopped; all notes off sent, and the dynamic correction switched back on."
                   + (filed ? " What was played is filed in " + dir.getFullPathName() + "." : juce::String()));
    else if (wholeRun)
        setStatus ("The run is over: " + juce::String ((int) sent.size()) + " commands sent, the latest " + juce::String (latest, 2)
                   + " ms after its time. Plan and send log are in " + dir.getFullPathName() + ".");
    else
        setStatus ("One figure played. Did you hear two notes, each growing louder?");

    sent.clear();
}

void PlayerStudyPage::timerCallback()
{
    if (! running) return;

    for (const auto& s : scheduler.takeSent (sendTag))
    {
        if (! s.ok) ++numRefused;
        latest = juce::jmax (latest, s.sent - s.due);
        sent.push_back (s);
    }

    const double position = juce::jmax (0.0, maket::OscScheduler::now() - runStart);
    if (scheduler.numWaiting() == 0 && (int) sent.size() >= numToSend)
    {
        finish (false);
    }
    else if (wholeRun)
    {
        int figure = 0;
        for (const auto& r : plan.repetitions) if (r.start <= position) ++figure;
        const auto& way = plan.ways[(size_t) plan.repetitions[(size_t) juce::jmax (0, figure - 1)].way];
        setStatus ("Running: " + asClock (position) + " of " + asClock (plan.length) + ", number " + juce::String (figure) + " of "
                   + juce::String ((int) plan.repetitions.size()) + " (" + way.name + ").");
    }
    else
    {
        setStatus ("Playing one figure...");
    }
}
