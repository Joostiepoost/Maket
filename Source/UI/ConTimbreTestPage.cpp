// ConTimbreTestPage.cpp

#include "ConTimbreTestPage.h"
#include "Theme.h"
#include "../Engine/ConTimbre.h"
#include "../Engine/TimingTest.h"
#include "../Formats/BachRoll.h"

namespace
{
    constexpr int margin = 28;
    constexpr int minimumHeight = 850;
    constexpr int sendTag = 3;              // the mark on this page's messages in the send thread's records
    constexpr int toggleWidth = 112;
    constexpr int toggleHeight = 24;

    constexpr double leadTime = 300.0;      // from the click to the first command, ms
    constexpr double logTimeout = 8000.0;   // after this Maket says that it is still waiting for Max's file, ms

    maket::OscMessage logCommand (const juce::String& word)
    {
        return maket::OscMessage ("/maket/log").addText (word);
    }

    juce::String fileSafe (const juce::String& text)
    {
        return text.retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_").trim();
    }

    juce::String asClock (double ms)
    {
        const int seconds = juce::jmax (0, juce::roundToInt (ms / 1000.0));
        return juce::String (seconds / 60) + ":" + juce::String (seconds % 60).paddedLeft ('0', 2);
    }

    int countOf (const std::vector<maket::contimbre::TimedMessage>& messages, int voice, const char* command)
    {
        int n = 0;
        for (const auto& m : messages)
            if (m.voice == voice && m.message.address == command) ++n;
        return n;
    }
}

ConTimbreTestPage::ConTimbreTestPage (maket::Settings& s, maket::OscScheduler& sch) : settings (s), scheduler (sch)
{
    setOpaque (true);

    for (auto* field : { &rollField, &fromField })
    {
        field->setFont (Theme::font (14.0f));
        field->setSelectAllWhenFocused (true);
        addAndMakeVisible (*field);
    }
    rollField.setTextToShowWhenEmpty ("choose a bach.roll text file, drop one here, or paste its path and press Return", Theme::textMuted);
    rollField.onReturnKey = [this]
    {
        const auto path = rollField.getText().trim().unquoted();
        if (path.isNotEmpty()) loadRoll (juce::File::getCurrentWorkingDirectory().getChildFile (path));
    };
    fromField.setText ("0", false);

    chooseButton.setButtonText (juce::String::fromUTF8 ("Choose\xe2\x80\xa6"));
    playButton.setButtonText ("Play");
    stopButton.setButtonText ("Stop");
    takeButton.setButtonText ("Start a ScAst take");
    finishButton.setButtonText ("Finish the ScAst take");
    logButton.setButtonText (juce::String::fromUTF8 ("Compare with a log file\xe2\x80\xa6"));

    chooseButton.onClick = [this] { chooseRoll(); };
    playButton.onClick   = [this] { play(); };
    stopButton.onClick   = [this] { stop(); };
    takeButton.onClick   = [this] { startTake(); };
    finishButton.onClick = [this] { finishTake(); };
    logButton.onClick    = [this] { chooseLog(); };

    noteOnFirstToggle.setButtonText ("note-on before gain, dynamic, glissando");
    glissandoToZeroToggle.setButtonText ("glissando to 0 without breakpoints");
    spacingToggle.setButtonText ("0.1 ms between commands");
    byNumberToggle.setButtonText ("by number: each note by a number of its own, the voice at place k of the roll on the ePlayer's voice named voice + k");
    normalizedToggle.setButtonText ("by number: the target of a dynamic ramp from 0 to 1 (64 is sent as 0.504)");
    normalizedToggle.setToggleState (true, juce::dontSendNotification);
    for (auto* toggle : { &byNumberToggle, &normalizedToggle, &noteOnFirstToggle, &glissandoToZeroToggle, &spacingToggle })
        addAndMakeVisible (*toggle);

    for (auto* button : { &chooseButton, &playButton, &stopButton, &takeButton, &finishButton, &logButton })
        addAndMakeVisible (*button);

    view.setMultiLine (true);
    view.setReadOnly (true);
    view.setScrollbarsShown (true);
    view.setCaretVisible (false);
    view.setFont (Theme::mono (12.5f));
    addAndMakeVisible (view);

    setState (State::idle);
    setStatus ("Choose a roll first.");
    startTimerHz (20);
}

ConTimbreTestPage::~ConTimbreTestPage()
{
    stopTimer();
}

int ConTimbreTestPage::getHeightFor (int width) const
{
    const int perRow = juce::jmax (1, (juce::jmin (width - 2 * margin, 900)) / toggleWidth);
    const int rows = juce::jmax (1, ((int) voiceToggles.size() + perRow - 1) / perRow);
    return minimumHeight + (rows - 1) * toggleHeight;
}

juce::File ConTimbreTestPage::folder() const
{
    return settings.getFolder ("timingFolder");
}

// --- Layout ---------------------------------------------------------------------------

void ConTimbreTestPage::resized()
{
    auto area = getLocalBounds().reduced (margin, 0);
    area.removeFromTop (24);

    titleArea    = area.removeFromTop (30);
    subtitleArea = area.removeFromTop (40);
    area.removeFromTop (10);
    targetArea   = area.removeFromTop (22);
    area.removeFromTop (14);

    const int width = juce::jmin (area.getWidth(), 900);

    rollLabelArea = area.removeFromTop (18).withWidth (width);
    auto rollRow = area.removeFromTop (28).withWidth (width);
    chooseButton.setBounds (rollRow.removeFromRight (100));
    rollRow.removeFromRight (8);
    rollField.setBounds (rollRow);
    area.removeFromTop (12);

    voicesLabelArea = area.removeFromTop (18).withWidth (width);
    const int perRow = juce::jmax (1, width / toggleWidth);
    const int rows = juce::jmax (1, ((int) voiceToggles.size() + perRow - 1) / perRow);
    auto voicesArea = area.removeFromTop (rows * toggleHeight).withWidth (width);
    for (int i = 0; i < (int) voiceToggles.size(); ++i)
        voiceToggles[(size_t) i]->setBounds (voicesArea.getX() + (i % perRow) * toggleWidth, voicesArea.getY() + (i / perRow) * toggleHeight,
                                             toggleWidth - 6, toggleHeight);
    area.removeFromTop (10);

    numberLabelArea = area.removeFromTop (18).withWidth (width);
    byNumberToggle.setBounds (area.removeFromTop (toggleHeight).withWidth (width));
    normalizedToggle.setBounds (area.removeFromTop (toggleHeight).withWidth (width));
    area.removeFromTop (8);

    waysLabelArea = area.removeFromTop (18).withWidth (width);
    auto waysRow = area.removeFromTop (toggleHeight).withWidth (width);
    noteOnFirstToggle.setBounds (waysRow.removeFromLeft (juce::jmin (300, width * 36 / 100)));
    glissandoToZeroToggle.setBounds (waysRow.removeFromLeft (juce::jmin (270, width * 33 / 100)));
    spacingToggle.setBounds (waysRow);
    area.removeFromTop (12);

    auto row = area.removeFromTop (30);
    playButton.setBounds (row.removeFromLeft (90));
    row.removeFromLeft (8);
    stopButton.setBounds (row.removeFromLeft (80));
    row.removeFromLeft (14);
    fromLabelArea = row.removeFromLeft (118);
    fromField.setBounds (row.removeFromLeft (70).reduced (0, 1));

    area.removeFromTop (8);
    auto row2 = area.removeFromTop (30);
    takeButton.setBounds (row2.removeFromLeft (160));
    row2.removeFromLeft (8);
    finishButton.setBounds (row2.removeFromLeft (180));
    row2.removeFromLeft (8);
    logButton.setBounds (row2.removeFromLeft (210));

    area.removeFromTop (8);
    statusArea = area.removeFromTop (38);
    area.removeFromTop (6);
    viewHeadingArea = area.removeFromTop (22);
    area.removeFromTop (4);
    view.setBounds (area.withTrimmedBottom (20));
}

void ConTimbreTestPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    g.setColour (Theme::text);
    g.setFont (Theme::font (22.0f, true));
    g.drawText ("conTimbre test", titleArea, juce::Justification::centredLeft, true);

    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (13.0f));
    g.drawFittedText ("Plays a roll on the conTimbre ePlayer as the Max patch does: program, gain, note, dynamic curve and glissando of every note. "
                      "And holds Maket's commands against what ScAst itself sends for the same roll, with the logging patch in Max.",
                      subtitleArea, juce::Justification::topLeft, 2, 1.0f);

    g.drawText ("ePlayer: " + settings.get ("ePlayerHost") + ", port " + settings.get ("ePlayerPort")
                + juce::String::fromUTF8 ("  \xc2\xb7  logging patch: port ") + settings.get ("logPatchPort")
                + juce::String::fromUTF8 ("  \xc2\xb7  files in ") + settings.get ("timingFolder"),
                targetArea, juce::Justification::centredLeft, true);

    g.setFont (Theme::font (12.0f));
    g.drawText ("Roll", rollLabelArea, juce::Justification::centredLeft, true);
    g.drawText (voiceToggles.empty() ? "Voices: the voices of the roll that have a name appear here"
                                     : "Voices to play and to compare (the name must be that of a voice of the orchestra loaded in the ePlayer)",
                voicesLabelArea, juce::Justification::centredLeft, true);
    g.drawText ("How the notes are addressed (off: by voice name and pitch, as ScAst does)", numberLabelArea, juce::Justification::centredLeft, true);
    g.drawText ("The ways of the Max patch, for playing and comparing (all off is Maket's own way)", waysLabelArea, juce::Justification::centredLeft, true);
    g.drawText ("from, in seconds:", fromLabelArea, juce::Justification::centredRight, true);

    g.setColour (statusIsProblem ? Theme::warning : Theme::text);
    g.setFont (Theme::font (13.5f));
    g.drawFittedText (status, statusArea, juce::Justification::topLeft, 2, 1.0f);

    g.setColour (Theme::text);
    g.setFont (Theme::font (15.0f, true));
    g.drawText ("Results", viewHeadingArea, juce::Justification::centredLeft, true);
}

// --- State ------------------------------------------------------------------------------

void ConTimbreTestPage::setState (State newState)
{
    state = newState;
    stateSince = maket::OscScheduler::now();

    const bool idle = state == State::idle;
    chooseButton.setEnabled (idle);
    rollField.setEnabled (idle);
    playButton.setEnabled (idle && hasRoll);
    takeButton.setEnabled (idle && hasRoll);
    logButton.setEnabled (idle && hasRoll);
    finishButton.setEnabled (state == State::take);
    stopButton.setEnabled (! idle);
    for (auto& toggle : voiceToggles) toggle->setEnabled (idle);
    for (auto* toggle : { &byNumberToggle, &normalizedToggle, &noteOnFirstToggle, &glissandoToZeroToggle, &spacingToggle }) toggle->setEnabled (idle);
}

void ConTimbreTestPage::setStatus (const juce::String& text, bool isProblem)
{
    if (text == status && isProblem == statusIsProblem) return;
    status = text;
    statusIsProblem = isProblem;
    repaint (statusArea);
}

void ConTimbreTestPage::addToView (const juce::String& text)
{
    view.moveCaretToEnd();
    view.insertTextAtCaret (text);
}

std::vector<int> ConTimbreTestPage::chosenVoices() const
{
    std::vector<int> voices;
    for (size_t i = 0; i < voiceToggles.size(); ++i)
        if (voiceToggles[i]->getToggleState()) voices.push_back (voiceOfToggle[i]);
    return voices;
}

maket::contimbre::MappingOptions ConTimbreTestPage::options() const
{
    maket::contimbre::MappingOptions o;
    o.noteOnFirst = noteOnFirstToggle.getToggleState();
    o.glissandoToZero = glissandoToZeroToggle.getToggleState();
    o.spacing = spacingToggle.getToggleState() ? 0.1 : 0.0;
    o.byNumber = byNumberToggle.getToggleState();
    o.normalizedTargets = normalizedToggle.getToggleState();
    return o;
}

juce::String ConTimbreTestPage::optionsInWords() const
{
    juce::StringArray ways;
    if (noteOnFirstToggle.getToggleState())     ways.add ("note-on before gain, dynamic and glissando");
    if (glissandoToZeroToggle.getToggleState()) ways.add ("a glissando to 0 for notes without pitch breakpoints");
    if (spacingToggle.getToggleState())         ways.add ("0.1 ms between the commands of one moment");
    const juce::String how = ! byNumberToggle.getToggleState() ? "by pitch, "
                           : normalizedToggle.getToggleState() ? "by number (ramp targets from 0 to 1), " : "by number (ramp targets from 0 to 127), ";
    return how + (ways.isEmpty() ? juce::String ("Maket's own way") : "with the Max patch's " + ways.joinIntoString ("; "));
}

bool ConTimbreTestPage::readyToSend()
{
    if (! hasRoll) { setStatus ("Choose a roll first.", true); return false; }
    if (chosenVoices().empty()) { setStatus ("No voice is switched on.", true); return false; }

    const auto host = settings.get ("ePlayerHost");
    scheduler.setPlayer (host, settings.getInt ("ePlayerPort"));
    scheduler.setLog (host, settings.getInt ("logPatchPort"));
    scheduler.takeSent (sendTag);       // what an earlier run left behind
    return true;
}

// --- The roll ---------------------------------------------------------------------------

void ConTimbreTestPage::chooseRoll()
{
    const auto start = rollFile.existsAsFile() ? rollFile.getParentDirectory() : settings.getFolder ("importsFolder");
    chooser = std::make_unique<juce::FileChooser> ("Choose a bach.roll text file", start, "*.txt;*.llll");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& c)
                          {
                              const auto file = c.getResult();
                              if (file != juce::File()) loadRoll (file);
                          });
}

void ConTimbreTestPage::filesDropped (const juce::StringArray& files, int, int)
{
    if (state == State::idle && files.size() == 1) loadRoll (juce::File (files[0]));
}

void ConTimbreTestPage::loadRoll (const juce::File& file)
{
    if (state != State::idle) return;

    maket::Roll read;
    juce::StringArray remarks;
    const auto result = file.existsAsFile() ? maket::readBachRollFile (file, read, remarks)
                                            : juce::Result::fail ("There is no file " + file.getFullPathName() + ".");
    if (result.failed())
    {
        setStatus (result.getErrorMessage(), true);
        return;
    }

    roll = std::move (read);
    rollFile = file;
    hasRoll = true;
    rollField.setText (file.getFullPathName(), false);

    // A toggle for every voice that has a name: those are the voices the Max patch plays.
    voiceToggles.clear();
    voiceOfToggle.clear();
    for (size_t v = 0; v < roll.voices.size(); ++v)
    {
        if (roll.voices[v].name.isEmpty()) continue;
        auto toggle = std::make_unique<juce::ToggleButton> (roll.voices[v].name);
        toggle->setToggleState (! roll.voices[v].isMuted(), juce::dontSendNotification);
        addAndMakeVisible (*toggle);
        voiceToggles.push_back (std::move (toggle));
        voiceOfToggle.push_back ((int) v);
    }

    // What the roll gives, per voice.
    const auto all = maket::contimbre::messagesForRoll (roll);
    juce::String text;
    text << "=== " << file.getFileName() << "   " << asClock (roll.length()) << " long, " << (int) roll.voices.size() << " voices, "
         << (int) all.size() << " commands for the ePlayer\n"
         << "  voice              notes  program  dynamic     gain  glissando\n";
    for (size_t v = 0; v < roll.voices.size(); ++v)
    {
        const auto& voice = roll.voices[v];
        const int i = (int) v;
        text << "  " << juce::String (i + 1).paddedLeft (' ', 2) << " " << (voice.name.isEmpty() ? juce::String ("(no name)") : voice.name).paddedRight (' ', 13);
        if (voice.name.isEmpty())
            text << "  not played: the Max patch plays the voices that have a name\n";
        else
            text << juce::String (countOf (all, i, "noteon")).paddedLeft (' ', 6) << juce::String (countOf (all, i, "program")).paddedLeft (' ', 9)
                 << juce::String (countOf (all, i, "dynamic")).paddedLeft (' ', 9) << juce::String (countOf (all, i, "gain")).paddedLeft (' ', 9)
                 << juce::String (countOf (all, i, "glissando")).paddedLeft (' ', 11) << (voice.isMuted() ? "   (muted in the roll)" : "") << "\n";
    }
    int markerCommands = 0;
    for (const auto& m : all) if (m.voice < 0) ++markerCommands;
    if (markerCommands > 0) text << "  and " << markerCommands << " commands in markers named cpe\n";
    for (const auto& remark : remarks) text << "  remark: " << remark << "\n";
    text << "\n";
    addToView (text);

    setState (State::idle);
    setStatus ("Read. Play sends it to the ePlayer; Start a ScAst take compares Maket's commands with ScAst's.");

    if (auto* parent = getParentComponent()) parent->resized();   // more voices can make the page higher
    resized();
    repaint();
}

// --- Playing ----------------------------------------------------------------------------

void ConTimbreTestPage::play()
{
    if (! readyToSend()) return;

    const auto fromText = fromField.getText().trim().replaceCharacter (',', '.');
    if (fromText.isNotEmpty() && ! fromText.containsOnly ("0123456789."))
    {
        setStatus ("From: a number of seconds, such as 0 or 72.5.", true);
        return;
    }
    playFrom = fromText.getDoubleValue() * 1000.0;
    playEnd = roll.length();
    if (playFrom >= playEnd)
    {
        setStatus ("The roll ends at " + juce::String (playEnd / 1000.0, 1) + " seconds.", true);
        return;
    }

    const auto messages = maket::contimbre::messagesFrom (maket::contimbre::messagesForRoll (roll, chosenVoices(), options()), playFrom);

    addToView ("=== " + juce::Time::getCurrentTime().formatted ("%d %b %H:%M:%S") + "   Play from " + juce::String (playFrom / 1000.0, 1)
               + " s, " + juce::String ((int) messages.size()) + " commands, " + optionsInWords() + "\n\n");

    playStart = maket::OscScheduler::now() + leadTime;
    for (const auto& m : messages)
        scheduler.sendAt (playStart + (m.time - playFrom), m.message, maket::OscScheduler::toPlayer, sendTag);

    numToSend = (int) messages.size();
    numSent = numRefused = 0;
    latest = 0.0;
    played.clear();
    playedHow = optionsInWords();
    setState (State::playing);
}

void ConTimbreTestPage::stop()
{
    if (state == State::playing)
    {
        scheduler.clear();
        for (const auto& s : scheduler.takeSent (sendTag)) played.push_back (s);
        writePlayLog();
        scheduler.sendImmediately (maket::contimbre::allNotesOff(), maket::OscScheduler::toPlayer, sendTag);
        setState (State::idle);
        setStatus ("Stopped, and all notes off sent to the ePlayer.");
    }
    else if (state == State::take)
    {
        scheduler.sendImmediately (logCommand ("tap").addInteger (0), maket::OscScheduler::toLog, sendTag);
        setState (State::idle);
        setStatus ("The take is dropped; the logging patch no longer listens to ScAst.");
    }
    else if (state == State::waitingForLog)
    {
        setState (State::idle);
        setStatus (juce::String::fromUTF8 ("No longer waiting. When the file is there, Compare with a log file\xe2\x80\xa6 reads it."));
    }
}

void ConTimbreTestPage::writePlayLog()
{
    const auto dir = folder();
    if (played.empty() || dir == juce::File() || ! dir.createDirectory()) return;

    juce::String text;
    text << "Maket playing " << rollFile.getFileName() << " from " << juce::String (playFrom / 1000.0, 1) << " s, " << playedHow << "\n"
         << "Columns, separated by a tab: time in the roll (ms), left Maket this much after it (ms), command\n";
    for (const auto& s : played)
        text << juce::String (playFrom + s.due - playStart, 3) << "\t" << juce::String (s.sent - s.due, 3) << "\t" << s.text << "\n";

    const auto name = juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S") + " "
                      + fileSafe (rollFile.getFileNameWithoutExtension()).substring (0, 40) + " maket play send log.txt";
    dir.getChildFile (name).replaceWithData (text.toRawUTF8(), text.getNumBytesAsUTF8());
    played.clear();
}

// --- A ScAst take -----------------------------------------------------------------------

void ConTimbreTestPage::startTake()
{
    if (! readyToSend()) return;

    const auto dir = folder();
    if (dir == juce::File() || ! dir.createDirectory())
    {
        setStatus ("The folder for the test files could not be made: " + settings.get ("timingFolder")
                   + ". Set another one on the Preferences page (Test results).", true);
        return;
    }

    const auto name = juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S") + " "
                      + fileSafe (rollFile.getFileNameWithoutExtension()).substring (0, 40) + " scast";
    maxLogFile = dir.getChildFile (name + " max log.txt");
    maxLogFile.deleteFile();
    lastLogSize = -1;

    // A fresh log in Max, listening to what ScAst sends to the ePlayer.
    scheduler.sendImmediately (logCommand ("reset"), maket::OscScheduler::toLog, sendTag);
    scheduler.sendImmediately (logCommand ("tap").addInteger (1), maket::OscScheduler::toLog, sendTag);

    setState (State::take);
    setStatus ("The logging patch in Max now notes what ScAst sends to the ePlayer. Play this roll in ScAst from its start to its end, "
               "then click Finish the ScAst take.");
}

void ConTimbreTestPage::finishTake()
{
    if (state != State::take) return;

    scheduler.sendImmediately (logCommand ("tap").addInteger (0), maket::OscScheduler::toLog, sendTag);
    scheduler.sendImmediately (maket::OscMessage ("/maket/log").addText ("write").addText (maxLogFile.getFullPathName()),
                               maket::OscScheduler::toLog, sendTag);
    lastLogSize = -1;
    setState (State::waitingForLog);
    setStatus ("Asked Max to write its log...");
}

void ConTimbreTestPage::chooseLog()
{
    if (! readyToSend()) return;

    chooser = std::make_unique<juce::FileChooser> ("Choose a log written by the logging patch", folder(), "*.txt");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& c)
                          {
                              const auto file = c.getResult();
                              if (file != juce::File()) compareWith (file);
                          });
}

void ConTimbreTestPage::compareWith (const juce::File& logFile)
{
    setState (State::idle);

    if (! logFile.existsAsFile())
    {
        setStatus ("There is no file " + logFile.getFullPathName() + ".", true);
        return;
    }

    const auto voices = chosenVoices();
    const auto expected = maket::contimbre::messagesForRoll (roll, voices, options());
    const auto log = maket::timing::readCollLog (logFile.loadFileAsString());
    const auto comparison = maket::contimbre::compareWithLog (expected, log);

    juce::StringArray names;
    for (int v : voices) names.add (roll.voices[(size_t) v].name);

    juce::String shown;
    shown << "=== " << juce::Time::getCurrentTime().formatted ("%d %b %H:%M:%S") << "   " << rollFile.getFileName()
          << " against " << logFile.getFileName() << "\n"
          << "Voices: " << names.joinIntoString (", ") << ". Maket's commands: " << optionsInWords() << ".\n"
          << maket::contimbre::describe (comparison) << "\n";
    addToView (shown);

    // Filed beside the log: the result in words, and Maket's own commands for the roll.
    const auto dir = logFile.getParentDirectory();
    const bool filed = dir.getChildFile ("mapping results.txt").appendText (shown, false, false, "\n");

    juce::String commands;
    commands << "Maket's commands for " << rollFile.getFileName() << ", voices " << names.joinIntoString (", ") << "\n"
             << "Columns, separated by a tab: time in the roll (ms), command\n";
    for (const auto& m : expected)
        commands << juce::String (m.time, 3) << "\t" << m.message.toText() << "\n";
    auto commandsName = logFile.getFileNameWithoutExtension();
    commandsName = (commandsName.endsWith (" max log") ? commandsName.dropLastCharacters (8) : commandsName) + " maket commands.txt";
    dir.getChildFile (commandsName).replaceWithData (commands.toRawUTF8(), commands.getNumBytesAsUTF8());

    if (! comparison.usable)
        setStatus ("The comparison gave no result; see below.", true);
    else if (comparison.numOnlyInLog == 0 && comparison.numOnlyInMaket == 0)
        setStatus ((comparison.numZeroGlissandi + comparison.numChordFault > 0
                        ? "Every command is the same on both sides (" + juce::String (comparison.matched) + ") or differs in a way that is known of the Max patch ("
                              + juce::String (comparison.numZeroGlissandi + comparison.numChordFault) + ")."
                        : "The same commands on both sides: " + juce::String (comparison.matched) + ".")
                   + (filed ? " Filed in \"mapping results.txt\" in " + dir.getFullPathName() + "." : juce::String()));
    else
        setStatus (juce::String (comparison.matched) + " commands are the same; " + juce::String (comparison.numOnlyInLog) + " only ScAst sends, "
                   + juce::String (comparison.numOnlyInMaket) + " only Maket. See below"
                   + (filed ? "; filed in \"mapping results.txt\" in " + dir.getFullPathName() + "." : juce::String (".")), true);
}

// --- The clock --------------------------------------------------------------------------

void ConTimbreTestPage::timerCallback()
{
    const double now = maket::OscScheduler::now();

    if (state == State::playing)
    {
        for (const auto& s : scheduler.takeSent (sendTag))
        {
            ++numSent;
            if (! s.ok) ++numRefused;
            latest = juce::jmax (latest, s.sent - s.due);
            played.push_back (s);
        }

        const double position = playFrom + juce::jmax (0.0, now - playStart);
        if (scheduler.numWaiting() == 0 && numSent >= numToSend)
        {
            setState (State::idle);
            writePlayLog();
            if (numRefused > 0)
                setStatus (juce::String (numRefused) + " of " + juce::String (numToSend) + " commands could not be sent: is the ePlayer's address right "
                           "on the Preferences page?", true);
            else
                setStatus ("Played to the end: " + juce::String (numToSend) + " commands sent, the latest " + juce::String (latest, 2) + " ms after its time.");
        }
        else
        {
            setStatus ("Playing: " + asClock (position) + " of " + asClock (playEnd) + ", " + juce::String (numSent) + " of "
                       + juce::String (numToSend) + " commands sent.");
        }
    }
    else if (state == State::waitingForLog)
    {
        // The file is there when it exists and has stopped growing.
        const auto size = maxLogFile.existsAsFile() ? maxLogFile.getSize() : (juce::int64) -1;
        if (size > 0 && size == lastLogSize)
        {
            compareWith (maxLogFile);
        }
        else if (now - stateSince > logTimeout)
        {
            setStatus ("Still waiting for Max to write its log; it does so when it is free. If it never comes: click write in the logging patch "
                       "and save as \"" + maxLogFile.getFileName() + "\" in " + maxLogFile.getParentDirectory().getFullPathName()
                       + "; Maket reads it as soon as it is there.", true);
        }
        lastLogSize = size;
    }
}
