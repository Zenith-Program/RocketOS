#pragma once
#include "RocketOS_SyncGeneral.h"
#include "RocketOS_SyncData.h"
#include <array>
#include <inplace_function.h> // for callbacks


#define RocketOS_Sync_EEPROM_CallbackSize 8
#define RocketOS_Sync_EEPROM_EmptyBytes {nullptr, 0}

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
            Bytes p_storedByteData;
            ConstBytes p_currentByteData;
            uint_t p_baseEEPROMAdress;
            EEPROMUpdateList* p_list;
            bool p_onList;

            /*Constructor
             * Contructor is protected so that only derrived EEPROMValue objects can be made.
             * Byte version of underlying data is passed through the constructor
             * 
            */
            EEPROMValue_Base(uint8_t* storedBytes, uint_t size) : p_storedByteData({storedBytes, size}), p_currentByteData(RocketOS_Sync_EEPROM_EmptyBytes) {}
        public:
            error_t save();
            uint_t getSize() const;
            void setAdress(uint_t);
            void setList(EEPROMUpdateList*);
            error_t update();
        protected:

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
            error_t saveAll();
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
            SyncData_BASE<T>* m_pairedVariable;
            uint_t m_pairedChannel;

        public:
            EEPROMValue() : EEPROMValue_Base(m_storedBytes, sizeof(T)), m_pairedVariable(nullptr), m_pairedChannel(0) {}

            template<uint_t tt_syncDataCallbackSize>
            error_t bind(SyncData<T, tt_syncDataCallbackSize>& syncData){
                //check that binding is possible
                if(alreadyBound()) return error_t::ERROR;
                if(!syncData.canBind()) return error_t::ERROR;
                //add callback to syncData
                auto result = syncData.addCallback([this](const T&){this->update();}, RocketOS_Sync_EEPROM_BindRefreshRate);
                //check that callback was added sucesfully
                if(result.error != error_t::GOOD) return result.error;
                //update internal binding state
                m_pairedVariable = &syncData;
                m_pairedChannel = result.data;
                p_currentByteData = syncData.getBytes();
                return error_t::GOOD;
            }

            error_t restore(){
                Serial.println("Restoring");
                return error_t::GOOD;
            }

        private:
            //internal helpers
            inline bool alreadyBound(){
                return m_pairedVariable != nullptr;
            }
        };
    }
}