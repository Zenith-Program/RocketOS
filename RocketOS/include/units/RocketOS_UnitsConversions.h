#pragma once
#include "RocketOS_UnitType.h"

namespace RocketOS{
    namespace Units{
        /*Temperature units conversion implementation--------------------------
         * Supported Units:
         * Degrees Fahrenheit (Fahrenheit_t) - No SI prefix or dimensionality available (non-linear unit)
         * Degrees Celsius (Celcius_t) - No SI prefix or dimensionality available (non-linear unit)
         * Kelvin (Kelvin_t) - SI prefix is available but no dimensionality (linear unit)
        */

        enum class TemperatureUnits : uint_t{
            Celsius, Fahrenheit, Kelvin
        };

        template<class T>
        using Celsius_t = Unit_t<T, TemperatureUnits, TemperatureUnits::Celsius, SIPrefix::none, 1>;

        template<class T>
        using Fahrenheit_t = Unit_t<T, TemperatureUnits, TemperatureUnits::Fahrenheit, SIPrefix::none, 1>;

        template<class T, SIPrefix t_P = SIPrefix::none>
        using Kelvin_t = Unit_t<T, TemperatureUnits, TemperatureUnits::Kelvin, t_P, 1>;


        //celsius conversions----------------------------------

        //celsius -> farenheit
        template<class T>
        struct Conversions<T, TemperatureUnits, TemperatureUnits::Celsius, TemperatureUnits::Fahrenheit, SIPrefix::none, SIPrefix::none, 1,1>{
            static inline T convert(T original){
                return original * static_cast<T>(9) / static_cast<T>(5) + static_cast<T>(32);
            }
        };

        //celsius -> [SIPrefix]kelvin
        template<class T, SIPrefix t_prefix>
        struct Conversions<T, TemperatureUnits, TemperatureUnits::Celsius, TemperatureUnits::Kelvin, SIPrefix::none, t_prefix, 1,1>{
            static inline T convert(T original){
                return Utils::scaleSIPrefix<T>(Utils::inverseSIPrefix(t_prefix), original + static_cast<T>(273.15));
            }
        };


        //farenheit conversions--------------------------------

        //farenheit -> celsius
        template<class T>
        struct Conversions<T, TemperatureUnits, TemperatureUnits::Fahrenheit, TemperatureUnits::Celsius, SIPrefix::none, SIPrefix::none, 1,1>{
            static inline T convert(T original){
                return (original - static_cast<T>(32)) * static_cast<T>(5) / static_cast<T>(9);
            }
        };

        //farenheit -> [SIPrefix]kelvin
        template<class T, SIPrefix t_prefix>
        struct Conversions<T, TemperatureUnits, TemperatureUnits::Fahrenheit, TemperatureUnits::Kelvin, SIPrefix::none, t_prefix, 1,1>{
            static inline T convert(T original){
                return Utils::scaleSIPrefix<T>(Utils::inverseSIPrefix(t_prefix), (original - static_cast<T>(32)) * static_cast<T>(5) / static_cast<T>(9) + static_cast<T>(273.15));
            }
        };


        //kelvin conversions-----------------------------------

        //[SIprefix]kelvin -> celsius
        template<class T, SIPrefix t_prefix>
        struct Conversions<T, TemperatureUnits, TemperatureUnits::Kelvin, TemperatureUnits::Celsius, t_prefix, SIPrefix::none, 1,1>{
            static inline T convert(T original){
                return Utils::scaleSIPrefix<T>(t_prefix, original) - static_cast<T>(273.15);
            }
        };

        //[SIPrefix]kelvin -> farenheit
        template<class T, SIPrefix t_prefix>
        struct Conversions<T, TemperatureUnits, TemperatureUnits::Kelvin, TemperatureUnits::Fahrenheit, t_prefix, SIPrefix::none, 1,1>{
            static inline T convert(T original){
                return (Utils::scaleSIPrefix<T>(t_prefix, original) - static_cast<T>(273.15)) * static_cast<T>(9) / static_cast<T>(5) + static_cast<T>(32);
            }
        };

        //[SIPrefix1]kelvin -> [SIPrefix2]kelvin
        template<class T, SIPrefix t_originalPrefix, SIPrefix t_newPrefix>
        struct Conversions<T, TemperatureUnits, TemperatureUnits::Kelvin, TemperatureUnits::Kelvin, t_originalPrefix, t_newPrefix, 1,1>{
            static inline T convert(T original){
                int_t combinedOrder = Utils::orderSIPrefix(t_originalPrefix) - Utils::orderSIPrefix(t_newPrefix);
                return Utils::scaleByPow10<T>(original, combinedOrder);
            }
        };
        



        /*Spatial units conversion implementation------------------------------
         *
         *
         * 
        */
       enum class SpatialUnits : uint_t{
            Meter, Foot, Gallon
       };

        template<class T_DataType, SIPrefix t_prefix = SIPrefix::none, int_t t_dim = 1>
        using Meter_t = Unit_t<T_DataType, SpatialUnits, SpatialUnits::Meter, t_prefix, t_dim>;

        template<class T_DataType, int_t t_dim = 1>
        using Foot_t = Unit_t<T_DataType, SpatialUnits, SpatialUnits::Foot, SIPrefix::none, t_dim>;

        template<class T_DataType>
        using Gallon_t = Unit_t<T_DataType, SpatialUnits, SpatialUnits::Gallon, SIPrefix::none, 3>;


        //meter conversions------------------------------------

        //[SIPrefix1]meter^[dimension] -> [SIPrefix2]meter^[dimension]
        template<class T, SIPrefix t_originalPrefix, SIPrefix t_newPrefix, int_t t_dim>
        struct Conversions<T, SpatialUnits, SpatialUnits::Meter, SpatialUnits::Meter, t_originalPrefix, t_newPrefix, t_dim,t_dim>{
            static inline T convert(T original){
                int_t combinedOrder = Utils::orderSIPrefix(t_originalPrefix) - Utils::orderSIPrefix(t_newPrefix);
                return Utils::scaleByPow10<T>(original, combinedOrder);
            }
        };

        //[SIPrefix]meter^[dimension] -> foot^[dimension]
        template<class T, SIPrefix t_prefix, int_t t_dim>
        struct Conversions<T, SpatialUnits, SpatialUnits::Meter, SpatialUnits::Foot, t_prefix, SIPrefix::none, t_dim,t_dim>{
            static inline T convert(T original){
                if(t_dim > 0) return Utils::scaleByPow(Utils::scaleByPow(Utils::scaleSIPrefix(t_prefix, original), static_cast<T>(328), t_dim ), static_cast<T>(100), -t_dim);
                else return Utils::scaleByPow(Utils::scaleByPow(Utils::scaleSIPrefix(t_prefix, original), static_cast<T>(100), t_dim), static_cast<T>(328), -t_dim);
            }
        };

        //[SIPrefix]meter^3 -> gallon
        template<class T, SIPrefix t_prefix>
        struct Conversions<T, SpatialUnits, SpatialUnits::Meter, SpatialUnits::Gallon, t_prefix, SIPrefix::none, 3,3>{
            static inline T convert(T original){
                return Utils::scaleSIPrefix(original, t_prefix) * static_cast<T>(264.2);
            }
        };


        //foot conversions-------------------------------------

        //foot^[dimension] -> [SIPrefix]meter^[dimension]
        template<class T, SIPrefix t_prefix, int_t t_dim>
        struct Conversions<T, SpatialUnits, SpatialUnits::Foot, SpatialUnits::Meter, SIPrefix::none, t_prefix, t_dim,t_dim>{
            static inline T convert(T original){
                if(t_dim < 0) return Utils::scaleSIPrefix(Utils::inverseSIPrefix(t_prefix), Utils::scaleByPow(Utils::scaleByPow(original, static_cast<T>(328), t_dim ), static_cast<T>(100), -t_dim));
                else return Utils::scaleSIPrefix(Utils::inverseSIPrefix(t_prefix),Utils::scaleByPow(Utils::scaleByPow(original, static_cast<T>(100), t_dim), static_cast<T>(328), -t_dim));
            }
        };

        //foot^3 -> gallon
        template<class T>
        struct Conversions<T, SpatialUnits, SpatialUnits::Foot, SpatialUnits::Gallon, SIPrefix::none, SIPrefix::none, 3,3>{
            static inline T convert(T original){
                return (original * static_cast<T>(748)) / static_cast<T>(100);
            }
        };



        //gallon conversions-----------------------------------

        //gallon -> ft^3
        template<class T>
        struct Conversions<T, SpatialUnits, SpatialUnits::Gallon, SpatialUnits::Foot, SIPrefix::none, SIPrefix::none, 3,3>{
            static inline T convert(T original){
                return (original * static_cast<T>(100)) / static_cast<T>(748);
            }
        };

        //gallon -> [SIPrefix]meter^3 
        template<class T, SIPrefix t_prefix>
        struct Conversions<T, SpatialUnits, SpatialUnits::Gallon, SpatialUnits::Meter, SIPrefix::none, t_prefix, 3,3>{
            static inline T convert(T original){
                return Utils::scaleSIPrefix(original / static_cast<T>(264.2), Utils::inverseSIPrefix(t_prefix));
            }
        };

    }
}