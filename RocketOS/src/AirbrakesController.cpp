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
    m_flightPathVelocityPartial = m_flightPlan.getVelocityPartial(m_observer.getVeritcalVelocity(), m_observer.getAngleToHorizontal());
    m_flightPathAnglePartial = m_flightPlan.getAnglePartial(m_observer.getVeritcalVelocity(), m_observer.getAngleToHorizontal());
}

//helpers

float_t Controller::airDensity(float_t altitude) const{
    return m_flightPlan.getGroundPressure() * MOLAR_MASS_OF_DRY_AIR / (IDEAL_GAS_CONSTANT * m_flightPlan.getGroundTemperature()) * std::pow(1-TEMPERATURE_LAPSE_RATE * altitude / m_flightPlan.getGroundTemperature(), GRAVITATIONAL_CONSTANT * MOLAR_MASS_OF_DRY_AIR / (IDEAL_GAS_CONSTANT * TEMPERATURE_LAPSE_RATE)-1);
}

float_t Controller::updateRule(float_t error, float_t verticalVelocity, float_t angle, float_t altitude) const{
    /*Formula
                    2 * mass * sin(angle) * (decayRate * error - verticalVelocity - g * (pathPartialVelocity + pathPartialAngle * sin(2 * angle) / (2 * verticalVelocity)))
    dragArea   =    -------------------------------------------------------------------------------------------------------------------------------------------------------
                                                airDensity * verticalVelocity^2 * pathPartialVelocity
    
    Derrived from 2d rocket dynamics
    */
    return 2 * m_flightPlan.getDryMass() * std::sin(angle) * (m_decayRate * error - verticalVelocity - GRAVITATIONAL_CONSTANT * (m_flightPlan.getVelocityPartial(verticalVelocity, angle) + m_flightPlan.getAnglePartial(verticalVelocity, angle) * std::sin(2 * angle) / (2 * verticalVelocity))) / (airDensity(altitude) * verticalVelocity * verticalVelocity * m_flightPlan.getVelocityPartial(verticalVelocity, angle));
}

//references
uint_t& Controller::getClockPeriodRef(){
    return m_clockPeriod;
}

bool& Controller::getActiveFlagRef(){
    return m_isActive;
}

const float_t& Controller::getVPartialRef() const{
    return m_flightPathVelocityPartial;
}

const float_t& Controller::getAnglePartialRef() const{
    return m_flightPathAnglePartial;
}