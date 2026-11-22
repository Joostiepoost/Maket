// OscTestPage.cpp

#include "OscTestPage.h"
#include "Theme.h"

namespace
{
    constexpr int margin = 28;
    constexpr int fieldHeight = 28;
    constexpr int minimumHeight = 620;

    const char* const fieldLabels[5] = { "Voice", "Program (may be empty)", "Pitch", "Dynamic", "Duration (ms)" };

    // The run of notes: how many, how far apart, how long each.
    constexpr int runLength = 8;
    constexpr double runSpacing = 250.0;
    constexpr double runNoteLength = 200.0;

    // A message asked for "now" is given a little time, so that it leaves at
    // a time that was set beforehand, like every message in a piece will.
    constexpr double leadTime = 30.0;

    // The mark on this page's messages in the send thread's records.
    constexpr int sendTag = 1;

    void styleField (juce::TextEditor& field, const juce::String& text)
    {
        field.setFont (Theme::font (14.0f));
        field.setText (text, false);
        field.setSelectAllWhenFocused (true);
    }
}

OscTestPage::OscTestPage (maket::Settings& s, maket::OscScheduler& sch) : settings (s), scheduler (sch)
{
    setOpaque (true);

    styleField (voiceField, "Cl");
    styleField (programField, "");
    styleField (pitchField, "60");
    styleField (dynamicField, "100");
    styleField (durationField, "1000");
    programField.setTextToShowWhenEmpty ("leave the program as it is", Theme::textMuted);

    for (auto* field : { &voiceField, &programField, &pitchField, &dynamicField, &durationField })
    {
        field->onReturnKey = [this] { playNote(); };
        addAndMakeVisible (*field);
    }

    copyToggle.setButtonText ("Send a copy of every message to the logging patch");
    copyToggle.setToggleState (true, juce::dontSendNotification);
    copyToggle.onClick  = [this] { repaint(); };
    addAndMakeVisible (copyToggle);

    noteButton.setButtonText ("Play note");
    runButton.setButtonText ("Play 8 notes");
    offButton.setButtonText ("All notes off");
    clearButton.setButtonText ("Clear log");
    saveButton.setButtonText (juce::String::fromUTF8 ("Save log\xe2\x80\xa6"));

    noteButton.onClick  = [this] { playNote(); };
    runButton.onClick   = [this] { playRun(); };
    offButton.onClick   = [this] { allNotesOff(); };
    clearButton.onClick = [this] { clearLog(); };
    saveButton.onClick  = [this] { saveLog(); };

    for (auto* button : { &noteButton, &runButton, &offButton, &clearButton, &saveButton })
        addAndMakeVisible (*button);

    logView.setMultiLine (true);
    logView.setReadOnly (true);
    logView.setScrollbarsShown (true);
    logView.setCaretVisible (false);
    logView.setFont (Theme::mono (12.5f));
    addAndMakeVisible (logView);

    clearLog();

    // Twenty times a second the page asks the send thread what has left
    // and adds it to the log.
    startTimerHz (20);
}

OscTestPage::~OscTestPage()
{
    stopTimer();
}

int OscTestPage::getHeightFor (int) const
{
    return minimumHeight;
}


// --- Layout ---------------------------------------------------------------------------

void OscTestPage::resized()
{
    auto area = getLocalBounds().reduced (margin, 0);
    area.removeFromTop (24);

    titleArea    = area.removeFromTop (30);
    subtitleArea = area.removeFromTop (40);
    area.removeFromTop (10);
    targetArea   = area.removeFromTop (22);
    area.removeFromTop (14);

    // The five fields in one row, each under its label.
    auto labels = area.removeFromTop (18);
    auto fields = area.removeFromTop (fieldHeight);
    const int gap = 12;
    const int total = juce::jmin (fields.getWidth(), 900);
    const int widths[5] = { (total - 4 * gap) * 24 / 100, (total - 4 * gap) * 34 / 100, (total - 4 * gap) * 13 / 100,
                            (total - 4 * gap) * 13 / 100, (total - 4 * gap) * 16 / 100 };
    juce::TextEditor* const editors[5] = { &voiceField, &programField, &pitchField, &dynamicField, &durationField };

    int x = fields.getX();
    for (int i = 0; i < 5; ++i)
    {
        labelArea[i] = { x, labels.getY(), widths[i], labels.getHeight() };
        editors[i]->setBounds (x, fields.getY(), widths[i], fields.getHeight());
        x += widths[i] + gap;
    }

    area.removeFromTop (12);
    copyToggle.setBounds (area.removeFromTop (24).withWidth (juce::jmin (area.getWidth(), 520)));

    area.removeFromTop (12);
    auto buttons = area.removeFromTop (30);
    noteButton.setBounds (buttons.removeFromLeft (110));
    buttons.removeFromLeft (8);
    runButton.setBounds (buttons.removeFromLeft (120));
    buttons.removeFromLeft (8);
    offButton.setBounds (buttons.removeFromLeft (120));
    buttons.removeFromLeft (28);
    clearButton.setBounds (buttons.removeFromLeft (100));
    buttons.removeFromLeft (8);
    saveButton.setBounds (buttons.removeFromLeft (110));

    area.removeFromTop (8);
    problemArea = area.removeFromTop (20);
    area.removeFromTop (8);
    logHeadingArea = area.removeFromTop (22);
    area.removeFromTop (4);
    logView.setBounds (area.withTrimmedBottom (20));
}

juce::String OscTestPage::whereTo() const
{
    juce::String s ("ePlayer: " + settings.get ("ePlayerHost") + ", port " + settings.get ("ePlayerPort"));
    if (copyToggle.getToggleState())
        s << juce::String::fromUTF8 ("  \xc2\xb7  logging patch: ") << settings.get ("ePlayerHost") << ", port " << settings.get ("logPatchPort");
    return s + juce::String::fromUTF8 ("  \xc2\xb7  set on the Preferences page");
}

void OscTestPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    g.setColour (Theme::text);
    g.setFont (Theme::font (22.0f, true));
    g.drawText ("OSC test", titleArea, juce::Justification::centredLeft, true);

    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (13.0f));
    g.drawFittedText ("Sends notes to the conTimbre ePlayer in Max and shows when each message left Maket. "
                      "A message that left has not necessarily arrived: whether Max got it shows in Max, by the sound "
                      "and in the logging patch.",
                      subtitleArea, juce::Justification::topLeft, 2, 1.0f);

    g.setFont (Theme::font (13.0f));
    g.drawText (whereTo(), targetArea, juce::Justification::centredLeft, true);

    g.setFont (Theme::font (12.0f));
    for (int i = 0; i < 5; ++i)
        g.drawText (fieldLabels[i], labelArea[i], juce::Justification::centredLeft, true);

    if (problem.isNotEmpty())
    {
        g.setColour (Theme::warning);
        g.setFont (Theme::font (13.0f));
        g.drawText (problem, problemArea, juce::Justification::centredLeft, true);
    }
    else if (saved.isNotEmpty())
    {
        g.setColour (Theme::textMuted);
        g.setFont (Theme::font (13.0f));
        g.drawText (saved, problemArea, juce::Justification::centredLeft, true);
    }

    g.setColour (Theme::text);
    g.setFont (Theme::font (15.0f, true));
    g.drawText ("Sent by Maket", logHeadingArea, juce::Justification::centredLeft, true);

    g.setColour (Theme::textMuted);
    g.setFont (Theme::font (12.0f));
    g.drawText (juce::String (scheduler.isRealtime() ? "send thread: realtime priority" : "send thread: ordinary priority")
                + (scheduler.numWaiting() > 0 ? juce::String::fromUTF8 ("  \xc2\xb7  waiting: ") + juce::String (scheduler.numWaiting())
                                              : juce::String()),
                logHeadingArea, juce::Justification::centredRight, true);
}

// --- Fields -----------------------------------------------------------------------------

bool OscTestPage::readFields()
{
    problem.clear();
    saved.clear();

    voice = voiceField.getText().trim();
    programName = programField.getText().trim();

    const auto pitchText = pitchField.getText().trim().replaceCharacter (',', '.');
    const auto dynamicText = dynamicField.getText().trim();
    const auto durationText = durationField.getText().trim().replaceCharacter (',', '.');

    auto isNumber = [] (const juce::String& s, bool decimals)
    {
        return s.isNotEmpty() && s.containsOnly (decimals ? "0123456789." : "0123456789")
               && s.retainCharacters (".").length() <= 1 && s != ".";
    };

    if (voice.isEmpty())
        problem = "Voice: the name of a voice in the orchestra that is loaded in the ePlayer.";
    else if (! isNumber (pitchText, true) || pitchText.getDoubleValue() > 127.0)
        problem = "Pitch: a MIDI note number from 0 to 127, decimals allowed (60 is middle C, 60.5 a quarter tone higher).";
    else if (! isNumber (dynamicText, false) || dynamicText.getIntValue() > 127)
        problem = "Dynamic: a whole number from 0 to 127.";
    else if (! isNumber (durationText, true) || durationText.getDoubleValue() < 1.0 || durationText.getDoubleValue() > 600000.0)
        problem = "Duration: from 1 to 600000 milliseconds.";

    repaint();
    if (problem.isNotEmpty()) return false;

    pitch = pitchText.getDoubleValue();
    dynamic = dynamicText.getIntValue();
    duration = durationText.getDoubleValue();
    return true;
}

int OscTestPage::targets() const
{
    return maket::OscScheduler::toPlayer | (copyToggle.getToggleState() ? (int) maket::OscScheduler::toLog : 0);
}

void OscTestPage::applyTargets()
{
    const auto host = settings.get ("ePlayerHost");
    scheduler.setPlayer (host, settings.getInt ("ePlayerPort"));
    scheduler.setLog (host, settings.getInt ("logPatchPort"));
}

// --- Sending ----------------------------------------------------------------------------

void OscTestPage::playNote()
{
    if (! readFields()) return;
    applyTargets();

    const double start = maket::OscScheduler::now() + leadTime;
    if (programName.isNotEmpty())
        scheduler.sendAt (start, maket::contimbre::program (voice, programName), targets(), sendTag);
    scheduler.sendAt (start, maket::contimbre::noteOn (voice, pitch, dynamic), targets(), sendTag);
    scheduler.sendAt (start + duration, maket::contimbre::noteOff (voice, pitch), targets(), sendTag);
}

void OscTestPage::playRun()
{
    if (! readFields()) return;
    applyTargets();

    const double start = maket::OscScheduler::now() + leadTime;
    if (programName.isNotEmpty())
        scheduler.sendAt (start, maket::contimbre::program (voice, programName), targets(), sendTag);

    // A rising run in semitones from the pitch, kept inside the range.
    for (int i = 0; i < runLength; ++i)
    {
        const double p = juce::jmin (127.0, pitch + i);
        scheduler.sendAt (start + i * runSpacing, maket::contimbre::noteOn (voice, p, dynamic), targets(), sendTag);
        scheduler.sendAt (start + i * runSpacing + runNoteLength, maket::contimbre::noteOff (voice, p), targets(), sendTag);
    }
}

void OscTestPage::allNotesOff()
{
    problem.clear();
    saved.clear();
    applyTargets();

    scheduler.clear();      // note-ons and note-offs that still wait are dropped
    scheduler.sendAt (maket::OscScheduler::now(), maket::contimbre::allNotesOff(), targets(), sendTag);
    repaint();
}

// --- The log ----------------------------------------------------------------------------

namespace
{
    juce::String logHeader()
    {
        return "    #    at (ms)   late (ms)   to    message\n";
    }

    juce::String column (const juce::String& text, int width)
    {
        return text.paddedLeft (' ', width);
    }
}

void OscTestPage::clearLog()
{
    entries.clear();
    firstDue = 0.0;
    saved.clear();
    logView.setText (logHeader(), false);
    repaint();
}

void OscTestPage::appendToLog (const std::vector<maket::OscScheduler::Sent>& sent)
{
    juce::String added;
    for (const auto& s : sent)
    {
        if (entries.empty()) firstDue = s.due;
        entries.push_back ({ s.due, s.sent, s.text, s.targets, s.ok });

        const juce::String to = juce::String ((s.targets & maket::OscScheduler::toPlayer) != 0 ? "P" : "-")
                              + ((s.targets & maket::OscScheduler::toLog) != 0 ? "L" : "-");

        added << column (juce::String ((int) entries.size()), 5)
              << column (juce::String (s.due - firstDue, 3), 11)
              << column (juce::String (s.sent - s.due, 3), 12)
              << "   " << to << "    " << s.text
              << (s.ok ? "" : "    (the system refused it)") << "\n";
    }

    logView.moveCaretToEnd();
    logView.insertTextAtCaret (added);
}

void OscTestPage::timerCallback()
{
    const auto sent = scheduler.takeSent (sendTag);
    if (! sent.empty()) appendToLog (sent);
    repaint (logHeadingArea);
}

juce::String OscTestPage::logAsText() const
{
    juce::String text;
    text << "Maket send log\n"
         << "Saved: " << juce::Time::getCurrentTime().toString (true, true) << "\n"
         << "ePlayer: " << settings.get ("ePlayerHost") << " port " << settings.get ("ePlayerPort")
         << "; logging patch: port " << settings.get ("logPatchPort") << "\n"
         << "Columns, separated by tabs: number, due (ms after the first message was due), late (ms: left Maket after it was due), "
            "to (P ePlayer, L logging patch), message\n";

    int n = 0;
    for (const auto& e : entries)
        text << ++n << "\t" << juce::String (e.due - firstDue, 3) << "\t" << juce::String (e.sent - e.due, 3) << "\t"
             << ((e.targets & maket::OscScheduler::toPlayer) != 0 ? "P" : "") << ((e.targets & maket::OscScheduler::toLog) != 0 ? "L" : "")
             << "\t" << e.text << (e.ok ? "" : "\trefused") << "\n";
    return text;
}

void OscTestPage::saveLog()
{
    problem.clear();
    const auto folder = settings.getFolder ("exportsFolder");
    const auto start = (folder.isDirectory() ? folder : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory))
                           .getChildFile ("Maket send log.txt");

    chooser = std::make_unique<juce::FileChooser> ("Save the send log", start, "*.txt");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (file == juce::File()) return;

                              const auto text = logAsText();
                              if (file.replaceWithData (text.toRawUTF8(), text.getNumBytesAsUTF8()))
                                  saved = "Saved: " + file.getFullPathName();
                              else
                                  problem = "The log could not be written to " + file.getFullPathName() + ".";
                              repaint();
                          });
}
