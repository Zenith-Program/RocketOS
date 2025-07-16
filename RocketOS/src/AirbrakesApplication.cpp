#include "airbrakes\AirbrakesApplication.h"

using namespace Airbrakes;
using namespace RocketOS;
using namespace RocketOS::Telemetry;
using namespace RocketOS::Persistent;
using namespace RocketOS::Shell;
using namespace RocketOS::Simulation;



Application::Application(char* telemetryBuffer, uint_t telemetryBufferSize, char* logBuffer, uint_t logBufferSize, float_t* flightPlanMem, uint_t flightPlanMemSize) : 
    //peripherals
    m_altimeter("altimeter", Airbrakes_CFG_AltimeterNominalGroundTemperature, Airbrakes_CFG_AltimeterNominalGroundPressure, Airbrakes_CFG_AltimeterSPIFrequency, TeensyTimerTool::TMR1),
    m_IMU("imu", Airbrakes_CFG_IMU_SPIFrequency, Airbrakes_CFG_IMU_SamplePeriod_us),
    m_actuator("motor"),
    //control syatems
    m_controller("controller", 100000, m_flightPlan, m_observer, Airbrakes_CFG_DecayRate),
    m_flightPlan("plan", m_sdCard, flightPlanMem, flightPlanMemSize, Airbrakes_CFG_DefaultFlightPlanFileName),
    //telemetry systems
    m_telemetry("telemetry", m_sdCard, telemetryBuffer, telemetryBufferSize, Airbrakes_CFG_DefaultTelemetryFile, Airbrakes_CFG_TelemetryRefreshPeriod_ms,
        DataLogSettings<float_t>{m_observer.getAltitudeRef(), "predicted altitude"}, 
        DataLogSettings<float_t>{m_observer.getHorizontalVelocityRef(), "predicted x velocity"},
        DataLogSettings<float_t>{m_observer.getVerticalVelocityRef(), "predicted y velocity"},
        DataLogSettings<float_t>{m_observer.getAngleRef(), "predicted angle"},
        DataLogSettings<float_t>{m_observer.getAngularVelocityRef(), "predicted angular velocity"},
        DataLogSettings<float_t>{m_controller.getErrorRef(), "controller error"},
        DataLogSettings<float_t>{m_controller.getFlightPathRef(), "flight path"},
        DataLogSettings<float_t>{m_controller.getVPartialRef(), "flght path velocity partial derivative"},
        DataLogSettings<float_t>{m_controller.getAnglePartialRef(), "flght path angle partial derivative"},
        DataLogSettings<float_t>{m_controller.getUpdateRuleDragRef(), "update rule drag area"},
        DataLogSettings<float_t>{m_controller.getAdjustedDragRef(), "adjusted drag area"},
        DataLogSettings<float_t>{m_controller.getRequestedDragRef(), "requested drag area"},
        DataLogSettings<bool>{m_controller.getClampFlagRef(), "update rule shutdown"},
        DataLogSettings<bool>{m_controller.getSaturationFlagRef(), "controller saturation"},
        DataLogSettings<bool>{m_controller.getFaultFlagRef(), "controller fault"}
    ),
    m_log("log", m_sdCard, logBuffer, logBufferSize, Airbrakes_CFG_DefaultLogFile),

    //persistent systems
    m_persistent("persistent",
        EEPROMSettings<uint_t>{m_controller.getClockPeriodRef(), Airbrakes_CFG_ControllerPeriod_us, "controller clock period"},
        EEPROMSettings<bool>{m_controller.getActiveFlagRef(), false, "controller flag"},
        EEPROMSettings<bool>{m_telemetry.getOverrideRef(), false, "telemetry override"},
        EEPROMSettings<bool>{m_log.getOverrideFlagRef(), false, "log override"},
        EEPROMSettings<FileName_t>{m_log.getNameBufferRef(), Airbrakes_CFG_DefaultLogFile, "log file name"},
        EEPROMSettings<FileName_t>{m_telemetry.getNameBufferRef(), Airbrakes_CFG_DefaultTelemetryFile, "telemetry file name"},
        EEPROMSettings<FileName_t>{m_flightPlan.getFileNameRef(), Airbrakes_CFG_DefaultFlightPlanFileName,"flight plan file"},
        EEPROMSettings<uint_t>{m_telemetry.getRefreshPeriodRef(), Airbrakes_CFG_TelemetryRefreshPeriod_ms, "telemetry refresh"},
        EEPROMSettings<bool>{m_HILEnabled, false, "simulation mode"},
        EEPROMSettings<uint_t>{m_HILRefreshPeriod, Airbrakes_CFG_HILRefresh_ms, "simulation refresh"},
        EEPROMSettings<float_t>{m_controller.getDecayRateRef(), Airbrakes_CFG_DecayRate, "controller decay rate"},
        EEPROMSettings<float_t>{m_controller.getCoastVelocityRef(), 0, "controller coast velocity"},
        EEPROMSettings<uint_t>{m_altimeter.getSPIFrequencyRef(), Airbrakes_CFG_AltimeterSPIFrequency, "altimeter SPI speed"},
        EEPROMSettings<float_t>{m_altimeter.getGroundTemperatureRef(), Airbrakes_CFG_AltimeterNominalGroundTemperature, "altimeter ground temperature"},
        EEPROMSettings<float_t>{m_altimeter.getGroundPressureRef(), Airbrakes_CFG_AltimeterNominalGroundPressure, "altimeter ground pressure"},
        EEPROMSettings<uint_t>{m_IMU.getSPIFrequencyRef(), Airbrakes_CFG_IMU_SPIFrequency, "imu spi speed"},
        EEPROMSettings<uint32_t>{m_IMU.getAccelerationSamplePeriodRef(), Airbrakes_CFG_IMU_SamplePeriod_us, "imu acceleration sample period"},
        EEPROMSettings<uint32_t>{m_IMU.getAngularVelocitySamplePeriodRef(), Airbrakes_CFG_IMU_SamplePeriod_us, "imu angular velocity sample period"},
        EEPROMSettings<uint32_t>{m_IMU.getOrientationSamplePeriodRef(), Airbrakes_CFG_IMU_SamplePeriod_us, "imu oreintation sample period"},
        EEPROMSettings<uint32_t>{m_IMU.getGravitySamplePeriodRef(), Airbrakes_CFG_IMU_SamplePeriod_us, "imu gravity sample period"},
        EEPROMSettings<float_t>{m_actuator.getActuatorLimitRef(), Airbrakes_CFG_MotorDefaultLimit, "actuator range limit"},
        EEPROMSettings<uint_t>{m_actuator.getEncoderStepsRef(), Airbrakes_CFG_MotorFullStrokeNumEncoderPositions, "actuator number of encoder positions"},
        EEPROMSettings<uint_t>{m_actuator.getMotorStepsRef(), Airbrakes_CFG_MotorFullStrokeNumSteps, "actuator number of steps"},
        EEPROMSettings<Motor::SteppingModes>{m_actuator.getSteppingModeRef(), Motor::SteppingModes::HalfStep, "actuator mode"}
    ),

    //serial systems
    m_inputBuffer(115200),

    //simulation systems
    m_TxHIL(
        m_controller.getRequestedDragRef(),
        m_controller.getFlightPathRef(),
        m_controller.getErrorRef(),
        m_controller.getUpdateRuleDragRef(),
        m_controller.getAdjustedDragRef(),
        m_observer.getAltitudeRef(),
        m_observer.getVerticalVelocityRef(),
        m_observer.getAngleRef()
    ),
    m_RxHIL(m_inputBuffer, 
        m_observer.getAltitudeRef(),
        m_observer.getVerticalVelocityRef(),
        m_observer.getAngleRef()
    ),
    m_HILRefreshPeriod(Airbrakes_CFG_SerialRefreshPeriod_ms),
    m_HILEnabled(false),

    //shell systems
    m_interpreter(m_inputBuffer, &c_root)
{}


void Application::initialize(){
    //=================
    error_t anyError = error_t::GOOD;
    error_t processError;
    m_inputBuffer.init();
    Serial.println("Initializing airbrakes application...");
    //resore EEPROM data
    result_t<bool> result = m_persistent.restore();
    if(result.error != error_t::GOOD){ 
        Serial.println("Error interfacing with EEPROM");
        anyError = result.error;
    }
    if(result.data) Serial.println("Restored system defaults because of detected EEPROM layout change");
    else Serial.println("Loaded persistent EEPROM data");
    //initialize SD card
    if(!m_sdCard.begin(SdioConfig(FIFO_SDIO))){
         Serial.println("Error initializing the SD card");
         anyError = error_t::ERROR;
    }
    else Serial.println("Initialized the SD card");
    //load flight plan
    processError = m_flightPlan.loadFromFile();
    if(processError == error_t::GOOD) Serial.printf("Loaded flight plan from '%s'\n", m_flightPlan.getFileName());
    else{
        if(processError == Controls::FlightPlan::ERROR_Formating) Serial.printf("Formatting error encountered when loading flight plan from '%s'\n", m_flightPlan.getFileName());
        else if(processError == Controls::FlightPlan::ERROR_Memory) Serial.printf("Failed to load flight plan from '%s' due to lack of allocated memory\n", m_flightPlan.getFileName());
        else if(processError == Controls::FlightPlan::ERROR_File) Serial.printf("Failed to open flight plan with file name '%s'\n", m_flightPlan.getFileName());
        else Serial.println("Failed to load the flight plan");
        anyError = error_t::ERROR;
    }
    //init altimeter
    if(m_altimeter.initialize() != error_t::GOOD){
        Serial.println("Failed to initialize the altimeter");
        anyError = error_t::ERROR;
    }
    else Serial.println("Initialized the altimeter");
    //init IMU
    if(m_IMU.initialize() != error_t::GOOD){
        Serial.println("Failed to detect the IMU");
        anyError = error_t::ERROR;
    }
    else Serial.println("Initialized the IMU");
    //init motor
    m_actuator.initialize();
    //start timers
    m_controller.resetInit();
    Serial.println("Initialized the controller");
    //final message
    if(anyError == error_t::GOOD) Serial.println("Successfully initialized all systems");
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
            noInterrupts();
            m_interpreter.readLine();
            interrupts();
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
    //update IMU
    m_IMU.updateBackground();
}