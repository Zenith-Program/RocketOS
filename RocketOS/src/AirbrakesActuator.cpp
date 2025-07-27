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
#define DRIVER_PIN_DIR 16 //low extends airbrakes, high retracts airbrakes

#define MOTOR_ENCODER_TOLERANCE 48
#define MOTOR_DIR_LATCH_TIME_us 20
#define ENCODER_DIF_PD_us 10000

#define MOTOR_DEFAULT_STEPPING SteppingModes::HalfStep

Actuator::Actuator(const char* name) : m_name(name), m_encoder(ENCODER_PIN_A, ENCODER_PIN_B), m_ActuatorLimit(1), m_numFullStrokeSteps(Airbrakes_CFG_MotorFullStrokeNumSteps), m_numFullStrokeEncoderPositions(Airbrakes_CFG_MotorFullStrokeNumEncoderPositions), m_state(States::Sleep), m_targetEncoderPosition(0), m_mode(MOTOR_DEFAULT_STEPPING), m_stepPeriod_us(getStepPeriod_us(Airbrakes_CFG_MotorDefaultSpeed, MOTOR_DEFAULT_STEPPING)){}

RocketOS::Shell::CommandList Actuator::getCommands(){
    return CommandList{m_name, c_rootCommands.data(), c_rootCommands.size(), c_rootCommandList.data(), c_rootCommandList.size()};
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
    m_state = States::Sleep;
    m_timer.end();
    digitalWriteFast(DRIVER_PIN_EN, HIGH);
}

void Actuator::wake(){
    m_state = States::Active;
    m_timer.begin([this](){
        this->stepISR();
    }, m_stepPeriod_us);
    digitalWriteFast(DRIVER_PIN_EN, LOW);
}

error_t Actuator::setSteppingCharacteristics(float_t units, SteppingModes mode){
    if(m_state == States::TareRetract || m_state == States::TareExtend || m_state == States::Zero) return error_t::ERROR; //don't allow change of stepping type or speed durring calibrations
    m_mode = mode;
    applySteppingMode(mode);
    //restart timer
    m_stepPeriod_us = getStepPeriod_us(units, mode);
    if(m_state == States::Active){
        m_timer.end();
        m_timer.begin([this](){
            this->stepISR();
        }, m_stepPeriod_us);
    }
    return error_t::GOOD;
}

error_t Actuator::setSteppingSpeed(float_t units){
    if(m_state == States::TareRetract || m_state == States::TareExtend || m_state == States::Zero) return error_t::ERROR; //don't allow change of speed durring calibrations
    m_stepPeriod_us = getStepPeriod_us(units, m_mode);
    //restart timer
    if(m_state == States::Active){
        m_timer.end();
        m_timer.begin([this](){
            this->stepISR();
        }, m_stepPeriod_us);
    }
    return error_t::GOOD;
}

error_t Actuator::setSteppingMode(SteppingModes newMode){
    if(m_state == States::TareRetract || m_state == States::TareExtend || m_state == States::Zero) return error_t::ERROR; //don't allow change of stepping type durring calibrations
    //compute step rate conversion to continue the same actuator speed
    int_t power = getPeriodConversionPower(m_mode, newMode);
    if(power >= 0) m_stepPeriod_us <<=  power;
    else m_stepPeriod_us >>= -power;
    //change stepping mode
    m_mode = newMode;
    applySteppingMode(m_mode);
    //restart timer
    if(m_state == States::Active){
        m_timer.end();
        m_timer.begin([this](){
            this->stepISR();
        }, m_stepPeriod_us);
    }
    return error_t::GOOD;
}

error_t Actuator::setTargetDeployment(float_t units){
    if(units < 0 || units > 1) return error_t::ERROR;
    m_targetEncoderPosition = getEncoderPositionFromUnitDeployment(units);
    return error_t::GOOD;
}

error_t Actuator::setActuatorLimit(float_t lim){
    if(lim < 0 || lim > 1) return error_t::ERROR;
    m_ActuatorLimit = lim;
    return error_t::GOOD;
}

float_t Actuator::getCurrentDeployment(){
    return getUnitDeploymentFromEncoderPosition(m_encoder.read());
}

bool Actuator::onTarget(){
    int_t currentEncoderError = m_currentEncoderPosition - m_targetEncoderPosition;
    if(abs(currentEncoderError) < MOTOR_ENCODER_TOLERANCE) return true;
    return false;
}

float_t Actuator::getTarget() const{
    return getUnitDeploymentFromEncoderPosition(m_targetEncoderPosition);
}

//calibration
void Actuator::beginTare(){
    //initialize calibration data
    bool needToStart = m_state == States::Sleep;
    m_state = States::TareRetract;
    m_encoderDifferentiator.reset();
    m_calibrationCycleCount = 0;
    m_difPd = 0;
    //start timer if nessesary
    if(needToStart){
        m_timer.end();
        m_timer.begin([this](){
            this->stepISR();
        }, m_stepPeriod_us);
        digitalWriteFast(DRIVER_PIN_EN, LOW);
    }
}

void Actuator::beginZero(){
    //initialize calibration data
    bool needToStart = (m_state == States::Sleep);
    m_state = States::Zero;
    m_encoderDifferentiator.reset();
    m_currentEncoderDerivative = 0;
    m_calibrationCycleCount = 0;
    m_difPd = 0;
    m_calibrationStepCount = 0;
    //start timer if nessesary
    if(needToStart){
        m_timer.end();
        m_timer.begin([this](){
            this->stepISR();
        }, m_stepPeriod_us);
        digitalWriteFast(DRIVER_PIN_EN, LOW);
    }
}

//helper functions
void Actuator::stepISR(){
    //update encoder position and speed
    m_currentEncoderPosition = m_encoder.read();
    if(m_difPd > ENCODER_DIF_PD_us){
        m_encoderDifferentiator.push(m_currentEncoderPosition);
        m_difPd = 0;
        m_calibrationCycleCount++;
    }
    m_currentEncoderDerivative = m_encoderDifferentiator.output();
    /*Active mode
     * This is the main mode used in flight. 
     * The actuator is given a target the motor is stepped in the direction so that it moves to the target.
     * If the actuator is 'close enough', the motor stops moving and holds its opsition
    */
    if(m_state == States::Active){
        int_t currentEncoderError = m_currentEncoderPosition - m_targetEncoderPosition;
        //check if close enough
        if(abs(currentEncoderError) < MOTOR_ENCODER_TOLERANCE){
            digitalWriteFast(DRIVER_PIN_STEP, LOW);
            return;
        }
        //choose direction based on error 
        if(currentEncoderError > 0) setDirection(Directions::Extend);
        else setDirection(Directions::Retract);
        //step
        digitalToggleFast(DRIVER_PIN_STEP);
        return;
    }
    /*Zeroing mode
     * for the tare and zero functions, the actuator is retracted until the airbrakes are fully retracted and get stuck. 
     * This is detected by the encoder's 'velocity' becoming inconsistent with retraction.
     * For the zero function, when this occurs the new reference point is saved and the motor returns to the active state.
     * This is the first step of the tare function; after the bottom if foung the the actuator extends fully to find the upper limit.
    */
    if(m_state == States::Zero || m_state == States::TareRetract){
        setDirection(Directions::Retract);
        //check change in actuator position is negative (moving up)
        if(m_encoderDifferentiator.output() <= 0 && m_calibrationCycleCount > 2 * m_encoderDifferentiator.size()){
            //zero the encoder
            m_encoder.write(0);
            m_targetEncoderPosition = 0;
            m_currentEncoderPosition = 0;
            m_encoderDifferentiator.reset();
            m_currentEncoderDerivative = 0;
            m_calibrationCycleCount = 0;
            //move to next state
            if(m_state == States::TareRetract) m_state = States::TareExtend;
            else m_state = States::Active;
            //make sure step waveform is good 
            digitalWriteFast(DRIVER_PIN_STEP, LOW);
            return;
        }
        //step
        digitalToggleFast(DRIVER_PIN_STEP);
        return;
    }
    /*Tare extension mode
     * this is the second step of the tare function. After finding the bottom, the actuator extends untill it gets stuck.
     * Like the zeroing mode, this is detected by finding irregularity in the velocity of the encoder.
     * Durring this extension, the totla number of encoder positions and steps is tracked and saved when the end is reached.
     * After this, the zero function is used to return the actuator to the fully retracted position.
    */
    if(m_state == States::TareExtend){
        setDirection(Directions::Extend);
        m_calibrationStepCount++;
        //check change in actuator position is negative (moving up)
        if(m_encoderDifferentiator.output() >= 0 && m_calibrationCycleCount > 2 * m_encoderDifferentiator.size()){
            //update actuator characteristics 
            m_numFullStrokeEncoderPositions = abs(m_encoder.read());
            m_numFullStrokeSteps = m_calibrationStepCount/2;
            m_numFullStrokeSteps >>= -getPeriodConversionPower(SteppingModes::FullStep, m_mode);
            //zero after taring
            beginZero();
            //make sure step waveform is good
            digitalWriteFast(DRIVER_PIN_STEP, LOW);
            return;
        }
        //step
        digitalToggleFast(DRIVER_PIN_STEP);
        return;
    }
}

uint_t Actuator::getStepPeriod_us(float_t unitsPerSecond, SteppingModes mode) const{
    float_t usPerStep = static_cast<float_t>(m_numFullStrokeEncoderPositions) * 1000000 / (unitsPerSecond * m_numFullStrokeSteps * getNumLimitedEncoderPositions());
    uint_t steppingModeMultiplier = 1 << abs(getPeriodConversionPower(mode, SteppingModes::FullStep));
    uint_t halfPeriod_us = static_cast<uint_t>(usPerStep/(2*steppingModeMultiplier));
    return halfPeriod_us;
}

void Actuator::applySteppingMode(SteppingModes mode){
    switch(mode){
        case SteppingModes::FullStep:
            digitalWriteFast(DRIVER_PIN_MS1, LOW);
            digitalWriteFast(DRIVER_PIN_MS2, LOW);
        break;
        case SteppingModes::HalfStep:
        default:
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
        digitalWriteFast(DRIVER_PIN_DIR, LOW);
        m_currentDirection = Directions::Extend;
        delayMicroseconds(MOTOR_DIR_LATCH_TIME_us);
        return;
    }
    if(dir == Directions::Retract && m_currentDirection != Directions::Retract){
        digitalWriteFast(DRIVER_PIN_DIR, HIGH);
        m_currentDirection = Directions::Retract;
        delayMicroseconds(MOTOR_DIR_LATCH_TIME_us);
        return;
    }
}

int_t Actuator::getEncoderPositionFromUnitDeployment(float_t units) const{
    return -static_cast<int_t>(units * getNumLimitedEncoderPositions());
}

float_t Actuator::getUnitDeploymentFromEncoderPosition(int_t encoderPos) const{
    return static_cast<float_t>(-encoderPos) / getNumLimitedEncoderPositions();
}

uint_t Actuator::getNumLimitedEncoderPositions() const{
    return m_ActuatorLimit * m_numFullStrokeEncoderPositions;
}

int_t Actuator::getPeriodConversionPower(SteppingModes initialMode, SteppingModes newMode){
    auto getOrder = [](SteppingModes mode){
        switch(mode){
            case SteppingModes::FullStep:
                return static_cast<int_t>(0);
            case SteppingModes::HalfStep:
            default:
                return static_cast<int_t>(1);
            case SteppingModes::QuarterStep:
                return static_cast<int_t>(2);
            case SteppingModes::MicroStep:
                return static_cast<int_t>(3);
        }
    };
    int_t initialOrder = getOrder(initialMode);
    int_t newOrder = getOrder(newMode);
    return initialOrder - newOrder;
}

//references
float_t& Actuator::getActuatorLimitRef(){
    return m_ActuatorLimit;
}

uint_t& Actuator::getEncoderStepsRef(){
    return m_numFullStrokeEncoderPositions;
}

uint_t& Actuator::getMotorStepsRef(){
    return m_numFullStrokeSteps;
}

SteppingModes& Actuator::getSteppingModeRef(){
    return m_mode;
}