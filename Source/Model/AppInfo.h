// AppInfo.h — the application's name, version and build stage, in one place.
//
// The version and the stage are set in CMakeLists.txt. The window shows them
// at the right of the tab row, and the test program writes them at the top
// of its report, so that it is always clear which build was tested.

#pragma once

namespace maket
{
    struct AppInfo
    {
        const char* name;       // "Maket"
        const char* version;    // three numbers, as "0.0.1"
        const char* stage;      // the build stage, as "0b"
    };

    const AppInfo& appInfo();
}
