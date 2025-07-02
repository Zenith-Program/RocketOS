#pragma once
#include "AirbrakesGeneral.h"
#include "RocketOS.h"
#include <SPI.h>
#include <array>

namespace Airbrakes{
    namespace Sensors{
        enum class IMUStates{
            Uninitialized, Asleep, Waking, Reseting, StartOrientationConfiguration, DoingOrientationConfiguration, StartAngularVelocityConfiguration, DoingAngularVelocityConfiguration, StartLinearAccelerationConfiguration, DoingLinearAccelerationConfiguration, Operational
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
            std::array<uint8_t, Airbrakes_CFG_IMUBufferSize> m_rxBuffer;
            std::array<uint8_t, Airbrakes_CFG_IMUBufferSize> m_txBuffer;
            std::array<uint_t, c_numSHTPChannels> m_sequenceNumbers;

        public:
            //interface
            BNO085_SPI(const char*, uint_t);
            error_t initialize();
            void sleep();
            error_t wake();
            IMUStates state() const;

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
            bool handleOrientationFeatureResponse(SHTPHeader);
            bool handleAngularVelocityFeatureResponse(SHTPHeader);
            bool handleLinearAccelerationFeatureResponse(SHTPHeader);

            //configuration
            SHTPHeader generateOrientationFeatureCommand();
            SHTPHeader generateAngularVelocityFeatureCommand();
            SHTPHeader generateLinearAccelerationFeatureCommand();
            SHTPHeader generateOrientationFeatureResponseCommand();
            SHTPHeader generateAngularVelocityFeatureResponseCommand();
            SHTPHeader generateLinearAccelerationFeatureResponseCommand();
        };
    }
}