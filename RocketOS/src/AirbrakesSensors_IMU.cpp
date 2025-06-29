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

//protocol
#define SHTP_CONTINUATION_BIT 0x80
#define SHTP_IGNORE_CHUNCK_SIZE 256
#define SHTP_HEADER_SIZE 4
#define SHTP_LENGTH_MSB 1
#define SHTP_LENGTH_LSB 1
#define SHTP_CHANNEL 2
#define SHTP_SEQUENCE 3


BNO085_SPI::BNO085_SPI(const char* name, uint_t frequency) : m_name(name), m_SPIFrequency(frequency), m_state(IMUStates::Uninitialized){
    m_txBuffer.fill(0xFF);
    m_sequenceNumbers.fill(0);
}

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

IMUStates BNO085_SPI::state() const{
    return m_state;
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
    while(m_state != IMUStates::Startup && m_state != IMUStates::Operational && timeout < WAKE_TIMEOUT_us);
    if(m_state == IMUStates::Operational || m_state == IMUStates::Startup) return error_t::GOOD;
    return error_t::ERROR;
}

void BNO085_SPI::serviceInterrupt(){
    if(m_state == IMUStates::Asleep){
        m_state = IMUStates::Startup;
        digitalWriteFast(P0_PIN, HIGH);
    }
    error_t badPacket = error_t::GOOD;
    uint_t count = 0; //debug
    noInterrupts();
    while(digitalReadFast(INTERRUPT_PIN) == LOW){
        /*
        if(badPacket != error_t::GOOD){
            count++;
            void flushChunck();
        }
        else{
            result_t<SHTPHeader> packet = readSHTP();
            if(packet.error != error_t::GOOD) badPacket = error_t::ERROR;
            respondToPacket(packet.data);
        }
        */
        //debug-----------------------------
        SPI1.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE2));
        digitalWriteFast(CS_PIN, LOW);
        SPI1.transfer(nullptr, m_rxBuffer.data(), m_rxBuffer.size());
        digitalWriteFast(CS_PIN, HIGH);
        SPI1.endTransaction();
        interrupts();
        for(uint_t i=0; i<m_rxBuffer.size(); i++){
            Serial.printf("%d ", m_rxBuffer[i]);
        }
        Serial.println();
        noInterrupts();
        //debug-----------------------------
    }
    interrupts();
}

result_t<BNO085_SPI::SHTPHeader> BNO085_SPI::readSHTP(){
    auto errorOut = [](SHTPHeader header, error_t error){
        digitalWriteFast(CS_PIN, HIGH);
        SPI1.endTransaction();
        return result_t<SHTPHeader>{header, error};
    };
    uint8_t headerBytes[SHTP_HEADER_SIZE];
    SHTPHeader header;
    SPI1.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE3));
    digitalWriteFast(CS_PIN, LOW);
    //read header
    SPI1.transfer(nullptr, headerBytes, SHTP_HEADER_SIZE);
    header.continuation = headerBytes[SHTP_LENGTH_MSB] & SHTP_CONTINUATION_BIT;
    header.length = static_cast<uint16_t>(headerBytes[SHTP_LENGTH_LSB]) | (static_cast<uint16_t>((headerBytes[SHTP_LENGTH_MSB] & ~SHTP_CONTINUATION_BIT)) << 8);
    header.channel = headerBytes[SHTP_CHANNEL];
    //check for invalid headers
    if(header.length <= SHTP_HEADER_SIZE) return errorOut(header, error_t(2));
    if(header.channel > c_numSHTPChannels) return errorOut(header, error_t(3));
    if(header.length-SHTP_HEADER_SIZE >= m_rxBuffer.size() && !header.continuation) return errorOut(header, error_t(4));
    //read payload
    if(!header.continuation){
        //store payload in rx buffer for simple packets
        SPI1.transfer(nullptr, m_rxBuffer.data(), header.length-SHTP_HEADER_SIZE);
    }
    else{
        //discard payload for continuation packets (not needed for application)
        SPI1.transfer(nullptr, header.length-SHTP_HEADER_SIZE);
    }
    digitalWriteFast(CS_PIN, HIGH);
    SPI1.endTransaction();
    return header;
}

void BNO085_SPI::flushChunck(){
    SPI1.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE3));
    digitalWriteFast(CS_PIN, LOW);
    SPI1.transfer(nullptr, SHTP_IGNORE_CHUNCK_SIZE);
    digitalWriteFast(CS_PIN, HIGH);
    SPI1.endTransaction();
}

error_t BNO085_SPI::sendSHTP(BNO085_SPI::SHTPHeader packet){
    if(packet.continuation || packet.channel > c_numSHTPChannels || packet.length <= SHTP_HEADER_SIZE || packet.length-SHTP_HEADER_SIZE >= m_txBuffer.size()) return error_t::ERROR;
    uint8_t headerBytes[SHTP_HEADER_SIZE];
    headerBytes[SHTP_LENGTH_LSB] = static_cast<uint8_t>(0x00FF & packet.length);
    headerBytes[SHTP_LENGTH_MSB] = static_cast<uint8_t>(0x7F00 & packet.length);
    headerBytes[SHTP_CHANNEL] = packet.channel;
    headerBytes[SHTP_SEQUENCE] = m_sequenceNumbers[packet.channel]++;
    noInterrupts();
    SPI1.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE3));
    digitalWriteFast(CS_PIN, LOW);
    //transfer header
    SPI1.transfer(headerBytes, SHTP_HEADER_SIZE);
    //transfer payload
    SPI1.transfer(m_txBuffer.data(), packet.length - SHTP_HEADER_SIZE);
    digitalWriteFast(CS_PIN, HIGH);
    SPI1.endTransaction();
    interrupts();
    return error_t::GOOD;
}

void BNO085_SPI::respondToPacket(BNO085_SPI::SHTPHeader packet){
    //for now
    Serial.printf("Packet - length: %d, channel: %d, continuation: %d\n", packet.length, packet.channel, (packet.continuation)? 1 : 0);
}