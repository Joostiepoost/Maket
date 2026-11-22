// BusyActivity.cpp — for systems other than macOS: nothing to do. See BusyActivity.h.

#include "BusyActivity.h"

namespace maket::platform
{

void* beginBusyActivity()           { return nullptr; }
void endBusyActivity (void*)        {}

} // namespace maket::platform
