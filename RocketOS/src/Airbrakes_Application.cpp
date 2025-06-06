#include "airbrakes\AirbrakesApplication.h"

using namespace Airbrakes;
using namespace RocketOS;
using namespace RocketOS::Telemetry;
using namespace RocketOS::Persistent;
using namespace RocketOS::Shell;
using namespace RocketOS::Simulation;



Application::Application() : 
m_telemetry(m_sdCard, 
    DataLogSettings<float_t>{m_sharedVariables.altitude, "altitude"}, 
    DataLogSettings<float_t>{m_sharedVariables.deployment, "deployment"},
    DataLogSettings<float_t>{m_sharedVariables.gain, "gain"}
), 
m_telemetryRefreshPeriod(Airbrakes_CFG_TelemetryRefreshPeriod_ms),
m_doLogging(false),
m_log(m_sdCard, m_logBuffer.data(), m_logBuffer.size()),
c_logCommands(m_log, "log"),
m_persistent(
    EEPROMSettings<float_t>{m_sharedVariables.gain, 0, "gain"}
),
c_persistentCommands(m_persistent, "persistent"),
m_inputBuffer(115200),
m_TxHIL(m_sharedVariables.deployment, m_sharedVariables.altitude),
m_RxHIL(m_inputBuffer, m_sharedVariables.altitude),
m_HILRefreshPeriod(Airbrakes_CFG_SerialRefreshPeriod_ms),
m_HILEnabled(false),
m_interpreter(m_inputBuffer, &c_root){}


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
    //start timers

    //final message
    if(error == error_t::GOOD) Serial.println("Sucesfully initialized all systems");
    else Serial.println("Initialization complete, some systems failed to initialize");
}

void Application::makeShutdownSafe(){
    //save non-volatile memory
    m_persistent.save();
    //save telemetry and logs
    m_telemetry.flush();
    m_log.flush();
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
    if(m_doLogging && m_telemetryRefresh > m_telemetryRefreshPeriod){
        m_telemetry.logLine();
        m_telemetryRefresh = 0;
    }
}