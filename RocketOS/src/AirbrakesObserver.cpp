#include "airbrakes\AirbrakesObserver.h"

using namespace Airbrakes;
//implementation of interface

Observer::Observer(Sensors::BNO085_SPI& imu, Sensors::MS5607_SPI& altimeter) : m_mode(ObserverModes::FullSimulation), m_imu(imu), m_altimeter(altimeter) {}

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
        m_timer.begin([this](){this->filterSimModeTimerISR();}, c_SamplePeriod_us);
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
        m_timer.begin([this](){this->sensorModeTimerISR();}, c_SamplePeriod_us);
        m_mode = ObserverModes::Sensor;
        return error_t::GOOD;
    }
    return error_t::ERROR;
}

//helpers
void Observer::sensorModeTimerISR(){
    readSensors();
    updateFilters();
    m_altimeter.updateAsync();
}

void Observer::filterSimModeTimerISR(){
    updateFilters();
}

void Observer::updateFilters(){
    //compute vertical velocity (derivative of altitude)
    m_verticalVelocityFilter.push(m_measuredAltitude);
    m_predictedVerticalVelocity = m_verticalVelocityFilter.output() / (c_SamplePeriod_us / 1000000.0);
    //compute altitude (lowpass of barometer reading)
    m_altitudeFilter.push(m_measuredAltitude);
    m_predictedAltitude = m_altitudeFilter.output();
    //compute acceleratrion (lowpass of IMU accelerometer)
    m_accelerationFilter.push(m_measuredVerticalAcceleration);
    m_predictedVerticalAcceleration = m_accelerationFilter.output();
    //compute angle to horizontal (lowpass of angle calculated from gravity)
    m_angleFilter.push(m_measuredAngleToHorizontal);
    m_predictedAngleToHorizontal = m_angleFilter.output();
}

error_t Observer::setupSensors(){
    error_t error = error_t::GOOD;
    if(!m_altimeter.initialized()){
        if(m_altimeter.initialize() != error_t::GOOD) error = ERROR_AltimeterInitialization;
    }
    if(m_imu.getState() != Sensors::IMUStates::Operational){
        if(m_imu.initialize() != error_t::GOOD) return ERROR_IMUInitialization;
    }
    m_imu.setSamplePeriod_us(c_SamplePeriod_us/2, Sensors::IMUData::AngularVelocity);
    m_imu.setSamplePeriod_us(c_SamplePeriod_us/2, Sensors::IMUData::LinearAcceleration);
    m_imu.setSamplePeriod_us(c_SamplePeriod_us/2, Sensors::IMUData::Orientation);
    m_imu.setSamplePeriod_us(c_SamplePeriod_us/2, Sensors::IMUData::Gravity);
    return error;
}

void Observer::readSensors(){
    //read altimeter
    m_measuredAltitude = m_altimeter.getLastAltitude();
    m_measuredPressure = m_altimeter.getLastPressure();
    m_measuredTemperature = m_altimeter.getLastTemperature();
    //read imu
    m_measuredLinearAcceleration = m_imu.getLastLinearAcceleration();
    m_measuredRotation = m_imu.getLastAngularVelocity();
    m_measuredGravity = m_imu.getLastGravity();
    m_measuredOrientation = m_imu.getLastOrientation();
    m_measuredVerticalAcceleration = m_measuredLinearAcceleration.z;
    m_measuredAngleToHorizontal = acos(m_measuredGravity.z / m_measuredGravity.magnitude());
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