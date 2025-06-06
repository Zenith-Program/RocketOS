#pragma once
#include "RocketOS.h"
#include "Airbrakes.cfg.h"

/*native type aliases
 *
 *
 * 
*/
namespace Airbrakes{
    using float_t = RocketOS::float_t;
    using uint_t = RocketOS::uint_t;
    using int_t = RocketOS::int_t;
}

/*configuration validity checks
 *
 *
 * 
*/

//Airbrakes_CFG_TelemetryBufferSize check
#ifndef Airbrakes_CFG_TelemetryBufferSize
    static_assert(false, "Airbrakes_CFG_TelemetryBufferSize must be defined in the file Airbrakes.cfg.h");
#else
    static_assert(Airbrakes_CFG_TelemetryBufferSize > 0, "Airbrakes_CFG_TelemetryBufferSize must be positive");
#endif

//Airbrakes_CFG_LogBufferSize check
#ifndef Airbrakes_CFG_LogBufferSize
    static_assert(false, "Airbrakes_CFG_LogBufferSize must be defined in the file Airbrakes.cfg.h");
#else
    static_assert(Airbrakes_CFG_LogBufferSize > 0, "Airbrakes_CFG_LogBufferSize must be positive");
#endif

