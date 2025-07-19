#include "airbrakes\AirbrakesObserver.h"

using namespace Airbrakes;
//implementation of interface

Observer::Observer(Sensors::BNO085_SPI& imu, Sensors::MS5607_SPI& altimeter) : m_imu(imu), m_altimeter(altimeter) {}
error_t Observer::setMode(ObserverModes mode){
    if(mode == m_mode) return error_t::GOOD;
    if(mode == ObserverModes::FullSimulation){
        m_timer.end();
        m_imu.stopAllSensors();
        m_mode = ObserverModes::FullSimulation;
        return error_t::GOOD;
    }
    if(mode == ObserverModes::FilteredSimulation){
        m_timer.end();
        m_imu.stopAllSensors();
        m_timer.begin([this](){this->filterSimModeTimerISR();}, m_baseSamplePeriod);
        m_mode = ObserverModes::FilteredSimulation;
        return error_t::GOOD;
    }
    if(mode == ObserverModes::Sensor){
        m_timer.end();
        setupSensors();
        if(setupSensors() != error_t::GOOD){
            m_mode = ObserverModes::FullSimulation;
            return error_t::ERROR;
        }
        m_timer.begin([this](){this->sensorModeTimerISR();}, m_baseSamplePeriod);
        m_mode = ObserverModes::Sensor;
        return error_t::GOOD;
    }
}

//controller interface
float_t Observer::getAltitude() const{
    return m_predictedAltitude;
}

float_t Observer::getVeritcalVelocity() const{
    return m_predictedVerticalVelocity;
}

float_t Observer::getVerticalAcceleration() const{
    return m_predictedVerticalAcceleration;
}

float_t Observer::getAngleToHorizontal() const{
    return m_predictedAngleToHorizontal;
}

//implementation of helpers

//implementation of references
float_t& Observer::getPredictedAltitudeRef(){
    return m_predictedAltitude;
}

float_t& Observer::getMeasuredAltitudeRef(){
    return m_measuredAltitude;
}

float_t& Observer::getPredictedVerticalVelocityRef(){
    return m_predictedVerticalVelocity;
}

float_t& Observer::getPredictedVerticalAccelerationRef(){
    return m_predictedVerticalAcceleration;
}

float_t& Observer::getMeasuredVerticalAccelerationRef(){
    return m_measuredVerticalAcceleration;
}

float_t& Observer::getPredictedAngleRef(){
    return m_predictedAngleToHorizontal;
}

float_t& Observer::getMeasuredAngleRef(){
    return m_measuredAngleToHorizontal;
}

//read only references for telemetry
const float_t& Observer::getPredictedAltitudeRef() const{
    return m_predictedAltitude;
}

const float_t& Observer::getMeasuredAltitudeRef() const{
    return m_measuredAltitude;
}

const float_t& Observer::getMeasuredTemperatureRef() const{
    return m_measuredTemperature;
}

const float_t& Observer::getMeasuredPressureRef() const{
    return m_measuredPressure;
}

const float_t& Observer::getPredictedVerticalVelocityRef() const{
    return m_predictedVerticalVelocity;
}

const float_t& Observer::getPredictedVerticalAccelerationRef() const{
    return m_predictedVerticalAcceleration;
}

const float_t& Observer::getMeasuredVerticalAccelerationRef() const{
    return m_measuredVerticalAcceleration;
}

const Sensors::Vector3& Observer::getMeasuredLinearAccelerationRef() const{
    return m_measuredLinearAcceleration;
}

const float_t& Observer::getPredictedAngleRef() const{
    return m_predictedAngleToHorizontal;
}

const float_t& Observer::getMeasuredAngleRef() const{
    return m_measuredAngleToHorizontal;
}

const Sensors::Vector3& Observer::getMeasuredGravityRef() const{
    return m_measuredGravity;
}

const Sensors::Vector3& Observer::getMeasuredRotationRef() const{
    return m_measuredRotation;
}

const Sensors::Quaternion& Observer::getMeasuredOrientationRef() const{
    return m_measuredOrientation;
}