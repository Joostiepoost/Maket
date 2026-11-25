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
    pass.messages = playing::messagesForPass (roll, pass.rollFrom, pass.rollTo, pass.firstNumber, pass.numbers);
    return pass;
}

void Transport::schedule (const Pass& pass)
{
    for (const auto& m : pass.messages)
        scheduler.sendAt (pass.clockStart + (m.time - pass.rollFrom), m.message, OscScheduler::toPlayer, tag);
    commands += (int) pass.messages.size();
}

bool Transport::play (double from, double standardPitch, juce::String& problem)
{
    if (! rollSet)          { problem = "No roll is open."; return false; }
    if (playing) stop();

    from = std::max (0.0, from);
    if (from >= length)     { problem = "The roll ends at " + juce::String (length / 1000.0, 1) + " seconds."; return false; }

    const double now = OscScheduler::now();
    looping = loopOn && hasLoop() && from < loopTo;
    auto first = nextPass (nullptr, from, now + leadTime);
    if (first.messages.empty() && ! looping)
    {
        problem = "Nothing plays from there: no note that is not muted, in a voice with a name.";
        return false;
    }

    commands = 0;
    passes.clear();
    started = from;
    playing = true;

    // The standard pitch first, ahead of the first note (stage 1f.8).
    scheduler.sendImmediately (contimbre::kammerton (standardPitch), OscScheduler::toPlayer, tag);

    endClock = first.clockStart;
    for (const auto& m : first.messages) endClock = std::max (endClock, first.clockStart + (m.time - first.rollFrom));
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

    playing = false;
    passes.clear();
}

} // namespace maket
