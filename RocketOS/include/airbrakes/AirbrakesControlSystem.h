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
            float_t m_altitude, m_velocity, m_angle;
            uint_t m_clockPeriod;
            IntervalTimer m_clock;
            bool m_isActive;

            RocketOS::Processing::FIRFilter<5> m_filter;
            RocketOS::Processing::AccumulationFilter<10, RocketOS::Processing::AccumulationFilterTypes::UNORDERED> m_linearCovariance;
            
        public:
            DemoController(const char* name, const FlightPlan& plan, uint_t clockPeriod) : m_name(name), m_flightPlan(plan), m_clockPeriod(clockPeriod), m_isActive(false), m_filter({3,-2,-8,2,5}), m_linearCovariance([](float_t a){
                return a*a;
            }){}

            RocketOS::Shell::CommandList getCommands() const{
                return {"controller", c_rootCommands.data(), c_rootCommands.size(), c_rootChildren.data(), c_rootChildren.size()};
            }

            void start(){
                m_clock.end();
                m_clock.begin([this](){this->clock();}, m_clockPeriod);
                m_isActive = true;
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
                auto val = m_flightPlan.getAltitude(m_velocity, m_angle);
                if(val.error != error_t::GOOD) {
                    Serial.println((uint_t)val.error);
                    return;
                }
                m_altitude = val.data;
            }


            //acessors to references for peristent storage, telemetry and HIL systems
            const auto& getAltitudeRef(){
                return m_altitude;
            }

            auto& getClockPeriodRef(){
                return m_clockPeriod;
            }

            auto& getActiveFlagRef(){
                return m_isActive;
            }

            auto& getVelocityRef(){
                return m_velocity;
            }

            auto& getAngleRef(){
                return m_angle;
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