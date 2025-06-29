#pragma once
#include "AirbrakesGeneral.h"
#include "RocketOS.h"
#include <SPI.h>
#include <array>

namespace Airbrakes{
    namespace Sensors{
        enum class IMUStates{
            Uninitialized, Asleep, Startup, Operational
        };

        class BNO085_SPI{
        private:
            static constexpr uint_t c_numSHTPChannels = 5;
            const char* const m_name;
            uint_t m_SPIFrequency;
            IMUStates m_state;
            std::array<uint8_t, Airbrakes_CFG_IMUBufferSize> m_rxBuffer;
            std::array<uint8_t, Airbrakes_CFG_IMUBufferSize> m_txBuffer;
            std::array<uint_t, c_numSHTPChannels> m_sequenceNumbers;


        public:
            BNO085_SPI(const char*, uint_t);
            void makeCurrentInstance();
            error_t initialize();
            IMUStates state() const;

        private:
            struct SHTPHeader{
                uint16_t length;
                uint8_t channel;
                bool continuation;
            };
        private:
            void reset();
            error_t wake();
            void serviceInterrupt();

            result_t<SHTPHeader> readSHTP();
            error_t sendSHTP(SHTPHeader);
            void flushChunck();

            void respondToPacket(SHTPHeader);

            
        };
    }
}