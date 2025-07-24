#pragma once
#include "RocketOS.h"
#include "AirbrakesGeneral.h"
#include "AirbrakesSensors_IMU.h"
#include "AirbrakesSensors_Altimeter.h"
#include <IntervalTimer.h>

namespace Airbrakes{

    enum class ObserverModes{
        Sensor, FilteredSimulation, FullSimulation
    };

    class Observer{
    public:
        //error codes
        static constexpr error_t ERROR_AltimeterInitialization = error_t(2);
        static constexpr error_t ERROR_IMUInitialization = error_t(3);
    private:
        static constexpr uint_t c_SamplePeriod_us = (Airbrakes_CFG_ObserverAltimeterMinSamplePeriod_us > Airbrakes_CFG_ObserverIMUMinSamplePeriod_us)? Airbrakes_CFG_ObserverAltimeterMinSamplePeriod_us : Airbrakes_CFG_ObserverIMUMinSamplePeriod_us;
        static constexpr uint_t c_FilterOrder = 2 * Airbrakes_CFG_ObserverFilterDelay_us / c_SamplePeriod_us;
        //state
        ObserverModes m_mode;
        IntervalTimer m_timer;

        //sensors
        Sensors::BNO085_SPI& m_imu;
        Sensors::MS5607_SPI& m_altimeter;

        //processing
        RocketOS::Processing::Differentiator<c_FilterOrder> m_verticalVelocityFilter;
        RocketOS::Processing::LowPass<c_FilterOrder> m_altitudeFilter;
        RocketOS::Processing::LowPass<c_FilterOrder> m_accelerationFilter;
        RocketOS::Processing::LowPass<c_FilterOrder> m_angleFilter;

        //values used by the controller 
        float_t m_predictedAltitude;
        float_t m_predictedVerticalVelocity;
        float_t m_predictedVerticalAcceleration;
        float_t m_predictedAngleToHorizontal;

        //raw readings
        float_t m_measuredAltitude;
        float_t m_measuredPressure;
        float_t m_measuredTemperature;
        Sensors::Vector3 m_measuredLinearAcceleration;
        float_t m_measuredVerticalAcceleration;
        Sensors::Vector3 m_measuredRotation;
        Sensors::Vector3 m_measuredGravity;
        Sensors::Quaternion m_measuredOrientation;
        float_t m_measuredAngleToHorizontal;

    public:
        Observer(Sensors::BNO085_SPI&, Sensors::MS5607_SPI&);
        error_t setMode(ObserverModes);

        //controller interface
        float_t getAltitude() const;
        float_t getVeritcalVelocity() const;
        float_t getVerticalAcceleration() const;
        float_t getAngleToHorizontal() const;

        //references for HIL overides
        float_t& getPredictedAltitudeRef();
        float_t& getMeasuredAltitudeRef();
        float_t& getPredictedVerticalVelocityRef();
        float_t& getPredictedVerticalAccelerationRef();
        float_t& getMeasuredVerticalAccelerationRef();
        float_t& getPredictedAngleRef();
        float_t& getMeasuredAngleRef();

        //read only references for telemetry
        const float_t& getPredictedAltitudeRef() const;
        const float_t& getMeasuredAltitudeRef() const;
        const float_t& getMeasuredTemperatureRef() const;
        const float_t& getMeasuredPressureRef() const;
        const float_t& getPredictedVerticalVelocityRef() const;
        const float_t& getPredictedVerticalAccelerationRef() const;
        const float_t& getMeasuredVerticalAccelerationRef() const;
        const Sensors::Vector3& getMeasuredLinearAccelerationRef() const;
        const float_t& getPredictedAngleRef() const;
        const float_t& getMeasuredAngleRef() const;
        const Sensors::Vector3& getMeasuredGravityRef() const;
        const Sensors::Vector3& getMeasuredRotationRef() const;
        const Sensors::Quaternion& getMeasuredOrientationRef() const;

    private:
        void sensorModeTimerISR();
        void filterSimModeTimerISR();
        error_t setupSensors();
        void updateFilters();
        void readSensors();

    private:
        // ######### command structure #########
        using Command = RocketOS::Shell::Command;
        using CommandList = RocketOS::Shell::CommandList;
        using arg_t = RocketOS::Shell::arg_t;

        // === ROOT COMMAND LIST ===

        // =========================
    };
}