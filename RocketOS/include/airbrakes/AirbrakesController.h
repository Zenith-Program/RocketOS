#pragma once
#include "RocketOS.h"
#include "AirbrakesGeneral.h"
#include "AirbrakesFlightPlan.h"
#include "AirbrakesObserver.h"
#include <IntervalTimer.h>

namespace Airbrakes{
    namespace Controls{
        class Controller{
        private:
            const char* const m_name;
            const FlightPlan& m_flightPlan;
            const Observer& m_observer;

            //control signals
            float_t m_error, m_flightPath, m_flightPathVelocityPartial, m_flightPathAnglePartial, m_updateRuleDragArea, m_adjustedDragArea, m_requestedDragArea;

            //state flags
            bool m_updateRuleClamped, m_isSaturated, m_fault;

            //parameters
            float_t m_decayRate, m_updateRuleShutdownVelocity;
            
            //timing
            uint_t m_clockPeriod;
            IntervalTimer m_clock;
            bool m_isActive;
            
        public:
            Controller(const char* name, uint_t clockPeriod, const FlightPlan& plan, const Observer& observer);

            RocketOS::Shell::CommandList getCommands() const;

            error_t start();
            void stop();
            void resetInit();
            bool isActive();

            void clock();


            //acessors to references for peristent storage, telemetry and HIL systems
            uint_t& getClockPeriodRef();
            bool& getActiveFlagRef();

            const float_t& getErrorRef() const;
            const float_t& getFlightPathRef() const;
            const float_t& getVPartialRef() const;
            const float_t& getAnglePartialRef() const;
            const float_t& getUpdateRuleDragRef() const;
            const float_t& getAdjustedDragRef() const;
            const float_t& getRequestedDragRef() const;

            const bool& getClampFlagRef() const;
            const bool& getSaturationFlagRef() const;
            const bool& getFaultFlagRef() const;

        private:
            float_t airDensity(float_t altitude) const;
            float_t updateRule(float_t error, float_t verticalVelocity, float_t angle, float_t altitude, float_t velocityPartial, float_t anglePartial) const;
            float_t getBestPossibleDragArea(float_t dragArea, float_t error) const;
            error_t newFlight();
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
                CommandList{"period", c_periodCommands.data(), c_periodCommands.size(), nullptr, 0},
            };
            // command list
            const std::array<Command, 2> c_rootCommands{
                Command{"start", "", [this](arg_t){
                    if(start() != error_t::GOOD) Serial.println("Error starting the controller");//log message
                }},
                Command{"stop", "", [this](arg_t){
                    stop();
                }}
            };
            // =========================
            

        };
    }
}