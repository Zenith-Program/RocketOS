#pragma once
#include "RocketOS_PersistentGeneral.h"
#include <type_traits>
#include <Arduino.h> //debug

namespace RocketOS{
    namespace Persistent{
        template<class T>
        struct EEPROMSettings{
            static_assert(std::is_trivially_copyable_v<T>, "Type must be trivialy copyable for a byte representation in EEPROM to be possible.");
            T& value;
            const T defaultValue;
            const char* name;
        };


        template<class T>
        class EEPROMValue{
            static_assert(std::is_trivially_copyable_v<T>, "Type must be trivialy copyable for a byte representation in EEPROM to be possible.");
        private:
            T& m_value;
            const T m_defaultValue;
            T m_savedValue;
            const char* const m_name;
            const uint_t m_EEPROMAdress;
        public:
            EEPROMValue(const EEPROMSettings<T>& settings, uint_t adress) : m_value(settings.value), m_defaultValue(settings.defaultValue), m_name(settings.name), m_EEPROMAdress(adress) {}

            error_t save(){
                const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&m_value);
                uint8_t* savedBytes = reinterpret_cast<uint8_t*>(&m_savedValue);
                uint_t size = sizeof(T);
                error_t error = error_t::GOOD;
                for(uint_t i=0; i<size; i++)
                    if(bytes[i] != savedBytes[i]){
                        if(saveByte(bytes[i], m_EEPROMAdress + i) == error_t::GOOD)
                            savedBytes[i] = bytes[i];
                        else    
                            error = error_t::ERROR;
                    }
                return error;
            }

            error_t restore(){
                auto result = readVal(m_EEPROMAdress);
                if(result.error != error_t::GOOD) return result.error;
                m_savedValue = result.data;
                m_value = m_savedValue;
            }

            uint32_t hash() const{
                return strHash(m_name);
            }

            error_t makeDefault(){
                m_savedValue = m_defaultValue;
                m_value = m_defaultValue;
                return saveValue(m_defaultValue, m_EEPROMAdress);
            }
            
        private:
            /*DJB2 cstring hash
             *
             *
             * 
            */
            static uint32_t strHash(const char* str){
                uint32_t hash = 5381;
                int_t c;
                while((c = *str++))
                    hash = ((hash << 5) + hash) + c;
                return hash;
            }

            static error_t saveByte(uint8_t byte, uint_t adress){
                Serial.print("Saving ");
                Serial.print(byte);
                Serial.print(" to location");
                Serial.println(adress);
                return error_t::GOOD;
            }

            static error_t saveValue(const T& value, uint_t adress){
                Serial.print("Saving object to adress ");
                Serial.println(adress);
                return error_t::GOOD;
            }

            static result_t<T> readVal(uint_t adress){
                Serial.print("Reading from adress ");
                Serial.println(adress);
                return error_t::GOOD;
            }
        };
    }
}