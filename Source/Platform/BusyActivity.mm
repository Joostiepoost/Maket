// BusyActivity.mm — macOS only. See BusyActivity.h.
//
// NSProcessInfo hands out a token for the activity. The token is kept alive
// here until endBusyActivity, by an explicit retain: without it the system
// frees the token right after the call that made it, the activity ends at
// once, and ending it later crashes. (That is what happened with JUCE's
// ScopedLowPowerModeDisabler on 22 Nov 2026: no effect, and a crash five
// seconds after the last message.)

#import <Foundation/Foundation.h>

#include "BusyActivity.h"

namespace maket::platform
{

void* beginBusyActivity()
{
    @autoreleasepool
    {
        // Asked for by the user, with timers as exact as the system has them.
        const NSActivityOptions options = NSActivityUserInitiatedAllowingIdleSystemSleep | NSActivityLatencyCritical;

        id activity = [[NSProcessInfo processInfo] beginActivityWithOptions: options
                                                                     reason: @"Maket is playing"];
        if (activity == nil) return nullptr;

       #if __has_feature(objc_arc)
        return (void*) CFBridgingRetain (activity);
       #else
        return (void*) [activity retain];
       #endif
    }
}

void endBusyActivity (void* token)
{
    if (token == nullptr) return;

    @autoreleasepool
    {
       #if __has_feature(objc_arc)
        id activity = CFBridgingRelease ((CFTypeRef) token);
        [[NSProcessInfo processInfo] endActivity: activity];
       #else
        id activity = (id) token;
        [[NSProcessInfo processInfo] endActivity: activity];
        [activity release];
       #endif
    }
}

} // namespace maket::platform
