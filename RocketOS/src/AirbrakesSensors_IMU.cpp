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
//page 22 in BNO08x Data Sheet
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

//sensor ID's
#define SHTP_ORIENTATION_ID 0x02
#define SHTP_ROTATION_ID 0x05
#define SHTP_LINEAR_ID 0x04
#define SHTP_GRAVITY_ID 0x06

// === hub initialization command (rx) ===
#define SHTP_INITIALIZATION_PAYLOAD_LENGTH 16
#define SHTP_INITIALIZATION_ID 0xF1
#define SHTP_INITIALIZATION_COMMAND 0x04
#define SHTP_INITIALIZATION_COMMAND_UNSOLICITED 0x84

#define SHTP_INITIALIZATION_ID_BYTE 0
#define SHTP_INITIALIZATION_COMMAND_BYTE 2
#define SHTP_INITIALIZATION_STATUS_BYTE 5
#define SHTP_INITIALIZATION_SUBSYSTEM_BYTE 6

// === reset complete command (rx) ===
//page 23 in BNO08x Data Sheet
#define SHTP_RESET_COMPLETE_PAYLOAD_LENGTH 1
#define SHTP_RESET_COMPLETE_STATUS_BYTE 0

#define SHTP_RESET_COMPLETE_SUCESS 1



// === set feature command (tx) ===
//Page 27 of BNO08x Data Sheet
#define SHTP_SET_FEATURE_PAYLOAD_LENGTH 16
#define SHTP_SET_FEATURE_ID 0xFD

#define SHTP_FEATURE_ID_BYTE 0
#define SHTP_FEATURE_SENSOR_ID_BYTE 1
#define SHTP_FEATURE_FLAGS_BYTE 2
#define SHTP_FEATURE_SENSITIVITY_LSB_BYTE 3
#define SHTP_FEATURE_SENSITIVITY_MSB_BYTE 4
#define SHTP_FEATURE_REPORT_INTERVAL_LSB_BYTE 5
#define SHTP_FEATURE_BATCH_INTERVAL_LSB_BYTE 9
#define SHTP_FEATURE_SENSOR_SPECIFIC_LSB_BYTE 13

#define SHTP_FEATURE_REPORT_INTERVAL_SIZE 4
#define SHTP_FEATURE_BATCH_INTERVAL_SIZE 4
#define SHTP_FEATURE_SENSOR_SPECIFIC_SIZE 4

//page 63 of SH-2 Reference Manual
#define SHTP_FEATURE_FLAG_SENSITIVITY_BIT 0b00000001
#define SHTP_FEATURE_FLAG_SENSITIVITY_ENABLE_BIT 0b00000010
#define SHTP_FEATURE_FLAG_WAKE_ENABLE_BIT 0b00000100
#define SHTP_FEATURE_FLAG_ALWAYS_ON_BIT 0b00001000

// === get feature request command ===
//page 64 of SH-2 Reference Manual
#define SHTP_FEATURE_REQUEST_PAYLOAD_LENGTH 2
#define SHTP_FEATURE_REQUEST_ID 0xFE

#define SHTP_FEATURE_REQUEST_ID_BYTE 0
#define SHTP_FEATURE_REQUEST_SENSOR_ID_BYTE 1

// === get feature response command ===
//page 65 of SH-2 Reference Manual
#define SHTP_FEATURE_RESPONSE_PAYLOAD_LENGTH 16
#define SHTP_FEATURE_RESPONSE_ID 0xFC

//same structure as set feature command



BNO085_SPI::BNO085_SPI(const char* name, uint_t frequency) : m_name(name), m_SPIFrequency(frequency), m_state(IMUStates::Uninitialized), m_resetComplete(false), m_hubInitialized(false), m_waking(false){
    m_txBuffer.fill(0xFF);
    m_rxBuffer.fill(0xFF);
    m_sequenceNumbers.fill(0);
}

RocketOS::Shell::CommandList BNO085_SPI::getCommands(){
    return CommandList{m_name, c_rootCommands.data(), c_rootCommands.size(), nullptr, 0};
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
    m_resetComplete = false;
    m_hubInitialized = false;
    //assert reset pin
    digitalWriteFast(RESET_PIN, LOW);
    delayNanoseconds(RESET_SIGNAL_DURRATION_ns);
    digitalWriteFast(RESET_PIN, HIGH);
}

void BNO085_SPI::wakeAsync(){
    m_waking = true;
    //assert wake pin
    digitalWriteFast(P0_PIN, LOW);
}

void BNO085_SPI::serviceInterrupt(){
    if(digitalReadFast(INTERRUPT_PIN) == LOW){
        //handle incoming packet
        result_t<BNO085_SPI::SHTPHeader> result = readSHTP();
        respondToPacket(result.data);
        
        Serial.println("test");//debug
        if(m_state == IMUStates::Reseting && m_hubInitialized && m_resetComplete) m_state = IMUStates::StartOrientationConfiguration;
        //configure sensors
        if(m_state == IMUStates::StartOrientationConfiguration){
            SHTPHeader header = generateFeatureCommand(IMUData::Orientation);
            sendSHTP(header);
            header = generateFeatureResponseCommand(IMUData::Orientation);
            sendSHTP(header);
            m_state = IMUStates::DoingOrientationConfiguration;
        }
        if(m_state == IMUStates::StartAngularVelocityConfiguration){
            SHTPHeader header = generateFeatureCommand(IMUData::Rotation);
            sendSHTP(header);
            header = generateFeatureResponseCommand(IMUData::Rotation);
            sendSHTP(header);
            m_state = IMUStates::DoingAngularVelocityConfiguration;
        }
        if(m_state == IMUStates::StartLinearAccelerationConfiguration){
            SHTPHeader header = generateFeatureCommand(IMUData::LinearAcceleration);
            sendSHTP(header);
            header = generateFeatureResponseCommand(IMUData::LinearAcceleration);
            sendSHTP(header);
            m_state = IMUStates::DoingLinearAccelerationConfiguration;
        }
        if(m_state == IMUStates::StartGravityConfiguration){
            SHTPHeader header = generateFeatureCommand(IMUData::Gravity);
            sendSHTP(header);
            header = generateFeatureResponseCommand(IMUData::Gravity);
            sendSHTP(header);
            m_state = IMUStates::DoingGravityConfiguration;
        }
        //handle wakeup
        if(m_waking){
            m_waking = false;
            //de-assert wake pin
            digitalWriteFast(P0_PIN, HIGH);
        }
    }
}

result_t<BNO085_SPI::SHTPHeader> BNO085_SPI::readSHTP(){
    auto errorOut = [](SHTPHeader header, error_t error){
        digitalWriteFast(CS_PIN, HIGH);
        SPI1.endTransaction();
        interrupts();
        return result_t<SHTPHeader>{header, error};
    };
    uint8_t headerBytes[SHTP_HEADER_SIZE];
    SHTPHeader header;
    noInterrupts();
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
    interrupts();
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
    Serial.printf("Packet - length: %d, channel: %d, continuation: %d\n", packet.length, packet.channel, (packet.continuation)? 1 : 0);
    for(uint_t i=0; i<packet.length - SHTP_HEADER_SIZE; i++)
        Serial.printf("%d ", m_rxBuffer[i]);
    Serial.println();
    //========================
    if(handleInitializeResponse(packet)) return;
    if(handleResetComplete(packet)) return;
    if(handleOrientationFeatureResponse(packet)) return;
    if(handleAngularVelocityFeatureResponse(packet)) return;
    if(handleLinearAccelerationFeatureResponse(packet)) return;
}

//command responses--------------------------------------------
bool BNO085_SPI::handleInitializeResponse(SHTPHeader packet){
    if(packet.continuation || packet.channel != SHTP_HUB_CHANNEL) return false;
    if(packet.length != SHTP_INITIALIZATION_PAYLOAD_LENGTH + SHTP_HEADER_SIZE) return false;
    if(m_rxBuffer[SHTP_INITIALIZATION_ID_BYTE] != SHTP_INITIALIZATION_ID) return false;
    if(m_rxBuffer[SHTP_INITIALIZATION_COMMAND_BYTE] != SHTP_INITIALIZATION_COMMAND && m_rxBuffer[SHTP_INITIALIZATION_COMMAND_BYTE] != SHTP_INITIALIZATION_COMMAND_UNSOLICITED) return false;
    if(m_rxBuffer[SHTP_INITIALIZATION_STATUS_BYTE] == 0 && m_rxBuffer[SHTP_INITIALIZATION_SUBSYSTEM_BYTE] == 1){
        //recived an initialization packet
        m_hubInitialized = true;
        return true;
    }
    return false;
}

bool BNO085_SPI::handleResetComplete(SHTPHeader packet){
    if(packet.continuation || packet.channel != SHTP_EXECUTABLE_CHANNEL) return false;
    if(packet.length != SHTP_RESET_COMPLETE_PAYLOAD_LENGTH + SHTP_HEADER_SIZE) return false;
    if(m_rxBuffer[SHTP_RESET_COMPLETE_STATUS_BYTE] == SHTP_RESET_COMPLETE_SUCESS){
        m_resetComplete = true;
        return true;
    }
    return false;
}

bool BNO085_SPI::handleGravityFeatureResponse(SHTPHeader){
    return false; //for now
}

bool BNO085_SPI::handleOrientationFeatureResponse(SHTPHeader){
    return false; //for now
}

bool BNO085_SPI::handleAngularVelocityFeatureResponse(SHTPHeader){
    return false; //for now
}

bool BNO085_SPI::handleLinearAccelerationFeatureResponse(SHTPHeader){
    return false; //for now
}

//command generation-------------------------------------------
BNO085_SPI::SHTPHeader BNO085_SPI::generateFeatureCommand(IMUData dataType){
    //this comes from page 27 in the BNO08x data sheet
    //create header
    SHTPHeader header;
    header.channel = SHTP_HUB_CHANNEL;
    header.continuation = false;
    header.length = SHTP_SET_FEATURE_PAYLOAD_LENGTH + SHTP_HEADER_SIZE;
    //command ID's
    m_txBuffer[SHTP_FEATURE_ID_BYTE] = SHTP_SET_FEATURE_ID;
    m_txBuffer[SHTP_FEATURE_SENSOR_ID_BYTE] = getReportID(dataType);
    //set sensitivity flags (not used)
    uint8_t flags = 0x00;
    flags |= SHTP_FEATURE_FLAG_SENSITIVITY_BIT;
    flags &= ~SHTP_FEATURE_FLAG_SENSITIVITY_ENABLE_BIT;
    flags &= ~SHTP_FEATURE_FLAG_WAKE_ENABLE_BIT;
    flags &= ~SHTP_FEATURE_FLAG_ALWAYS_ON_BIT;
    m_txBuffer[SHTP_FEATURE_FLAGS_BYTE] = flags;
    m_txBuffer[SHTP_FEATURE_SENSITIVITY_LSB_BYTE] = 0;
    m_txBuffer[SHTP_FEATURE_SENSITIVITY_MSB_BYTE] = 0;
    //set sample interval
    uint32_t interval_us = static_cast<uint32_t>(m_samplePeriod_us);
    m_txBuffer[SHTP_FEATURE_REPORT_INTERVAL_LSB_BYTE] = static_cast<uint8_t>(interval_us);
    m_txBuffer[SHTP_FEATURE_REPORT_INTERVAL_LSB_BYTE + 1] = static_cast<uint8_t>(interval_us >> 8);
    m_txBuffer[SHTP_FEATURE_REPORT_INTERVAL_LSB_BYTE + 2] = static_cast<uint8_t>(interval_us >> 16);
    m_txBuffer[SHTP_FEATURE_REPORT_INTERVAL_LSB_BYTE + 3] = static_cast<uint8_t>(interval_us >> 24);
    //set batch size (not used)
    m_txBuffer[SHTP_FEATURE_BATCH_INTERVAL_LSB_BYTE] = 0;
    m_txBuffer[SHTP_FEATURE_BATCH_INTERVAL_LSB_BYTE + 1] = 0;
    m_txBuffer[SHTP_FEATURE_BATCH_INTERVAL_LSB_BYTE + 2] = 0;
    m_txBuffer[SHTP_FEATURE_BATCH_INTERVAL_LSB_BYTE + 3] = 0;
    //set sensor specific data (not used by implemented sensors)
    m_txBuffer[SHTP_FEATURE_SENSOR_SPECIFIC_LSB_BYTE] = 0;
    m_txBuffer[SHTP_FEATURE_SENSOR_SPECIFIC_LSB_BYTE + 1] = 0;
    m_txBuffer[SHTP_FEATURE_SENSOR_SPECIFIC_LSB_BYTE + 2] = 0;
    m_txBuffer[SHTP_FEATURE_SENSOR_SPECIFIC_LSB_BYTE + 3] = 0;
    return header;
}

BNO085_SPI::SHTPHeader BNO085_SPI::generateFeatureResponseCommand(IMUData dataType){
    SHTPHeader header;
    header.channel = SHTP_HUB_CHANNEL;
    header.continuation = false;
    header.length = SHTP_HEADER_SIZE + SHTP_FEATURE_REQUEST_PAYLOAD_LENGTH;
    m_txBuffer[SHTP_FEATURE_REQUEST_ID_BYTE] = SHTP_FEATURE_REQUEST_ID;
    m_txBuffer[SHTP_FEATURE_REQUEST_SENSOR_ID_BYTE] = getReportID(dataType);
    return header;
}

uint8_t BNO085_SPI::getReportID(IMUData data){
    switch(data){
        case IMUData::Orientation: return SHTP_ORIENTATION_ID;
        case IMUData::Rotation: return SHTP_ROTATION_ID;
        case IMUData::LinearAcceleration: return SHTP_LINEAR_ID;
        case IMUData::Gravity: return SHTP_GRAVITY_ID;
        default: return 0x00;
    }
}
