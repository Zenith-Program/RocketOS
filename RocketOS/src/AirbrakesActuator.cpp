#include "airbrakes\AirbrakesActuator.h"

using namespace Airbrakes;
using namespace Motor;

#define ENCODER_PIN_A 6
#define ENCODER_PIN_B 9

#define DRIVER_PIN_EN 23 //high is disabled, low is enabled
#define DRIVER_PIN_MS1 22 //stepping type selection
#define DRIVER_PIN_MS2 21
#define DRIVER_PIN_I1 20 //current limiters
#define DRIVER_PIN_I2 19
#define DRIVER_PIN_SLP 18 //high is awake, low is sleep
#define DRIVER_PIN_STEP 17
#define DRIVER_PIN_DIR 16

#define MOTOR_ENCODER_TOLERANCE 100

Actuator::Actuator(const char* name) : m_name(name), m_encoder(ENCODER_PIN_A, ENCODER_PIN_B), m_currentEncoderEndPosition(Airbrakes_CFG_MotorFullStrokeNumEncoderPositions), m_active(false), m_targetEncoderPosition(0), m_mode(SteppingModes::FullStep), m_stepPeriod_us(getStepPeriod_us(Airbrakes_CFG_MotorDefaultSpeed, SteppingModes::FullStep)){}

RocketOS::Shell::CommandList Actuator::getCommands(){
    return CommandList{m_name, c_rootCommands.data(), c_rootCommands.size(), nullptr, 0};
}


void Actuator::initialize(){
    //diable driver
    pinMode(DRIVER_PIN_EN, OUTPUT);
    digitalWriteFast(DRIVER_PIN_EN, HIGH);
    //setup mode pins
    pinMode(DRIVER_PIN_MS1, OUTPUT);
    pinMode(DRIVER_PIN_MS2, OUTPUT);
    applySteppingMode(m_mode);
    //set current limit
    pinMode(DRIVER_PIN_I1, OUTPUT);
    digitalWriteFast(DRIVER_PIN_I1, LOW);
    pinMode(DRIVER_PIN_I2, OUTPUT);
    digitalWriteFast(DRIVER_PIN_I2, LOW);
    //wake driver
    pinMode(DRIVER_PIN_SLP, OUTPUT);
    digitalWriteFast(DRIVER_PIN_SLP, HIGH);
    //initialize stepping pins
    pinMode(DRIVER_PIN_STEP, OUTPUT);
    digitalWriteFast(DRIVER_PIN_STEP, LOW);
    pinMode(DRIVER_PIN_DIR, OUTPUT);
    setDirection(Directions::Retract);
}

void Actuator::sleep(){
    m_active = false;
    m_timer.end();
    digitalWriteFast(DRIVER_PIN_EN, HIGH);
}

void Actuator::wake(){
    m_active = true;
    m_timer.begin([this](){
        this->stepISR();
    }, m_stepPeriod_us);
    digitalWriteFast(DRIVER_PIN_EN, LOW);
}

void Actuator::setSteppingCharacteristics(float_t units, SteppingModes mode){
    m_mode = mode;
    applySteppingMode(mode);
    m_stepPeriod_us = getStepPeriod_us(units, mode);
    if(m_active){
        m_timer.end();
        m_timer.begin([this](){
            this->stepISR();
        }, m_stepPeriod_us);
    }
}

void Actuator::setSteppingSpeed(float_t units){
    m_stepPeriod_us = getStepPeriod_us(units, m_mode);
    if(m_active){
        m_timer.end();
        m_timer.begin([this](){
            this->stepISR();
        }, m_stepPeriod_us);
    }
}

void Actuator::setSteppingMode(SteppingModes mode){
    //later
}

error_t Actuator::setTargetDeployment(float_t units){
    if(units < 0 || units > 1) return error_t::ERROR;
    m_targetEncoderPosition = getEncoderPositionFromUnitDeployment(units);
    return error_t::GOOD;
}

float_t Actuator::getCurrentDeployment(){
    return getUnitDeploymentFromEncoderPosition(abs(m_encoder.read()));
}

//helper functions
void Actuator::stepISR(){
    m_currentEncoderPosition = m_encoder.read();
    m_currentEncoderError = m_currentEncoderPosition - m_targetEncoderPosition;
    if(abs(m_currentEncoderError) < MOTOR_ENCODER_TOLERANCE){
        digitalWriteFast(DRIVER_PIN_STEP, LOW);
        return;
    } 
    if(m_currentEncoderError > 0) setDirection(Directions::Extend);
    else setDirection(Directions::Retract);
    digitalWriteFast(DRIVER_PIN_STEP, HIGH);
    delayMicroseconds(5);
    digitalWriteFast(DRIVER_PIN_STEP, LOW);
}

uint_t Actuator::getStepPeriod_us(float_t unitsPerSecond, SteppingModes mode) const{
    float_t usPerStep = static_cast<float_t>(c_numFullStrokeEncoderPositionns) * 1000000 / (unitsPerSecond * c_numFullStrokeSteps * m_currentEncoderEndPosition);
    uint_t steppingModeMultiplier = 1;
    if(mode == SteppingModes::MicroStep) steppingModeMultiplier <<= 3;
    if(mode == SteppingModes::QuarterStep) steppingModeMultiplier <<= 2;
    if(mode == SteppingModes::HalfStep) steppingModeMultiplier <<= 1;
    uint_t halfPeriod_us = static_cast<uint_t>(usPerStep/(2*steppingModeMultiplier));
    return halfPeriod_us;
}

void Actuator::applySteppingMode(SteppingModes mode){
    switch(mode){
        case SteppingModes::FullStep:
        default:
            digitalWriteFast(DRIVER_PIN_MS1, LOW);
            digitalWriteFast(DRIVER_PIN_MS2, LOW);
        break;
        case SteppingModes::HalfStep:
            digitalWriteFast(DRIVER_PIN_MS1, HIGH);
            digitalWriteFast(DRIVER_PIN_MS2, LOW);
        break;
        case SteppingModes::QuarterStep:
            digitalWriteFast(DRIVER_PIN_MS1, LOW);
            digitalWriteFast(DRIVER_PIN_MS2, HIGH);
        break;
        case SteppingModes::MicroStep:
            digitalWriteFast(DRIVER_PIN_MS1, HIGH);
            digitalWriteFast(DRIVER_PIN_MS2, HIGH);
        break;
    }
}

void Actuator::setDirection(Directions dir){
    if(dir == Directions::Extend && m_currentDirection != Directions::Extend){
        Serial.println("Extending"); //debug
        digitalWriteFast(DRIVER_PIN_DIR, LOW);
        m_currentDirection = Directions::Extend;
        delay(1);
        return;
    }
    if(dir == Directions::Retract && m_currentDirection != Directions::Retract){
        Serial.println("Retracting"); //debug
        digitalWriteFast(DRIVER_PIN_DIR, HIGH);
        m_currentDirection = Directions::Retract;
        delay(1);
        return;
    }
}

int_t Actuator::getEncoderPositionFromUnitDeployment(float_t units) const{
    return -static_cast<int_t>(units * m_currentEncoderEndPosition);
}

float_t Actuator::getUnitDeploymentFromEncoderPosition(int_t encoderPos) const{
    return static_cast<float_t>(-encoderPos) / m_currentEncoderEndPosition;
}

//references
const int_t& Actuator::getEncoderPosRef() const{
    return m_currentEncoderPosition;
}

const int_t& Actuator::getTargetEncoderRef() const{
    return m_targetEncoderPosition;
}
const int_t& Actuator::getErrorRef() const{
    return m_currentEncoderError;
}