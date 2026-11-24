// MusicFont.cpp

#include "MusicFont.h"
#include "BinaryData.h"

namespace
{
    constexpr float referenceHeight = 200.0f;
}

MusicFont& MusicFont::get()
{
    static MusicFont font;
    return font;
}

MusicFont::MusicFont()
{
    typeface = juce::Typeface::createSystemTypefaceFor (BinaryData::Bravura_otf, (size_t) BinaryData::Bravura_otfSize);
    if (typeface == nullptr) return;

    // A black note head is exactly one staff space high in a SMuFL font: that sets the scale.
    const auto head = rawPath (noteheadBlack);
    const float height = head.getBounds().getHeight();
    if (head.isEmpty() || height <= 0.0f) return;

    unitsPerSpace = height;
    loaded = true;
}

juce::Path MusicFont::rawPath (juce::juce_wchar sign) const
{
    juce::Path p;
    if (typeface == nullptr) return p;

    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText (juce::Font (juce::FontOptions (typeface).withHeight (referenceHeight)),
                          juce::String::charToString (sign), 0.0f, 0.0f);
    glyphs.createPath (p);
    return p;
}

const juce::Path& MusicFont::path (juce::juce_wchar sign)
{
    auto found = cache.find (sign);
    if (found != cache.end()) return found->second;

    auto p = rawPath (sign);
    if (loaded) p.applyTransform (juce::AffineTransform::scale (1.0f / unitsPerSpace));
    return cache.emplace (sign, std::move (p)).first->second;
}

void MusicFont::draw (juce::Graphics& g, juce::juce_wchar sign, float x, float y, float space)
{
    const auto& p = path (sign);
    if (! p.isEmpty())
        g.fillPath (p, juce::AffineTransform::scale (space).translated (x, y));
}

float MusicFont::noteheadWidth()
{
    const auto& p = path (noteheadBlack);
    return p.isEmpty() ? 1.18f : p.getBounds().getRight();
}
