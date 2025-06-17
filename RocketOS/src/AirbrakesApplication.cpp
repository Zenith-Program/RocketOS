#include "airbrakes\AirbrakesApplication.h"

using namespace Airbrakes;
using namespace RocketOS;
using namespace RocketOS::Telemetry;
using namespace RocketOS::Persistent;
using namespace RocketOS::Shell;
using namespace RocketOS::Simulation;



Application::Application(char* telemetryBuffer, uint_t telemetryBufferSize, char* logBuffer, uint_t logBufferSize, float_t* flightPlanMem, uint_t flightPlanMemSize) : 
    //control syatems
    m_controller("controller", m_flightPlan, 100000),
    m_flightPlan("plan", m_sdCard, flightPlanMem, flightPlanMemSize, Airbrakes_CFG_DefaultFlightPlanFileName),
    //telemetry systems
    m_telemetry("telemetry", m_sdCard, telemetryBuffer, telemetryBufferSize, Airbrakes_CFG_DefaultTelemetryFile, Airbrakes_CFG_TelemetryRefreshPeriod_ms,
        DataLogSettings<float_t>{m_controller.getAltitudeRef(), "altitude"}, 
        DataLogSettings<float_t>{m_controller.getVelocityRef(), "velocity"},
        DataLogSettings<float_t>{m_controller.getAngleRef(), "angle"}
    ),
    m_log("log", m_sdCard, logBuffer, logBufferSize, Airbrakes_CFG_DefaultLogFile),

    //persistent systems
    m_persistent("persistent",
        EEPROMSettings<uint_t>{m_controller.getClockPeriodRef(), 50, "controller clock period"},
        EEPROMSettings<bool>{m_controller.getActiveFlagRef(), false, "controller flag"},
        EEPROMSettings<bool>{m_telemetry.getOverrideRef(), false, "telemetry override"},
        EEPROMSettings<FileName_t>{m_log.getNameBufferRef(), Airbrakes_CFG_DefaultLogFile, "log file name"},
        EEPROMSettings<FileName_t>{m_telemetry.getNameBufferRef(), Airbrakes_CFG_DefaultTelemetryFile, "telemetry file name"},
        EEPROMSettings<FileName_t>{m_flightPlan.getFileNameRef(), Airbrakes_CFG_DefaultFlightPlanFileName,"flight plan file"},
        EEPROMSettings<uint_t>{m_telemetry.getRefreshPeriodRef(), 100, "telemetry refresh"},
        EEPROMSettings<bool>{m_HILEnabled, false, "simulation mode"},
        EEPROMSettings<uint_t>{m_HILRefreshPeriod, 10, "simulation refresh"}
    ),

    //serial systems
    m_inputBuffer(115200),

    //simulation systems
    m_TxHIL(
        m_controller.getAltitudeRef(),
        m_controller.getVelocityRef(),
        m_controller.getAngleRef()
    ),
    m_RxHIL(m_inputBuffer, 
        m_controller.getVelocityRef(),
        m_controller.getAngleRef()
    ),
    m_HILRefreshPeriod(Airbrakes_CFG_SerialRefreshPeriod_ms),
    m_HILEnabled(false),

    //shell systems
    m_interpreter(m_inputBuffer, &c_root)
{}


void Application::initialize(){
    error_t error = error_t::GOOD;
    m_inputBuffer.init();
    Serial.println("Initializing airbrakes application...");
    //resore EEPROM data
    result_t<bool> result = m_persistent.restore();
    if(result.error != error_t::GOOD){ 
        Serial.println("Error interfacing with EEPROM");
        error = result.error;
    }
    if(result.data) Serial.println("Restored system defaults because of detected EEPROM layout change");
    else Serial.println("Loaded persistent EEPROM data");
    //initialize SD card
    if(!m_sdCard.begin(SdioConfig(FIFO_SDIO))){
         Serial.println("Error initializing the SD card");
         error = error_t::ERROR;
    }
    else Serial.println("Initialized the SD card");
    //load flight plan
    error_t loadError = m_flightPlan.loadFromFile();
    if(loadError == error_t::GOOD) Serial.printf("Sucesfully loaded flight plan from '%s'\n", m_flightPlan.getFileName());
    else{
        if(loadError == error_t(2)) Serial.printf("Formatting error encountered when loading flight plan from '%s'\n", m_flightPlan.getFileName());
        else if(loadError == error_t(3)) Serial.printf("Failed to load flight plan from '%s' due to lack of allocated memory\n", m_flightPlan.getFileName());
        else if(loadError == error_t(4)) Serial.printf("Failed to open flight plan with file name '%s'\n", m_flightPlan.getFileName());
        else Serial.println("Failed to load the flight plan");
        error = error_t::ERROR;
    }
    //start timers
    m_controller.resetInit();
    Serial.println("Initialized the controller");
    //final message
    if(error == error_t::GOOD) Serial.println("Sucesfully initialized all systems");
    else Serial.println("Initialization complete, some systems failed to initialize");
}

void Application::makeShutdownSafe(bool printErrors){
    //save non-volatile memory
    if(m_persistent.save() != error_t::GOOD && printErrors) Serial.println("Error saving persistent EEPROM data");
    //save telemetry and logs
    if(m_telemetry.flush() != error_t::GOOD && printErrors) Serial.println("Error flushing telemetry file");
    if(m_log.flush() != error_t::GOOD && printErrors) Serial.println("Error flushing log file");
}

void Application::updateBackground(){
    //handle HIL updates and command inputs
    if(m_serialRefresh > m_HILRefreshPeriod){
        if(m_HILEnabled) m_TxHIL.sendUpdate();
        m_inputBuffer.update();
         while(m_inputBuffer.hasData()){
            m_interpreter.readLine();
            if(m_HILEnabled) m_RxHIL.readLine();
            m_inputBuffer.clear();
            m_inputBuffer.update();
         }
         m_serialRefresh = 0;
    }
    //make telemetry log
    if(m_controller.isActive() && m_telemetry.ready()){
        m_telemetry.logLine();
        m_telemetry.clearReady();
    }
}