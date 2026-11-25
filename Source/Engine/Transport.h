// Transport.h — plays a roll on the conTimbre ePlayer: from any point, to
// the end or round a loop, and stops.
//
// Everything is handed to the send thread (OscScheduler) ahead of time, each
// command with its moment; nothing waits for the interface. Without a loop
// the whole remainder of the roll is handed over at once. With a loop one
// pass at a time: `service`, called a few times a second, keeps some seconds
// of passes waiting. Where the sound is at a moment follows from the clock
// alone, so a playhead can be drawn or left out without any effect on the
// sound.
//
// A pass is what `playing::messagesForPass` gives: muted and solo applied,
// notes sounding at its start played from there, notes sounding at the end
// of a loop ended there. Each pass of a loop numbers its notes anew.
//
// Since 2e a voice plays on conTimbre, on Maket's synth or not at all. The
// synth's notes of a pass are handed to it with the same clock times, so
// both engines are told the same moments.
//
// No interface code.

#pragma once

#include <JuceHeader.h>
#include <deque>
#include <vector>

#include "../Model/Roll.h"
#include "ConTimbreMapping.h"
#include "OscScheduler.h"
#include "Playing.h"
#include "Synth.h"

namespace maket
{

class Transport
{
public:
    /** `tag` marks this transport's commands in the send thread. */
    Transport (OscScheduler&, int tag);
    ~Transport();

    /** The roll to play (a copy is kept). Stops what plays. */
    void setRoll (const Roll&);
    bool hasRoll() const                    { return rollSet; }
    double rollLength() const               { return length; }

    /** What sounds each voice (stage 2e). Without this every voice gets its default (`playing::defaultEngines`).
        Takes effect at the next play. */
    void setEngines (std::vector<playing::VoiceEngine> newEngines)     { engines = std::move (newEngines); }
    const std::vector<playing::VoiceEngine>& getEngines() const        { return engines; }

    /** Where the synth's notes go (null: nowhere), and by how many ms they are moved against the ePlayer's
        commands (positive: later). */
    void setSynth (NoteSink* sink)          { synth = sink; }
    void setSynthOffset (double ms)         { synthOffset = ms; }

    /** The loop: from `start` to `end` (ms), and whether play goes round it. Takes effect at the next play. */
    void setLoop (double start, double end);
    void setLoopOn (bool shouldLoop)        { loopOn = shouldLoop; }
    bool isLoopOn() const                   { return loopOn; }
    double loopStart() const                { return loopFrom; }
    double loopEnd() const                  { return loopTo; }
    bool hasLoop() const                    { return loopTo - loopFrom >= shortestLoop; }

    /** Plays from `from` (ms). The standard pitch is sent first. False, with the reason, when nothing can play. */
    bool play (double from, double standardPitch, juce::String& problem);

    /** Stops: forgets what waits and ends every note that sounds. */
    void stop();

    /** To be called a few times a second while playing: hands the next passes of a loop to the send thread,
        and notices the end of the roll. */
    void service();

    bool isPlaying() const                  { return playing; }

    /** Where the sound is now, in ms of the roll; when not playing, where play last began. */
    double position() const;

    double startedFrom() const              { return started; }
    double stoppedAt() const                { return stopped; }     // where the sound was at the last stop (the end, after a run to the end)

    /** How many commands the last play has handed over so far. */
    int numCommands() const                 { return commands; }

    static constexpr double leadTime = 120.0;       // from the call to the first note, ms
    static constexpr double shortestLoop = 50.0;    // ms
    static constexpr double ahead = 4000.0;         // how far ahead of the sound a loop's passes are handed over, ms

    // --- The reckoning, apart from the clock: for the console checks ---

    /** One pass: the part of the roll from `rollFrom` to `rollTo`, its first moment due at `clockStart`. */
    struct Pass
    {
        double rollFrom = 0.0, rollTo = 0.0;
        double clockStart = 0.0;
        std::vector<contimbre::TimedMessage> messages;     // for the ePlayer; times of the roll
        std::vector<SynthNote> synthNotes;                  // for the synth; times of the roll
        int firstNumber = 1, numbers = 0;
    };

    /** A pass's synth notes with their times on the clock, as they are handed to the synth. */
    static std::vector<SynthNote> onClock (const Pass&, double offsetMs);

    /** Where in the roll the sound is at `clock`, given the passes handed over (in order). */
    static double positionAt (const std::deque<Pass>& passes, double clock);

    /** The pass that follows `previous` (or the first, when `previous` is null) for a play from `from`. */
    Pass nextPass (const Pass* previous, double from, double clockStart) const;

private:
    void schedule (const Pass&);

    OscScheduler& scheduler;
    const int tag;
    Roll roll;
    std::vector<playing::VoiceEngine> engines;
    NoteSink* synth = nullptr;
    double synthOffset = 0.0;
    bool rollSet = false;
    double length = 0.0;
    double loopFrom = 0.0, loopTo = 0.0;
    bool loopOn = false;

    bool playing = false, looping = false;
    std::deque<Pass> passes;
    double started = 0.0, stopped = 0.0;
    double endClock = 0.0;      // without a loop: when the last command is due
    int commands = 0;

    JUCE_DECLARE_NON_COPYABLE (Transport)
};

} // namespace maket
