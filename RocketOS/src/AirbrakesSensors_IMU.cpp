#include "airbrakes\AirbrakesSensors_IMU.h"
#include <SPI.h>
#include <Arduino.h>

using namespace Airbrakes;
using namespace Sensors;

//Pins
#define P0_PIN 29
#define RESET_PIN 33
#define INTERRUPT_PIN 25
#define CS_PIN 38

//timings
#define RESET_SIGNAL_DURRATION_us 100
#define WAKE_TIMEOUT_us 200000

BNO085_SPI::BNO085_SPI(const char* name, uint_t frequency) : m_name(name), m_SPIFrequency(frequency), m_state(IMUStates::Uninitialized){}
error_t BNO085_SPI::initialize(){
    //initialize wake pin
    pinMode(P0_PIN, OUTPUT);
    digitalWriteFast(P0_PIN, HIGH);
    //initialize reset pin
    pinMode(RESET_PIN, OUTPUT);
    digitalWriteFast(RESET_PIN, HIGH);
    //initialize interrupt pin
    pinMode(INTERRUPT_PIN, INPUT_PULLUP);
    RocketOS::Utilities::inplaceInterrupt<INTERRUPT_PIN>::configureInterrupt([this](){this->serviceInterrupt();}, FALLING);
    //initialize select pin
    pinMode(CS_PIN, OUTPUT);
    digitalWriteFast(CS_PIN, HIGH);
    //initialize SPI1
    SPI1.begin();
    //reset bno
    reset();
    //wake bno
    if(wake() != error_t::GOOD) return error_t::ERROR;
    return error_t::GOOD;
}

bool BNO085_SPI::initialized() const{
    return false; //for now
}


//helper functions
void BNO085_SPI::reset(){
    digitalWriteFast(P0_PIN, HIGH);
    digitalWriteFast(RESET_PIN, LOW);
    delayMicroseconds(RESET_SIGNAL_DURRATION_us);
    digitalWriteFast(RESET_PIN, HIGH);
    m_state = IMUStates::Asleep;
}

error_t BNO085_SPI::wake(){
    if(m_state == IMUStates::Uninitialized) return error_t::ERROR;
    if(m_state != IMUStates::Asleep) return error_t::GOOD;
    digitalWriteFast(P0_PIN, LOW);
    elapsedMicros timeout = 0;
    while(m_state != IMUStates::Operational && timeout < WAKE_TIMEOUT_us);
    //Serial.println(static_cast<uint_t>(timeout));
    if(m_state == IMUStates::Operational) return error_t::GOOD;
    return error_t::ERROR;
}

void BNO085_SPI::serviceInterrupt(){
    if(m_state == IMUStates::Asleep){
        m_state = IMUStates::Operational;
        digitalWriteFast(P0_PIN, HIGH);
    }
}