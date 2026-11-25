// Synth.h — Maket's own first, plain synth: one sine per note.
//
// A note begins and ends at moments on the send thread's clock
// (OscScheduler::now), the same clock the ePlayer's commands are timed on,
// and `render` is told at which moment its first sample will be heard. So a
// synth note and a conTimbre note that begin together are handed out for
// the same moment.
//
//   pitch       the note's cents, on the standard pitch (A = 6900 cents);
//               a glide follows the note's breakpoints along bach's curve
//   loudness    velocity / 127, times the modulation slot (slot 2, 0 to 127)
//               along its curve where the note has one; 10 ms attack and
//               50 ms release
//   timbre      a sine. The waveshape slot and the other timbres of the
//               proposal come with the discussion on the synth (before
//               step 7).
//
// Notes are handed over ahead of time from any thread; `render` runs in the
// audio thread. No audio device is known here: the console test program
// renders into memory and measures the result.

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <cmath>
#include <vector>

namespace maket
{

/** One note for the synth. Times in ms on the clock of OscScheduler::now(). */
struct SynthNote
{
    double start = 0.0, end = 0.0;
    double cents = 6000.0;
    int velocity = 100;

    struct GlidePoint { double position = 0.0, cents = 0.0, slope = 0.0; };     // position 0 to 1 along the note; cents from the note's pitch
    std::vector<GlidePoint> glide;                                              // empty: no glide

    struct LevelPoint { double ms = 0.0, value = 0.0, slope = 0.0; };           // ms after the start; 0 to 127
    std::vector<LevelPoint> levels;                                             // empty: the level is that of the velocity alone
};

/** Something that takes notes to sound them: the synth, or a stand-in for the console checks. */
class NoteSink
{
public:
    virtual ~NoteSink() = default;
    virtual void addNotes (std::vector<SynthNote> notes) = 0;
    /** Ends what sounds and forgets what waits. */
    virtual void allNotesOff() = 0;
};

class Synth : public NoteSink
{
public:
    Synth();

    static constexpr double attackMs = 10.0, releaseMs = 50.0;
    static constexpr int maxVoices = 128;

    void prepare (double sampleRateToUse);
    double getSampleRate() const                    { return sampleRate; }

    /** The frequency of the A above middle C. */
    void setStandardPitch (double hz)               { standardPitch.store (hz); }
    /** The level of one note at full velocity, in dB under full scale. */
    void setLevel (double dB)                       { gain.store (std::pow (10.0, dB / 20.0)); }

    void addNotes (std::vector<SynthNote> notes) override;
    void allNotesOff() override;

    /** Adds the synth's sound to `left` and `right` (either may be null). `clockOfFirstSample` is the moment,
        on the clock of OscScheduler::now(), at which the first of these samples is heard. */
    void render (float* left, float* right, int numSamples, double clockOfFirstSample);

    /** How many notes sound or wait. */
    int numNotes() const;

    static double frequencyOf (double cents, double standardPitchHz)    { return standardPitchHz * std::pow (2.0, (cents - 6900.0) / 1200.0); }

    /** The pitch offset of a note's glide at a place along it (0 to 1), and the level of its curve at a time. */
    static double glideAt (const SynthNote&, double position);
    static double levelAt (const SynthNote&, double ms);

private:
    struct Voice
    {
        SynthNote note;
        double phase = 0.0;
        double lastAmplitude = 0.0, lastIncrement = 0.0;
        bool started = false, cut = false;
        double cutAt = 0.0;
    };

    juce::CriticalSection lock;
    std::vector<SynthNote> waiting;         // in the order they were given
    std::vector<Voice> voices;              // what sounds
    std::vector<SynthNote> finished;        // set aside in the audio thread, freed outside it
    double sampleRate = 48000.0;
    std::atomic<double> standardPitch { 442.0 }, gain { 0.125 };
    std::atomic<bool> offWanted { false };

    JUCE_DECLARE_NON_COPYABLE (Synth)
};

} // namespace maket
