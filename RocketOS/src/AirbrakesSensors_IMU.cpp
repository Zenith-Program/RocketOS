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
#define FIRST_WAKEUP_PERIOD_us 1000
#define MINIMUM_NOMINAL_WAKEUP_PERIOD_us 100000
#define NOMINAL_WAKEUP_PERIOD_GAIN 2
#define NO_WAKEUP 0
#define RESET_TIMEOUT_us 200000 


//protocol 
#define NULL_PACKET SHTPHeader{0xFFFF, 0xFF, true}
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
#define SHTP_ORIENTATION_ID 0x05
#define SHTP_ANGULAR_VELOCITY_ID 0x02
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
#define SHTP_SET_FEATURE_PAYLOAD_LENGTH 17
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
#define SHTP_FEATURE_RESPONSE_PAYLOAD_LENGTH 17 
#define SHTP_FEATURE_RESPONSE_ID 0xFC
/*same structure as set feature command*/

// === batch report ===
#define SHTP_BATCH_REPORT_LENGTH 5
#define SHTP_BATCH_REPORT_ID 0xFB

#define SHTP_BATCH_REPORT_ID_BYTE 0
#define SHTP_BATCH_REPORT_DELTA_LSB 1
#define SHTP_BATCH_REPORT_DELTA_SIZE 4

// === vector report ===
#define SHTP_VECTOR_REPORT_LENGTH 10
#define SHTP_VECTOR_REPORT_ID_BYTE 0
#define SHTP_VECTOR_REPORT_SEQUENCE_BYTE 1 //ignored
#define SHTP_VECTOR_REPORT_STATUS_BYTE 2 //low bits give sensor accuracy (used), upper bits used for delay (not used)
#define SHTP_VECTOR_REPORT_DELAY_BYTE 3 //ignored (this may be usefull for other applications)
#define SHTP_VECTOR_REPORT_X_LSB_BYTE 4
#define SHTP_VECTOR_REPORT_X_MSB_BYTE 5
#define SHTP_VECTOR_REPORT_Y_LSB_BYTE 6
#define SHTP_VECTOR_REPORT_Y_MSB_BYTE 7
#define SHTP_VECTOR_REPORT_Z_LSB_BYTE 8
#define SHTP_VECTOR_REPORT_Z_MSB_BYTE 9

#define SHTP_LINEAR_ACCELERATION_Q_POINT 8 //location of the binary decimal point
#define SHTP_ANGULAR_VELOCITY_Q_POINT 9 //location of the binary decimal point
#define SHTP_GRAVITY_Q_POINT 8 //location of the binary decimal point

#define SHTP_VECTOR_REPORT_ACCURACY_BITS 0x03

// === orientation report ===
#define SHTP_ORIENTATION_REPORT_LENGTH 14
#define SHTP_ORIENTATION_REPORT_ID_BYTE 0
#define SHTP_ORIENTATION_REPORT_SEQUENCE_BYTE 1
#define SHTP_ORIENTATION_REPORT_STATUS_BYTE 2
#define SHTP_ORIENTATION_REPORT_DELAY_BYTE 3
#define SHTP_ORIENTATION_REPORT_I_LSB_BYTE 4
#define SHTP_ORIENTATION_REPORT_I_MSB_BYTE 5
#define SHTP_ORIENTATION_REPORT_J_LSB_BYTE 6
#define SHTP_ORIENTATION_REPORT_J_MSB_BYTE 7
#define SHTP_ORIENTATION_REPORT_K_LSB_BYTE 8
#define SHTP_ORIENTATION_REPORT_K_MSB_BYTE 9
#define SHTP_ORIENTATION_REPORT_REAL_LSB_BYTE 10
#define SHTP_ORIENTATION_REPORT_REAL_MSB_BYTE 11
#define SHTP_ORIENTATION_REPORT_ACCURACY_LSB_BYTE 12
#define SHTP_ORIENTATION_REPORT_ACCURACY_MSB_BYTE 13

#define SHTP_ORIENTATION_Q_POINT 14 //location of the binary decimal point

#define SHTP_ORIENTATION_REPORT_ACCURACY_BITS 0x03

// === tare function ===
//page 49 of SH2 manual
#define SHTP_TARE_PAYLOAD_LENGTH 12
#define SHTP_TARE_ID 0xF2
#define SHTP_TARE_COMMAND 0x03
#define SHTP_TARE_SUBCOMMAND 0x00
#define SHTP_TARE_X_BIT 0b00000001
#define SHTP_TARE_Y_BIT 0b00000010
#define SHTP_TARE_Z_BIT 0b00000100
#define SHTP_TARE_USE_ROTATION_VECTOR 0x00

#define SHTP_TARE_ID_BYTE 0
#define SHTP_TARE_SEQUENCE_BYTE 1
#define SHTP_TARE_COMMAND_BYTE 2
#define SHTP_TARE_SUBCOMMAND_BYTE 3
#define SHTP_TARE_AXIS_BYTE 4
#define SHTP_TARE_VECTOR_BYTE 5
#define SHTP_TARE_FIRST_RESERVED_BYTE 6
#define SHTP_TARE_NUM_RESERVED_BYTES 6

// === persist tare function ===
//page 49 of SH2 manual
#define SHTP_PERSIST_TARE_PAYLOAD_LENGTH 12
#define SHTP_PERSIST_TARE_ID 0xF2
#define SHTP_PERSIST_TARE_COMMAND 0x03
#define SHTP_PERSIST_TARE_SUBCOMMAND 0x01

#define SHTP_PERSIST_TARE_ID_BYTE 0
#define SHTP_PERSIST_TARE_SEQUENCE_BYTE 1
#define SHTP_PERSIST_TARE_COMMAND_BYTE 2
#define SHTP_PERSIST_TARE_SUBCOMMAND_BYTE 3
#define SHTP_PERSIST_TARE_FIRST_RESERVED_BYTE 4
#define SHTP_PERSIST_TARE_NUM_RESERVED_BYTES 8



//public interface implementation
BNO085_SPI::BNO085_SPI(const char* name, uint_t frequency, uint32_t samplePeriod) : m_name(name), m_SPIFrequency(frequency), m_state(IMUStates::Uninitialized), m_resetComplete(false), m_hubInitialized(false), m_waking(false), m_tareSequenceNumber(0), m_wakeTimer(0), m_wakeTime(NO_WAKEUP),
    m_linearAccelerationStatus(IMUSensorStatus::Disabled), m_angularVelocityStatus(IMUSensorStatus::Disabled), m_gravityStatus(IMUSensorStatus::Disabled), m_orientationStatus(IMUSensorStatus::Disabled), 
    m_linearAccelerationSamplePeriod_us(samplePeriod), m_angularVelocitySamplePeriod_us(samplePeriod), m_gravitySamplePeriod_us(samplePeriod), m_orientationSamplePeriod_us(samplePeriod)
    {
        m_txBuffer.fill(0xFF);
        m_rxBuffer.fill(0xFF);
        m_sequenceNumbers.fill(0);
    }

RocketOS::Shell::CommandList BNO085_SPI::getCommands(){
    return CommandList{m_name, c_rootCommands.data(), c_rootCommands.size(), c_rootCommandList.data(), c_rootCommandList.size()};
}

IMUStates BNO085_SPI::getState() const{
    return m_state;
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
    SPI1.setMISO(MISO_PIN);
    SPI1.begin();
    //reset bno085
    resetAsync();
    elapsedMicros timeout = 0;
    uint_t timeoutDurration_us = RESET_TIMEOUT_us;
    while(m_state != IMUStates::Operational && m_state != IMUStates::Configuring && timeout < timeoutDurration_us);
    switch(m_state){
        case IMUStates::Operational: 
        case IMUStates::Configuring: return error_t::GOOD;
        default: return error_t::ERROR;
    }
}

void BNO085_SPI::setSamplePeriod_us(uint32_t period, IMUData dataType){
    m_txQueue.push(makeFeatureCallback(dataType, period));
    m_txQueue.push(makeFeatureResponseCallback(dataType));
}

void BNO085_SPI::setSamplePeriod_us(uint32_t period){
    getSamplePeriod(IMUData::AngularVelocity) = period;
    getSamplePeriod(IMUData::LinearAcceleration) = period;
    getSamplePeriod(IMUData::Orientation) = period;
    getSamplePeriod(IMUData::Gravity) = period;
    m_txQueue.push(makeFeatureCallback(IMUData::AngularVelocity, period));
    m_txQueue.push(makeFeatureCallback(IMUData::LinearAcceleration, period));
    m_txQueue.push(makeFeatureCallback(IMUData::Orientation, period));
    m_txQueue.push(makeFeatureCallback(IMUData::Gravity, period));
}

void BNO085_SPI::stopSensor(IMUData dataType){
    m_txQueue.push(makeFeatureCallback(dataType, 0));
    m_txQueue.push(makeFeatureResponseCallback(dataType));
}

void BNO085_SPI::startSensor(IMUData dataType){
    m_txQueue.push(makeFeatureCallback(dataType, getSamplePeriod(dataType)));
    m_txQueue.push(makeFeatureResponseCallback(dataType));
}

void BNO085_SPI::tare(){
    m_txQueue.push(makeTareCallback());
    m_txQueue.push(makeTarePersistCallback());
}


//helper functions

void BNO085_SPI::resetAsync(){
    m_state = IMUStates::Reseting;
    m_resetComplete = false;
    m_hubInitialized = false;
    m_linearAccelerationStatus = IMUSensorStatus::Disabled;
    m_angularVelocityStatus = IMUSensorStatus::Disabled;
    m_gravityStatus = IMUSensorStatus::Disabled;
    m_orientationStatus = IMUSensorStatus::Disabled;
    m_txQueue.clear();
    m_wakeTime = NO_WAKEUP;
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
        //prepare tx SHTP packet
        result_t<txCallback_t> nextTransmission = m_txQueue.pop();
        SHTPHeader txPacket = NULL_PACKET;
        if(nextTransmission.error == error_t::GOOD){
            txPacket = nextTransmission.data.operator()();
            //debugPrintTx(txPacket, true); //debug
        }
        //send and receive a SHTP packet
        result_t<BNO085_SPI::SHTPHeader> result = doSHTP(txPacket);
        //interpret received SHTP packet
        if(result.error == error_t::GOOD){
             respondToPacket(result.data);
             //debugPrintRx(result.data, true); //debug
        }
        //do state transition
        if(m_state == IMUStates::Reseting && m_hubInitialized && m_resetComplete){
            m_state = IMUStates::Configuring;
            m_txQueue.push(makeFeatureCallback(IMUData::Orientation, m_orientationSamplePeriod_us));
            m_txQueue.push(makeFeatureCallback(IMUData::LinearAcceleration, m_linearAccelerationSamplePeriod_us));
            m_txQueue.push(makeFeatureCallback(IMUData::AngularVelocity, m_angularVelocitySamplePeriod_us));
            m_txQueue.push(makeFeatureCallback(IMUData::Gravity, m_gravitySamplePeriod_us));
            m_txQueue.push(makeFeatureResponseCallback(IMUData::Orientation));
            m_txQueue.push(makeFeatureResponseCallback(IMUData::LinearAcceleration));
            m_txQueue.push(makeFeatureResponseCallback(IMUData::AngularVelocity));
            m_txQueue.push(makeFeatureResponseCallback(IMUData::Gravity));
            //set first wakeup
            m_wakeTimer = 0;
            m_wakeTime = FIRST_WAKEUP_PERIOD_us;
        }
        if(m_state == IMUStates::Configuring && m_angularVelocityStatus != IMUSensorStatus::Disabled && m_linearAccelerationStatus != IMUSensorStatus::Disabled && m_orientationStatus != IMUSensorStatus::Disabled && m_gravityStatus != IMUSensorStatus::Disabled){
            m_state = IMUStates::Operational;
        }
        //handle wakeups
        m_wakeTimer = 0;
        if(m_waking){
            digitalWriteFast(P0_PIN, HIGH);
            m_waking = false;
        }
    }
}

void BNO085_SPI::updateBackground(){
    if(!m_txQueue.empty() && m_wakeTime != NO_WAKEUP && m_wakeTimer >= m_wakeTime){
        m_wakeTime = max(MINIMUM_NOMINAL_WAKEUP_PERIOD_us, getMaxSamplePeriod() * NOMINAL_WAKEUP_PERIOD_GAIN);
        m_wakeTimer = 0;
        wakeAsync();
    }
}

result_t<BNO085_SPI::SHTPHeader> BNO085_SPI::doSHTP(SHTPHeader txPacket){
    bool doTx = !(txPacket.continuation || txPacket.channel > c_numSHTPChannels || txPacket.length <= SHTP_HEADER_SIZE || txPacket.length-SHTP_HEADER_SIZE >= m_txBuffer.size());
    bool failRx = false;
    SHTPHeader rxHeader;
    uint8_t rxHeaderBytes[SHTP_HEADER_SIZE];
    uint8_t txHeaderBytes[SHTP_HEADER_SIZE];
    //set tx header bytes
    if(doTx){
        txHeaderBytes[SHTP_LENGTH_LSB] = static_cast<uint8_t>(0x00FF & txPacket.length);
        txHeaderBytes[SHTP_LENGTH_MSB] = static_cast<uint8_t>(0x7F00 & txPacket.length);
        txHeaderBytes[SHTP_CHANNEL] = txPacket.channel;
        txHeaderBytes[SHTP_SEQUENCE] = m_sequenceNumbers[txPacket.channel]++;
    }
    //begin SPI transaction
    noInterrupts();
    SPI1.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE3));
    digitalWriteFast(CS_PIN, LOW);
    //exchange headers
    SPI1.transfer((doTx)? txHeaderBytes : nullptr, rxHeaderBytes, SHTP_HEADER_SIZE);
    //interpret rx header
    rxHeader.continuation = rxHeaderBytes[SHTP_LENGTH_MSB] & SHTP_CONTINUATION_BIT;
    rxHeader.length = static_cast<uint16_t>(rxHeaderBytes[SHTP_LENGTH_LSB]) | (static_cast<uint16_t>((rxHeaderBytes[SHTP_LENGTH_MSB] & ~SHTP_CONTINUATION_BIT)) << 8);
    rxHeader.channel = rxHeaderBytes[SHTP_CHANNEL];
    //check for invalid rx header
    if(rxHeader.length <= SHTP_HEADER_SIZE || rxHeader.channel >= c_numSHTPChannels) failRx = true;
    //determine remaining transfer size
    uint_t numAfterOverflow = 0;
    uint_t remainingTransferSize = 0;
    if(!failRx){
        if(rxHeader.length-SHTP_HEADER_SIZE > m_rxBuffer.size()){
            remainingTransferSize = m_rxBuffer.size();
            numAfterOverflow = (rxHeader.length-SHTP_HEADER_SIZE) - m_rxBuffer.size();
        }
        else {
            remainingTransferSize = rxHeader.length - SHTP_HEADER_SIZE;
        }
    }
    if(doTx) remainingTransferSize = max(remainingTransferSize, txPacket.length - SHTP_HEADER_SIZE);
    //transfer payloads
    if(remainingTransferSize > 0) SPI1.transfer((doTx)? m_txBuffer.data() : nullptr, m_rxBuffer.data(), remainingTransferSize);
    //skip remaining BNO payload if too large
    if(numAfterOverflow > 0) SPI1.transfer(nullptr, numAfterOverflow);
    //end SPI transaction
    digitalWriteFast(CS_PIN, HIGH);
    SPI1.endTransaction();
    interrupts();
    return {rxHeader, (!failRx && !rxHeader.continuation && remainingTransferSize != 0)? error_t::GOOD : error_t::ERROR};
}

void BNO085_SPI::respondToPacket(BNO085_SPI::SHTPHeader packet){
    if(handleInitializeResponse(packet)) return;
    if(handleResetComplete(packet)) return;
    if(handleFeatureResponse(IMUData::LinearAcceleration, packet)) return;
    if(handleFeatureResponse(IMUData::AngularVelocity, packet)) return;
    if(handleFeatureResponse(IMUData::Orientation, packet)) return;
    if(handleFeatureResponse(IMUData::Gravity, packet)) return;
    if(handleVectorReport(IMUData::LinearAcceleration, packet)) return;
    if(handleVectorReport(IMUData::AngularVelocity, packet)) return;
    if(handleVectorReport(IMUData::Gravity, packet)) return;
    if(handleOrientationReport(packet)) return;
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

bool BNO085_SPI::handleFeatureResponse(IMUData dataType, SHTPHeader packet){
    if(packet.length != SHTP_FEATURE_RESPONSE_PAYLOAD_LENGTH + SHTP_HEADER_SIZE) return false;
    if(packet.channel != SHTP_HUB_CHANNEL) return false;
    if(packet.continuation) return false;
    if(m_rxBuffer[SHTP_FEATURE_ID_BYTE] != SHTP_FEATURE_RESPONSE_ID) return false;
    if(m_rxBuffer[SHTP_FEATURE_SENSOR_ID_BYTE] == getReportID(dataType)){
        uint32_t actualPeriod_us = static_cast<uint32_t>(m_rxBuffer[SHTP_FEATURE_REPORT_INTERVAL_LSB_BYTE]) | 
        static_cast<uint32_t>(m_rxBuffer[SHTP_FEATURE_REPORT_INTERVAL_LSB_BYTE + 1] << 8) |
        static_cast<uint32_t>(m_rxBuffer[SHTP_FEATURE_REPORT_INTERVAL_LSB_BYTE + 2] << 16) |
        static_cast<uint32_t>(m_rxBuffer[SHTP_FEATURE_REPORT_INTERVAL_LSB_BYTE + 3] << 24);
        //check if sensor is shutdown
        if(actualPeriod_us == 0){
             getStatus(dataType) = IMUSensorStatus::Disabled;
             return true;
        }
        //update actual sample period
        getSamplePeriod(dataType) = actualPeriod_us;
        //check if sensor is starting up
        if(getStatus(dataType) == IMUSensorStatus::Disabled) getStatus(dataType) = IMUSensorStatus::Unreliable;
        return true;
    }
    return false;
}

bool BNO085_SPI::handleVectorReport(IMUData dataType, SHTPHeader packet){
    if(packet.length != SHTP_BATCH_REPORT_LENGTH + SHTP_VECTOR_REPORT_LENGTH + SHTP_HEADER_SIZE) return false;
    if(packet.channel != SHTP_INPUT_SENSOR_CHANNEL) return false;
    if(packet.continuation) return false;
    //check for batch report id
    if(m_rxBuffer[SHTP_BATCH_REPORT_ID_BYTE] != SHTP_BATCH_REPORT_ID) return false;
    //check sensor report ID
    if(m_rxBuffer[SHTP_VECTOR_REPORT_ID_BYTE + SHTP_BATCH_REPORT_LENGTH] != getReportID(dataType)) return false;
    //read status value
    IMUSensorStatus status = IMUSensorStatus::Unreliable;
    uint8_t statusBits = m_rxBuffer[SHTP_VECTOR_REPORT_STATUS_BYTE + SHTP_BATCH_REPORT_LENGTH] & SHTP_VECTOR_REPORT_ACCURACY_BITS;
    if(statusBits == 1) status = IMUSensorStatus::LowAccuracy;
    if(statusBits == 2) status = IMUSensorStatus::ModerateAccuracy;
    if(statusBits == 3) status = IMUSensorStatus::HighAccuracy;
    getStatus(dataType) = status;
    //read vector value
    int16_t xValue = static_cast<int16_t>(m_rxBuffer[SHTP_VECTOR_REPORT_X_LSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) | (static_cast<int16_t>(m_rxBuffer[SHTP_VECTOR_REPORT_X_MSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) << 8);
    int16_t yValue = static_cast<int16_t>(m_rxBuffer[SHTP_VECTOR_REPORT_Y_LSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) | (static_cast<int16_t>(m_rxBuffer[SHTP_VECTOR_REPORT_Y_MSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) << 8);
    int16_t zValue = static_cast<int16_t>(m_rxBuffer[SHTP_VECTOR_REPORT_Z_LSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) | (static_cast<int16_t>(m_rxBuffer[SHTP_VECTOR_REPORT_Z_MSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) << 8);
    float_t xFloat = static_cast<float_t>(xValue) / (1u << getQPoint(dataType));
    float_t yFloat = static_cast<float_t>(yValue) / (1u << getQPoint(dataType));
    float_t zFloat = static_cast<float_t>(zValue) / (1u << getQPoint(dataType));
    Vector3& storageValue = getVector(dataType);
    storageValue.x = xFloat;
    storageValue.y = yFloat;
    storageValue.z = zFloat;
    return true;
}

bool BNO085_SPI::handleOrientationReport(SHTPHeader packet){
    if(packet.length != SHTP_BATCH_REPORT_LENGTH + SHTP_ORIENTATION_REPORT_LENGTH + SHTP_HEADER_SIZE) return false;
    if(packet.channel != SHTP_INPUT_SENSOR_CHANNEL) return false;
    if(packet.continuation) return false;
    //check for batch report id
    if(m_rxBuffer[SHTP_BATCH_REPORT_ID_BYTE] != SHTP_BATCH_REPORT_ID) return false;
    //check report id
    if(m_rxBuffer[SHTP_ORIENTATION_REPORT_ID_BYTE + SHTP_BATCH_REPORT_LENGTH] != getReportID(IMUData::Orientation)) return false;
    //read status value
    IMUSensorStatus status = IMUSensorStatus::Unreliable;
    uint8_t statusBits = m_rxBuffer[SHTP_ORIENTATION_REPORT_STATUS_BYTE + SHTP_BATCH_REPORT_LENGTH] & SHTP_ORIENTATION_REPORT_ACCURACY_BITS;
    if(statusBits == 1) status = IMUSensorStatus::LowAccuracy;
    if(statusBits == 2) status = IMUSensorStatus::ModerateAccuracy;
    if(statusBits == 3) status = IMUSensorStatus::HighAccuracy;
    m_orientationStatus = status;
    //read quaternion value
    int16_t rValue = static_cast<int16_t>(m_rxBuffer[SHTP_ORIENTATION_REPORT_REAL_LSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) | (static_cast<int16_t>(m_rxBuffer[SHTP_ORIENTATION_REPORT_REAL_MSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) << 8);
    int16_t iValue = static_cast<int16_t>(m_rxBuffer[SHTP_ORIENTATION_REPORT_I_LSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) | (static_cast<int16_t>(m_rxBuffer[SHTP_ORIENTATION_REPORT_I_MSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) << 8);
    int16_t jValue = static_cast<int16_t>(m_rxBuffer[SHTP_ORIENTATION_REPORT_J_LSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) | (static_cast<int16_t>(m_rxBuffer[SHTP_ORIENTATION_REPORT_J_MSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) << 8);
    int16_t kValue = static_cast<int16_t>(m_rxBuffer[SHTP_ORIENTATION_REPORT_K_LSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) | (static_cast<int16_t>(m_rxBuffer[SHTP_ORIENTATION_REPORT_K_MSB_BYTE + SHTP_BATCH_REPORT_LENGTH]) << 8);
    float_t rFloat = static_cast<float_t>(rValue) / (1u << getQPoint(IMUData::Orientation));
    float_t iFloat = static_cast<float_t>(iValue) / (1u << getQPoint(IMUData::Orientation));
    float_t jFloat = static_cast<float_t>(jValue) / (1u << getQPoint(IMUData::Orientation));
    float_t kFloat = static_cast<float_t>(kValue) / (1u << getQPoint(IMUData::Orientation));
    m_currentOrientation.r = rFloat;
    m_currentOrientation.i = iFloat;
    m_currentOrientation.j = jFloat;
    m_currentOrientation.k = kFloat;
    return true;
}

//command generation-------------------------------------------
BNO085_SPI::SHTPHeader BNO085_SPI::generateFeatureCommand(IMUData dataType, uint32_t samplePeriod){
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
    uint32_t interval_us = samplePeriod;
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

BNO085_SPI::txCallback_t BNO085_SPI::makeFeatureCallback(IMUData dataType, uint32_t sampleRate){
    return [this, dataType, sampleRate](){
        return this->generateFeatureCommand(dataType, sampleRate);
    };
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

BNO085_SPI::txCallback_t BNO085_SPI::makeFeatureResponseCallback(IMUData dataType){
    return [this, dataType](){
        return this->generateFeatureResponseCommand(dataType);
    };
}

BNO085_SPI::SHTPHeader BNO085_SPI::generateTareCommand(){
    SHTPHeader header;
    header.channel = SHTP_HUB_CHANNEL;
    header.continuation = false;
    header.length = SHTP_HEADER_SIZE + SHTP_TARE_PAYLOAD_LENGTH;
    m_txBuffer[SHTP_TARE_ID_BYTE] = SHTP_TARE_ID;
    m_txBuffer[SHTP_TARE_SEQUENCE_BYTE] = m_tareSequenceNumber++;
    m_txBuffer[SHTP_TARE_COMMAND_BYTE] = SHTP_TARE_COMMAND;
    m_txBuffer[SHTP_TARE_SUBCOMMAND_BYTE] = SHTP_TARE_SUBCOMMAND;
    m_txBuffer[SHTP_TARE_AXIS_BYTE] = SHTP_TARE_X_BIT | SHTP_TARE_Y_BIT | SHTP_TARE_Z_BIT;
    m_txBuffer[SHTP_TARE_VECTOR_BYTE] = SHTP_TARE_USE_ROTATION_VECTOR;
    for(uint_t i=0; i<SHTP_TARE_NUM_RESERVED_BYTES; i++)
        m_txBuffer[SHTP_TARE_FIRST_RESERVED_BYTE + i] = 0;
    return header;
}

BNO085_SPI::txCallback_t BNO085_SPI::makeTareCallback(){
    return [this](){
        return this->generateTareCommand();
    };
}

BNO085_SPI::SHTPHeader BNO085_SPI::generateTarePersistCommand(){
    SHTPHeader header;
    header.channel = SHTP_HUB_CHANNEL;
    header.continuation = false;
    header.length = SHTP_HEADER_SIZE + SHTP_PERSIST_TARE_PAYLOAD_LENGTH;
    m_txBuffer[SHTP_PERSIST_TARE_ID_BYTE] = SHTP_PERSIST_TARE_ID;
    m_txBuffer[SHTP_PERSIST_TARE_SEQUENCE_BYTE] = m_tareSequenceNumber++;
    m_txBuffer[SHTP_PERSIST_TARE_COMMAND_BYTE] = SHTP_PERSIST_TARE_COMMAND;
    m_txBuffer[SHTP_PERSIST_TARE_SUBCOMMAND_BYTE] = SHTP_PERSIST_TARE_SUBCOMMAND;
    for(uint_t i=0; i<SHTP_PERSIST_TARE_NUM_RESERVED_BYTES; i++)
        m_txBuffer[SHTP_PERSIST_TARE_FIRST_RESERVED_BYTE + i] = 0;
    return header;
}

BNO085_SPI::txCallback_t BNO085_SPI::makeTarePersistCallback(){
    return [this](){
        return this->generateTarePersistCommand();
    };
}

//general helper functions
uint8_t BNO085_SPI::getReportID(IMUData data){
    switch(data){
        case IMUData::Orientation: return SHTP_ORIENTATION_ID;
        case IMUData::AngularVelocity: return SHTP_ANGULAR_VELOCITY_ID;
        case IMUData::LinearAcceleration: return SHTP_LINEAR_ID;
        case IMUData::Gravity: return SHTP_GRAVITY_ID;
        default: return 0x00;
    }
}

uint_t& BNO085_SPI::getSamplePeriod(IMUData data){
    switch(data){
        case IMUData::Orientation: return m_orientationSamplePeriod_us;
        case IMUData::AngularVelocity: return m_angularVelocitySamplePeriod_us;
        case IMUData::LinearAcceleration: return m_linearAccelerationSamplePeriod_us;
        case IMUData::Gravity: 
        default: return m_gravitySamplePeriod_us;
    }
}

IMUSensorStatus& BNO085_SPI::getStatus(IMUData data){
    switch(data){
        case IMUData::Orientation: return m_orientationStatus;
        case IMUData::AngularVelocity: return m_angularVelocityStatus;
        case IMUData::LinearAcceleration: return m_linearAccelerationStatus;
        case IMUData::Gravity: 
        default: return m_gravityStatus;
    }
}

uint_t BNO085_SPI::getQPoint(IMUData data){
    switch(data){
        case IMUData::Orientation: return SHTP_ORIENTATION_Q_POINT;
        case IMUData::AngularVelocity: return SHTP_ANGULAR_VELOCITY_Q_POINT;
        case IMUData::LinearAcceleration: return SHTP_LINEAR_ACCELERATION_Q_POINT;
        case IMUData::Gravity: 
        default: return SHTP_GRAVITY_Q_POINT;
    }
}

Vector3& BNO085_SPI::getVector(IMUData data){
    switch(data){
        case IMUData::AngularVelocity: return m_currentAngularVelocity;
        case IMUData::LinearAcceleration: return m_currentLinearAcceleration;
        case IMUData::Gravity:
        default: return m_currentGravity;
    }
}

uint_t BNO085_SPI::getMaxSamplePeriod() const{
    return max(m_linearAccelerationSamplePeriod_us, max(m_orientationSamplePeriod_us, max(m_gravitySamplePeriod_us, m_angularVelocitySamplePeriod_us)));
}

//references
uint_t& BNO085_SPI::getSPIFrequencyRef(){
    return m_SPIFrequency;
}

uint32_t& BNO085_SPI::getAccelerationSamplePeriodRef(){
    return m_linearAccelerationSamplePeriod_us;
}

uint32_t& BNO085_SPI::getAngularVelocitySamplePeriodRef(){
    return m_angularVelocitySamplePeriod_us;
}

uint32_t& BNO085_SPI::getOrientationSamplePeriodRef(){
    return m_orientationSamplePeriod_us;
}

uint32_t& BNO085_SPI::getGravitySamplePeriodRef(){
    return m_gravitySamplePeriod_us;
}

const Vector3& BNO085_SPI::getAccelerationRef() const{
    return m_currentLinearAcceleration;
}

const Vector3& BNO085_SPI::getAngularVelocitryRef() const{
    return m_currentAngularVelocity;
}

const Vector3& BNO085_SPI::getGravityRef() const{
    return m_currentGravity;
}

const Quaternion& BNO085_SPI::getOrientationRef() const{
    return m_currentOrientation;
}

//output struct print functionality
void Vector3::print() const{
    Serial.printf("<%.2f, %.2f, %.2f>", x, y, z);
}

void Vector3::println() const{
    Serial.printf("<%.2f, %.2f, %.2f>\n", x, y, z);
}

void Quaternion::print() const{
    Serial.printf("%.2f + %.2fi + %.2fj + %.2fk", r, i, j, k);
}

void Quaternion::println() const{
    Serial.printf("%.2f + %.2fi + %.2fj + %.2fk\n", r, i, j, k);
}

//debugging
void BNO085_SPI::debugPrintRx(SHTPHeader packet, bool printBuffer){
    Serial.printf("Rx: Packet - length: %d, channel: %d, continuation: %d\n", packet.length, packet.channel, (packet.continuation)? 1 : 0);
    if(printBuffer){
        for(uint_t i=0; i<min(packet.length - SHTP_HEADER_SIZE, m_rxBuffer.size()); i++)
            Serial.printf("%d ", m_rxBuffer[i]);
        Serial.println();
    }
}
void BNO085_SPI::debugPrintTx(SHTPHeader packet, bool printBuffer){
    Serial.printf("Tx: Packet - length: %d, channel: %d, continuation: %d\n", packet.length, packet.channel, (packet.continuation)? 1 : 0);
    if(printBuffer){
        for(uint_t i=0; i<min(packet.length - SHTP_HEADER_SIZE, m_txBuffer.size()); i++)
            Serial.printf("%d ", m_txBuffer[i]);
        Serial.println();
    }
}

