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
            std::array<EEPROMValue_Base*, c_size> m_updateListMemory;
            EEPROMUpdateList m_updateList;

        public:
            EEPROM() : m_polyValues(buildPolyArray(std::make_index_sequence<c_size>())), m_updateList(m_updateListMemory.data(), m_updateListMemory.size()) {
                initUpdateListReferences();
            }

            EEPROMValue_Base* getPoly(uint_t i){
                if(i >= c_size) return nullptr;
                return m_polyValues[i];
            }

            template<uint_t tt_index>
            auto& get(){
                return std::get<tt_index>(m_values);
            }

            error_t restoreAll() {
                return forEach_error([](auto& element){return element.restore();}, std::make_index_sequence<c_size>());
            }

            error_t saveChanges(){
                return m_updateList.saveAll();
            }

        private:
            //helpers
            template<std::size_t... tt_indexSeq>
            std::array<EEPROMValue_Base*, c_size> buildPolyArray(std::index_sequence<tt_indexSeq...>){
                return {{static_cast<EEPROMValue_Base*>(&std::get<tt_indexSeq>(m_values))... }};
            }

            template<class TT_Func, std::size_t... tt_indexSeq>
            error_t forEach_error(TT_Func&& func, std::index_sequence<tt_indexSeq...>){
                error_t error = error_t::GOOD;
                ((error = ((error_t::GOOD != func(std::get<tt_indexSeq>(m_values)))? error_t::ERROR : error_t::GOOD)), ...);
                return error;
            }

            template<class TT_Func, std::size_t... tt_indexSeq>
            void forEach(TT_Func&& func, std::index_sequence<tt_indexSeq...>){
                (func(std::get<tt_indexSeq>(m_values)), ...);
            }

            void initUpdateListReferences(){
                forEach([this](auto& element){element.setList(&this->m_updateList);}, std::make_index_sequence<c_size>());
            }
        };
    }
}