// Staves.cpp

#include "Staves.h"

namespace maket::notation
{

namespace
{
    Staff gStaff (int octaves = 0)
    {
        // The G clef: top line F5, ten steps above middle C; the clef on the G line.
        static const int tops[5] = { -4, 3, 10, 17, 24 };
        const int top = tops[octaves + 2];
        return { ClefSign::g, octaves, top, top - 6 };
    }

    Staff fStaff (int octaves = 0)
    {
        // The F clef: top line A3, two steps under middle C; the clef on the F line.
        static const int tops[5] = { -16, -9, -2, 5, 12 };
        const int top = tops[octaves + 2];
        return { ClefSign::f, octaves, top, top - 2 };
    }

    Staff cStaff (int topLineStep)
    {
        return { ClefSign::c, 0, topLineStep, 0 };      // the clef on the line of middle C
    }
}

double ClefLayout::unitsOf (double step) const
{
    const double top = staves.empty() ? 10.0 : (double) staves.front().topLineStep;
    return topLineUnits + stepUnits * (top - step);
}

double ClefLayout::bottomUnits() const
{
    return staves.empty() ? topLineUnits + 8.0 * stepUnits : unitsOf (staves.back().bottomLineStep());
}

ClefLayout clefLayout (const juce::String& clefName)
{
    ClefLayout c;
    c.name = clefName;
    const auto n = clefName.trim();

    if (n == "G")               c.staves = { gStaff() };
    else if (n == "F")          c.staves = { fStaff() };
    else if (n == "FG")         c.staves = { gStaff(), fStaff() };
    else if (n == "FFGG")       c.staves = { gStaff (2), gStaff(), fStaff(), fStaff (-2) };
    else if (n == "FFG")        c.staves = { gStaff(), fStaff(), fStaff (-2) };
    else if (n == "FGG")        c.staves = { gStaff (2), gStaff(), fStaff() };
    else if (n == "FF")         c.staves = { fStaff(), fStaff (-2) };
    else if (n == "GG")         c.staves = { gStaff (2), gStaff() };
    else if (n == "G8va")       c.staves = { gStaff (1) };
    else if (n == "G8vb")       c.staves = { gStaff (-1) };
    else if (n == "G15ma")      c.staves = { gStaff (2) };
    else if (n == "G15mb")      c.staves = { gStaff (-2) };
    else if (n == "F8va")       c.staves = { fStaff (1) };
    else if (n == "F8vb")       c.staves = { fStaff (-1) };
    else if (n == "F15ma")      c.staves = { fStaff (2) };
    else if (n == "F15mb")      c.staves = { fStaff (-2) };
    else if (n == "Soprano")    c.staves = { cStaff (8) };
    else if (n == "Mezzo")      c.staves = { cStaff (6) };
    else if (n == "Alto")       c.staves = { cStaff (4) };
    else if (n == "Tenor")      c.staves = { cStaff (2) };
    else if (n == "Barytone")   c.staves = { cStaff (0) };
    else if (n == "Percussion") c.staves = { { ClefSign::percussion, 0, 10, 6 } };     // notes lie as under a G clef
    else if (n == "None")       c.staves = { { ClefSign::none, 0, 10, 6 } };
    else
    {
        c.staves = { gStaff() };
        c.known = false;
    }
    return c;
}

std::vector<int> ledgerSteps (const ClefLayout& clef, int step)
{
    // The staff the head lies on or nearest to.
    int best = -1, bestDistance = 1 << 20;
    for (int i = 0; i < clef.numStaves(); ++i)
    {
        const auto& s = clef.staves[(size_t) i];
        const int distance = step > s.topLineStep ? step - s.topLineStep
                           : step < s.bottomLineStep() ? s.bottomLineStep() - step : 0;
        if (distance < bestDistance) { bestDistance = distance; best = i; }
    }

    std::vector<int> lines;
    if (best < 0 || bestDistance < 2) return lines;

    const auto& s = clef.staves[(size_t) best];
    if (step > s.topLineStep)
        for (int line = s.topLineStep + 2; line <= step; line += 2) lines.push_back (line);
    else
        for (int line = s.bottomLineStep() - 2; line >= step; line -= 2) lines.push_back (line);
    return lines;
}

RollLayout layoutVoices (const Roll& roll)
{
    // The header's lists: one item per voice (voicespacing: one more, the space above the first voice comes first).
    auto numbers = [&roll] (const char* name)
    {
        std::vector<double> out;
        if (const auto* entry = roll.headerEntry (name))
            for (size_t i = 1; i < entry->content.items.size(); ++i)
                out.push_back (entry->content.items[i].isNumber() ? entry->content.items[i].value : 0.0);
        return out;
    };
    const auto spacing = numbers ("voicespacing");
    const auto hidden = numbers ("hidevoices");
    const auto parts = numbers ("numparts");

    const int numVoices = (int) roll.voices.size();

    // numparts: how many voices share each set of staves, ensemble by ensemble.
    std::vector<bool> firstOfEnsemble ((size_t) numVoices, true);
    {
        int voice = 0;
        for (double p : parts)
        {
            const int count = juce::jmax (1, juce::roundToInt (p));
            for (int k = 1; k < count && voice + k < numVoices; ++k) firstOfEnsemble[(size_t) (voice + k)] = false;
            voice += count;
        }
    }

    RollLayout layout;
    double offset = spacing.empty() ? defaultSpacingUnits : spacing[0];

    for (int v = 0; v < numVoices; ++v)
    {
        VoicePlace place;
        place.voice = v;
        place.clef = clefLayout (roll.voices[(size_t) v].clef.isEmpty() ? juce::String ("G") : roll.voices[(size_t) v].clef);
        place.hidden = (size_t) v < hidden.size() && hidden[(size_t) v] > 0.5;
        place.ownsStaves = firstOfEnsemble[(size_t) v];
        place.offset = offset;
        layout.voices.push_back (place);

        // As bach: the offset moves on after the last part of an ensemble, and not for a hidden voice.
        const bool lastOfEnsemble = v == numVoices - 1 || firstOfEnsemble[(size_t) (v + 1)];
        if (! place.hidden && lastOfEnsemble)
        {
            const double after = (size_t) (v + 1) < spacing.size() ? spacing[(size_t) (v + 1)] : defaultSpacingUnits;
            offset += place.clef.numStaves() * staffAdvanceUnits + after;
        }
    }

    layout.height = offset + topLineUnits;
    return layout;
}

} // namespace maket::notation
