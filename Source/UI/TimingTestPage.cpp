// TimingTestPage.cpp

#include "TimingTestPage.h"
#include "Theme.h"
#include "../Engine/ConTimbre.h"
#include "../Formats/BachRoll.h"

namespace
{
    constexpr int margin = 28;
    constexpr int minimumHeight = 680;
    constexpr int sendTag = 2;              // the mark on this page's messages in the send thread's records

    constexpr double leadTime = 700.0;      // from the reset of the log to the start of the passage, ms
    constexpr double beforeWrite = 600.0;   // from the last message to asking Max for its log, ms
    constexpr double logTimeout = 8000.0;   // how long Max gets to write its log, ms

    const char* const fieldLabels[4] = { "Condition in Max, in your own words (overdrive, audio, what is on screen)",
                                         "Passage", "Voice", "Dynamic" };

    maket::OscMessage logCommand (const juce::String& word)
    {
        return maket::OscMessage ("/maket/log").addText (word);
    }

    juce::String fileSafe (const juce::String& text)
    {
        return text.retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_").trim();
    }
}

TimingTestPage::TimingTestPage (maket::Settings& s, maket::OscScheduler& sch) : settings (s), scheduler (sch)
{
    setOpaque (true);

    for (auto* field : { &conditionField, &voiceField, &dynamicField })
    {
        field->setFont (Theme::font (14.0f));
        field->setSelectAllWhenFocused (true);
        addAndMakeVisible (*field);
    }
    conditionField.setTextToShowWhenEmpty ("for example: overdrive on, audio on, only the logging patch open", Theme::textMuted);
    voiceField.setText ("Cl", false);
    dynamicField.setText ("60", false);

    passageBox.addItem ("Dense: 1403 notes in 23 seconds", 1);
    passageBox.addItem ("Long: a note every 100 ms for 5 minutes", 2);
    passageBox.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (passageBox);

    playerToggle.setButtonText ("Play the notes on the ePlayer as well, on this voice and at this dynamic");
    playerToggle.setToggleState (true, juce::dontSendNotification);
    addAndMakeVisible (playerToggle);

    runButton.setButtonText ("Run from Maket");
    bachStartButton.setButtonText ("Start a bach.roll take");
    bachFinishButton.setButtonText ("Finish the bach.roll take");
    stopButton.setButtonText ("Stop");
    passagesButton.setButtonText ("Write the passages as bach.roll files");
    readButton.setButtonText ("Read the log again");

    runButton.onClick        = [this] { runFromMaket(); };
    bachStartButton.onClick  = [this] { startBachTake(); };
    bachFinishButton.onClick = [this] { finishBachTake(); };
    stopButton.onClick       = [this] { stop(); };
    passagesButton.onClick   = [this] { writePassages (true); };
    readButton.onClick       = [this] { readLog(); };

    for (auto* button : { &runButton, &bachStartButton, &bachFinishButton, &stopButton, &passagesButton, &readButton })
        addAndMakeVisible (*button);

    view.setMultiLine (true);
    view.setReadOnly (true);
    view.setScrollbarsShown (true);
    view.setCaretVisible (false);
    view.setFont (Theme::mono (12.5f));
    addAndMakeVisible (view);

    setState (State::idle);
    setStatus ("Open \"Maket OSC log\" in Max first. Then say under which condition you test, and run.");
    startTimerHz (20);
}

TimingTestPage::~TimingTestPage()
{
    stopTimer();
}

int TimingTestPage::getHeightFor (int) const
{
    return minimumHeight;
}

juce::File TimingTestPage::folder() const
{
    return settings.getFolder ("timingFolder");
}

maket::timing::Passage TimingTestPage::chosenPassage() const
{
    return passageBox.getSelectedId() == 2 ? maket::timing::longPassage (5.0) : maket::timing::densePassage();
}

// --- Layout ---------------------------------------------------------------------------

void TimingTestPage::resized()
{
    auto area = getLocalBounds().reduced (margin, 0);
    area.removeFromTop (24);

    titleArea    = area.removeFromTop (30);
    subtitleArea = area.removeFromTop (40);
    area.removeFromTop (10);
    targetArea   = area.removeFromTop (22);
    area.removeFromTop (14);

    const int width = juce::jmin (area.getWidth(), 900);

    labelArea[0] = area.removeFromTop (18).withWidth (width);
    conditionField.setBounds (area.removeFromTop (28).withWidth (width));
    area.removeFromTop (12);

    auto labels = area.removeFromTop (18).withWidth (width);
    auto fields = area.removeFromTop (28).withWidth (width);
    const int gap = 12;
    const int widths[3] = { (width - 2 * gap) * 56 / 100, (width - 2 * gap) * 26 / 100, (width - 2 * gap) * 18 / 100 };
    juce::Component* const boxes[3] = { &passageBox, &voiceField, &dynamicField };

    int x = fields.getX();
    for (int i = 0; i < 3; ++i)
    {
        labelArea[i + 1] = { x, labels.getY(), widths[i], labels.getHeight() };
        boxes[i]->setBounds (x, fields.getY(), widths[i], fields.getHeight());
        x += widths[i] + gap;
    }

    area.removeFromTop (10);
    playerToggle.setBounds (area.removeFromTop (24).withWidth (juce::jmin (area.getWidth(), 520)));

    area.removeFromTop (12);
    auto row1 = area.removeFromTop (30);
    runButton.setBounds (row1.removeFromLeft (150));
    row1.removeFromLeft (8);
    bachStartButton.setBounds (row1.removeFromLeft (170));
    row1.removeFromLeft (8);
    bachFinishButton.setBounds (row1.removeFromLeft (190));
    row1.removeFromLeft (8);
    stopButton.setBounds (row1.removeFromLeft (80));

    area.removeFromTop (8);
    auto row2 = area.removeFromTop (30);
    passagesButton.setBounds (row2.removeFromLeft (270));
    row2.removeFromLeft (8);
    readButton.setBounds (row2.removeFromLeft (150));

    area.removeFromTop (8);
    statusArea = area.removeFromTop (38);
    area.removeFromTop (6);
    viewHeadingArea = area.removeFromTop (22);
    area.removeFromTop (4);
    view.setBounds (area.withTrimmedBottom (20));
}

void TimingTestPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    g.setColour (Theme::text);
    g.setFont (Theme::font (22.0f, true));
    g.drawText ("Timing test", titleArea, juce::Justification::centredLeft, true);

    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (13.0f));
    g.drawFittedText ("Plays a fixed passage and measures how evenly its messages arrive in Max, with the logging patch. "
                      "The same passage played by bach.roll inside Max is the reference.",
                      subtitleArea, juce::Justification::topLeft, 2, 1.0f);

    g.drawText ("logging patch: " + settings.get ("ePlayerHost") + ", port " + settings.get ("logPatchPort")
                + juce::String::fromUTF8 ("  \xc2\xb7  ePlayer: port ") + settings.get ("ePlayerPort")
                + juce::String::fromUTF8 ("  \xc2\xb7  files in ") + settings.get ("timingFolder"),
                targetArea, juce::Justification::centredLeft, true);

    g.setFont (Theme::font (12.0f));
    for (int i = 0; i < 4; ++i)
        g.drawText (fieldLabels[i], labelArea[i], juce::Justification::centredLeft, true);

    g.setColour (statusIsProblem ? Theme::warning : Theme::text);
    g.setFont (Theme::font (13.5f));
    g.drawFittedText (status, statusArea, juce::Justification::topLeft, 2, 1.0f);

    g.setColour (Theme::text);
    g.setFont (Theme::font (15.0f, true));
    g.drawText ("Results", viewHeadingArea, juce::Justification::centredLeft, true);
}

// --- State ------------------------------------------------------------------------------

void TimingTestPage::setState (State newState)
{
    state = newState;
    stateSince = maket::OscScheduler::now();

    const bool idle = state == State::idle;
    runButton.setEnabled (idle);
    bachStartButton.setEnabled (idle);
    bachFinishButton.setEnabled (state == State::bachTake);
    stopButton.setEnabled (! idle);
    passagesButton.setEnabled (idle);
    readButton.setEnabled (idle && kind != Kind::none);
    passageBox.setEnabled (idle);
}

void TimingTestPage::setStatus (const juce::String& text, bool isProblem)
{
    status = text;
    statusIsProblem = isProblem;
    repaint (statusArea);
}

void TimingTestPage::addToView (const juce::String& text)
{
    view.moveCaretToEnd();
    view.insertTextAtCaret (text);
}

bool TimingTestPage::prepare (const juce::String& playedBy)
{
    condition = conditionField.getText().trim();
    if (condition.isEmpty())
    {
        setStatus ("Say first under which condition you test: the result is filed under it.", true);
        return false;
    }

    onPlayer = playedBy == "maket" && playerToggle.getToggleState();
    if (onPlayer)
    {
        const auto d = dynamicField.getText().trim();
        if (voiceField.getText().trim().isEmpty() || d.isEmpty() || ! d.containsOnly ("0123456789") || d.getIntValue() > 127)
        {
            setStatus ("For the ePlayer: the name of a voice of the loaded orchestra, and a dynamic from 0 to 127.", true);
            return false;
        }
    }

    const auto dir = folder();
    if (dir == juce::File() || ! dir.createDirectory())
    {
        setStatus ("The folder for the timing files could not be made: " + settings.get ("timingFolder")
                   + ". Set another one on the Preferences page (Test results).", true);
        return false;
    }

    passage = chosenPassage();

    const auto now = juce::Time::getCurrentTime();
    when = now.formatted ("%d %b %H:%M:%S");
    takeName = now.formatted ("%Y%m%d-%H%M%S") + " " + passage.name + " " + playedBy;
    const auto tail = fileSafe (condition).substring (0, 40);
    if (tail.isNotEmpty()) takeName << " " << tail;

    maxLogFile = dir.getChildFile (takeName + " max log.txt");
    sendLogFile = dir.getChildFile (takeName + " send log.txt");
    maxLogFile.deleteFile();
    lastLogSize = -1;

    const auto host = settings.get ("ePlayerHost");
    scheduler.setPlayer (host, settings.getInt ("ePlayerPort"));
    scheduler.setLog (host, settings.getInt ("logPatchPort"));

    sent.clear();
    scheduler.takeSent (sendTag);       // what an earlier take left behind
    return true;
}

// --- A run from Maket -------------------------------------------------------------------

void TimingTestPage::runFromMaket()
{
    if (! prepare ("maket")) return;
    kind = Kind::maketRun;

    const auto voice = voiceField.getText().trim();
    const int dynamic = dynamicField.getText().trim().getIntValue();

    // A fresh log in Max, then the passage.
    scheduler.sendImmediately (logCommand ("reset"), maket::OscScheduler::toLog, sendTag);
    startTime = maket::OscScheduler::now() + leadTime;

    for (size_t i = 0; i < passage.notes.size(); ++i)
    {
        const auto& n = passage.notes[i];
        const double on = startTime + n.onset;
        const double off = on + n.duration;

        if (onPlayer) scheduler.sendAt (on, maket::contimbre::noteOn (voice, n.pitch, dynamic), maket::OscScheduler::toPlayer, sendTag);
        scheduler.sendAt (on, maket::timing::tickMessage ((int) i, false), maket::OscScheduler::toLog, sendTag);

        if (onPlayer) scheduler.sendAt (off, maket::contimbre::noteOff (voice, n.pitch), maket::OscScheduler::toPlayer, sendTag);
        scheduler.sendAt (off, maket::timing::tickMessage ((int) i, true), maket::OscScheduler::toLog, sendTag);
    }

    expectedRecords = 1 + passage.numMessages() * (onPlayer ? 2 : 1);
    setState (State::running);
}

// --- A take from bach.roll --------------------------------------------------------------

void TimingTestPage::startBachTake()
{
    if (! prepare ("bach")) return;
    kind = Kind::bachTake;

    writePassages (false);
    scheduler.sendImmediately (logCommand ("reset"), maket::OscScheduler::toLog, sendTag);

    setState (State::bachTake);
    setStatus ("The log in Max is fresh. Now press play in the bach.roll of \"Maket bach reference\" (the passage: "
               + passage.name + "). When it has played to its end, click Finish the bach.roll take.");
}

void TimingTestPage::finishBachTake()
{
    if (state != State::bachTake) return;
    askMaxToWrite();
}

void TimingTestPage::stop()
{
    const bool waitingForMax = state == State::waitingForLog;

    scheduler.clear();
    if (onPlayer && kind == Kind::maketRun && ! waitingForMax)
        scheduler.sendImmediately (maket::contimbre::allNotesOff(), maket::OscScheduler::toPlayer, sendTag);

    // A take that only waits for Max's file can still be read later, by Read the log again.
    if (! waitingForMax) kind = Kind::none;
    setState (State::idle);
    setStatus (waitingForMax ? "No longer waiting. When the file is there, Read the log again files the take."
                             : "Stopped. Nothing was filed for this take.");
}

void TimingTestPage::writePassages (bool tellInStatus)
{
    const auto dir = folder();
    if (dir == juce::File() || ! dir.createDirectory())
    {
        setStatus ("The folder for the timing files could not be made: " + settings.get ("timingFolder") + ".", true);
        return;
    }

    const auto d = dynamicField.getText().trim();
    const int velocity = (d.isNotEmpty() && d.containsOnly ("0123456789")) ? juce::jlimit (1, 127, d.getIntValue()) : 60;

    const auto denseFile = dir.getChildFile ("timing passage dense.txt");
    const auto longFile = dir.getChildFile ("timing passage long.txt");
    const auto r1 = maket::writeBachRollFile (denseFile, maket::timing::passageAsRoll (maket::timing::densePassage(), velocity));
    const auto r2 = maket::writeBachRollFile (longFile, maket::timing::passageAsRoll (maket::timing::longPassage (5.0), velocity));

    if (r1.failed() || r2.failed())
        setStatus (r1.failed() ? r1.getErrorMessage() : r2.getErrorMessage(), true);
    else if (tellInStatus)
        setStatus ("Written: \"timing passage dense.txt\" and \"timing passage long.txt\" in " + dir.getFullPathName()
                   + ". Open one in the bach.roll of \"Maket bach reference\" with its read message.");
}

// --- The log ----------------------------------------------------------------------------

void TimingTestPage::askMaxToWrite()
{
    scheduler.sendImmediately (maket::OscMessage ("/maket/log").addText ("write").addText (maxLogFile.getFullPathName()),
                               maket::OscScheduler::toLog, sendTag);
    lastLogSize = -1;
    setState (State::waitingForLog);
    setStatus ("Asked Max to write its log...");
}

void TimingTestPage::timerCallback()
{
    const double now = maket::OscScheduler::now();

    if (state == State::running)
    {
        const auto more = scheduler.takeSent (sendTag);
        sent.insert (sent.end(), more.begin(), more.end());

        const double left = (startTime + passage.length() - now) / 1000.0;
        setStatus ("Playing the " + passage.name + " passage: " + juce::String ((int) sent.size()) + " of "
                   + juce::String (expectedRecords) + " messages sent, " + juce::String (juce::jmax (0, juce::roundToInt (left))) + " seconds to go.");

        if (scheduler.numWaiting() == 0 && (int) sent.size() >= expectedRecords)
            setState (State::beforeWrite);
    }
    else if (state == State::beforeWrite)
    {
        if (now - stateSince >= beforeWrite) askMaxToWrite();
    }
    else if (state == State::waitingForLog)
    {
        // The file is there when it exists and has stopped growing.
        const auto size = maxLogFile.existsAsFile() ? maxLogFile.getSize() : (juce::int64) -1;
        if (size > 0 && size == lastLogSize)
        {
            readLog();
        }
        else if (now - stateSince > logTimeout)
        {
            // Max writes the file when its interface is free (not while a
            // window is being dragged), so Maket goes on waiting. Stop ends it.
            setStatus ("Still waiting for Max to write its log; it does so when it is free. If it never comes: click write in the "
                       "logging patch and save as \"" + maxLogFile.getFileName() + "\" in "
                       + maxLogFile.getParentDirectory().getFullPathName() + "; Maket reads it as soon as it is there.", true);
        }
        lastLogSize = size;
    }
}

void TimingTestPage::readLog()
{
    if (kind == Kind::none) return;

    if (! maxLogFile.existsAsFile())
    {
        setState (State::idle);
        setStatus ("There is no file \"" + maxLogFile.getFileName() + "\" in " + maxLogFile.getParentDirectory().getFullPathName() + ".", true);
        return;
    }

    const auto log = maket::timing::readCollLog (maxLogFile.loadFileAsString());
    const auto dir = maxLogFile.getParentDirectory();

    juce::String shown, rows;
    auto file = [&] (const juce::String& playedBy, const maket::timing::Result& result)
    {
        shown << "=== " << when << "   " << passage.name << " passage, played by " << playedBy << "   [" << condition << "]\n"
              << maket::timing::describe (result, passage) << "\n";
        rows << maket::timing::tableRow (when, passage.name, playedBy, condition, result);
    };

    bool usable = false;
    if (kind == Kind::maketRun)
    {
        const auto result = maket::timing::compareMaketRun (passage, sent, log);
        usable = result.usable;
        file (onPlayer ? "Maket (with the ePlayer)" : "Maket (ticks only)", result);

        // bach.roll played at the same time: its notes are in the same log.
        if (maket::timing::hasBachLines (log))
            file ("bach.roll, while Maket played", maket::timing::compareBachTake (passage, log));

        // Maket's own side of the take.
        juce::String text;
        text << "Maket send log, timing test\n" << takeName << "\nCondition: " << condition << "\n"
             << "Columns, separated by tabs: due (ms from the start of the passage), late (ms), to (P ePlayer, L logging patch), message\n";
        for (const auto& s : sent)
            text << juce::String (s.due - startTime, 3) << "\t" << juce::String (s.sent - s.due, 3) << "\t"
                 << ((s.targets & maket::OscScheduler::toPlayer) != 0 ? "P" : "L") << "\t" << s.text << (s.ok ? "" : "\trefused") << "\n";
        sendLogFile.replaceWithData (text.toRawUTF8(), text.getNumBytesAsUTF8());
    }
    else
    {
        const auto result = maket::timing::compareBachTake (passage, log);
        usable = result.usable;
        file ("bach.roll", result);
    }

    addToView (shown);

    // The table of results and the same in words, both added to at every take.
    const auto table = dir.getChildFile ("timing results.md");
    if (! table.existsAsFile())
        table.appendText ("# Maket timing tests (stage 1d)\n\nEvery line is one take. Times in milliseconds: how far the messages arrived "
                          "from the time they were due, after the two clocks were laid on each other at the middle message.\n\n"
                          + maket::timing::tableHeading(), false, false, "\n");
    table.appendText (rows, false, false, "\n");
    dir.getChildFile ("timing details.txt").appendText (shown, false, false, "\n");

    setState (State::idle);
    setStatus (usable ? "Filed in \"timing results.md\" in " + dir.getFullPathName() + ". Change one thing in Max, say so in Condition, and run again."
                      : "The take gave no result; see below.", ! usable);
}
