#pragma once
#include "RocketOS_Shell.cfg.h"
#include "RocketOSGeneral.h"

/*Configuration Validity Checks
 * These are compile time checks that ensure all configuration macros are exist and are valid.
 * If a macro is missing or invalid a compilation error will be thrown.
 *
*/

//RocketOS_Shell_TokenBufferSize check
#ifndef RocketOS_Shell_TokenBufferSize
    static_assert(false, "RocketOS_Shell_TokenBufferSize must be defined in the file RocketOS_Shell.cfg.h");
#else
    static_assert(RocketOS_Shell_TokenBufferSize > 0, "RocketOS_Shell_TokenBufferSize must be positive");
#endif

//RocketOS_Shell_InterpreterCommandNameBufferSize check
#ifndef RocketOS_Shell_InterpreterCommandNameBufferSize
    static_assert(false, "RocketOS_Shell_InterpreterCommandNameBufferSize must be defined in the file RocketOS_Shell.cfg.h");
#else
    static_assert(RocketOS_Shell_InterpreterCommandNameBufferSize > 0, "RocketOS_Shell_InterpreterCommandNameBufferSize must be positive");
#endif

//RocketOS_Shell_CommandCallbackCaptureSize check
#ifndef RocketOS_Shell_CommandCallbackCaptureSize
    static_assert(false, "RocketOS_Shell_CommandCallbackCaptureSize must be defined in the file RocketOS_Shell.cfg.h");
#else
    static_assert(RocketOS_Shell_CommandCallbackCaptureSize > 0, "RocketOS_Shell_CommandCallbackCaptureSize must be positive");
#endif
