// BusyActivity.h — tells the operating system that Maket is busy with
// something that must happen on time.
//
// macOS saves energy by "napping" an app that is not in front: its timers
// then fire late, by up to a tenth of a second, and so would Maket's
// messages (measured on 22 Nov 2026). An app that says it is busy with
// something the user asked for is left alone. The Mac itself may still go
// to sleep when it is idle.
//
// On macOS the work is done in BusyActivity.mm, a few lines of Objective-C;
// on other systems, in BusyActivity.cpp, nothing happens.

#pragma once

namespace maket::platform
{

/** Starts being busy. The value that comes back must be given to
    endBusyActivity once; it is null where the system has no such thing. */
void* beginBusyActivity();

/** Ends it. A null value is allowed and does nothing. */
void endBusyActivity (void* activity);

} // namespace maket::platform
