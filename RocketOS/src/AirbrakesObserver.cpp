#include "airbrakes\AirbrakesObserver.h"

using namespace Airbrakes;

//reference acesors for HIL & Telementry
float_t& Observer::getAltitudeRef(){
    return m_predictedAltitude;
}

float_t& Observer::getVerticalVelocityRef(){
    return m_predictedYVelocity;
}

float_t& Observer::getHorizontalVelocityRef(){
    return m_predictedXVelocity;
}

float_t& Observer::getVerticalAccelerationRef(){
    return m_predictedYAccel;
}

float_t& Observer::getHorizontalAccelerationRef(){
    return m_predictedXAccel;
}

float_t& Observer::getAngleRef(){
    return m_predictedAngle;
}

float_t& Observer::getAngularVelocityRef(){
    return m_predictedAngularVelocity;
}

const float_t& Observer::getAltitudeRef() const{
    return m_predictedAltitude;
}

const float_t& Observer::getVerticalVelocityRef() const{
    return m_predictedYVelocity;
}

const float_t& Observer::getHorizontalVelocityRef() const{
    return m_predictedXVelocity;
}

const float_t& Observer::getVerticalAccelerationRef() const{
    return m_predictedYAccel;
}

const float_t& Observer::getHorizontalAccelerationRef() const{
    return m_predictedXAccel;
}

const float_t& Observer::getAngleRef() const{
    return m_predictedAngle;
}

const float_t& Observer::getAngularVelocityRef() const{
    return m_predictedAngularVelocity;
}

//read only acessors
float_t Observer::getAltitude() const{
    return m_predictedAltitude;
}

float_t Observer::getVeritcalVelocity() const{
    return m_predictedYVelocity;
}

float_t Observer::getHorizontalVelocity() const{
    return m_predictedXVelocity;
}

float_t Observer::getVerticalAcceleration() const{
    return m_predictedYAccel;
}

float_t Observer::getHorizontalAcceleration() const{
    return m_predictedXAccel;
}

float_t Observer::getAngleToHorizontal() const{
    return m_predictedAngle;
}

float_t Observer::getAngularVelocityToHorizontal() const{
    return m_predictedAngularVelocity;
}