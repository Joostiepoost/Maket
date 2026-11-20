// AppInfo.cpp

#include "AppInfo.h"

namespace maket
{

const AppInfo& appInfo()
{
    static const AppInfo info { "Maket", MAKET_VERSION, MAKET_BUILD_STAGE };
    return info;
}

} // namespace maket
