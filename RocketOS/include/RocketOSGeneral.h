#pragma once
#include "RocketOS.cfg.h"
#include <cstdint>

/*Native data types
*/

#ifndef RocketOS_CFG_NativeWordWidth
static_assert(false, "RocketOS_CFG_NativeWordWidth must be defined in the file RocketOS.cfg.h");
#else
#if RocketOS_CFG_NativeWordWidth == 8
namespace RocketOS{
    using uint_t = uint8_t;
    using int_t = int8_t;
    using float_t = float;
}
#elif RocketOS_CFG_NativeWordWidth == 16
namespace RocketOS{
    using uint_t = uint16_t;
    using int_t = int16_t;
    using float_t = float;
}
#elif RocketOS_CFG_NativeWordWidth == 32
static_assert(sizeof(float) == 4);
namespace RocketOS{
    using uint_t = uint32_t;
    using int_t = int32_t;
    using float_t = float;
}
#elif RocketOS_CFG_NativeWordWidth == 64
static_assert(sizeof(double) == 8);
namespace RocketOS{
    using uint_t = uint64_t;
    using int_t = int64_t;
    using float_t = double;
}
#else
static_assert(false, "Only 8, 16, 32 & 64 bit architectures are supported");
#endif
#endif

/*error type
 *
 *
 * 
*/
namespace RocketOS{
    class error_t{
    private:
        uint_t m_code;

    public:
        error_t(uint_t);
        operator bool() const;
        operator uint_t() const;
        bool operator==(const error_t&) const;
        bool operator!=(const error_t&) const;

        static error_t GOOD;
        static error_t ERROR;
    };
}