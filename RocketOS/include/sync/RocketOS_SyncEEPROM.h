#pragma once
#include "RocketOS_SyncEEPROMMirror.h"
#include <tuple>
#include <array>
#include <utility>
#include <EEPROM.h> //storing to non-volatile memory

namespace RocketOS{
    namespace Sync{


        template<class... T_types>
        class EEPROM{
            static constexpr uint_t c_size = sizeof...(T_types);

            std::tuple<EEPROMValue<T_types>...> m_values;
            std::array<EEPROMValue_Base*, c_size> m_polyValues;

        public:
            EEPROM() : m_polyValues(buildPolyArray(std::make_index_sequence<c_size>())) {}

            EEPROMValue_Base* getPoly(uint_t i){
                return m_polyValues[i];
            }

            template<uint_t tt_index>
            auto& get(){
                return std::get<tt_index>(m_values);
            }

        private:
            //helpers
            template<std::size_t... tt_indexSeq>
            std::array<EEPROMValue_Base*, c_size> buildPolyArray(std::index_sequence<tt_indexSeq...>){
                return {{static_cast<EEPROMValue_Base*>(&std::get<tt_indexSeq>(m_values))... }};
            }
        };
    }
}