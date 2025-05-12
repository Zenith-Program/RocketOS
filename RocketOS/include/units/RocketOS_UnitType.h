#pragma once
#include "..\..\include\RocketOSGeneral.h"

/*Units template description
 * The RocketOS unit system works by assigning a different type to values with different units. 
 * The types assosiated with units of the same kind (ex. feet, meters & inches or C, F & kelvin) have implicit type conversions between one antother.
 * The template type Unit_t is used for all of the unit types. It's 5 template parameters distinguish different unit types from eachother.
 * Unit_t template parameters (template parameter types excluded):
 * 
 *                  Unit_t<T_DataType, T_UnitType, t_unit, t_prefix, t_dim>      
 *                
 * T_DataType - Represents the underlying data type (ex. uint_t, float_t, int_t). This allows for a different unit type to be created for each arethmentic type
 * T_UnitType - Tells which kind of unit the type is (ex. spacial, temperature, force). T_UnitType is designed to be an enumeration that contains all of the units of that kind
 * t_unit - Tells the exact unit the type represents. This must be a member of T_UnitType (ex. for T_UnitType = SpacialUnits, t_unit could be Foot, Meter, etc)
 * t_prefix - Indicates what SI prefix (if any) the unit has. t_prefix must be an SIPrefixes enumeration type
 * t_dim - Indicates the dimension of the unit. This allows units like m^3 or kg^-1 to be given a type
 * (t_ is a naming convention to signify template parameters. T_ is used for type params (ex. int) and t_ for value params (ex. 5))
 * 
 * Type aliases are provided for units to eliminate unused parameters and make usage easier (ex. Foot_t or Kelvin_t). 
 * Alisaes are defined along with conversion logic in the file RocketOS_UnitsConversions.h
 * Look at the units subsystem readme in the folder subsyatem_units for a more detailed explanation and examples
*/



namespace RocketOS{
    namespace Units{
        enum class SIPrefix : uint_t{
            femto, pico, nano, micro, milli, centi, deci, none, deca, hecto, kilo, mega, giga, tera, peta
        };

        class Utils{
        public:
            template<class T_DataType>
            static inline constexpr T_DataType scaleSIPrefix(SIPrefix p, T_DataType value=1){
                switch(p){
                    case SIPrefix::femto: return value / static_cast<T_DataType>(1000000000000000);
                    case SIPrefix::pico: return value / static_cast<T_DataType>(1000000000000);
                    case SIPrefix::nano: return value / static_cast<T_DataType>(1000000000);
                    case SIPrefix::micro: return value / static_cast<T_DataType>(1000000);
                    case SIPrefix::milli: return value / static_cast<T_DataType>(1000);
                    case SIPrefix::centi: return value / static_cast<T_DataType>(100);
                    case SIPrefix::deci: return value / static_cast<T_DataType>(10);
                    case SIPrefix::deca: return value * static_cast<T_DataType>(10);
                    case SIPrefix::hecto: return value * static_cast<T_DataType>(100);
                    case SIPrefix::kilo: return value * static_cast<T_DataType>(1000);
                    case SIPrefix::mega: return value * static_cast<T_DataType>(1000000);
                    case SIPrefix::giga: return value * static_cast<T_DataType>(1000000000);
                    case SIPrefix::tera: return value * static_cast<T_DataType>(1000000000000);
                    case SIPrefix::peta: return value * static_cast<T_DataType>(1000000000000000);
                    default: return value;
                }
            }

            static inline constexpr SIPrefix inverseSIPrefix(SIPrefix p){
                switch(p){
                    case SIPrefix::femto: return SIPrefix::peta;
                    case SIPrefix::pico: return SIPrefix::tera;
                    case SIPrefix::nano: return SIPrefix::giga;
                    case SIPrefix::micro: return SIPrefix::mega;
                    case SIPrefix::milli: return SIPrefix::kilo;
                    case SIPrefix::centi: return SIPrefix::hecto;
                    case SIPrefix::deci: return SIPrefix::deca;
                    case SIPrefix::deca: return SIPrefix::deci;
                    case SIPrefix::hecto: return SIPrefix::centi;
                    case SIPrefix::kilo: return SIPrefix::milli;
                    case SIPrefix::mega: return SIPrefix::micro;
                    case SIPrefix::giga: return SIPrefix::nano;
                    case SIPrefix::tera: return SIPrefix::pico;
                    case SIPrefix::peta: return SIPrefix::femto;
                    default: return SIPrefix::none;
                }
            }

            static inline constexpr int_t orderSIPrefix(SIPrefix p){
                switch(p){
                    case SIPrefix::femto: return -15;
                    case SIPrefix::pico: return -12;
                    case SIPrefix::nano: return -9;
                    case SIPrefix::micro: return -6;
                    case SIPrefix::milli: return -3;
                    case SIPrefix::centi: return -2;
                    case SIPrefix::deci: return -1;
                    case SIPrefix::deca: return 1;
                    case SIPrefix::hecto: return 2;
                    case SIPrefix::kilo: return 3;
                    case SIPrefix::mega: return 6;
                    case SIPrefix::giga: return 9;
                    case SIPrefix::tera: return 12;
                    case SIPrefix::peta: return 15;
                    default: return 0;
                }
            }

            template<class T>
            static inline constexpr T scaleByPow10(T value, int_t power){
                T powerVal = 1;
                for(int_t i=0; i<abs(power); i++)
                    powerVal *= 10;
                if(power < 0) return value/powerVal;
                return value * powerVal;
            }

            template<class T>
            static inline constexpr T scaleByPow(T value, T base, int_t power){
                T powerVal = 1;
                for(int_t i=0; i<abs(power); i++)
                    powerVal *= base;
                if(power < 0) return value/powerVal;
                return value * powerVal;
            }
        };

        template<class T_DataType, class T_UnitType, T_UnitType t_oldUnit, T_UnitType t_newUnit, SIPrefix t_oldPrefix, SIPrefix t_newPrefix, int_t t_oldDim, int_t t_newDim>
        struct Conversions{
            static inline T_DataType convert(T_DataType){
                static_assert(sizeof(T_DataType) == 0, "Conversion between these two units is not possible. Check that the dimensions match (ex. ft^2 -> km^2) and that both the underlying type and the unit type are not being converted at the same time. Only unit type (ex. ft -> m or cm^3 -> gallon) or underlying type (ex. int -> float) can be implicitly converted not both");
            }
        };

        template<class T_DataType, class T_UnitType, T_UnitType t_unit, SIPrefix t_prefix, int_t t_dim>
        class Unit_t{
        private:
            T_DataType m_data;
        public:
            /*Copy & Constructors
             * default constructs to zero
             * copy, move, and assignemt are supported
            */
            Unit_t() : m_data(0){}
            Unit_t(const Unit_t& old) : m_data(old.m_data){}
            Unit_t(Unit_t&& old) : m_data(old.m_data){}
            void operator=(const Unit_t& old){
               m_data = old.m_data;
            }

            /*Conversion to underlying type
             * Constructor and assignment with the underlying type allows constant types to be used (ex. Unit_t<...> a = 5)
             * Underlying data type conversions allow for conversion to non-unit type. This conversion is explicit, so float a = Unit_t<float, ...>(5) will not compile.
             * Use float a = static_cast<float>(Uint_t<float, ...>(5)) or float a = Uint_t<float, ...>.value() instead
            */
            Unit_t(const T_DataType& data) : m_data(data){}
            void operator=(const T_DataType& data){
                m_data = data;
            }
            explicit operator T_DataType() const{
                return m_data;
            }
            T_DataType value() const{
                return m_data;
            }
            T_DataType& data(){
                return m_data;
            }
            const T_DataType& data() const{
                return m_data;
            }
            
            /*Addition operations
             * +, -, ++, --, +=, -= operators are implemented using the underlying data type's operators
            */
            Unit_t operator+(const Unit_t& op2) const{
                return m_data + op2.data();
            }

            Unit_t operator-(const Unit_t& op2) const{
                return m_data - op2.data();
            }

            Unit_t operator+() const{
                return *this;
            }

            Unit_t operator-() const{
                return -m_data;
            }

            Unit_t operator++() {
                m_data++;
            }

            Unit_t operator++(int) {
                m_data++;
            }

            Unit_t operator--() {
                m_data--;
            }

            Unit_t operator--(int) {
                m_data--;
            }

            void operator+=(const Unit_t& op){
                m_data += op.data();
            }

            void operator-=(const Unit_t& op){
                m_data -= op.data();
            }

            /*Comparison operations
             * comparisons can be made in the same way as with the underlying type
             * ==, !=, <=, >=, <, > operators are implemented using the underlying data type's operators
             * 
            */
            bool operator==(const Unit_t& op) const{
                return m_data == op.data();
            }

            bool operator!=(const Unit_t& op) const{
                return m_data != op.data();
            }

            bool operator>(const Unit_t& op) const{
                return m_data > op.data();
            }

            bool operator<(const Unit_t& op) const{
                return m_data < op.data();
            }

            bool operator>=(const Unit_t& op) const{
                return m_data >= op.data();
            }

            bool operator<=(const Unit_t& op) const{
                return m_data <= op.data();
            }

            /*Scalar operations
             * scalar multiplication is supported for general units
             * *, /, *=, /= operators are implemented using the underlying data type's implementation of them
             * 
            */
            Unit_t operator*(T_DataType s) const{
                return m_data * s;
            }
            Unit_t operator/(T_DataType s) const{
                return m_data / s;
            }
            void operator*=(T_DataType s){
                m_data *= s;
            }
            void operator/=(T_DataType s){
                m_data /= s;
            }
            friend Unit_t operator*(const T_DataType& s, const Unit_t& op){
                return s * op.data();
            }
            friend Unit_t operator/(const T_DataType& s, const Unit_t& op){
                return s / op.data();
            }


            /*Conversions
             *
             *
             * 
            */

            template<T_UnitType tt_newUnit, SIPrefix tt_newPrefix, int_t tt_newDim>
            operator Unit_t<T_DataType, T_UnitType, tt_newUnit, tt_newPrefix, tt_newDim>() const{
                return Conversions<T_DataType, T_UnitType, t_unit, tt_newUnit, t_prefix, tt_newPrefix, t_dim, tt_newDim>::convert(m_data);
            }

            template<class TT_NewDataType>
            operator Unit_t<TT_NewDataType, T_UnitType, t_unit, t_prefix, t_dim>() const{
                return static_cast<TT_NewDataType>(m_data);
            }

            /*multiplication
             *
             *
             * 
            */
            template<T_UnitType tt_otherUnit, SIPrefix tt_otherPrefix, int_t tt_otherDim>
            auto operator*(const Unit_t<T_DataType, T_UnitType, tt_otherUnit, tt_otherPrefix, tt_otherDim>& other){
                return Unit_t<T_DataType, T_UnitType, t_unit, t_prefix, t_dim + tt_otherDim>(m_data * static_cast<Unit_t<T_DataType, T_UnitType, t_unit, t_prefix, tt_otherDim>>(other).data());
            }

            template<T_UnitType tt_otherUnit, SIPrefix tt_otherPrefix, int_t tt_otherDim>
            auto operator/(const Unit_t<T_DataType, T_UnitType, tt_otherUnit, tt_otherPrefix, tt_otherDim>& other){
                return Unit_t<T_DataType, T_UnitType, t_unit, t_prefix, t_dim - tt_otherDim>(m_data / static_cast<Unit_t<T_DataType, T_UnitType, t_unit, t_prefix, tt_otherDim>>(other).data());
            }

        };

         


    }
}