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
#define MISO_PIN 39

//timings
#define RESET_SIGNAL_DURRATION_ns 100
#define WAKE_TIMEOUT_us 200000
#define RESET_TIMEOUT_us 200000

//protocol
#define SHTP_CONTINUATION_BIT 0x80
#define SHTP_HEADER_SIZE 4u
#define SHTP_LENGTH_MSB 1
#define SHTP_LENGTH_LSB 0
#define SHTP_CHANNEL 2
#define SHTP_SEQUENCE 3

//channels
#define SHTP_COMMAND_CHANNEL 0
#define SHTP_EXECUTABLE_CHANNEL 1
#define SHTP_HUB_CHANNEL 2
#define SHTP_INPUT_SENSOR_CHANNEL 3
#define SHTP_WAKE_INPUT_SENSOR_CHANNEL 4
#define SHTP_GYRO_CHANNEL 5

//initialization command (rx)
#define SHTP_INITIALIZATION_LENGTH 16
#define SHTP_INITIALIZATION_ID 0xF1
#define SHTP_INITIALIZATION_COMMAND 0x04
#define SHTP_INITIALIZATION_COMMAND_UNSOLICITED 0x84

#define SHTP_INITIALIZATION_ID_BYTE 0
#define SHTP_INITIALIZATION_COMMAND_BYTE 2
#define SHTP_INITIALIZATION_STATUS_BYTE 5
#define SHTP_INITIALIZATION_SUBSYSTEM_BYTE 6

//set feature command (tx)
#define SHTP_SET_FEATURE_LENGTH 16
#define SHTP_SET_FEATURE_ID 0xFD

#define SHTP_SET_FEATURE_ORIENTATION_ID
#define SHTP_SET_FEATURE_ANGULAR_ID 
#define SHTP_SET_FEATURE_LINEAR_ID 

#define SHTP_SET_FEATURE_ID_BYTE 0
#define SHTP_SET_FEATURE_SENSOR_ID_BYTE 1
#define SHTP_SET_FEATURE_FLAGS_BYTE 2
#define SHTP_SET_FEATURE_SENSITIVITY_LSB_BYTE 3
#define SHTP_SET_FEATURE_SENSITIVITY_MSB_BYTE 4
#define SHTP_SET_FEATURE_REPORT_INTERVAL_LSB_BYTE 5
#define SHTP_SET_FEATURE_BATCH_INTERVAL_LSB_BYTE 9

#define SHTP_SET_FEATURE_REPORT_INTERVAL_SIZE 4
#define SHTP_SET_FEATURE_BATCH_INTERVAL_SIZE 4



BNO085_SPI::BNO085_SPI(const char* name, uint_t frequency) : m_name(name), m_SPIFrequency(frequency), m_state(IMUStates::Uninitialized){
    m_txBuffer.fill(0xFF);
    m_rxBuffer.fill(0xFF);
    m_sequenceNumbers.fill(0);
}

error_t BNO085_SPI::initialize(){
    //shutdown any previously initialized syatems
    SPI1.end();
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
    SPI1.setMISO(MISO_PIN);
    SPI1.begin();
    //reset bno085
    resetAsync();
    elapsedMicros timeout = 0;
    while(m_state != IMUStates::Operational && timeout < RESET_TIMEOUT_us);
    switch(m_state){
        case IMUStates::Operational: return error_t::GOOD;
        case IMUStates::StartOrientationConfiguration:
        case IMUStates::DoingOrientationConfiguration:
        case IMUStates::StartAngularVelocityConfiguration:
        case IMUStates::DoingAngularVelocityConfiguration:
        case IMUStates::StartLinearAccelerationConfiguration:
        case IMUStates::DoingLinearAccelerationConfiguration: return ERROR_ConfigurationTimeout;
        default: return ERROR_ResetTimeout;
    }
}

IMUStates BNO085_SPI::state() const{
    return m_state;
}


//helper functions

void BNO085_SPI::resetAsync(){
    m_state = IMUStates::Reseting;
    //assert reset pin
    digitalWriteFast(RESET_PIN, LOW);
    delayNanoseconds(RESET_SIGNAL_DURRATION_ns);
    digitalWriteFast(RESET_PIN, HIGH);
}

void BNO085_SPI::wakeAsync(){
    m_state = IMUStates::Waking;
    //assert wake pin
    digitalWriteFast(P0_PIN, LOW);
}

void BNO085_SPI::serviceInterrupt(){
    if(digitalReadFast(INTERRUPT_PIN) == LOW){
        //handle reconfiguration after wakeup
        if(m_state == IMUStates::Waking){
            m_state = IMUStates::StartOrientationConfiguration;
            //de-assert wake pin
            digitalWriteFast(P0_PIN, HIGH);
        }
        //handle incoming packets
        result_t<BNO085_SPI::SHTPHeader> result = readSHTP();
        while(result.error == error_t::GOOD){
            respondToPacket(result.data);
            result = readSHTP();
        }
        //configure sensors
        if(m_state == IMUStates::StartOrientationConfiguration){
            SHTPHeader header = generateOrientationFeatureCommand();
            sendSHTP(header);
            header = generateOrientationFeatureResponseCommand();
            sendSHTP(header);
            m_state = IMUStates::DoingOrientationConfiguration;
        }
        if(m_state == IMUStates::StartAngularVelocityConfiguration){
            SHTPHeader header = generateAngularVelocityFeatureCommand();
            sendSHTP(header);
            header = generateAngularVelocityFeatureResponseCommand();
            sendSHTP(header);
            m_state = IMUStates::DoingAngularVelocityConfiguration;
        }
        if(m_state == IMUStates::StartLinearAccelerationConfiguration){
            SHTPHeader header = generateLinearAccelerationFeatureCommand();
            sendSHTP(header);
            header = generateLinearAccelerationFeatureResponseCommand();
            sendSHTP(header);
            m_state = IMUStates::DoingLinearAccelerationConfiguration;
        }
    }
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
    if(header.length <= SHTP_HEADER_SIZE) return errorOut(header, ERROR_HeaderLength);
    if(header.channel > c_numSHTPChannels) return errorOut(header, ERROR_HeaderChannel);

    if(header.length-SHTP_HEADER_SIZE >= m_rxBuffer.size() && !header.continuation){
        SPI.transfer(nullptr, header.length-SHTP_HEADER_SIZE);
        return errorOut(header, ERROR_RxBufferOverflow);
    }
    if(header.continuation){
        //discard payload for continuation packets (not needed for application)
        SPI1.transfer(nullptr, header.length-SHTP_HEADER_SIZE);
        return errorOut(header, ERROR_ContinuationPacket);
        
    }
    //store payload in rx buffer for simple packets
    SPI1.transfer(nullptr, m_rxBuffer.data(), header.length-SHTP_HEADER_SIZE);
    //end SPI transaction
    digitalWriteFast(CS_PIN, HIGH);
    SPI1.endTransaction();
    return header;
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
    //debug==================
    /*
    Serial.printf("Packet - length: %d, channel: %d, continuation: %d\n", packet.length, packet.channel, (packet.continuation)? 1 : 0);
    for(uint_t i=0; i<packet.length - SHTP_HEADER_SIZE; i++)
        Serial.printf("%d ", m_rxBuffer[i]);
    Serial.println();
    */
    //========================
    if(handleInitializeResponse(packet)) return;
    if(handleOrientationFeatureResponse(packet)) return;
    if(handleAngularVelocityFeatureResponse(packet)) return;
    if(handleLinearAccelerationFeatureResponse(packet)) return;
}

bool BNO085_SPI::handleInitializeResponse(SHTPHeader packet){
    if(packet.continuation || packet.channel != SHTP_HUB_CHANNEL) return;
    if(packet.length != SHTP_INITIALIZATION_LENGTH + SHTP_HEADER_SIZE) return;
    if(m_rxBuffer[SHTP_INITIALIZATION_ID_BYTE] != SHTP_INITIALIZATION_ID) return;
    if(m_rxBuffer[SHTP_INITIALIZATION_COMMAND_BYTE] != SHTP_INITIALIZATION_COMMAND && m_rxBuffer[SHTP_INITIALIZATION_COMMAND_BYTE] != SHTP_INITIALIZATION_COMMAND_UNSOLICITED) return;
    if(m_rxBuffer[SHTP_INITIALIZATION_STATUS_BYTE] == 0 && m_rxBuffer[SHTP_INITIALIZATION_SUBSYSTEM_BYTE] == 1){
        //recived an initialization packet
        m_state = IMUStates::StartOrientationConfiguration;
        return true;
    }
    return false;
}

BNO085_SPI::SHTPHeader BNO085_SPI::generateOrientationFeatureCommand(){

}
BNO085_SPI::SHTPHeader BNO085_SPI::generateAngularVelocityFeatureCommand(){

}
BNO085_SPI::SHTPHeader BNO085_SPI::generateLinearAccelerationFeatureCommand(){

}
BNO085_SPI::SHTPHeader BNO085_SPI::generateOrientationFeatureResponseCommand(){

}
BNO085_SPI::SHTPHeader BNO085_SPI::generateAngularVelocityFeatureResponseCommand(){

}
BNO085_SPI::SHTPHeader BNO085_SPI::generateLinearAccelerationFeatureResponseCommand(){

}
