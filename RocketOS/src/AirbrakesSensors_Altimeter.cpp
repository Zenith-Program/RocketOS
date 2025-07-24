#include "airbrakes\AirbrakesSensors_Altimeter.h"
#include <Arduino.h>
#include <SPI.h>

using namespace Airbrakes;
using namespace Sensors;

#define CS_PIN 10
#define RESET_COMMAND 0x1E
#define PROM_COMMAND_BASE 0xA0
#define PRESSURE_CONVERSION_COMMAND 0x48
#define TEMPERATURE_CONVERSION_COMMAND 0x58
#define ADC_READ_COMMAND 0x00

#define BLOCKING_TIMEOUT_ms 25

//natural constants
#define ISA_LAPSE_RATE 0.0065           // Units: K/m
#define ISA_GRAVITY 9.80665             // Units: m/s^2
#define ISA_MOLAR_MASS_AIR 0.0289652    // Units: kg/mol
#define ISA_IDEAL_GAS 8.31446           // Units: j/(mol*K)

MS5607_SPI::MS5607_SPI(const char* name, float_t groundTemperature, float_t groundPressure, uint_t frequency, TeensyTimerTool::TimerGenerator* timer) : m_name(name), m_SPIFrequency(frequency), m_timer(timer), m_state(AltimeterStates::Standby), m_newData(false), m_groundLevelTemperature_k(groundTemperature), m_groundLevelPressure_pa(groundPressure) {}

RocketOS::Shell::CommandList MS5607_SPI::getCommands(){
    return CommandList{m_name, c_rootCommands.data(), c_rootCommands.size(), c_rootCommandList.data(), c_rootCommandList.size()};
}

error_t MS5607_SPI::initialize(){
    //configuration into SPI mode

    //setup SPI
    pinMode(CS_PIN, OUTPUT);
    digitalWriteFast(CS_PIN, HIGH);
    SPI.begin();
    resetDevice();
    //read calibration data
    result_t<uint16_t> result;
    for(uint_t i=0; i<c_numCalibrationCoefficients && result.error == error_t::GOOD; i++){
        result = getCalibrationCoefficient(i);
        m_calibrationCoeffieicents[i] = result.data;
    }
    if(result.error != error_t::GOOD){
        markAsUninitialized();
        return result.error;
    }
    return error_t::GOOD;
}

bool MS5607_SPI::initialized() const{
    return m_calibrationCoeffieicents[0] != 0;
}

error_t MS5607_SPI::updateBlocking(){
    if(m_state == AltimeterStates::Standby){
        m_state = AltimeterStates::Blocking;
        beginPressureConversion();
        delay(10);
        if(readPressureVal() != error_t::GOOD) return error_t::ERROR;
        beginTemperatureConversion();
        delay(10);
        if(readTemperatureVal() != error_t::GOOD) return error_t::ERROR;
        m_newData = true;
        m_state = AltimeterStates::Standby;
        return error_t::GOOD;
    }
    return error_t::GOOD;
}

void MS5607_SPI::updateAsync(){
    if(m_state == AltimeterStates::Standby){
        m_state = AltimeterStates::Async;
        beginPressureConversion();
        m_timer.begin([this](){this->asyncStep1();});
        m_timer.trigger(10000);
    }
}


result_t<float_t> MS5607_SPI::getNewPressure(){
    if(!initialized()) return ERROR_NotInitialized;
    if(updateBlocking() != error_t::GOOD) return ERROR_NotResponsive;
    updateOutputValues();
    return m_pressure_pa;
}
result_t<float_t> MS5607_SPI::getNewTemperature(){
    if(!initialized()) return ERROR_NotInitialized;
    if(updateBlocking() != error_t::GOOD) return ERROR_NotResponsive;
    updateOutputValues();
    return m_temperature_k;
}

result_t<float_t> MS5607_SPI::getNewAltitude(){
    if(!initialized()) return ERROR_NotInitialized;
    if(updateBlocking() != error_t::GOOD) return ERROR_NotResponsive;
    updateOutputValues();
    return m_altitude_m;
}

float_t MS5607_SPI::getLastPressure(){
    updateOutputValues();
    return m_pressure_pa;
}

float_t MS5607_SPI::getLastTemperature(){
    updateOutputValues();
    return m_temperature_k;
}

float_t MS5607_SPI::getLastAltitude(){
    updateOutputValues();
    return m_altitude_m;
}

error_t MS5607_SPI::zero(){
    if(!initialized()) return ERROR_NotInitialized;
    if(updateBlocking() != error_t::GOOD) return ERROR_NotResponsive;
    updateOutputValues();
    m_groundLevelPressure_pa = m_pressure_pa;
    m_groundLevelTemperature_k = m_temperature_k;
    updateOutputValues();
    return error_t::GOOD;
}


//helper functions
void MS5607_SPI::resetDevice(){
    noInterrupts();
    SPI.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(RESET_COMMAND);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
    interrupts();
    delay(5);
}

result_t<uint16_t> MS5607_SPI::getCalibrationCoefficient(uint_t n){
    if(n >= c_numCalibrationCoefficients) return error_t::ERROR;
    noInterrupts();
    SPI.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(PROM_COMMAND_BASE + 2 * (n));
    uint16_t coeffecient = SPI.transfer16(0xFFFF);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
    interrupts();
    if(coeffecient == 0 || coeffecient == 0xFFFF) return {coeffecient, error_t::ERROR};
    return coeffecient;
}

void MS5607_SPI::markAsUninitialized(){
    m_calibrationCoeffieicents[0] = 0;
}

void MS5607_SPI::beginPressureConversion(){
    noInterrupts();
    SPI.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(PRESSURE_CONVERSION_COMMAND);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
    interrupts();
}

void MS5607_SPI::beginTemperatureConversion(){
    noInterrupts();
    SPI.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(TEMPERATURE_CONVERSION_COMMAND);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
    interrupts();
}

error_t MS5607_SPI::readPressureVal(){
    noInterrupts();
    SPI.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(ADC_READ_COMMAND);
    uint8_t byte1 = SPI.transfer(0xFF);
    uint8_t byte2 = SPI.transfer(0xFF);
    uint8_t byte3 = SPI.transfer(0xFF);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
    interrupts();
    if((byte1 == 0x00 && byte2 == 0x00 && byte3 == 0x00) || (byte1 == 0xFF && byte2 == 0xFF && byte3 == 0xFF)) return error_t::ERROR;
    m_pressureADC = 0x00000000 | static_cast<uint32_t>(byte1) << 16 | static_cast<uint32_t>(byte2) << 8 | static_cast<uint32_t>(byte3);
    return error_t::GOOD;
}

error_t MS5607_SPI::readTemperatureVal(){
    noInterrupts();
    SPI.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(ADC_READ_COMMAND);
    uint8_t byte1 = SPI.transfer(0xFF);
    uint8_t byte2 = SPI.transfer(0xFF);
    uint8_t byte3 = SPI.transfer(0xFF);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
    interrupts();
    if((byte1 == 0x00 && byte2 == 0x00 && byte3 == 0x00) || (byte1 == 0xFF && byte2 == 0xFF && byte3 == 0xFF)) return error_t::ERROR;
    m_temperatureADC = 0x00000000 | static_cast<uint32_t>(byte1) << 16 | static_cast<uint32_t>(byte2) << 8 | static_cast<uint32_t>(byte3);
    return error_t::GOOD;
}

void MS5607_SPI::asyncStep1(){
    readPressureVal();
    beginTemperatureConversion();
    m_timer.begin([this]{this->asyncStep2();});
    m_timer.trigger(10000);
}

void MS5607_SPI::asyncStep2(){
    readTemperatureVal();
    m_state = AltimeterStates::Standby;
    m_newData = true;
}

void MS5607_SPI::updateOutputValues(){
    if(m_newData){
        //compute temperature and pressure from ADC readings (MS5607 data sheet)
        float_t dT = static_cast<float_t>(m_temperatureADC) - (static_cast<float_t>(m_calibrationCoeffieicents[5]) * static_cast<float_t>(1 << 8));
        float_t TEMP = 29315 + ((dT*static_cast<float_t>(m_calibrationCoeffieicents[6])) / static_cast<float_t>(1 << 23));
        float_t OFF = (static_cast<float_t>(m_calibrationCoeffieicents[2]) * static_cast<float_t>(1 << 17)) + ((static_cast<float_t>(m_calibrationCoeffieicents[4]) * dT)  / static_cast<float_t>(1 << 6));
        float_t SENS = (static_cast<float_t>(m_calibrationCoeffieicents[1]) * static_cast<float_t>(1 << 16)) + ((static_cast<float_t>(m_calibrationCoeffieicents[3]) * dT)  / static_cast<float_t>(1 << 7));
        float_t P = static_cast<float_t>((m_pressureADC * (SENS / static_cast<float_t>(1 << 21)) - OFF) / static_cast<float_t>(1 << 15));
        m_temperature_k = TEMP / 100;
        m_pressure_pa = P;
        //compute altitude from ISA model
        m_altitude_m = m_groundLevelTemperature_k / ISA_LAPSE_RATE * (pow(m_groundLevelPressure_pa/m_pressure_pa, ISA_IDEAL_GAS * ISA_LAPSE_RATE / (ISA_GRAVITY * ISA_MOLAR_MASS_AIR))-1);
    }
}

//references
uint_t& MS5607_SPI::getSPIFrequencyRef(){
    return m_SPIFrequency;
}

float_t& MS5607_SPI::getGroundPressureRef(){
    return m_groundLevelPressure_pa;
}

float_t& MS5607_SPI::getGroundTemperatureRef(){
    return m_groundLevelTemperature_k;
}