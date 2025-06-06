#pragma once
#include "RocketOS_PersistentGeneral.h"
#include "RocketOS_PersistentEEPROMValue.h"
#include <tuple>
#include <array>
#include <EEPROM.h> //EEPROM hadrware abstraction

/*EEPROM backup system (EEPROMBackup class)
 * Program variables can be saved to non-volatile EEPROM memory through the use of the EEPROMBackup class.
 * The EEPROMBackup class is a variadic template class, meaning that it can be instantiated for an arbitrary list of variabes, each with a potentially different type.
 * The only restriction is that the types should be tirvially copyable, meaning that copying their byte representation into memory is all that is nessesary to copy the infromation they contain.
 * The list of variables that are saved by the EEPROMBackup class is determined at compile time by passing the variables as references when constructing the EEPROMBackup object.
 * In addition to it's reference, each variable requires a default value and an identification string to be associated with it. These 3 values are combined into a single initilization struct called EEPROMSettings.
 * A series of EEPROMSettings objects is passed to the EEPROMBackup constructor.
 * Ex. How to use EEPROMBackup (example cpp code)
 
 //variables we want to save
 uint32_t programState;
 float altitude; 
 std::array<char, 20> telemetryFileName;
 //creating eeprom backup object
 EEPROMBackup<uint32_t, float, std::array<char, 20>> eeprom{
    EEPROMSettings<uint32_t>{programState, 0, "program state"},
    EEPROMSettings<float>{altitude, 0, "altitude"},
    EEPROMSettings<std::array<char, 20>>{telemetryFileName, "file.csv", "telemetry file"}
 };

 * When the list or layout of the variables saved into EEPROM is changed, the previous storage format will no longer be valid. When this occurs, the default value is used when initializing the new memory layout.
 * The process of updating the memory layout when the list of variables is modified is handled automatically be the EEPROM backup system.
 * Identification strings can be used for debuging, but their primary purpose is to check if the EEPROM memory layout has been changed since the last time the program ran.
 * Each time that the program starts, the hash of the ID strings is compared to a previously stored hash value. If they differ, the memory layout is re-initialized.
 * Assuming no hash collisions (very unlikely), any change to the ID strings (addition, subtraction, change, reordering), will tell the system to re-initialize the non-volatile memory.
*/

namespace RocketOS{
    namespace Persistent{
        template<class... T_types>
        class EEPROMBackup{
        private:
            static constexpr uint_t c_baseAdress = RocketOS_Persistent_EEPROMBaseAdress;
            static constexpr std::size_t c_size = sizeof...(T_types);
            std::tuple<EEPROMValue<T_types>...> m_values;
            
        public:
            EEPROMBackup(EEPROMSettings<T_types>... settings) : m_values(constructValues(std::make_index_sequence<c_size>(), settings...)) {}

            uint32_t hash() const{
                return mergeHashes(std::make_index_sequence<c_size>());
            }

            result_t<bool> restore(){
                if(getSavedHash() == hash()){
                    error_t error = restoreAll(std::make_index_sequence<c_size>());
                    return {false, error};
                }
                error_t error = makeAllDefault(std::make_index_sequence<c_size>());
                error = (saveHash(hash()) == error_t::GOOD && error == error_t::GOOD)? error_t::GOOD : error_t::ERROR;
                return {true, error};
            }

            error_t restoreDefaults(){
                error_t error = makeAllDefault(std::make_index_sequence<c_size>());
                error = (saveHash(hash()) == error_t::GOOD && error == error_t::GOOD)? error_t::GOOD : error_t::ERROR;
                return error;
            }

            error_t save(){
                return saveAll(std::make_index_sequence<c_size>());
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
                ((error = (std::get<tt_indexSeq>(m_values).makeDefault() == error_t::GOOD && error == error_t::GOOD)? error_t::GOOD : error_t::ERROR), ...);
                return error;
            }

            template<std::size_t... tt_indexSeq>
            error_t saveAll(std::index_sequence<tt_indexSeq...>){
                error_t error = error_t::GOOD;
                ((error = (std::get<tt_indexSeq>(m_values).save() == error_t::GOOD&& error == error_t::GOOD)? error_t::GOOD : error_t::ERROR), ...);
                return error;
            }

            //Hash computation helper functions--------------------------------
            template<std::size_t... tt_indexSeq>
            uint32_t mergeHashes(std::index_sequence<tt_indexSeq...>) const{
                uint32_t hash = static_cast<uint32_t>(c_baseAdress);
                ((hash = mergeSubHashes(hash, std::get<tt_indexSeq>(m_values).hash())), ...);
                return hash;
            }

            result_t<uint32_t> getSavedHash(){
                constexpr uint_t savedHashAdress = alignToFour(c_baseAdress);
                uint32_t hashValue;
                EEPROM.get(savedHashAdress, hashValue);
                return hashValue;
            }

            error_t saveHash(uint32_t newHash){
                constexpr uint_t savedHashAdress = alignToFour(c_baseAdress);
                EEPROM.put(savedHashAdress, newHash);
                return error_t::GOOD;
            }

            static constexpr uint32_t mergeSubHashes(uint32_t a, uint32_t b){
                return circularShift(a ^ b, 13);
            }

            static constexpr uint32_t circularShift(uint32_t value, int_t n){
                int_t shiftAmount = (n % 32 + 32)%32;
                uint32_t topBits = value << shiftAmount;
                uint32_t bottomBits = value >> (32 - shiftAmount);
                return topBits | bottomBits;
            }

            //EEPROM adress assignment helper functions------------------------
            template<std::size_t... tt_indexSeq>
            static constexpr auto constructValues(std::index_sequence<tt_indexSeq...>, const EEPROMSettings<T_types>&... settings){
                constexpr auto adresses = computeAdresses(alignToFour(alignToFour(c_baseAdress) + sizeof(uint32_t)));
                static_assert(adresses[c_size] <= RocketOS_Persistent_EEPROMMaxSize, "There is not enough space allocated to EEPROM to span the current set of variables. Decreace the number of variables or change the EEPROM config settings for both the board and RocketOS_Persistent_EEPROMMaxSize");
                return std::make_tuple(EEPROMValue<T_types>(settings, adresses[tt_indexSeq])...);
            }

            static constexpr auto computeAdresses(const uint_t base){
                std::array<uint_t, c_size+1> result{};
                uint_t current = base;
                uint_t i=0;
                ((result[i++] = exchange(current, alignToFour(current + sizeof(T_types)))), ...);
                result[c_size] = current;
                return result;
            }

            static constexpr uint_t alignToFour(uint_t adress){
                return (adress + RocketOS_Persistent_EEPROMAlignment-1) - (adress + RocketOS_Persistent_EEPROMAlignment-1)%RocketOS_Persistent_EEPROMAlignment;
            }
            
            static constexpr uint_t exchange(uint_t& oldVal, uint_t newVal){
                uint_t temp = oldVal;
                oldVal = newVal;
                return temp;
            } 

        };
    }
}