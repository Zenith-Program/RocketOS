#pragma once
#include "AirbrakesGeneral.h"
#include "RocketOS.h"
#include <SPI.h>
#include <array>

namespace Airbrakes{
    namespace Sensors{
        enum class IMUStates{
            Uninitialized, Reseting, 
            StartOrientationConfiguration, DoingOrientationConfiguration, 
            StartAngularVelocityConfiguration, DoingAngularVelocityConfiguration, 
            StartLinearAccelerationConfiguration, DoingLinearAccelerationConfiguration, 
            StartGravityConfiguration, DoingGravityConfiguration, 
            Operational
        };

        enum class IMUData{
            Orientation, Rotation, LinearAcceleration, Gravity
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
            static constexpr uint_t c_numSHTPChannels = 5;

            //data
            const char* const m_name;
            uint_t m_SPIFrequency;
            IMUStates m_state;
            bool m_resetComplete, m_hubInitialized, m_waking;
            std::array<uint8_t, Airbrakes_CFG_IMUBufferSize> m_rxBuffer;
            std::array<uint8_t, Airbrakes_CFG_IMUBufferSize> m_txBuffer;
            std::array<uint_t, c_numSHTPChannels> m_sequenceNumbers;

            Vector3 m_currentLinearAcceleration;
            Vector3 m_currentAngularVelocity;
            Vector3 m_currentGravity;
            Quaternion m_currentOrientation;

            uint_t m_samplePeriod_us;

        public:
            //interface
            BNO085_SPI(const char*, uint_t);
            error_t initialize();
            void sleep();
            IMUStates state() const;
            RocketOS::Shell::CommandList getCommands();

        private:
            //structures
            struct SHTPHeader{
                uint16_t length;
                uint8_t channel;
                bool continuation;
            };

            //helpers
            void resetAsync();
            void wakeAsync();
            void serviceInterrupt();

            //SHTP communication
            result_t<SHTPHeader> readSHTP();
            error_t sendSHTP(SHTPHeader);

            //packet handling
            void respondToPacket(SHTPHeader);   
            bool handleInitializeResponse(SHTPHeader);
            bool handleResetComplete(SHTPHeader);
            bool handleGravityFeatureResponse(SHTPHeader);
            bool handleOrientationFeatureResponse(SHTPHeader);
            bool handleAngularVelocityFeatureResponse(SHTPHeader);
            bool handleLinearAccelerationFeatureResponse(SHTPHeader);

            //configuration
            SHTPHeader generateFeatureCommand(IMUData);
            SHTPHeader generateFeatureResponseCommand(IMUData);
            static uint8_t getReportID(IMUData);

        private:
            // ######### command structure #########
            using Command = RocketOS::Shell::Command;
            using CommandList = RocketOS::Shell::CommandList;
            using arg_t = RocketOS::Shell::arg_t;

            // === ROOT COMMAND LIST ===
                //child command lists

                //sub command list

                //commands
                const std::array<Command, 1> c_rootCommands{
                    Command{"wake", "", [this](arg_t){
                        wakeAsync();
                    }}
                };
        };
    }
}