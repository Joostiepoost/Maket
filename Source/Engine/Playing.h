// Playing.h — what of a roll is played, from where to where.
//
// Before the mapping to conTimbre's commands (ConTimbreMapping) the roll is
// cut to what has to sound:
//
//   - what is muted does not play, and when anything in the roll is solo only
//     that plays (bach's rule: a note counts as muted or solo when it is so
//     itself or its chord or its voice is);
//   - play can begin at any point. A note that has begun before it and has
//     not ended plays its remainder, as bach does ("play partial notes"): it
//     begins at the point, at the pitch its glide has reached there, with
//     the rest of its glide and of its dynamics;
//   - play can end at a point (the end of a loop): a note still sounding
//     there ends there.
//
// The cut roll is a roll like any other; the mapping does not know of it.
//
// No interface code.

#pragma once

#include <JuceHeader.h>
#include <vector>

#include "../Model/Roll.h"
#include "ConTimbreMapping.h"
#include "Synth.h"

namespace maket::playing
{

/** Milliseconds a note that is entered in its middle takes to reach the dynamic it would have had there. */
constexpr double catchUpTime = 20.0;

/** What sounds a voice (stage 2e; a stopgap until the instruments of step 7). */
enum class VoiceEngine { conTimbre, synth, silent };

/** The engine each voice gets when nothing is chosen: a voice whose notes name a technique (slot 1) plays on
    conTimbre; a voice without a clef that tells a pitch (Percussion, None) is silent; any other voice plays on
    the synth. */
std::vector<VoiceEngine> defaultEngines (const Roll&);

/** The roll with the voices of one engine only; the others stay in their place, without chords, so that the
    numbers of the voices stay what they are. */
Roll forEngine (const Roll&, const std::vector<VoiceEngine>& engines, VoiceEngine which);

/** True when a note, a chord or a voice of the roll is solo. */
bool anySolo (const Roll&);

/** The roll without what does not play: muted notes, chords and voices, and, when anything is solo,
    everything that is not. Voices keep their place (an emptied voice stays, without chords). */
Roll audible (const Roll&);

/** The pitch offset of a note's glide at a place along it (0 to 1), in cents. */
double glideCentsAt (const Note&, double position);

/** The part of the roll between `from` and `to` (ms): notes that begin in it, and the remainder of notes
    that sound at `from`. Times stay those of the roll. A note cut at `from` begins there at the pitch its
    glide has reached; its glide and its dynamics (slot 2) go on from where they were, the dynamics reaching
    their value of that moment in `catchUpTime`; its gain (slot 3) starts at the last step before. A note
    still sounding at `to` ends there. */
Roll excerpt (const Roll&, double from, double to);

/** The commands for one pass over the part of `roll` from `from` to `to`: `audible`, `excerpt`, the mapping
    by number with notes numbered from `firstNumber`, and before them, at `from`, the last program and gain
    every voice was given earlier in the roll. Times are those of the roll. `numbersUsed` gets how many
    note numbers the pass took. */
std::vector<contimbre::TimedMessage> messagesForPass (const Roll& roll, double from, double to, int firstNumber, int& numbersUsed);

/** The same for a roll whose voices play on different engines: the commands of its conTimbre voices. */
std::vector<contimbre::TimedMessage> messagesForPass (const Roll& roll, const std::vector<VoiceEngine>& engines,
                                                     double from, double to, int firstNumber, int& numbersUsed);

/** The notes of the synth's voices for one pass: `audible` and `excerpt` as for conTimbre, so muted and solo, a
    note entered in its middle and the end of a loop are the same for both. `start` and `end` are times of the roll. */
std::vector<SynthNote> synthNotesForPass (const Roll& roll, const std::vector<VoiceEngine>& engines, double from, double to);

/** The numbers of the notes that sound at `time` (after their `ctnote`, before their `ctnoteoff`) in a pass. */
std::vector<int> soundingAt (const std::vector<contimbre::TimedMessage>& pass, double time);

} // namespace maket::playing
