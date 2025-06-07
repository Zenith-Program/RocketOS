#pragma once
#include "RocketOS.h"
#include "AirbrakesGeneral.h"
#include <IntervalTimer.h>

namespace Airbrakes{
    namespace Controls{
        class DemoController{
        private:
            const char* const m_name;
            float_t m_altitude, m_deployment, m_gain;
            uint_t m_clockPeriod;
            IntervalTimer m_clock;
            bool m_isActive;
            
        public:
            DemoController(const char* name, uint_t clockPeriod) : m_name(name), m_gain(0), m_clockPeriod(clockPeriod), m_isActive(false){}

            RocketOS::Shell::CommandList getCommands() const{
                return {"controller", c_rootCommands.data(), c_rootCommands.size(), c_rootChildren.data(), c_rootChildren.size()};
            }

            void start(){
                m_clock.end();
                m_clock.begin([this](){m_deployment = -m_gain * m_altitude;}, m_clockPeriod);
                m_isActive = true;
            }

            void stop(){
                m_clock.end();
                m_isActive = false;
            }

            //acessors to references for peristent storage, telemetry and HIL systems
            auto& getAltitudeRef(){
                return m_altitude;
            }

            const auto& getDeploymentRef(){
                return m_deployment;
            }

            auto& getGainRef(){
                return m_gain;
            }

            auto& getClockPeriodRef(){
                return m_clockPeriod;
            }

            auto& getActiveFlagRef(){
                return m_isActive;
            }

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
                        Serial.println(" ms");
                    }},
                    Command{"set", "u", [this](arg_t args){
                        m_clockPeriod = args[0].getUnsignedData();
                        if(m_isActive) start();
                    }}
                };
                // ===========================

                // === GAIN COMMAND LIST ===
                // command list
                const std::array<Command, 2> c_gainCommands{
                    Command{"", "", [this](arg_t){
                        Serial.println(m_gain);
                    }},
                    Command{"set", "f", [this](arg_t args){
                        float_t newGain = args[0].getFloatData();
                        if(newGain >= 0) m_gain = newGain;
                    }}
                };
                //==========================
            // --------------------------------
            // subcommand list
            const std::array<CommandList, 2> c_rootChildren{
                CommandList{"period", c_periodCommands.data(), c_periodCommands.size(), nullptr, 0},
                CommandList{"gain", c_gainCommands.data(), c_gainCommands.size(), nullptr, 0}
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