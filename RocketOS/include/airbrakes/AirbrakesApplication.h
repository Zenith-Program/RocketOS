#pragma once
#include "AirbrakesGeneral.h"
#include "RocketOS.h"
#include <Arduino.h> //serial printing, elapsedmillis

namespace Airbrakes{

    class Application{
    private:
        //varables shared between systems
        struct{
            RocketOS::float_t altitude, deployment, gain;
        } m_sharedVariables;

        //sd card systems
        SdFat m_sdCard;
        RocketOS::Telemetry::DataLog<Airbrakes_CFG_TelemetryBufferSize, float_t, float_t, float_t> m_telemetry;
        uint_t m_telemetryRefreshPeriod;
        elapsedMillis m_telemetryRefresh;
        bool m_doLogging;
        std::array<char, Airbrakes_CFG_LogBufferSize> m_logBuffer;
        RocketOS::Telemetry::SDFile m_log;
        const RocketOS::Telemetry::FileCommands c_logCommands;

        //non-volatile storage systems
        RocketOS::Persistent::EEPROMBackup<float_t> m_persistent;
        const RocketOS::Persistent::EEPROMCommands<float_t> c_persistentCommands;

        //serial port systems
        RocketOS::SerialInput m_inputBuffer;
        elapsedMillis m_serialRefresh;

        //HIL systems
        RocketOS::Simulation::TxHIL<float_t, float_t> m_TxHIL;
        RocketOS::Simulation::RxHIL<float_t> m_RxHIL;
        uint_t m_HILRefreshPeriod;
        bool m_HILEnabled;

        //command systems
        RocketOS::Shell::Interpreter m_interpreter;
    public:
        Application();
        
        void initialize();
        void makeShutdownSafe();

        void updateBackground(); 

    private:
        using Command = RocketOS::Shell::Command;
        using CommandList = RocketOS::Shell::CommandList;
        using arg_t = RocketOS::Shell::arg_t;
        //command structure
    
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
        const std::array<CommandList, 3> c_rootChildren{
            c_logCommands.getCommands(),
            c_persistentCommands.getCommands(),
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
                m_persistent.save();
                m_log.flush();
                m_telemetry.flush();
            }}
        };
        //command list object
        const CommandList c_root = {"root", c_rootCommands.data(), c_rootCommands.size(), c_rootChildren.data(), c_rootChildren.size()};

    };
} 
