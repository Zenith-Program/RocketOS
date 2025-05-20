#pragma once
#include "RocketOS_SyncGeneral.h"
#include <inplace_function.h>
#include <array>
#include <Arduino.h> //for elapsedMicros

namespace RocketOS{
    namespace Sync{
        struct ByteData{
            const uint8_t* data;
            uint_t size;
        };

        using callback_t = teensy::inplace_function<void(ByteData), RocketOS_Sync_CallbackCaptureSize>;

        struct callbackData{
            uint_t updatePeriod_us;
            elapsedMicros lastUpdate;
            callback_t callback;
        };

        class SyncData_Base{
            protected:
                uint8_t* const p_byteData;
                const uint_t p_byteDataSize;
                callbackData* const p_callbacks;
                const uint_t p_numCallbacks;
                uint_t p_nextFreeChannel;
            public:
                ByteData data() const;
                error_t update(const ByteData, uint_t = 0);
                result_t<uint_t> addCallback(callback_t, uint_t);
                error_t popCallback();
                bool canBind() const;
            protected:
                constexpr SyncData_Base(uint8_t* rawData, uint_t rawDataSize, callbackData* callbacks, uint_t numCallbacks) : p_byteData(rawData), p_byteDataSize(rawDataSize), p_callbacks(callbacks), p_numCallbacks(numCallbacks), p_nextFreeChannel(0) {}
                inline error_t copyByteData(const ByteData);
            };

        template<class T, uint_t t_callbackListSize=RocketOS_Sync_DefaultCallbackListSize>
        class SyncData : public SyncData_Base{
            private:
                union{
                    T m_data;
                    uint8_t m_rawData[sizeof(T)];
                };
                std::array<callbackData, t_callbackListSize> m_callbacks;
            public:
                constexpr SyncData() : SyncData_Base(m_rawData, sizeof(T), m_callbacks.data(), m_callbacks.size()) {}
                constexpr SyncData(const T& data) : SyncData_Base(ByteData{m_rawData, sizeof(T)}, m_callbacks.data(), m_callbacks.size()), m_data(data) {}
                void operator=(const T& data){
                    m_data = data;
                    for(uint_t i=0; i<p_nextFreeChannel; i++)
                        if(p_callbacks[i].updatePeriod_us <= p_callbacks[i].lastUpdate){
                            p_callbacks[i].lastUpdate = 0;
                            p_callbacks->callback(ByteData{m_rawData, sizeof(T)});
                        }
                }
                operator T() const {return m_data;} 

                template<uint_t tt_otherCallbackSize>
                error_t bind(SyncData<T, tt_otherCallbackSize>& other, uint_t refresh=RocketOS_Sync_DefaultRefreshPeriod_us){
                    //check that binding is possible
                    if(!canBind() || !other.canBind()) return error_t::ERROR;
                    //prepare callback captures
                    auto other_ptr = &other;
                    uint_t otherChannel = other.p_nextFreeChannel;
                    uint_t thisChannel = p_nextFreeChannel;
                    //add callback to other in this object
                    error_t error = addCallback([other_ptr, otherChannel](ByteData data){other_ptr->update(data, otherChannel);}, refresh).error;
                    if(error != error_t::GOOD) return error;
                    //add callback to this in other object
                    error = other.addCallback([this, thisChannel](ByteData data){this->update(data, thisChannel);}, refresh).error;
                    if(error != error_t::GOOD){
                        popCallback();
                        return error;
                    }
                    other.update(ByteData{p_byteData, p_byteDataSize}, otherChannel);
                    return error_t::GOOD;
                }
        };
    }
}