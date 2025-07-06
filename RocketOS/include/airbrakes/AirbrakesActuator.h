#pragma once
#include "AirbrakesGeneral.h"
#include "RocketOS.h"
#include <Encoder.h>

namespace Airbrakes{
    namespace Motor{
        class Actuator{
        private:
            const char* const m_name;
            Encoder m_encoder;
        public:
            Actuator(const char*);
            RocketOS::Shell::CommandList getCommands();

        private:
            // ######### command structure #########
            using Command = RocketOS::Shell::Command;
            using CommandList = RocketOS::Shell::CommandList;
            using arg_t = RocketOS::Shell::arg_t;

            // === ROOT COMMAND LIST ===
                //commands
                const std::array<Command, 2> c_rootCommands{
                    Command{"read", "", [this](arg_t){
                        Serial.println(m_encoder.read());
                    }},
                    Command{"zero", "", [this](arg_t){
                       m_encoder.write(0);
                    }}
                };
            // =========================
        };
    }
}