#pragma once
#include "RocketOS_UnitsGeneral.h"

namespace RocketOS{
    namespace Units{


         /*General Unit_t template description
        */
       template<typename T_value_type, class T_unit_type, T_unit_type T_unit, int_t T_dim, bool T_scalar>
       class UnitBase{
        protected:
            T_value_type m_data;
        public:
            //constructors
            UnitBase(const UnitBase& old) : m_data(old.m_data){}
            UnitBase(UnitBase&& old) : m_data(old.m_data){}
            void operator=(const UnitBase& old){
                m_data = old.m_data;
            }

            //conversion to underlying data
            UnitBase() : m_data(0){}
            UnitBase(const T_value_type& newData) : m_data(newData){}
            void operator=(const T_value_type& newData){ 
                m_data = newData;
            }
            operator T_value_type() const{
                return data;
            }

            //arethmetic with underlying data
            UintBase operator+(T_value_type n) const{
                UnitBase result = *this;
                result.m_data+=n;
                return result;
            }

       };


        /*General Unit_t template description
        */
        template<typename T_value_type, class T_unit_type, T_unit_type T_unit, int_t T_dim, bool T_scalar>
        class Unit_t : public UnitBase<T_value_type, T_unit_type, T_unit, T_dim, T_scalar>;


        /*Unit_t template specialization for scalar units
        */
        template<typename T_value_type, class T_unit_type, T_unit_type T_unit, int_t T_dim>
        class Unit_t<T_value_type, T_unit_type, T_unit, T_dim, true> : public UnitBase<T_value_type, T_unit_type, T_unit, T_dim, true>{
        public:
            //functionality with value type 

        };

        /*Unit_t template specialization for non-scalar units
        */
        template<typename T_value_type, class T_unit_type, T_unit_type T_unit>
        class Unit_t<T_value_type, T_unit_type, T_unit, 1, false> : public UnitBase<T_value_type, T_unit_type, T_unit, 1, false>{
        public:
           //functionality with value type 

       };
    }
}