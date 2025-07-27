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
#include "AirbrakesActuator.h"
#include "AirbrakesDetectionParameters.h"
#include <Arduino.h> //serial printing, elapsedmillis

// ===simulation control macros ===
//#define NO_TX_HIL
//#define NO_RX_HIL

// === state names ===
#define APP_STANDBY_STATE_NAME "standby"
#define APP_ARMED_STATE_NAME "armed"
#define APP_BOOST_STATE_NAME "boost"
#define APP_COAST_STATE_NAME "coast"
#define APP_RECOVERY_STATE_NAME "recovery"


namespace Airbrakes{

    class Application{
    private:
        enum class ProgramStates{
            Standby, Armed, ReArmed, Boost, Coast, Recovery
        };
    private:
        // --- program logic systems ---
        ProgramStates m_state;
        const char* m_stateName;
        uint_t m_stateTransitionCounter;
        elapsedMillis m_stateTransitionTimer, m_stateTransitionSampleTimer;
        uint_t m_stateTransitionSamplePeriod_ms;
        bool m_armFlag;
        EventDetection m_launchDetectionParameters, m_burnoutDetectionParameters, m_apogeeDetectionParameters;
        // --- peripheral hardware systems ---
        Sensors::MS5607_SPI m_altimeter;
        Sensors::BNO085_SPI m_imu;
        Motor::Actuator m_actuator;
        bool m_actuateInFlight;
        // --- control system ---
        Controls::Controller m_controller;
        Controls::FlightPlan m_flightPlan;
        Observer m_observer;
        const ObserverModes m_simulationType;
        // --- sd card systems ---
        SdFat m_sdCard;
        DataLogWithCommands<
            const char*,    //state
            float_t,        //predicted altitude
            float_t,        //predicted vertical velocity
            float_t,        //predicted vertical acceleration
            float_t,        //predicted angle to horizontal
            float_t,        //measured altitude
            float_t,        //measured pressure
            float_t,        //measured temperature
            float_t,        //measured acceleration x
            float_t,        //measured acceleration y
            float_t,        //measured acceleration z
            float_t,        //measured rotation x
            float_t,        //measured rotation y
            float_t,        //measured rotation z
            float_t,        //measured gravity x
            float_t,        //measured gravity y
            float_t,        //measured gravity z
            float_t,        //measured orientation r
            float_t,        //measured orientation i
            float_t,        //measured orientation j
            float_t,        //measured orientation k
            float_t,        //measured angle to horizontal
            float_t,        //controller error
            float_t,        //controller flight path
            float_t,        //controller flight path velocity partial
            float_t,        //controller flight path angle partial
            float_t,        //controller update rule drag
            float_t,        //controller adjusted drag
            float_t,        //controller requested drag
            bool,           //controller update rule clamp flag
            bool,           //controller saturation flag
            bool            //controller fault flag
        > m_telemetry;
        SDFileWithCommands m_log;
        bool m_bufferFlightTelemetry;

        // --- non-volatile storage systems ---
        EEPROMWithCommands<
            uint_t,                             //controller update period
            bool,                               //controller enable
            bool,                               //telemetry buffer mode flag
            bool,                               //telemetry override
            bool,                               //log override
            bool,                               //buffer flight telemetry flag
            FileName_t,                         //log file name
            FileName_t,                         //telemetry file name
            FileName_t,                         //flight plan file name
            uint_t,                             //telemetry refresh period
            bool,                               //simulation mode enable
            uint_t,                             //simulation refresh period
            float_t,                            //controller decay rate
            float_t,                            //controller coast velocity
            uint_t,                             //altimeter SPI speed
            float_t,                            //altimeter ground pressure
            float_t,                            //altimeter ground temperature
            uint_t,                             //imu SPI speed
            uint32_t,                           //imu acceleration sample period
            uint32_t,                           //imu angular velocity sample period
            uint32_t,                           //imu orientation sample period
            uint32_t,                           //imu gravity sample period
            float_t,                            //motor range limit
            uint_t,                             //motor encoder steps
            uint_t,                             //motor steps
            Motor::SteppingModes,               //motor stepping mode
            bool,                               //actuate in flight flag
            EventDetection::DetectionData,      //launch detection
            EventDetection::DetectionData,      //burnout detection
            EventDetection::DetectionData,      //apogee detection
            uint_t                              //detection sample period
        > m_persistent;

        // --- serial port systems ---
        RocketOS::SerialInput m_inputBuffer;
        elapsedMillis m_serialRefresh;

        // --- HIL systems ---
#ifndef NO_TX_HIL
        RocketOS::Simulation::TxHIL<
            float_t,    //drag area
            float_t,    //flight path
            float_t,    //error
            float_t,    //update rule drag
            float_t,    //adjusted drag
            float_t,    //altitude echo
            float_t,    //velocity echo
            float_t,    //acceleration echo
            float_t     //angle echo
        > m_TxHIL;
#endif
#ifndef NO_RX_HIL
        RocketOS::Simulation::RxHIL<
            float_t,    //altitude
            float_t,    //vertical velocity
            float_t,    //vertical acceleration
            float_t     //angle
        > m_RxHIL;
#endif
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
        void standbyTasks();
        void armedTasks();
        void boostTasks();
        void coastTasks();
        void recoveryTasks();

        void gotoState(ProgramStates);
        void initStandby();
        void initArmed();
        void initReArmed();
        void initBoost();
        void initCoast();
        void initRecovery();

        void logPrint(const char*);

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
                        if(m_observer.setMode(m_simulationType) == error_t::ERROR){
                            Serial.println("Failed to start simulation");
                            return;
                        }
                        m_HILEnabled = true;
                    }},
                    Command{"stop", "", [this](arg_t){
                        if(m_observer.setMode(ObserverModes::Sensor) == error_t::ERROR){
                            Serial.println("Error stopping simulation");
                        }
                        m_HILEnabled = false;
                    }},
                    Command{"", "", [this](arg_t){
                        if(m_HILEnabled) Serial.println("Simulation is active");
                        else Serial.println("Simulation is inactive");
                    }}
                };
            // =============================

            // === FLIGHT SUBCOMMAND ===
                //children command lists
                // === SAMPLE SUBCOMMAND ===
                    //list of commands
                    const std::array<Command, 2> c_flightSampleCommands{
                        Command{"", "", [this](arg_t){
                            Serial.print(m_stateTransitionSamplePeriod_ms);
                            Serial.println("ms");
                        }},
                        Command{"set", "u", [this](arg_t args){
                            m_stateTransitionSamplePeriod_ms = args[0].getUnsignedData();
                        }}
                    };
                // =========================

                // === BUFFER SUBCOMMAND ===
                //list of commands
                const std::array<Command, 3> c_flightBufferCommands{
                    Command{"", "", [this](arg_t){
                        if(m_bufferFlightTelemetry) Serial.println("Buffer mode is enabled");
                        else Serial.println("Buffer mode is is disabled");
                    }},
                    Command{"set", "", [this](arg_t){
                        m_bufferFlightTelemetry = true;
                    }},
                    Command{"clear", "", [this](arg_t){
                        m_bufferFlightTelemetry = false;
                    }}
                };
                // ==============================

                // === ACTUATE SUBCOMMAND ===
                const std::array<Command, 3> c_flightActuateCommands{
                    Command{"", "", [this](arg_t){
                        if(m_actuateInFlight) Serial.println("Actuators are enabled for flight");
                        else Serial.println("Actuators are disabled for flight");
                    }},
                    Command{"set", "", [this](arg_t){
                        m_actuateInFlight = true;
                    }},
                    Command{"clear", "", [this](arg_t){
                        m_actuateInFlight = false;
                    }}
                };
                // ==========================
                //----------------------

                //list of subcommands
                const std::array<CommandList, 7> c_flightSubCommands{
                    CommandList{"buffer", c_flightBufferCommands.data(), c_flightBufferCommands.size(), nullptr, 0},
                    CommandList{"actuate", c_flightActuateCommands.data(), c_flightActuateCommands.size(), nullptr, 0},
                    CommandList{"sample", c_flightSampleCommands.data(), c_flightSampleCommands.size(), nullptr, 0},
                    m_launchDetectionParameters.getCommands(), 
                    m_burnoutDetectionParameters.getCommands(), 
                    m_apogeeDetectionParameters.getCommands(),
                    m_flightPlan.getCommands(),
                };
            // ============================

            

            //list of subcommands
            const std::array<CommandList, 9> c_rootChildren{
                CommandList{"flight", nullptr, 0, c_flightSubCommands.data(), c_flightSubCommands.size()},
                m_controller.getCommands(),
                m_log.getCommands(),
                m_telemetry.getCommands(),
                m_persistent.getCommands(),
                CommandList{"sim", c_simCommands.data(), c_simCommands.size(), c_simChildren.data(), c_simChildren.size()},
                m_altimeter.getCommands(),
                m_imu.getCommands(),
                m_actuator.getCommands()
                
            };
            //list of local commands
            const std::array<Command, 5> c_rootCommands{
                Command{"arm", "", [this](arg_t){
                    m_armFlag = true;
                }},
                Command{"disarm", "", [this](arg_t){
                    m_armFlag = false;
                }},
                Command{"state", "", [this](arg_t){
                    switch(m_state){
                        case ProgramStates::Standby:
                        default:
                            Serial.println(APP_STANDBY_STATE_NAME);
                        break;
                        case ProgramStates::Armed:
                        case ProgramStates::ReArmed:
                            Serial.println(APP_ARMED_STATE_NAME);
                        break;
                        case ProgramStates::Boost:
                            Serial.println(APP_BOOST_STATE_NAME);
                        break;
                        case ProgramStates::Coast:
                            Serial.println(APP_COAST_STATE_NAME);
                        break;
                        case ProgramStates::Recovery:
                            Serial.println(APP_RECOVERY_STATE_NAME);
                        break;
                    }
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
