// Main.cpp — the application object and its main window (after Projet's).
// The contents live in MainComponent.

#include <JuceHeader.h>
#include "MainComponent.h"

class MaketApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise (const juce::String& commandLine) override
    {
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
        openFromCommandLine (commandLine);
    }

    /** A file dropped on Maket's icon or "opened with" Maket while it runs. */
    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        openFromCommandLine (commandLine);
    }

    /** Opens what is named on the command line, or dropped on Maket's icon: a project's folder is opened as the
        project; a bach roll goes into the library of the open project, or, with no project open, is shown by itself. */
    void openFromCommandLine (const juce::String& commandLine)
    {
        if (mainWindow == nullptr) return;
        auto* main = dynamic_cast<MainComponent*> (mainWindow->getContentComponent());
        if (main == nullptr) return;

        juce::StringArray words;
        words.addTokens (commandLine, " ", "\"'");
        juce::Array<juce::File> files;
        for (auto word : words)
        {
            word = word.trim().unquoted();
            if (word.isEmpty()) continue;
            const auto file = juce::File::getCurrentWorkingDirectory().getChildFile (word);
            if (file.exists()) files.add (file);
        }
        // One path with spaces and without quotes, as before.
        if (files.isEmpty())
        {
            const auto whole = juce::File::getCurrentWorkingDirectory().getChildFile (commandLine.trim().unquoted());
            if (commandLine.trim().isNotEmpty() && whole.exists()) files.add (whole);
        }
        main->openFiles (files);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name)
            : DocumentWindow (name,
                              juce::Desktop::getInstance().getDefaultLookAndFeel()
                                  .findColour (juce::ResizableWindow::backgroundColourId),
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);
            setResizable (true, true);
            setResizeLimits (1000, 560, 8000, 5000);
            centreWithSize (1400, 860);

            // As it was left the last time (stage 3a), when that still fits a screen.
            if (auto* main = dynamic_cast<MainComponent*> (getContentComponent()))
                if (main->storedWindowBounds().isNotEmpty())
                    restoreWindowStateFromString (main->storedWindowBounds());
            setVisible (true);
        }

        ~MainWindow() override
        {
            keepBounds();
        }

        void closeButtonPressed() override
        {
            keepBounds();
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

        void keepBounds()
        {
            if (auto* main = dynamic_cast<MainComponent*> (getContentComponent()))
                main->storeWindowBounds (getWindowStateAsString());
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (MaketApplication)
