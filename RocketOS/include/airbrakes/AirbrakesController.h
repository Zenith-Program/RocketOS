#pragma once
#include "RocketOS.h"
#include "AirbrakesGeneral.h"
#include "AirbrakesFlightPlan.h"
#include "AirbrakesObserver.h"
#include "AirbrakesActuator.h"
#include <IntervalTimer.h>

namespace Airbrakes{
    namespace Controls{
        class Controller{
        private:
            const char* const m_name;
            const FlightPlan& m_flightPlan;
            const Observer& m_observer;
            Motor::Actuator& m_motor;

            //control signals
            float_t m_error, m_flightPath, m_flightPathVelocityPartial, m_flightPathAnglePartial, m_updateRuleDragArea, m_adjustedDragArea, m_requestedDragArea, m_currentDragArea;

            //state flags
            bool m_updateRuleClamped, m_isSaturated, m_fault;

            //parameters
            float_t m_decayRate, m_updateRuleShutdownVelocity;
            
            //timing
            uint_t m_clockPeriod;
            IntervalTimer m_clock;
            bool m_isActive;
            
        public:
            Controller(const char* name, uint_t clockPeriod, const FlightPlan& plan, const Observer& observer, Motor::Actuator& motor, float_t decayDate);

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
            const float_t& getCurrentDragRef() const;

            const bool& getClampFlagRef() const;
            const bool& getSaturationFlagRef() const;
            const bool& getFaultFlagRef() const;

            float_t& getDecayRateRef();
            float_t& getCoastVelocityRef();

        private:
            float_t airDensity(float_t altitude) const;
            float_t updateRule(float_t error, float_t verticalVelocity, float_t angle, float_t altitude, float_t velocityPartial, float_t anglePartial) const;
            float_t getBestPossibleDragArea(float_t dragArea, float_t error) const;
            result_t<float_t> convertDragAreaToMotorPosition(float_t) const;
            result_t<float_t> convertMotorPositionToDragArea(float_t) const;
            error_t newFlight();

            static float_t sinSquared(float_t);
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

                // === DECAY RATE COMMAND LIST ===
                    //command list
                    const std::array<Command, 2> c_decayCommands{
                        Command{"", "", [this](arg_t){
                            Serial.println(m_decayRate);
                        }},
                        Command{"set", "f", [this](arg_t args){
                            float_t newDecayRate = args[0].getFloatData();
                            if(newDecayRate >=0) Serial.println("Warning: Decay rate should probably be negative");
                            m_decayRate = newDecayRate;
                        }}
                    };
                // ===============================

                // === COAST VELOCITY COMMAND LIST ===
                    //command list
                    const std::array<Command, 2> c_coastCommands{
                        Command{"", "", [this](arg_t){
                            Serial.print(m_updateRuleShutdownVelocity);
                            Serial.println("m/s");
                        }},
                        Command{"set", "f", [this](arg_t args){
                            float_t newVelocity = args[0].getFloatData();
                            if(newVelocity < 0) Serial.println("Value should be positive");
                            else m_updateRuleShutdownVelocity = newVelocity;
                        }}
                    };
                // ===================================

            // --------------------------------
            // subcommand list
            const std::array<CommandList, 3> c_rootChildren{
                CommandList{"period", c_periodCommands.data(), c_periodCommands.size(), nullptr, 0},
                CommandList{"decay", c_decayCommands.data(), c_decayCommands.size(), nullptr, 0},
                CommandList{"coast", c_coastCommands.data(), c_coastCommands.size(), nullptr, 0}
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