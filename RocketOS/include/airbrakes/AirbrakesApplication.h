#pragma once
#include "AirbrakesGeneral.h"
#include "RocketOS.h"
#include "AirbrakesPersistent.h"
#include "AirbrakesTelemetry.h"
#include "AirbrakesController.h"
#include "AirbrakesFlightPlan.h"
#include "AirbrakesObserver.h"
#include <Arduino.h> //serial printing, elapsedmillis

namespace Airbrakes{

    class Application{
    private:
        // --- sensor readings ---

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
            float_t    //observed angular velocity
        > m_telemetry;
        SDFileWithCommands m_log;

        // --- non-volatile storage systems ---
        EEPROMWithCommands<
            uint_t,         //controller update period
            bool,           //controller enable
            bool,           //telemetry log override
            FileName_t,     //log file name
            FileName_t,     //telemetry file name
            FileName_t,     //flight plan file name
            uint_t,         //telemetry refresh period
            bool,           //simulation mode enable
            uint_t          //simulation refresh period
        > m_persistent;

        // --- serial port systems ---
        RocketOS::SerialInput m_inputBuffer;
        elapsedMillis m_serialRefresh;

        // --- HIL systems ---
        RocketOS::Simulation::TxHIL<
            float_t,    //partial v value
            float_t,    //partial angle value
            float_t,    //echo of velocity
            float_t     //echo of angle
        > m_TxHIL;
        RocketOS::Simulation::RxHIL<
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
            const std::array<CommandList, 6> c_rootChildren{
                m_controller.getCommands(),
                m_flightPlan.getCommands(),
                m_log.getCommands(),
                m_telemetry.getCommands(),
                m_persistent.getCommands(),
                CommandList{"sim", c_simCommands.data(), c_simCommands.size(), c_simChildren.data(), c_simChildren.size()}
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
