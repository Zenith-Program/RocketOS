#include "airbrakes\AirbrakesApplication.h"

using namespace Airbrakes;
using namespace RocketOS;
using namespace RocketOS::Telemetry;
using namespace RocketOS::Persistent;
using namespace RocketOS::Shell;
using namespace RocketOS::Simulation;



Application::Application() : 
//telemetry systems
m_telemetry("telemetry", m_sdCard, Airbrakes_CFG_DefaultTelemetryFile, Airbrakes_CFG_TelemetryRefreshPeriod_ms,
    DataLogSettings<float_t>{m_sharedVariables.altitude, "altitude"}, 
    DataLogSettings<float_t>{m_sharedVariables.deployment, "deployment"},
    DataLogSettings<float_t>{m_sharedVariables.gain, "gain"}
),
m_doLogging(false),
m_log("log", m_sdCard, Airbrakes_CFG_DefaultLogFile),

//persistent systems
m_persistent("persistent",
    EEPROMSettings<float_t>{m_sharedVariables.gain, 0, "gain"},
    EEPROMSettings<TelemetryFileName_t>{m_log.getNameBufferRef(), "log.txt", "log file name"},
    EEPROMSettings<TelemetryFileName_t>{m_telemetry.getNameBufferRef(), "telemetry.csv", "telemetry file name"},
    EEPROMSettings<uint_t>{m_telemetry.getRefreshPeriodRef(), 100, "telemetry refresh"},
    EEPROMSettings<bool>{m_HILEnabled, false, "simulation mode"},
    EEPROMSettings<uint_t>{m_HILRefreshPeriod, 10, "simulation refresh"}
),

//serial systems
m_inputBuffer(115200),

//simulation systems
m_TxHIL(
    m_sharedVariables.deployment, 
    m_sharedVariables.altitude
),
m_RxHIL(m_inputBuffer, 
    m_sharedVariables.altitude
),
m_HILRefreshPeriod(Airbrakes_CFG_SerialRefreshPeriod_ms),
m_HILEnabled(false),

//shell systems
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
    if(m_doLogging && m_telemetry.isRefreshed()){
        m_telemetry.logLine();
        m_telemetry.clearRefresh();
    }
}