#pragma once
#include "RocketOS.cfg.h"
#include <cstdint>

/*Native data types
 *
 *
 * 
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

/*config validity checks
 *
 *
 * 
*/

//RocketOS_CFG_SerialRxBufferSize check
#ifndef RocketOS_CFG_SerialRxBufferSize
    static_assert(false, "RocketOS_CFG_SerialRxBufferSize must be defined in the file RocketOS.cfg.h");
#else
    static_assert(RocketOS_CFG_SerialRxBufferSize > 0, "RocketOS_CFG_SerialRxBufferSize must be positive");
#endif

/*error handling
 *
 *
 * 
*/
namespace RocketOS{
    /*error type
     *
     *
     * 
    */
    class error_t{
    private:
        uint_t m_code;

    public:
        error_t() = default;
        constexpr error_t(uint_t c) : m_code(c){}
        operator bool() const;
        operator uint_t() const;
        bool operator==(const error_t&) const;
        bool operator!=(const error_t&) const;

        static error_t GOOD;
        static error_t ERROR;
    };

    /*result type
     *
     *
     * 
    */
    template<class T>
    struct result_t{
        T data;
        error_t error;

        operator T() const {return data;} 
        operator error_t() const {return error;} 
        inline result_t() : error(error_t::GOOD){}
        inline result_t(T newData) : data(newData), error(error_t::GOOD){}
        inline result_t(error_t newError) : error(newError) {}
        inline result_t(T newData, error_t newError) : data(newData), error(newError) {}
    };

}

/*inplace function type
*/

#ifdef RocketOS_CFG_UsingTeensyTimerTool
#include <TeensyTimerTool.h>
namespace RocketOS{
    template<class T_Signiature, std::size_t t_size>
    using inplaceFunction_t = TeensyTimerTool::stdext::inplace_function<T_Signiature, t_size>;
}
#else
#include <inplace_function.h>
namespace RocketOS{
    template<class T_Signiature, std::size_t t_size>
    using inplaceFunction_t = teensy::inplace_function<T_Signiature, t_size>;
}
#endif

