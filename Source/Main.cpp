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

    /** Opens the roll named on the command line, if there is one. */
    void openFromCommandLine (const juce::String& commandLine)
    {
        const auto path = commandLine.trim().unquoted();
        if (path.isEmpty() || mainWindow == nullptr) return;
        const auto file = juce::File::getCurrentWorkingDirectory().getChildFile (path);
        if (file.existsAsFile())
            if (auto* main = dynamic_cast<MainComponent*> (mainWindow->getContentComponent()))
                main->openRollFile (file);
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
