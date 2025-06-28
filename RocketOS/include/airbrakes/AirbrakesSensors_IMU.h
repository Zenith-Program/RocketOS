#pragma once
#include "AirbrakesGeneral.h"
#include "RocketOS.h"
#include <SPI.h>
#include <array>

namespace Airbrakes{
    namespace Sensors{
        enum class IMUStates{
            Uninitialized, Asleep, Operational
        };

        class BNO085_SPI{
        private:
            const char* const m_name;
            uint_t m_SPIFrequency;
            IMUStates m_state;
            std::array<uint8_t, Airbrakes_CFG_IMUBufferSize> m_rxBuffer;
            std::array<uint8_t, Airbrakes_CFG_IMUBufferSize> m_txBuffer;


        public:
            BNO085_SPI(const char*, uint_t);
            void makeCurrentInstance();
            error_t initialize();
            bool initialized() const;

        private:
            void reset();
            error_t wake();
            void serviceInterrupt();
            
        };
    }
}