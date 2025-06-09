#pragma once
#include "RocketOS.h"
#include "AirbrakesGeneral.h"
#include <IntervalTimer.h>

namespace Airbrakes{
    namespace Controls{
        class DemoController{
        private:
            const char* const m_name;
            float_t m_altitude, m_deployment, m_Pgain, m_Dgain;
            uint_t m_clockPeriod;
            IntervalTimer m_clock;
            bool m_isActive;

            elapsedMicros m_clockDif;
            float_t m_prevAltitude, m_prevDifferentAltitude;
            float_t m_velocity;
            bool m_startup;
            float_t m_deltaT;
            
        public:
            DemoController(const char* name, uint_t clockPeriod) : m_name(name), m_Pgain(0), m_Dgain(0), m_clockPeriod(clockPeriod), m_isActive(false){}

            RocketOS::Shell::CommandList getCommands() const{
                return {"controller", c_rootCommands.data(), c_rootCommands.size(), c_rootChildren.data(), c_rootChildren.size()};
            }

            void start(){
                m_clock.end();
                m_clock.begin([this](){this->clock();}, m_clockPeriod);
                m_isActive = true;
                m_prevAltitude = 0;
                m_prevDifferentAltitude = 0;
                m_clockDif = 0;
                m_startup = true;
            }

            void stop(){
                m_clock.end();
                m_isActive = false;
            }

            void resetInit(){
                if(m_isActive) m_clock.begin([this](){this->clock();}, m_clockPeriod);
            }
            
            bool isActive(){
                return m_isActive;
            }

            void clock(){
                if(m_altitude != m_prevAltitude){
                    Serial.println(m_clockDif);
                    m_prevDifferentAltitude = m_prevAltitude;
                    m_prevAltitude = m_altitude;
                    m_deltaT = static_cast<float_t>(static_cast<uint_t>(m_clockDif))/1000000.0;
                    m_clockDif = 0;
                    if(m_prevAltitude != 0 && m_prevDifferentAltitude != 0) m_startup = false;
                    Serial.println(m_deltaT);
                    if(m_deltaT != 0 && !m_startup) m_velocity = (m_altitude - m_prevDifferentAltitude)/m_deltaT;
                    m_deployment = -m_Pgain * m_altitude - m_Dgain * m_velocity;
                    Serial.println(m_velocity);
                    Serial.println(m_deployment);
                }
                
            }


            //acessors to references for peristent storage, telemetry and HIL systems
            auto& getAltitudeRef(){
                return m_altitude;
            }

            const auto& getDeploymentRef(){
                return m_deployment;
            }

            auto& getPGainRef(){
                return m_Pgain;
            }

            auto& getDGainRef(){
                return m_Dgain;
            }

            auto& getClockPeriodRef(){
                return m_clockPeriod;
            }

            auto& getActiveFlagRef(){
                return m_isActive;
            }

            const auto& getVelocityRef(){
                return m_velocity;
            }

            const auto& getDeltaTRef(){
                return m_deltaT;
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
                        Serial.print("P: ");
                        Serial.println(m_Pgain);
                        Serial.print("D: ");
                        Serial.println(m_Dgain);
                    }},
                    Command{"set", "ff", [this](arg_t args){
                        float_t newPGain = args[0].getFloatData();
                        float_t newDGain = args[1].getFloatData();
                        if(newPGain >= 0) m_Pgain = newPGain;
                        if(newDGain >= 0) m_Dgain = newDGain;
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