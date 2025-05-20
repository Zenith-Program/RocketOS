#include "sync\RocketOS_SyncData.h"
#include <cstring> //for memcpy

using namespace RocketOS;
using namespace Sync;

ByteData SyncData_Base::data() const{
    return {p_byteData, p_byteDataSize};
}

error_t SyncData_Base::update(const ByteData data, uint_t source){
    //copy new data
    error_t error = copyByteData(data);
    if(error != error_t::GOOD) return error;
    //call all callbacks except for the source
    source--;
    for(uint_t i=0; i<p_nextFreeChannel; i++)
        if(i != source && p_callbacks[i].updatePeriod_us <= p_callbacks[i].lastUpdate){
            p_callbacks[i].lastUpdate = 0;
            p_callbacks->callback(data);
        }
    return error_t::GOOD;
}

result_t<uint_t> SyncData_Base::addCallback(callback_t callback, uint_t refresh){
    if(p_nextFreeChannel >= p_numCallbacks) return error_t::ERROR;
    p_callbacks[p_nextFreeChannel] = callbackData{refresh, 0, callback};
    p_nextFreeChannel++;
    return p_nextFreeChannel-1;
}

error_t SyncData_Base::popCallback(){
    if(p_nextFreeChannel == 0) return error_t::ERROR; //no callback to pop
    p_nextFreeChannel--;
    return error_t::GOOD;
}

bool SyncData_Base::canBind() const{
    return p_nextFreeChannel < p_numCallbacks;
}

inline error_t SyncData_Base::copyByteData(const ByteData newData){
    if(newData.size != p_byteDataSize) return error_t::ERROR;
    std::memcpy(p_byteData, newData.data, p_byteDataSize);
    return error_t::GOOD;
}