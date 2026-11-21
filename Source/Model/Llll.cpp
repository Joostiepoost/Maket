// Llll.cpp

#include "Llll.h"

#include <algorithm>
#include <cmath>

namespace maket
{

Llll Llll::makeInteger (juce::int64 n)
{
    Llll a;
    a.kind = Kind::integer;
    a.num = n;
    a.value = (double) n;
    return a;
}

Llll Llll::makeReal (double x)
{
    Llll a;
    a.kind = Kind::real;
    a.value = x;
    return a;
}

Llll Llll::makeSymbol (const juce::String& s)
{
    Llll a;
    a.kind = Kind::symbol;
    a.text = s;
    return a;
}

juce::String Llll::head() const
{
    return (isList() && ! items.empty() && items.front().isSymbol()) ? items.front().text : juce::String();
}

const Llll* Llll::find (const char* name) const
{
    for (const auto& item : items)
        if (item.isList() && ! item.items.empty() && item.items.front().isSymbol (name))
            return &item;
    return nullptr;
}

int Llll::countAtoms() const
{
    if (! isList()) return 1;
    int n = 0;
    for (const auto& item : items) n += item.countAtoms();
    return n;
}

int Llll::depth() const
{
    if (! isList()) return 0;
    int deepest = 0;
    for (const auto& item : items) deepest = std::max (deepest, item.depth());
    return deepest + 1;
}

juce::String Llll::toLine() const
{
    if (! isList())
    {
        if (spelling.isNotEmpty()) return spelling;
        switch (kind)
        {
            case Kind::integer:  return juce::String (num);
            case Kind::rational: return juce::String (num) + "/" + juce::String (den);
            case Kind::real:
            case Kind::pitch:    return juce::String (value, 6);
            case Kind::symbol:   return text;
            case Kind::list:     break;
        }
        return {};
    }

    if (spelling.isNotEmpty()) return spelling;      // an empty list that was written as nil

    juce::String s ("[");
    for (const auto& item : items) s << " " << item.toLine();
    return s + " ]";
}

bool Llll::sameContent (const Llll& a, const Llll& b, double tolerance)
{
    if (a.isList() != b.isList()) return false;

    if (a.isList())
    {
        if (a.items.size() != b.items.size()) return false;
        for (size_t i = 0; i < a.items.size(); ++i)
            if (! sameContent (a.items[i], b.items[i], tolerance)) return false;
        return true;
    }

    if (a.isNumber() != b.isNumber()) return false;
    if (a.isNumber()) return std::abs (a.value - b.value) <= tolerance;
    return a.text == b.text;
}

} // namespace maket
