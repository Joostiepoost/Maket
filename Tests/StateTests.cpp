// StateTests.cpp — stage 3a: what the window remembers, and the sound card's
// clock held against the computer's.
//
// The window itself, the audio switch and the dragging of the panels' edges
// are tried in the app; here the parts without interface code: the file the
// state is kept in, and the reckoning of how far two clocks run apart, on
// made-up audio blocks.

#include <JuceHeader.h>
#include <cmath>
#include <cstdint>

#include "TestReport.h"
#include "../Source/Model/WindowState.h"
#include "../Source/Engine/StreamClock.h"

using namespace testreport;

namespace
{
    /** Feeds a clock `seconds` of blocks of 512 samples at 48 kHz, from a sound card that runs `ppm` millionths fast
        against the computer. With the device's own times, or with the times of calls that come up to `jitter` ms off. */
    void run (maket::StreamClock& clock, double seconds, double ppm, bool deviceTimes, double jitter, juce::Random& random, double& computerMs)
    {
        const double rate = 48000.0;
        const double perBlock = 512 * 1000.0 / rate / (1.0 + ppm * 1.0e-6);       // the computer's time one block of the card takes
        const int blocks = (int) (seconds * 1000.0 / perBlock);
        for (int i = 0; i < blocks; ++i)
        {
            const auto host = (std::uint64_t) std::llround (computerMs * 1.0e6);
            const double called = computerMs + (jitter > 0.0 ? (random.nextDouble() * 2.0 - 1.0) * jitter : 0.0);
            clock.blockTime (deviceTimes ? computerMs - 3.0 : called, 512, rate, deviceTimes ? &host : nullptr, 5.0);
            computerMs += perBlock;
        }
    }
}

void testState (const juce::File& work)
{
    line ("## 12. The window's state and the two clocks (stage 3a)");
    line();
    line ("Maket remembers how its window was left. And it holds the sound card's clock against the computer's: sound files "
          "will run on the first, the ePlayer's commands run on the second.");
    line();
    line ("| Check | Passed |");
    line ("| --- | --- |");

    // --- The window's state ---
    {
        const auto file = work.getChildFile ("window state test.json");
        file.deleteFile();

        maket::WindowState fresh (file);
        fresh.load();
        const bool firstValues = fresh.listWidth() == 200 && fresh.inspectorWidth() == 280 && fresh.listShown() && fresh.inspectorShown()
                                 && fresh.tab() == 0 && fresh.windowBounds().isEmpty();
        const bool nothingWritten = fresh.save() && ! file.existsAsFile();

        fresh.setListWidth (260);
        fresh.setInspectorWidth (410);
        fresh.setListShown (false);
        fresh.setTab (3);
        fresh.setWindowBounds ("120 80 1500 900");
        const bool written = fresh.save() && file.existsAsFile();

        maket::WindowState again (file);
        again.load();
        const bool same = again.listWidth() == 260 && again.inspectorWidth() == 410 && ! again.listShown() && again.inspectorShown()
                          && again.tab() == 3 && again.windowBounds() == "120 80 1500 900";
        checkRow (firstValues && nothingWritten && written && same,
                  "The window's state: first values when there is no file (list 200, inspector 280, both shown, the first tab); nothing is written "
                  "until something changes; what is written (widths, a hidden list, the tab in front, the window's place) is read back the same");

        again.setListWidth (20);
        again.setInspectorWidth (5000);
        const bool limits = again.listWidth() == maket::WindowState::listWidthMin && again.inspectorWidth() == maket::WindowState::inspectorWidthMax;

        file.replaceWithText ("{ \"maket-window\": 1, \"listWidth\": 99999, \"inspectorWidth\": \"wide\", \"listShown\": 3, \"tab\": -4 }");
        maket::WindowState odd (file);
        odd.load();
        const bool oddOk = odd.listWidth() == maket::WindowState::listWidthMax && odd.inspectorWidth() == 280 && odd.listShown() && odd.tab() == 0;

        file.replaceWithText ("this is not a state file");
        maket::WindowState broken (file);
        broken.load();
        const bool brokenOk = broken.listWidth() == 200 && broken.inspectorWidth() == 280 && broken.inspectorShown();
        file.deleteFile();
        checkRow (limits && oddOk && brokenOk, "A width stays within its limits (list 140 to 520, inspector 200 to 640); values in the file that make no sense, "
                                               "and a file that is no state file at all, give the first values instead");
    }

    // --- The two clocks ---
    {
        juce::Random random (11);
        juce::String found;
        bool ok = true;
        for (double ppm : { 20.0, -35.0, 0.0, 250.0 })
        {
            maket::StreamClock clock;
            double t = 500000.0;
            run (clock, 90.0, ppm, true, 0.0, random, t);
            ok = ok && std::abs (clock.driftPpm() - ppm) < 0.05 && clock.driftSeconds() > 89.0 && clock.usesDeviceTime();
            found += juce::String (clock.driftPpm(), 2) + ", ";
        }
        checkRow (ok, "A sound card made to run 20 millionths fast, 35 slow, exactly even and 250 fast against the computer is measured as "
                      + found.trimEnd().dropLastCharacters (1) + " after a minute and a half, from the times the device gives");

        maket::StreamClock clock;
        double t = 500000.0;
        run (clock, 10.0, 40.0, true, 0.0, random, t);
        const bool tooSoon = clock.driftSeconds() < maket::StreamClock::driftFrom;
        run (clock, 50.0, 40.0, true, 0.0, random, t);
        const bool measured = std::abs (clock.driftPpm() - 40.0) < 0.05 && clock.driftSeconds() > 59.0;
        t += 32.0;                                              // three blocks lost: the device stood still for a moment
        run (clock, 5.0, 40.0, true, 0.0, random, t);
        const bool anew = clock.driftSeconds() < 6.0;
        run (clock, 60.0, 40.0, true, 0.0, random, t);
        const bool afterwards = std::abs (clock.driftPpm() - 40.0) < 0.05;
        checkRow (tooSoon && measured && anew && afterwards, "Nothing is said in the first 20 seconds; after a break in the sound (three blocks lost) the measuring begins anew "
                                                             "and comes to the same figure, instead of taking the break for a fast clock");

        maket::StreamClock calls;
        t = 500000.0;
        run (calls, 600.0, 25.0, false, 4.0, random, t);
        checkRow (! calls.usesDeviceTime() && std::abs (calls.driftPpm() - 25.0) < 15.0 && calls.driftSeconds() > 590.0,
                  "Where the device gives no times, the times of the calls serve (they come up to 4 ms off here): over ten minutes a card 25 millionths fast is measured as "
                  + juce::String (calls.driftPpm(), 1));
    }
    line();
}
