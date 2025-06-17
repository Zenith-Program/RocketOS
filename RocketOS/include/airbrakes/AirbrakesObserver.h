#pragma once
#include "AirbrakesGeneral.h"

namespace Airbrakes{
    class Observer{
    private:
        float_t m_predictedAltitude;
        float_t m_predictedYVelocity;
        float_t m_predictedXVelocity;
        float_t m_predictedAngle;
    public:

        //reference acesors for HIL & Telementry
        float_t& getAltitudeRef();
        float_t& getVerticalVelocityRef();
        float_t& getHorizontalVelocityRef();
        float_t& getAngleRef();

        const float_t& getAltitudeRef() const;
        const float_t& getVerticalVelocityRef() const;
        const float_t& getHorizontalVelocityRef() const;
        const float_t& getAngleRef() const;

        //read only acessors
        float_t getAltitude() const;
        float_t getVeritcalVelocity() const;
        float_t getHorizontalVelocity() const;
        float_t getAngleToHorizontal() const;

    };
}