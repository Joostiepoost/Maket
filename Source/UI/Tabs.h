// Tabs.h — the four tabs of the main window, declared once.
//
// The tab row, the View menu, the empty pages and the inspector all read this
// one table, so a tab's name or description is changed in a single place.

#pragma once

namespace Tabs
{
    enum Index { library = 0, buildSurface, render, utilities, count };

    struct Info
    {
        const char* name;             // on the tab and in the View menu
        const char* holds;            // what the tab will hold (proposal, section 14)
        const char* inspectorShows;   // what the inspector shows while this tab is in front
        const char* arrives;          // the build steps that fill it
    };

    /** The description of one tab; `index` is one of Tabs::Index. */
    const Info& info (int index);
}
