#include "sync\RocketOS_SyncEEPROMMirror.h"
//#include <EEPROM.h> // non-volatile memory acess

//debug
#include <Arduino.h> //Serial debug

using namespace RocketOS;
using namespace Sync;

//implementation for EEPROMValue_Base class------------------------------------
error_t EEPROMValue_Base::update(){
    if(p_list == nullptr) return error_t::ERROR;
    if(!p_onList){
        p_list->push(this);
        p_onList = true;
    } 
    return error_t::GOOD;
}

error_t EEPROMValue_Base::save(){
    Serial.println("Saving");
    p_onList = false;
    return error_t::GOOD;
}

uint_t EEPROMValue_Base::getSize() const{
    return p_storedByteData.size;
}

void EEPROMValue_Base::setAdress(uint_t adress){
    p_baseEEPROMAdress = adress;
}

void EEPROMValue_Base::setList(EEPROMUpdateList* list){
    p_list = list;
}

//implementation for EEPROMUpdateList class------------------------------------------
error_t EEPROMUpdateList::push(EEPROMValue_Base* value){
    if(m_nextOpen >= m_positions + m_size) return error_t::ERROR;
    *m_nextOpen = value;
    m_nextOpen++;
    return error_t::GOOD;
}

result_t<EEPROMValue_Base*> EEPROMUpdateList::pop(){
    if(m_nextOpen <= m_positions) return error_t::ERROR;
    m_nextOpen--;
    return *(m_nextOpen);
}

error_t EEPROMUpdateList::saveAll(){
    error_t error = error_t::GOOD;
    auto result = pop();
    while(result.error == error_t::GOOD){
        if(result.data->save() != error_t::GOOD) error = error_t::ERROR;
        result = pop();
    }
    return error;
}