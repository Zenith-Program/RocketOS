#pragma once
#include "AirbrakesGeneral.h"
#include "RocketOS.h"
#include "AirbrakesPersistent.h"
#include "AirbrakesTelemetry.h"
#include "AirbrakesController.h"
#include "AirbrakesFlightPlan.h"
#include "AirbrakesObserver.h"
#include "AirbrakesSensors_Altimeter.h"
#include "AirbrakesSensors_IMU.h"
#include <Arduino.h> //serial printing, elapsedmillis

namespace Airbrakes{

    class Application{
    private:
        // --- sensor readings ---
        Sensors::MS5607_SPI m_altimeter;
        Sensors::BNO085_SPI m_IMU;
        // --- control system ---
        Controls::Controller m_controller;
        Controls::FlightPlan m_flightPlan;
        Observer m_observer;

        // --- sd card systems ---
        SdFat m_sdCard;
        DataLogWithCommands<
            float_t,    //observed altitude
            float_t,    //observed velocity x
            float_t,    //observed velocity y
            float_t,    //observed angle
            float_t,    //observed angular velocity
            float_t,    //controller error
            float_t,    //controller flight path
            float_t,    //controller flight path velocity partial
            float_t,    //controller flight path angle partial
            float_t,    //controller update rule drag
            float_t,    //controller adjusted drag
            float_t,    //controller requested drag
            bool,       //controller update rule clamp flag
            bool,       //controller saturation flag
            bool        //controller fault flag
        > m_telemetry;
        SDFileWithCommands m_log;

        // --- non-volatile storage systems ---
        EEPROMWithCommands<
            uint_t,         //controller update period
            bool,           //controller enable
            bool,           //telemetry override
            bool,           //log override
            FileName_t,     //log file name
            FileName_t,     //telemetry file name
            FileName_t,     //flight plan file name
            uint_t,         //telemetry refresh period
            bool,           //simulation mode enable
            uint_t,         //simulation refresh period
            float_t,        //controller decay rate
            float_t,        //controller coast velocity
            uint_t,         //altimeter SPI speed
            float_t,        //altimeter ground pressure
            float_t,        //altimeter ground temperature
            uint_t,         //imu SPI speed
            uint32_t,       //imu acceleration sample period
            uint32_t,       //imu angular velocity sample period
            uint32_t,       //imu orientation sample period
            uint32_t        //imu gravity sample period
        > m_persistent;

        // --- serial port systems ---
        RocketOS::SerialInput m_inputBuffer;
        elapsedMillis m_serialRefresh;

        // --- HIL systems ---
        RocketOS::Simulation::TxHIL<
            float_t,    //drag area
            float_t,    //flight path
            float_t,    //error
            float_t,    //update rule drag
            float_t,    //adjusted drag
            float_t,    //echo of altitude
            float_t,    //echo of velocity
            float_t     //echo of angle
        > m_TxHIL;
        RocketOS::Simulation::RxHIL<
            float_t,    //altitude
            float_t,    //velocity
            float_t     //angle
        > m_RxHIL;
        uint_t m_HILRefreshPeriod;
        bool m_HILEnabled;

        // --- shell systems ---
        RocketOS::Shell::Interpreter m_interpreter;

    public:
        Application(char* telemetryBuffer, uint_t telemetryBufferSize, char* logBuffer, uint_t logBufferSize, float_t* flightPlanMem, uint_t flightPlanMemSize);
        
        void initialize();
        void makeShutdownSafe(bool printErrors=true);
        void updateBackground(); 

    private:
        // ######### command structure #########
        using Command = RocketOS::Shell::Command;
        using CommandList = RocketOS::Shell::CommandList;
        using arg_t = RocketOS::Shell::arg_t;
        // === ROOT COMMAND LIST ===
            // === SIMULATION SUBCOMMAND ===
                // === REFRESH SUBCOMMAND ===
                    //list of local commands
                    const std::array<Command, 2> c_simRefreshCommands{
                        Command{"", "", [this](arg_t){
                            Serial.print(m_HILRefreshPeriod);
                            Serial.println("ms");
                        }},
                        Command{"set", "u", [this](arg_t args){
                            m_HILRefreshPeriod = args[0].getUnsignedData();
                        }}
                    };
                // ==========================
                //list of subcommands
                const std::array<CommandList, 1> c_simChildren{
                    CommandList{"refresh", c_simRefreshCommands.data(), c_simRefreshCommands.size(), nullptr, 0}
                };
                //list of local commands
                const std::array<Command, 3> c_simCommands{
                    Command{"start", "", [this](arg_t){
                        m_HILEnabled = true;
                    }},
                    Command{"stop", "", [this](arg_t){
                        m_HILEnabled = false;
                    }},
                    Command{"", "", [this](arg_t){
                        if(m_HILEnabled) Serial.println("Simulation is active");
                        else Serial.println("Simulation is inactive");
                    }}
                };
            // =============================
            //list of subcommands
            const std::array<CommandList, 8> c_rootChildren{
                m_controller.getCommands(),
                m_flightPlan.getCommands(),
                m_log.getCommands(),
                m_telemetry.getCommands(),
                m_persistent.getCommands(),
                CommandList{"sim", c_simCommands.data(), c_simCommands.size(), c_simChildren.data(), c_simChildren.size()},
                m_altimeter.getCommands(),
                m_IMU.getCommands()
            };
            //list of local commands
            const std::array<Command, 4> c_rootCommands{
                Command{"echo", "", [](arg_t){
                    Serial.println("echo");
                }},
                Command{"commands", "", [this](arg_t){
                    c_root.printAllCommands();
                }},
                Command{"safe", "", [this](arg_t){
                    makeShutdownSafe();
                }},
                Command{"restartSD", "", [this](arg_t){
                    if(!m_sdCard.begin(SdioConfig(FIFO_SDIO))) Serial.println("Error re-initializing the SD card");
                }}
            };
            //command list object
            const CommandList c_root = {"root", c_rootCommands.data(), c_rootCommands.size(), c_rootChildren.data(), c_rootChildren.size()};
        // =========================

    };
} 
