#include "airbrakes\AirbrakesController.h"

using namespace Airbrakes;
using namespace Airbrakes::Controls;

//Natural Constants
#define TEMPERATURE_LAPSE_RATE 0.0065       //unit: K/m
#define GRAVITATIONAL_CONSTANT 9.80665      //unit: m/s^2
#define IDEAL_GAS_CONSTANT 8.31446          //unit: J/mol/K
#define MOLAR_MASS_OF_DRY_AIR 0.0289652     //unit: kg/mol


Controller::Controller(const char* name, uint_t clockPeriod, const FlightPlan& plan, const Observer& observer) : 
    m_name(name), m_flightPlan(plan), m_observer(observer), m_clockPeriod(clockPeriod), m_isActive(false){}

RocketOS::Shell::CommandList Controller::getCommands() const{
    return {"controller", c_rootCommands.data(), c_rootCommands.size(), c_rootChildren.data(), c_rootChildren.size()};
}

void Controller::start(){
    m_clock.end();
    m_clock.begin([this](){this->clock();}, m_clockPeriod);
    m_isActive = true;
}

void Controller::stop(){
    m_clock.end();
    m_isActive = false;
}

void Controller::resetInit(){
    if(m_isActive) m_clock.begin([this](){this->clock();}, m_clockPeriod);
}

bool Controller::isActive(){
    return m_isActive;
}

void Controller::clock(){
    float_t errorSignal = m_observer.getAltitude() - m_flightPlan.getAltitude(m_observer.getVeritcalVelocity(), m_observer.getAngleToHorizontal());
    float_t derivativeErrorSignal = m_observer.getVeritcalVelocity() - directionalFlightPathDerivative(m_observer.getVeritcalVelocity(), m_observer.getAngleToHorizontal(), m_observer.getVerticalAcceleration(), m_observer.getAngularVelocityToHorizontal());
    m_desiredVerticalAcceleration = m_proportionalGain * errorSignal + m_derivativeGain * derivativeErrorSignal;
    m_requestedDragArea = getDragAreaFromAcceleration(m_desiredVerticalAcceleration);
}

//helpers

float_t Controller::directionalFlightPathDerivative(float_t currentVerticalVelocity, float_t currentAngleToHorizontal, float_t verticalAcceleration, float_t angularVelocity) const{
    float_t magnitude = std::sqrt(verticalAcceleration * verticalAcceleration + angularVelocity * angularVelocity);
    float_t accelUnit = verticalAcceleration / magnitude;
    float_t angularVelocityUnit = angularVelocity / magnitude;
    return m_flightPlan.getAltitude(currentVerticalVelocity + accelUnit, currentAngleToHorizontal + angularVelocityUnit) - m_flightPlan.getAltitude(currentVerticalVelocity, currentAngleToHorizontal);
}

float_t Controller::airDensity(float_t altitude) const{
    return m_flightPlan.getGroundPressure() * MOLAR_MASS_OF_DRY_AIR / (IDEAL_GAS_CONSTANT * m_flightPlan.getGroundTemperature()) * std::pow(1-TEMPERATURE_LAPSE_RATE * altitude / m_flightPlan.getGroundTemperature(), GRAVITATIONAL_CONSTANT *MOLAR_MASS_OF_DRY_AIR / (IDEAL_GAS_CONSTANT * TEMPERATURE_LAPSE_RATE)-1);
}

float_t Controller::getDragAreaFromAcceleration(float_t desiredVeritcalAcceleration) const{
    return 2 * desiredVeritcalAcceleration / (airDensity(m_observer.getAltitude()) * std::sqrt(m_observer.getHorizontalVelocity() * m_observer.getHorizontalVelocity() + m_observer.getVeritcalVelocity() * m_observer.getVeritcalVelocity()) * m_observer.getVeritcalVelocity());
}

//references
uint_t& Controller::getClockPeriodRef(){
    return m_clockPeriod;
}

bool& Controller::getActiveFlagRef(){
    return m_isActive;
}

float_t& Controller::getPGainRef(){
    return m_proportionalGain;
}
float_t& Controller::getDGainRef(){
    return m_derivativeGain;
}

const float_t& Controller::getRequestedDragAreaRef() const{
    return m_requestedDragArea;
}

const float_t& Controller::getDesiredAccelRef() const{
    return m_desiredVerticalAcceleration;
}
