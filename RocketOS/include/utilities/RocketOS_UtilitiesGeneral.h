#pragma once
#include "RocketOS_Utilities.cfg.h"
#include "RocketOSGeneral.h"

/*Configuration Validity Checks
 * These are compile time checks that ensure all configuration macros are exist and are valid.
 * If a macro is missing or invalid a compilation error will be thrown.
 *
*/

//RocketOS_Utilities_InterruptCallbackCaptureSize check
#ifndef RocketOS_Utilities_InterruptCallbackCaptureSize
    static_assert(false, "RocketOS_Utilities_InterruptCallbackCaptureSize must be defined in the file RocketOS_Utilities.cfg.h");
#else
    static_assert(RocketOS_Utilities_InterruptCallbackCaptureSize > 0, "RocketOS_Utilities_InterruptCallbackCaptureSize must be positive");
#endif