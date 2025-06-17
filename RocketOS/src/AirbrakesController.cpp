#include "airbrakes\AirbrakesController.h"

using namespace Airbrakes;
using namespace Airbrakes::Controls;

DemoController::DemoController(const char* name, uint_t clockPeriod, const FlightPlan& plan, const Observer& observer) : 
    m_name(name), m_flightPlan(plan), m_observer(observer), m_clockPeriod(clockPeriod), m_isActive(false){}

RocketOS::Shell::CommandList DemoController::getCommands() const{
    return {"controller", c_rootCommands.data(), c_rootCommands.size(), c_rootChildren.data(), c_rootChildren.size()};
}

void DemoController::start(){
    m_clock.end();
    m_clock.begin([this](){this->clock();}, m_clockPeriod);
    m_isActive = true;
}

void DemoController::stop(){
    m_clock.end();
    m_isActive = false;
}

void DemoController::resetInit(){
    if(m_isActive) m_clock.begin([this](){this->clock();}, m_clockPeriod);
}

bool DemoController::isActive(){
    return m_isActive;
}

void DemoController::clock(){
    
}

uint_t& DemoController::getClockPeriodRef(){
    return m_clockPeriod;
}

bool& DemoController::getActiveFlagRef(){
    return m_isActive;
}
