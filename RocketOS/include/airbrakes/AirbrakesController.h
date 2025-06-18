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
            //controller parameters
            float_t m_proportionalGain;
            float_t m_derivativeGain;

            //control output
            float_t m_requestedDragArea;
            float_t m_desiredVerticalAcceleration;

            uint_t m_clockPeriod;
            IntervalTimer m_clock;
            bool m_isActive;
            
        public:
            Controller(const char* name, uint_t clockPeriod, const FlightPlan& plan, const Observer& observer);

            RocketOS::Shell::CommandList getCommands() const;

            void start();
            void stop();
            void resetInit();
            bool isActive();

            void clock();


            //acessors to references for peristent storage, telemetry and HIL systems
            uint_t& getClockPeriodRef();
            bool& getActiveFlagRef();
            float_t& getPGainRef();
            float_t& getDGainRef();
            const float_t& getRequestedDragAreaRef() const;
            const float_t& getDesiredAccelRef() const;

        private:
            float_t directionalFlightPathDerivative(float_t currentVerticalVelocity, float_t currentAngleToHorizontal, float_t verticalAcceleration, float_t angularVelocity) const;
            float_t airDensity(float_t altitude) const;
            float_t getDragAreaFromAcceleration(float_t desiredVeritcalAcceleration) const;
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

                // === GAIN COMMAND LIST ===
                    // children command lists
                    // === P COMMAND LIST ===
                        //commands
                        const std::array<Command, 2> c_PGainCommands = {
                            Command{"", "", [this](arg_t){
                                Serial.println(m_proportionalGain);
                            }},
                            Command{"set", "f", [this](arg_t args){
                                m_proportionalGain = args[0].getFloatData();
                            }}
                        };
                    // ======================

                    // === D COMMAND LIST ===
                        //commands
                        const std::array<Command, 2> c_DGainCommands{
                            Command{"", "", [this](arg_t){
                                Serial.println(m_derivativeGain);
                            }},
                            Command{"set", "f", [this](arg_t args){
                                m_derivativeGain = args[0].getFloatData();
                            }}
                        };
                    // ======================
                    //command lists
                    std::array<CommandList, 2> c_gainCommandLists{
                        CommandList{"p", c_PGainCommands.data(), c_PGainCommands.size(), nullptr, 0},
                        CommandList{"d", c_DGainCommands.data(), c_DGainCommands.size(), nullptr, 0}
                    };
                    //commands
                    std::array<Command, 2> c_gainCommands{
                        Command{"", "", [this](arg_t){
                            Serial.print("P: ");
                            Serial.println(m_proportionalGain);
                            Serial.print("D: ");
                            Serial.println(m_derivativeGain);
                        }},
                        Command{"set", "ff", [this](arg_t args){
                            m_proportionalGain = args[0].getFloatData();
                            m_derivativeGain = args[1].getFloatData();
                        }}
                    };
                // =========================

            // --------------------------------
            // subcommand list
            const std::array<CommandList, 2> c_rootChildren{
                CommandList{"period", c_periodCommands.data(), c_periodCommands.size(), nullptr, 0},
                CommandList{"gain", c_gainCommands.data(), c_gainCommands.size(), c_gainCommandLists.data(), c_gainCommandLists.size()}
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