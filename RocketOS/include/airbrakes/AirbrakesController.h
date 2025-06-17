#pragma once
#include "RocketOS.h"
#include "AirbrakesGeneral.h"
#include "AirbrakesFlightPlan.h"
#include <IntervalTimer.h>

namespace Airbrakes{
    namespace Controls{
        class DemoController{
        private:
            const char* const m_name;
            const FlightPlan& m_flightPlan;

            //inputs
            const float_t& m_currentAltitude;
            const float_t& m_currentVerticalVelocity;
            const float_t& m_currentHorizontalVelocity;
            const float_t& m_currentAngle;

            //outputs
            float_t m_requestedDragArea;

            uint_t m_clockPeriod;
            IntervalTimer m_clock;
            bool m_isActive;
            
        public:
            DemoController(const char* name, uint_t clockPeriod, const FlightPlan& plan, const float_t& altitude, const float_t& xVelocity, const float_t& yVelocity, const float_t& angle);

            RocketOS::Shell::CommandList getCommands() const;

            void start();
            void stop();
            void resetInit();
            bool isActive();

            void clock();


            //acessors to references for peristent storage, telemetry and HIL systems
            auto& getClockPeriodRef();
            auto& getActiveFlagRef();

        private:
            // ######### command structure #########
            using Command = RocketOS::Shell::Command;
            using CommandList = RocketOS::Shell::CommandList;
            using arg_t = RocketOS::Shell::arg_t;
            // === ROOT COMAND LIST ===
            // children command lists ---------
                // === PERIOD COMMAND LIST ===
                //command list
                const std::array<Command, 2> c_periodCommands{
                    Command{"", "", [this](arg_t){
                        Serial.print(m_clockPeriod);
                        Serial.println(" us");
                    }},
                    Command{"set", "u", [this](arg_t args){
                        m_clockPeriod = args[0].getUnsignedData();
                        if(m_isActive) start();
                    }}
                };
                // ===========================

            // --------------------------------
            // subcommand list
            const std::array<CommandList, 1> c_rootChildren{
                CommandList{"period", c_periodCommands.data(), c_periodCommands.size(), nullptr, 0}
            };
            // command list
            const std::array<Command, 2> c_rootCommands{
                Command{"start", "", [this](arg_t){
                    start();
                }},
                Command{"stop", "", [this](arg_t){
                    stop();
                }}
            };
            // =========================
            

        };
    }
}