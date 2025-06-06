#include "RocketOSSerial.h"
#include <Arduino.h> //For Serial

using namespace RocketOS;

SerialInput::SerialInput(uint_t baud) : m_hasData(false),  m_baud(baud) {
    //nothing to do
}

int_t SerialInput::size() const{
    return c_size;
}

char& SerialInput::operator[](int_t index){
    int_t m = (index % size() + size()) % size();
	return m_rxBuffer[m];
}

char& SerialInput::at(int_t index){
    int_t m = (index % size() + size()) % size();
	return m_rxBuffer[m];
}

char SerialInput::operator[](int_t index) const{
    int_t m = (index % size() + size()) % size();
	return m_rxBuffer[m];
}

char SerialInput::at(int_t index) const{
    int_t m = (index % size() + size()) % size();
	return m_rxBuffer[m];
}

void SerialInput::copy(char* newBuffer, int_t start, int_t stop) const{
    for (int_t i = start; i < stop; i++)
		newBuffer[i - start] = at(i);
}

error_t SerialInput::init(){
    Serial.begin(m_baud);
    Serial.setTimeout(10);
    while(!Serial) {
        delay(10);
    }
    return error_t::GOOD;
}

error_t SerialInput::update(){
    if(Serial.available()){
        m_hasData = true;
        Serial.readBytesUntil('\r', m_rxBuffer, c_size);
        if(Serial.available() && Serial.peek() == '\n') Serial.read();
    }
    return error_t::GOOD;
}

bool SerialInput::hasData() const{
    return m_hasData;
}

void SerialInput::clear(){
    m_hasData = false;
}