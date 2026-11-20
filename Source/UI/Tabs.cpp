// Tabs.cpp

#include "Tabs.h"

namespace Tabs
{

const Info& info (int index)
{
    static const Info table[count] =
    {
        { "Library and edition",
          "The library with thumbnails on the left; in the rest, the editor of the open snippet: "
          "the roll, the waveform editor or the curve editor.",
          "The selected note and its slots, the voice, the slot list.",
          "The roll arrives at step 2, the library at step 3, editing at steps 4 to 6." },

        { "Build surface",
          "Tracks with blocks and containers, and the rulers.",
          "The selected block or container, its snippet, the track.",
          "Tracks, audio snippets and linked note blocks arrive at step 3; containers and nesting at step 9." },

        { "Render",
          "The render as a roll, and the list of the project's renders.",
          "The selected note and its slots, the voice, the block the note came from.",
          "Arrives at step 10." },

        { "Utilities",
          "Instruments and routing tables, plugin windows, the orchestra sheet and catalogue, "
          "capture, quantize settings, preferences.",
          "The selected instrument or setting.",
          "The preferences page is here since stage 0c; instruments arrive at step 7." }
    };

    if (index < 0) index = 0;
    if (index >= count) index = count - 1;
    return table[index];
}

} // namespace Tabs
