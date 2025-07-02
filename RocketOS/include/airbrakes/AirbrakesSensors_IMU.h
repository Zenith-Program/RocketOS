#pragma once
#include "AirbrakesGeneral.h"
#include "RocketOS.h"
#include <SPI.h>
#include <array>
#include <TeensyTimerTool.h>

namespace Airbrakes{
    namespace Sensors{

        enum class IMUStates{
            Uninitialized, Reseting, Configuring, Operational
        };

        enum class IMUSensorStatus{
            Disabled, Unreliable, LowAccuracy, ModerateAccuracy, HighAccuracy
        };

        struct Vector3{
            float_t x; 
            float_t y; 
            float_t z;
        };

        struct Quaternion{
            float_t r;
            float_t i;
            float_t j;
            float_t k;
        };

        class BNO085_SPI{
        private:
            //structures

            enum class IMUData{
                Orientation, Rotation, LinearAcceleration, Gravity
            };

            struct SHTPHeader{
                uint16_t length;
                uint8_t channel;
                bool continuation;
            };

            using txCallback_t = RocketOS::inplaceFunction_t<SHTPHeader(void), Airbrakes_CFG_IMUTxCallbackCaptureSize>;

        private:
            //error codes
            static constexpr error_t ERROR_HeaderLength = error_t(5);
            static constexpr error_t ERROR_HeaderChannel = error_t(6);
            static constexpr error_t ERROR_RxBufferOverflow = error_t(7);
            static constexpr error_t ERROR_ContinuationPacket = error_t(8);
        public:
            static constexpr error_t ERROR_ResetTimeout = error_t(2);
            static constexpr error_t ERROR_ConfigurationTimeout = error_t(3);
            static constexpr error_t ERROR_WakeTimeout = error_t(4);

        private:
            //constants
            static constexpr uint_t c_numSHTPChannels = 6;

            //data
            const char* const m_name;
            uint_t m_SPIFrequency;
            IMUStates m_state;
            bool m_resetComplete, m_hubInitialized, m_waking;
            std::array<uint8_t, Airbrakes_CFG_IMUBufferSize> m_rxBuffer;
            std::array<uint8_t, Airbrakes_CFG_IMUBufferSize> m_txBuffer;
            std::array<uint_t, c_numSHTPChannels> m_sequenceNumbers;
            RocketOS::Utilities::Queue<txCallback_t, Airbrekes_CFG_IMUTxQueueSize> m_txQueue;
            TeensyTimerTool::OneShotTimer m_timer;

            Vector3 m_currentLinearAcceleration;
            Vector3 m_currentAngularVelocity;
            Vector3 m_currentGravity;
            Quaternion m_currentOrientation;
            IMUSensorStatus m_LinearAccelerationStatus, m_angularVelocityStatus, m_gravityStatus, m_orientationStatus;
            uint32_t m_LinearAccelerationSamplePeriod_us, m_angularVelocitySamplePeriod_us, m_gravitySamplePeriod_us, m_orientationSamplePeriod_us;

        public:
            //interface
            BNO085_SPI(const char*, uint_t, uint32_t, TeensyTimerTool::TimerGenerator*);
            error_t initialize();
            IMUStates getState() const;
            RocketOS::Shell::CommandList getCommands();

        private:

            //helpers
            void resetAsync();
            void wakeAsync();
            void serviceInterrupt();
            void serviceTimer();
            void debugPrintRx(SHTPHeader, bool = true);
            void debugPrintTx(SHTPHeader, bool = true);

            //SHTP communication
            result_t<SHTPHeader> doSHTP(SHTPHeader);

            //packet handling
            void respondToPacket(SHTPHeader);   
            bool handleInitializeResponse(SHTPHeader);
            bool handleResetComplete(SHTPHeader);
            bool handleFeatureResponse(IMUData, SHTPHeader);

            //configuration
            SHTPHeader generateFeatureCommand(IMUData);
            txCallback_t makeFeatureCallback(IMUData);
            SHTPHeader generateFeatureResponseCommand(IMUData);
            txCallback_t makeFeatureResponseCallback(IMUData);

            static uint8_t getReportID(IMUData);
            uint_t& getSamplePeriod(IMUData);
            IMUSensorStatus& getStatus(IMUData);

        private:
            // ######### command structure #########
            using Command = RocketOS::Shell::Command;
            using CommandList = RocketOS::Shell::CommandList;
            using arg_t = RocketOS::Shell::arg_t;

            // === ROOT COMMAND LIST ===
                //child command lists

                //sub command list

                //commands
                const std::array<Command, 2> c_rootCommands{
                    Command{"wake", "", [this](arg_t){
                        wakeAsync();
                    }},
                    Command{"status", "", [this](arg_t){
                        IMUStates state = getState();
                        if(state == IMUStates::Uninitialized) Serial.println("Uninitialized");
                        if(state == IMUStates::Reseting) Serial.println("Reseting");
                        if(state == IMUStates::Configuring) Serial.println("Configuring");
                        if(state == IMUStates::Operational) Serial.println("Operational");
                    }}
                };
        };
    }
}