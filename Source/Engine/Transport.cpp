// Transport.cpp — see Transport.h.

#include "Transport.h"
#include "ConTimbre.h"
#include "Playing.h"

#include <algorithm>
#include <cmath>

namespace maket
{

Transport::Transport (OscScheduler& schedulerToUse, int tagToUse) : scheduler (schedulerToUse), tag (tagToUse) {}

Transport::~Transport()
{
    if (playing) stop();
}

void Transport::setRoll (const Roll& newRoll)
{
    if (playing) stop();
    roll = newRoll;
    engines = playing::defaultEngines (roll);
    preparedValid = false;
    rollSet = true;
    length = roll.length();
    started = stopped = 0.0;
}

void Transport::setLoop (double start, double end)
{
    loopFrom = std::max (0.0, std::min (start, end));
    loopTo = std::max (start, end);
}

Transport::Pass Transport::nextPass (const Pass* previous, double from, double clockStart) const
{
    Pass pass;
    pass.clockStart = clockStart;
    pass.firstNumber = previous != nullptr ? previous->firstNumber + previous->numbers : 1;

    const bool round = loopOn && hasLoop() && (previous != nullptr || from < loopTo);
    if (previous == nullptr)
    {
        pass.rollFrom = from;
        pass.rollTo = round ? loopTo : std::numeric_limits<double>::max();
    }
    else
    {
        pass.rollFrom = loopFrom;
        pass.rollTo = loopTo;
    }
    if (! preparedValid)
    {
        prepared = playing::prepare (roll, engines);
        preparedValid = true;
    }
    pass.messages = playing::messagesForPass (prepared, pass.rollFrom, pass.rollTo, pass.firstNumber, pass.numbers);
    pass.synthNotes = playing::synthNotesForPass (prepared, pass.rollFrom, pass.rollTo);
    return pass;
}

std::vector<SynthNote> Transport::onClock (const Pass& pass, double offsetMs)
{
    auto notes = pass.synthNotes;
    for (auto& n : notes)
    {
        n.start = pass.clockStart + (n.start - pass.rollFrom) + offsetMs;
        n.end = pass.clockStart + (n.end - pass.rollFrom) + offsetMs;
    }
    return notes;
}

void Transport::schedule (const Pass& pass)
{
    // The synth first: that is quick, and its audio device asks for its notes a little ahead of their time.
    if (synth != nullptr && ! pass.synthNotes.empty()) synth->addNotes (onClock (pass, synthOffset));

    // In order of time and a few thousand at once: the send thread has the first commands while the later ones
    // of a very large roll are still being handed over.
    std::vector<std::pair<double, OscMessage>> batch;
    const size_t most = 4000;
    batch.reserve (std::min (most, pass.messages.size()));
    for (const auto& m : pass.messages)
    {
        batch.emplace_back (pass.clockStart + (m.time - pass.rollFrom), m.message);
        if (batch.size() == most) { scheduler.sendAt (batch, OscScheduler::toPlayer, tag); batch.clear(); }
    }
    if (! batch.empty()) scheduler.sendAt (batch, OscScheduler::toPlayer, tag);
    commands += (int) pass.messages.size();
}

bool Transport::play (double from, double standardPitch, juce::String& problem)
{
    if (! rollSet)          { problem = "No roll is open."; return false; }
    if (playing) stop();

    from = std::max (0.0, from);
    if (from >= length)     { problem = "The roll ends at " + juce::String (length / 1000.0, 1) + " seconds."; return false; }

    looping = loopOn && hasLoop() && from < loopTo;
    auto first = nextPass (nullptr, from, 0.0);
    if (first.messages.empty() && first.synthNotes.empty() && ! looping)
    {
        problem = "Nothing plays from there: no note that is not muted, in a voice that plays.";
        return false;
    }

    commands = 0;
    passes.clear();
    started = from;
    playing = true;

    // The standard pitch first, ahead of the first note (stage 1f.8).
    scheduler.sendImmediately (contimbre::kammerton (standardPitch), OscScheduler::toPlayer, tag);

    // The clock is read only now: working out a very large roll takes a moment, and the first note keeps its lead.
    first.clockStart = OscScheduler::now() + leadTime;

    endClock = first.clockStart;
    for (const auto& m : first.messages) endClock = std::max (endClock, first.clockStart + (m.time - first.rollFrom));
    for (const auto& n : first.synthNotes) endClock = std::max (endClock, first.clockStart + (n.end - first.rollFrom));
    schedule (first);
    passes.push_back (std::move (first));
    service();
    return true;
}

void Transport::service()
{
    if (! playing) return;
    const double now = OscScheduler::now();

    if (! looping)
    {
        if (now > endClock + 50.0)
        {
            playing = false;
            stopped = length;
            passes.clear();
        }
        return;
    }

    // A loop: keep passes waiting up to `ahead` beyond now.
    for (;;)
    {
        const auto& last = passes.back();
        const double lastEnd = last.clockStart + (last.rollTo - last.rollFrom);
        if (lastEnd > now + ahead) break;
        auto next = nextPass (&last, loopFrom, lastEnd);
        schedule (next);
        passes.push_back (std::move (next));
    }

    // Passes that are over are no longer needed.
    while (passes.size() > 1 && passes[1].clockStart <= now)
        passes.pop_front();
}

double Transport::positionAt (const std::deque<Pass>& passes, double clock)
{
    if (passes.empty()) return 0.0;
    const Pass* current = &passes.front();
    for (const auto& p : passes)
        if (p.clockStart <= clock) current = &p;
    const double into = std::max (0.0, clock - current->clockStart);
    return current->rollFrom + (current->rollTo < std::numeric_limits<double>::max() ? std::min (into, current->rollTo - current->rollFrom) : into);
}

double Transport::position() const
{
    if (! playing) return started;
    return std::min (length, positionAt (passes, OscScheduler::now()));
}

void Transport::stop()
{
    if (! playing) return;
    const double now = OscScheduler::now();
    stopped = std::min (length, positionAt (passes, now));

    scheduler.clear (tag);

    // Every note that sounds is ended by its number; `all_notes_off` after them for whatever else rings.
    for (const auto& pass : passes)
    {
        if (pass.clockStart > now) continue;
        const double end = pass.rollTo < std::numeric_limits<double>::max() ? pass.clockStart + (pass.rollTo - pass.rollFrom) : std::numeric_limits<double>::max();
        if (now >= end) continue;
        for (int number : playing::soundingAt (pass.messages, pass.rollFrom + (now - pass.clockStart)))
            scheduler.sendImmediately (contimbre::ctNoteOff (number), OscScheduler::toPlayer, tag);
    }
    scheduler.sendImmediately (contimbre::allNotesOff(), OscScheduler::toPlayer, tag);
    if (synth != nullptr) synth->allNotesOff();

    playing = false;
    passes.clear();
}

} // namespace maket
