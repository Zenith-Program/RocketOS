#pragma once
#include "AirbrakesGeneral.h"
#include "RocketOS.h"
#include "AirbrakesPersistent.h"
#include "AirbrakesTelemetry.h"
#include "AirbrakesControlSystem.h"
#include <Arduino.h> //serial printing, elapsedmillis

namespace Airbrakes{

    class Application{
        using TelemetryFileName_t = std::array<char, Airbrakes_CFG_FileNameBufferSize>;
    private:
        // --- control system ---
        Controls::DemoController m_controller;

        // --- sd card systems ---
        SdFat m_sdCard;
        DataLogWithCommands<Airbrakes_CFG_TelemetryBufferSize, Airbrakes_CFG_FileNameBufferSize, 
        float_t,    //environment value
        float_t,    //control input
        float_t     //gain
        > m_telemetry;
        bool m_doLogging;
        SDFileWithCommands<Airbrakes_CFG_LogBufferSize, Airbrakes_CFG_FileNameBufferSize> m_log;

        // --- non-volatile storage systems ---
        EEPROMWithCommands<
        float_t,                //controller gain value
        uint_t,                 //controller update period
        bool,                   //controller enable
        TelemetryFileName_t,    //log file name
        TelemetryFileName_t,    //telemetry file name
        uint_t,                 //telemetry refresh period
        bool,                   //simulation mode enable
        uint_t                  //simulation refresh period
        > m_persistent;

        // --- serial port systems ---
        RocketOS::SerialInput m_inputBuffer;
        elapsedMillis m_serialRefresh;

        // --- HIL systems ---
        RocketOS::Simulation::TxHIL<
        float_t,    //control signal
        float_t     //echo of environment input
        > m_TxHIL;
        RocketOS::Simulation::RxHIL<
        float_t     //environment input
        > m_RxHIL;
        uint_t m_HILRefreshPeriod;
        bool m_HILEnabled;

        // --- shell systems ---
        RocketOS::Shell::Interpreter m_interpreter;

    public:
        Application();
        
        void initialize();
        void makeShutdownSafe();

        void updateBackground(); 

    private:
        // ######### command structure #########
        using Command = RocketOS::Shell::Command;
        using CommandList = RocketOS::Shell::CommandList;
        using arg_t = RocketOS::Shell::arg_t;
        // === ROOT COMMAND LIST ===
        // children command lists ---------
            // === SIMULATION SUBCOMMAND ===
            // children command lists ---------
                // === REFRESH SUBCOMMAND ===
                const std::array<Command, 2> c_simRefreshCommands{
                    Command{"", "", [this](arg_t){
                        Serial.print(m_HILRefreshPeriod);
                        Serial.println("ms");
                    }},
                    Command{"set", "u", [this](arg_t args){
                        m_HILRefreshPeriod = args[0].getUnsignedData();
                    }}
                };
            //---------------------------------
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
        // ------------------------------------
        //list of subcommands
        const std::array<CommandList, 5> c_rootChildren{
            m_controller.getCommands(),
            m_log.getCommands(),
            m_telemetry.getCommands(),
            m_persistent.getCommands(),
            CommandList{"sim", c_simCommands.data(), c_simCommands.size(), c_simChildren.data(), c_simChildren.size()}
        };
        //list of local commands
        const std::array<Command, 3> c_rootCommands{
            Command{"echo", "", [](arg_t){
                Serial.println("echo");
            }},
            Command{"commands", "", [this](arg_t){
                c_root.printAllCommands();
            }},
            Command{"safe", "", [this](arg_t){
                makeShutdownSafe();
            }}
        };
        //command list object
        const CommandList c_root = {"root", c_rootCommands.data(), c_rootCommands.size(), c_rootChildren.data(), c_rootChildren.size()};

    };
} 
