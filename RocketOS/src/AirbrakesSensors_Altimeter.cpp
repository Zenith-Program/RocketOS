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

BarometerSPI::BarometerSPI(const char* name, uint_t frequency, TeensyTimerTool::TimerGenerator* timer) : m_name(name), m_SPIFrequency(frequency), m_timer(timer), m_inAsyncUpdate(false), m_newData(false) {}

RocketOS::Shell::CommandList BarometerSPI::getCommands(){
    return CommandList{m_name, c_rootCommands.data(), c_rootCommands.size(), c_rootCommandList.data(), c_rootCommandList.size()};
}

error_t BarometerSPI::initialize(){
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

bool BarometerSPI::initialized() const{
    return m_calibrationCoeffieicents[0] != 0;
}

error_t BarometerSPI::updateBlocking(){
    beginPressureConversion();
    delay(10);
    if(readPressureVal() != error_t::GOOD) return error_t::ERROR;
    beginTemperatureConversion();
    delay(10);
    if(readTemperatureVal() != error_t::GOOD) return error_t::ERROR;
    m_newData = true;
    return error_t::GOOD;
}

void BarometerSPI::updateAsync(){
    if(!m_inAsyncUpdate){
        m_inAsyncUpdate = true;
        beginPressureConversion();
        m_timer.begin([this](){this->asyncStep1();});
        m_timer.trigger(10000);
    }
}


result_t<float_t> BarometerSPI::getPressure(){
    if(!initialized()) return error_t(2);
    if(updateBlocking() != error_t::GOOD) return error_t(3);
    updateOutputValues();
    return m_pressure_pa;
}
result_t<float_t> BarometerSPI::getTemperature(){
    if(!initialized()) return error_t(2);
    if(updateBlocking() != error_t::GOOD) return error_t(3);
    updateOutputValues();
    return m_temperature_k;
}

float_t BarometerSPI::getPressureAsync(){
    updateOutputValues();
    return m_pressure_pa;
}

float_t BarometerSPI::getTemperatureAsync(){
    updateOutputValues();
    return m_temperature_k;
}

//helper functions
void BarometerSPI::resetDevice(){
    SPI.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(RESET_COMMAND);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
    delay(5);
}

result_t<uint16_t> BarometerSPI::getCalibrationCoefficient(uint_t n){
    if(n >= c_numCalibrationCoefficients) return error_t::ERROR;
    SPI.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(PROM_COMMAND_BASE + 2 * (n));
    uint16_t coeffecient = SPI.transfer16(0xFFFF);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
    if(coeffecient == 0 || coeffecient == 0xFFFF) return {coeffecient, error_t::ERROR};
    return coeffecient;
}

void BarometerSPI::markAsUninitialized(){
    m_calibrationCoeffieicents[0] = 0;
}

void BarometerSPI::beginPressureConversion(){
    SPI.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(PRESSURE_CONVERSION_COMMAND);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
}

void BarometerSPI::beginTemperatureConversion(){
    SPI.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(TEMPERATURE_CONVERSION_COMMAND);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
}

error_t BarometerSPI::readPressureVal(){
    SPI.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(ADC_READ_COMMAND);
    uint8_t byte1 = SPI.transfer(0xFF);
    uint8_t byte2 = SPI.transfer(0xFF);
    uint8_t byte3 = SPI.transfer(0xFF);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
    if((byte1 == 0x00 && byte2 == 0x00 && byte3 == 0x00) || (byte1 == 0xFF && byte2 == 0xFF && byte3 == 0xFF)) return error_t::ERROR;
    m_pressureADC = 0x00000000 | static_cast<uint32_t>(byte1) << 16 | static_cast<uint32_t>(byte2) << 8 | static_cast<uint32_t>(byte3);
    return error_t::GOOD;
}

error_t BarometerSPI::readTemperatureVal(){
    SPI.beginTransaction(SPISettings(m_SPIFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(ADC_READ_COMMAND);
    uint8_t byte1 = SPI.transfer(0xFF);
    uint8_t byte2 = SPI.transfer(0xFF);
    uint8_t byte3 = SPI.transfer(0xFF);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
    if((byte1 == 0x00 && byte2 == 0x00 && byte3 == 0x00) || (byte1 == 0xFF && byte2 == 0xFF && byte3 == 0xFF)) return error_t::ERROR;
    m_temperatureADC = 0x00000000 | static_cast<uint32_t>(byte1) << 16 | static_cast<uint32_t>(byte2) << 8 | static_cast<uint32_t>(byte3);
    return error_t::GOOD;
}

void BarometerSPI::asyncStep1(){
    readPressureVal();
    beginTemperatureConversion();
    m_timer.begin([this]{this->asyncStep2();});
    m_timer.trigger(10000);
}

void BarometerSPI::asyncStep2(){
    readTemperatureVal();
    m_inAsyncUpdate = false;
    m_newData = true;
}

void BarometerSPI::updateOutputValues(){
    if(m_newData){
       int32_t dT = static_cast<int32_t>(m_temperatureADC) - static_cast<int32_t>(m_calibrationCoeffieicents[5] << 8);
       int32_t TEMP = 29315 + ((dT*m_calibrationCoeffieicents[6]) >> 23);
       int64_t OFF = (static_cast<int64_t>(m_calibrationCoeffieicents[2]) << 17) + ((static_cast<int64_t>(m_calibrationCoeffieicents[4]) * dT) >> 6);
       int64_t SENS = (static_cast<int64_t>(m_calibrationCoeffieicents[1]) << 16) + ((static_cast<int64_t>(m_calibrationCoeffieicents[3]) * dT) >> 7);
       int32_t P = static_cast<int32_t>((m_pressureADC * (SENS >> 21) - OFF) >> 15);
       m_temperature_k = static_cast<float_t>(TEMP) / 100;
       m_pressure_pa = static_cast<float_t>(P);
    }
}

//references
uint_t& BarometerSPI::getSPIFrequencyRef(){
    return m_SPIFrequency;
}