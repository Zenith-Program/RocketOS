#pragma once
#include "RocketOS_SyncGeneral.h"
#include <inplace_function.h>
#include <array>
#include <Arduino.h> //for elapsedMicros

/* RocketOS Synchronization System
 * A general rocket system will have a set of state variables associated with it's intended function (ex. measured altitude, current mode of operation, measured actuator position, etc.)
 * Beyond the program's logic of how these variables define the behavior of the system, many of these variables will need to be exported in real time for things like telemetry, simulation, cross-device syncronization, reset recovery, etc.
 * The RocketOS Synchronization System is designed to handle the process of synchronizing variables to external sources in real time, allowing the programer to focus on their logic as it relates to the core focus of the application.
 * The system is centered around the SyncData type, which is a template type that allows any object to be used for synchronization. 
 * Variables declared as a SyncData type can be bound to external systems (ex. telemetry, simulation, etc.) or to eachother so that when a given variable is updated, other systems it is bound to will also be updated.
 * 
 * Example using the airbrakes system of how this can be used:
 * 
 *          -----------------
 *          | Telemetry Log |  <------+--------+-------------|             
 *          -----------------         |        |             |
 *                            |-------)--------)---[Actuator Position]
 *                            v       |        |
 *      ------------------------      |        |-------------|
 *      | MATLAB HIL Interface |  ----)----------->[Measured Altitude]
 *      ------------------------      |             
 *                                    |
 *                                    |--------------[Flight State]
 *      ------------------------                           ^
 *      | Non-Volatile storage |  <------------------------|
 *      ------------------------
 * 
 * The flight state is loged by telemetry and saved in nonvolatile memory to recover from a reset. If a reset happens the variable is restored from the non-volatile memory.
 * The measured altitude logged to telemetry but also is updated to matlabs produced simulation value durring HIL.
 * The Actuator position logged to telemetry and is passed to matlab's simulation durring HIL.
 * The external sources (MALTAB, telemetry, non-volatile memory, etc.) may need to both be updated by changes to the variables but also update the variables. This is why a synchronization system is used. 
 * 
 * The operation of the SyncData class is actually relatively simple:
 * SyncData objects, in addition to the data they store,  have a list of callback functions that are executed whenever the underlying value is updated. 
 * When a variable is bound to another object, a callback function for the object it was bound to is added to it's callback list allowing it to update that object.
 * 
*/

namespace RocketOS{
    namespace Sync{
        /*callback_t
         * sype alias for update callback functions used by SyncData
         * callbacks accept a single parameter of the type underlying data type which is the new value being sent to its dependents
         * 
        */
        template<class T>
        using callback_t = teensy::inplace_function<void(const T&), RocketOS_Sync_CallbackCaptureSize>;

        /*callbackData
         * struct which stores information for each update callback.
         * arduino's elapsedmicros is used to limit updates from happening too often.
         * this protects against recursive updating and from frequently updated variables.
        */
       template <class T>
        struct callbackData{
            uint_t updatePeriod_us;
            callback_t<T> callback;
        };

        /*SyncData class
         * 
         *
         * 
        */
        template<class T>
        class SyncData_BASE{
        protected:
            /*data members
             * m_data - value of the underlying variable
             * m_bytes - bytes of the underlying variable
             * m_callbacks - list of callback functions
             * m_nextFreeChannel - stores how many callbacks have been bound and where to place the next one
            */
            union{
                T m_data;
                uint8_t m_bytes[sizeof(T)];
            };
            callbackData<T>* const m_callbacks;
            const uint_t m_callbacksSize;
            uint_t m_nextFreeChannel;
            elapsedMicros m_lastUpdate;
        public:

            /*construction & base type conversion
             *
             *
             * 
            */
            constexpr SyncData_BASE(callbackData<T>* callbacks, uint_t size) : m_callbacks(callbacks), m_callbacksSize(size), m_nextFreeChannel(0), m_lastUpdate(0) {}
            constexpr SyncData_BASE(callbackData<T>* callbacks, uint_t size, const T& data) : m_data(data), m_callbacks(callbacks), m_callbacksSize(size), m_nextFreeChannel(0), m_lastUpdate(0) {}

            /*update function
             *
             *
             * 
            */
           void update(const T& data){
                if(m_data == data) return;
                //copy new data
                m_data = data;
                //call all callbacks except for the source
                for(uint_t i=0; i<m_nextFreeChannel; i++)
                    if(m_callbacks[i].updatePeriod_us <= m_lastUpdate)
                        m_callbacks[i].callback(m_data);
                m_lastUpdate = 0;
            }

            void update(const T& data, uint_t sourceChannel){
                if(m_data == data) return;
                //copy new data
                m_data = data;
                //call all callbacks except for the source
                for(uint_t i=0; i<m_nextFreeChannel; i++)
                    if(i != sourceChannel && m_callbacks[i].updatePeriod_us <= m_lastUpdate)
                        m_callbacks[i].callback(m_data);
                m_lastUpdate = 0;
            }

            /*addCallback function
             *
             *
             * 
            */
            result_t<uint_t> addCallback(callback_t<T> callback, uint_t refresh){
                if(m_nextFreeChannel >= m_callbacksSize) return error_t::ERROR;
                m_callbacks[m_nextFreeChannel] = callbackData<T>{refresh, callback};
                m_nextFreeChannel++;
                return m_nextFreeChannel-1;
            }

            /*popBind function
             *
             *
             * 
            */
            error_t popCallback(){
                if(m_nextFreeChannel == 0) return error_t::ERROR; //no callback to pop
                m_nextFreeChannel--;
                return error_t::GOOD;
            }

            /*canBind function
             *
             *
             * 
            */
            bool canBind() const{
                return m_nextFreeChannel < m_callbacksSize;
            }

            /*nextFree function
             *
             *
             * 
            */
            result_t<uint_t> nextFree() const{
                if(m_nextFreeChannel >= m_callbacksSize) return {m_nextFreeChannel, error_t::ERROR};
                return m_nextFreeChannel;
            }

            /*getReference function
             *
             *
             * 
            */
            ConstBytes getBytes() const{
                return {m_bytes, sizeof(T)};
            }
        };

        template<class T, uint_t tt_callbackSize=RocketOS_Sync_DefaultCallbackListSize>
        class SyncData : public SyncData_BASE<T>{
        private:
            std::array<callbackData<T>, tt_callbackSize> m_callbackArr;
        public:
            constexpr SyncData() : SyncData_BASE<T>(m_callbackArr.data(), m_callbackArr.size()) {}
            constexpr SyncData(const T& data) : SyncData_BASE<T>(m_callbackArr.data(), m_callbackArr.size(), data) {}

            /*Underlying type conversion operaotrs
             *
             *
             * 
            */

            void operator=(const T& data){
                this->update(data);
            }
            operator T() const {return this->m_data;} 

            /*variable - variable bind
             *
             *
             * 
            */
            template<uint_t tt_otherCallbackSize>
            error_t bind(SyncData<T, tt_otherCallbackSize>& other, uint_t refresh=RocketOS_Sync_DefaultRefreshPeriod_us){
                //check that binding is possible
                if(!this->canBind() || !other.canBind()) return error_t::ERROR;
                //prepare callback captures
                auto other_ptr = &other;
                uint_t otherChannel = other.nextFree();
                uint_t thisChannel = this->m_nextFreeChannel;
                //add callback to other in this object
                error_t error = this->addCallback([other_ptr, otherChannel](const T& data){other_ptr->update(data, otherChannel);}, refresh);
                if(error != error_t::GOOD) return error;
                //add callback to this in other object
                error = other.addCallback([this, thisChannel](const T& data){this->update(data, thisChannel);}, refresh);
                if(error != error_t::GOOD){
                    this->popCallback();
                    return error;
                }
                other.update(this->m_data, otherChannel);
                return error_t::GOOD;
            }
        };
    }
}