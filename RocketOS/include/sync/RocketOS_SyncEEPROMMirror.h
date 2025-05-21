#pragma once
#include "RocketOS_SyncGeneral.h"
#include "RocketOS_SyncData.h"
#include <array>
#include <inplace_function.h> // for callbacks


#define RocketOS_Sync_EEPROM_CallbackSize 8

namespace RocketOS{
    namespace Sync{

        using EEPROMCallback_t = teensy::inplace_function<void(void), RocketOS_Sync_EEPROM_CallbackSize>;

        class EEPROMUpdateList;

        /*EEPROMValue_Base base class
         *
         *
         * 
        */
        class EEPROMValue_Base{
        protected:
            EEPROMCallback_t p_restoreCallback;
            Bytes p_storedByteData;
            Bytes p_nextByteData;
            uint_t p_baseEEPROMAdress;
            EEPROMUpdateList* p_list;
            bool p_onList;

            /*Constructor
             * Contructor is protected so that only derrived EEPROMValue objects can be made.
             * Byte version of underlying data is passed through the constructor
             * 
            */
            EEPROMValue_Base(uint8_t* storedBytes, uint8_t* nextBytes, uint_t size) : p_restoreCallback(nullptr), p_storedByteData({storedBytes, size}), p_nextByteData({nextBytes, size}) {}
        public:
            error_t restore();
            error_t save();
            uint_t getSize() const;
            void setAdress(uint_t);
            void setList(EEPROMUpdateList*);
        };

        /*EEPROMUpdateList
         * After an non-volatile mirrored value has been updated it adds itself to the list inside the emulated EEPROM object.
         * When the EEPROM object is triggered, it goes through the list and initiates the process of rewriting the non-volatile memory.
         * 
        */
        class EEPROMUpdateList{
            EEPROMValue_Base** const m_positions;
            const uint_t m_size;
            EEPROMValue_Base** m_nextOpen;
        public:
            constexpr EEPROMUpdateList(EEPROMValue_Base** arr, uint_t size) : m_positions(arr), m_size(size), m_nextOpen(arr) {};
            error_t push(EEPROMValue_Base* value);
            result_t<EEPROMValue_Base*> pop();
        };


        /*EEPROMValue derrived class
         *
         *
         * 
        */
        template<class T>
        class EEPROMValue : public EEPROMValue_Base{
        private:
            union{
                T m_storedValue;
                uint8_t m_storedBytes[sizeof(T)];
            };
            union{
                T m_nextValue;
                uint8_t m_nextBytes[sizeof(T)];
            };

        public:
            EEPROMValue() : EEPROMValue_Base(m_storedBytes, m_nextBytes, sizeof(T)) {}

            template<uint_t tt_syncDataCallbackSize>
            error_t bind(SyncData<T, tt_syncDataCallbackSize>& syncData){
                //check that binding is possible
                if(p_restoreCallback != nullptr) return error_t::ERROR;
                if(!syncData.canBind()) return error_t::ERROR;
                //create internal callback
                auto syncData_p = &syncData;
                auto value_p = &m_storedValue;
                p_restoreCallback = [syncData_p, value_p](void){syncData_p->update(*value_p, syncData_p->nextFree());};
                //add callback to syncData
                error_t error = syncData.addCallback([this](const T& data){this->update(data);}, RocketOS_Sync_EEPROM_BindRefreshRate);
                if(error != error_t::GOOD){
                    p_restoreCallback = nullptr;
                    return error_t::ERROR;
                }
                return error_t::GOOD;
            }

            error_t update(const T& data){
                if(p_list == nullptr) return error_t::ERROR;
                if(m_nextValue == data) return error_t::GOOD;
                if(!p_onList){
                    p_list->push(this);
                    p_onList = true;
                } 
                m_nextValue = data;
                return error_t::GOOD;
            }
        };
    }
}