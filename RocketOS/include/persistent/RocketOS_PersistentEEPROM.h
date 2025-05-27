#pragma once
#include "RocketOS_PersistentGeneral.h"
#include "RocketOS_PersistentEEPROMValue.h"
#include <tuple>
#include <array>

namespace RocketOS{
    namespace Persistent{
        template<class... T_types>
        class EEPROM{
        private:
            static constexpr uint_t c_baseAdress = 0x00000000; //some arbitrary value for now
            static constexpr std::size_t c_size = sizeof...(T_types);
            std::tuple<EEPROMValue<T_types>...> m_values;
        public:
            EEPROM(EEPROMSettings<T_types>... settings) : m_values(constructValues(std::make_index_sequence<c_size>(), settings...)) {}

            uint32_t hash() const{
                return mergeHashes(std::make_index_sequence<c_size>());
            }

            result_t<bool> restore(){
                if(getSavedHash() == hash()){
                    error_t error = restoreAll(std::make_index_sequence(c_size));
                    return {false, error};
                }
                error_t error = makeAllDefault(std::make_index_sequence(c_size));
                return {true, error};
            }

            error_t save(){
                return error_t::GOOD;
            }

        private:
            //restoration helper functions-------------------------------------
            template<std::size_t... tt_indexSeq>
            error_t restoreAll(std::index_sequence<tt_indexSeq...>){
                error_t error = error_t::GOOD;
                ((error = (std::get<tt_indexSeq>(m_values).restore() == error_t::GOOD)? error_t::GOOD : error_t::ERROR), ...);
                return error;
            }

            template<std::size_t... tt_indexSeq>
            error_t makeAllDefault(std::index_sequence<tt_indexSeq...>){
                error_t error = error_t::GOOD;
                ((error = (std::get<tt_indexSeq>(m_values).makeDefault() == error_t::GOOD)? error_t::GOOD : error_t::ERROR), ...);
                return error;
            }

            //Hash computation helper functions--------------------------------
            template<std::size_t... tt_indexSeq>
            uint32_t mergeHashes(std::index_sequence<tt_indexSeq...>) const{
                uint32_t hash = static_cast<uint32_t>(c_baseAdress);
                ((hash = hash ^ std::get<tt_indexSeq>(m_values).hash()), ...);
                return hash;
            }

            result_t<uint32_t> getSavedHash(){
                constexpr uint_t savedHashAdress = alignToFour(c_baseAdress);
                Serial.println("Reading saved hash");
                return 0; //forNow
            }

            error_t saveHash(uint32_t){
                constexpr uint_t savedHashAdress = alignToFour(c_baseAdress);
                Serial.println("Saving hash");
                return error_t::GOOD; //for now
            }

            //EEPROM adress assignment helper functions------------------------
            template<std::size_t... tt_indexSeq>
            static constexpr auto constructValues(std::index_sequence<tt_indexSeq...>, const EEPROMSettings<T_types>&... settings){
                constexpr auto adresses = computeAdresses(alignToFour(alignToFour(c_baseAdress) + sizeof(uint32_t)));
                return std::make_tuple(EEPROMValue<T_types>(settings, adresses[tt_indexSeq])...);
            }

            static constexpr auto computeAdresses(const uint_t base){
                std::array<uint_t, sizeof... (T_types)> result{};
                uint_t current = base;
                uint_t i=0;
                ((result[i++] = exchange(current, alignToFour(current + sizeof(T_types)))), ...);
                return result;
            }

            static constexpr uint_t alignToFour(uint_t adress){
                return (adress + 3) - (adress + 3)%4;
            }
            
            static constexpr uint_t exchange(uint_t& oldVal, uint_t newVal){
                uint_t temp = oldVal;
                oldVal = newVal;
                return temp;
            } 
        };
    }
}