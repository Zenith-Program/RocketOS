#include "airbrakes\AirbrakesController.h"

using namespace Airbrakes;
using namespace Airbrakes::Controls;

//Natural Constants
#define TEMPERATURE_LAPSE_RATE 0.0065       //unit: K/m
#define GRAVITATIONAL_CONSTANT 9.80665      //unit: m/s^2
#define IDEAL_GAS_CONSTANT 8.31446          //unit: J/mol/K
#define MOLAR_MASS_OF_DRY_AIR 0.0289652     //unit: kg/mol


Controller::Controller(const char* name, uint_t clockPeriod, const FlightPlan& plan, const Observer& observer, float_t decayRate) : 
    m_name(name), m_flightPlan(plan), m_observer(observer), m_fault(false), m_decayRate(decayRate), m_updateRuleShutdownVelocity(0), m_clockPeriod(clockPeriod), m_isActive(false){}

RocketOS::Shell::CommandList Controller::getCommands() const{
    return {"controller", c_rootCommands.data(), c_rootCommands.size(), c_rootChildren.data(), c_rootChildren.size()};
}

error_t Controller::start(){
    m_clock.end();
    if(newFlight() != error_t::GOOD){
        m_isActive = false;
        return error_t::ERROR;
    }
    m_clock.begin([this](){this->clock();}, m_clockPeriod);
    m_isActive = true;
    return error_t::GOOD;
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
    //read current state from the observer
    float_t currentAltitude = m_observer.getAltitude();
    float_t currentVerticalVelocity = m_observer.getVeritcalVelocity();
    float_t currentAngle = m_observer.getAngleToHorizontal();
    //get flight path parameters from flight plan
    if(!m_flightPlan.isLoaded()){
        if(m_fault == false);//log error
        m_fault = true;
    }
    m_flightPath = m_flightPlan.getAltitude(currentVerticalVelocity, currentAngle);
    m_flightPathVelocityPartial = m_flightPlan.getVelocityPartial(currentVerticalVelocity, currentAngle);
    m_flightPathAnglePartial = m_flightPlan.getAnglePartial(currentVerticalVelocity, currentAngle);
    //use update rule to compute base airbrake deployment
    m_error = currentAltitude - m_flightPath;
    if(currentVerticalVelocity < m_updateRuleShutdownVelocity){
         m_updateRuleClamped = true;
    }
    else{
        m_updateRuleDragArea = updateRule(m_error, currentVerticalVelocity, currentAngle, currentAltitude, m_flightPathVelocityPartial, m_flightPathAnglePartial);
    }
    //fine tune deployment with accumulator
    m_adjustedDragArea = m_updateRuleDragArea;
    //limit control input to the physical range of the actuators
    m_requestedDragArea = getBestPossibleDragArea(m_adjustedDragArea, m_error);
    m_isSaturated = (m_requestedDragArea != m_adjustedDragArea);
}

//helpers

float_t Controller::airDensity(float_t altitude) const{
    return m_flightPlan.getGroundPressure() * MOLAR_MASS_OF_DRY_AIR / (IDEAL_GAS_CONSTANT * m_flightPlan.getGroundTemperature()) * std::pow(1-TEMPERATURE_LAPSE_RATE * altitude / m_flightPlan.getGroundTemperature(), GRAVITATIONAL_CONSTANT * MOLAR_MASS_OF_DRY_AIR / (IDEAL_GAS_CONSTANT * TEMPERATURE_LAPSE_RATE)-1);
}

float_t Controller::updateRule(float_t error, float_t verticalVelocity, float_t angle, float_t altitude, float_t velocityPartial, float_t anglePartial) const{
    /*Formula
                    2 * mass * sin(angle) * (decayRate * error - verticalVelocity - g * (pathPartialVelocity + pathPartialAngle * sin(2 * angle) / (2 * verticalVelocity)))
    dragArea   =    -------------------------------------------------------------------------------------------------------------------------------------------------------
                                                airDensity * verticalVelocity^2 * pathPartialVelocity
    
    Derrived from 2d rocket dynamics
    */
    return 2 * m_flightPlan.getDryMass() * std::sin(angle) * (m_decayRate * error - verticalVelocity - GRAVITATIONAL_CONSTANT * (velocityPartial + anglePartial * std::sin(2 * angle) / (2 * verticalVelocity))) / (airDensity(altitude) * verticalVelocity * verticalVelocity * velocityPartial);
}

float_t Controller::getBestPossibleDragArea(float_t dragArea, float_t error) const{
    if(dragArea <= m_flightPlan.getMaxDragArea() && dragArea >= m_flightPlan.getMinDragArea()) return dragArea;
    if(error > 0) return m_flightPlan.getMaxDragArea();
    return m_flightPlan.getMinDragArea();
}

error_t Controller::newFlight(){
    if(!m_flightPlan.isLoaded()) return error_t::ERROR;
    m_updateRuleClamped = false;
    m_isSaturated = false;
    m_requestedDragArea = m_flightPlan.getMinDragArea();
    return error_t::GOOD;
}

//references
uint_t& Controller::getClockPeriodRef(){
    return m_clockPeriod;
}

bool& Controller::getActiveFlagRef(){
    return m_isActive;
}


const float_t& Controller::getErrorRef() const{
    return m_error;
}
const float_t& Controller::getFlightPathRef() const{
    return m_flightPath;
}
const float_t& Controller::getVPartialRef() const{
    return m_flightPathVelocityPartial;
}

const float_t& Controller::getAnglePartialRef() const{
    return m_flightPathAnglePartial;
}

const float_t& Controller::getUpdateRuleDragRef() const{
    return m_updateRuleDragArea;
}

const float_t& Controller::getAdjustedDragRef() const{
    return m_adjustedDragArea;
}

const float_t& Controller::getRequestedDragRef() const{
    return m_requestedDragArea;
}


const bool& Controller::getClampFlagRef() const{
    return m_updateRuleClamped;
}

const bool& Controller::getSaturationFlagRef() const{
    return m_isSaturated;
}

const bool& Controller::getFaultFlagRef() const{
    return m_fault;
}


float_t& Controller::getDecayRateRef(){
    return m_decayRate;
}

float_t& Controller::getCoastVelocityRef(){
    return m_updateRuleShutdownVelocity;
}