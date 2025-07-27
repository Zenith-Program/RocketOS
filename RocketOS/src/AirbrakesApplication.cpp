#include "airbrakes\AirbrakesApplication.h"

using namespace Airbrakes;
using namespace RocketOS;
using namespace RocketOS::Telemetry;
using namespace RocketOS::Persistent;
using namespace RocketOS::Shell;
using namespace RocketOS::Simulation;


Application::Application(char* telemetryBuffer, uint_t telemetryBufferSize, char* logBuffer, uint_t logBufferSize, float_t* flightPlanMem, uint_t flightPlanMemSize) : 
    //program logic
    m_state(ProgramStates::Standby),
    m_stateName(APP_STANDBY_STATE_NAME),
    m_stateTransitionSamplePeriod_ms(Airbrakes_CFG_EventDetectionSamplePeriod_ms),
    m_armFlag(false),
    m_launchDetectionParameters("launch", Airbrakes_CFG_LaunchMaximumAltitude_m, Airbrakes_CFG_LaunchMinimumVelocity_mPerS, Airbrakes_CFG_LaunchMinimumAcceleration_mPerS2, Airbrakes_CFG_LaunchMinimumSamples, Airbrakes_CFG_LaunchMinimumTime_ms),
    m_burnoutDetectionParameters("burnout", Airbrakes_CFG_BurnoutMinimumAltitude_m, Airbrakes_CFG_BurnoutMinimumVelocity_mPerS, Airbrakes_CFG_BurnoutMaximumAcceleration_mPerS2, Airbrakes_CFG_BurnoutMinimumSamples, Airbrakes_CFG_BurnoutMinimumTime_ms),
    m_apogeeDetectionParameters("apogee", Airbrakes_CFG_ApogeeMinimumAltitude_m, Airbrakes_CFG_ApogeeMaximumVelocity_mPerS, Airbrakes_CFG_ApogeeMaximumAcceleration_mPerS2, Airbrakes_CFG_ApogeeMinimumSamples, Airbrakes_CFG_ApogeeMinimumTime_ms),
    //peripherals
    m_altimeter("altimeter", Airbrakes_CFG_AltimeterNominalGroundTemperature, Airbrakes_CFG_AltimeterNominalGroundPressure, Airbrakes_CFG_AltimeterSPIFrequency, TeensyTimerTool::TMR1),
    m_imu("imu", Airbrakes_CFG_IMU_SPIFrequency, Airbrakes_CFG_IMU_SamplePeriod_us),
    m_actuator("motor"),
    m_actuateInFlight(true),
    //control syatems
    m_controller("controller", 100000, m_flightPlan, m_observer, m_actuator, Airbrakes_CFG_DecayRate),
    m_flightPlan("plan", m_sdCard, flightPlanMem, flightPlanMemSize, Airbrakes_CFG_DefaultFlightPlanFileName),
    m_observer(m_imu, m_altimeter),
    m_simulationType(ObserverModes::FullSimulation),
    //telemetry systems
    m_telemetry("telemetry", m_sdCard, telemetryBuffer, telemetryBufferSize, Airbrakes_CFG_DefaultTelemetryFile, Airbrakes_CFG_TelemetryRefreshPeriod_ms,
        DataLogSettings<const char*>{m_stateName, "State"},
        DataLogSettings<float_t>{m_observer.getPredictedAltitudeRef(), "Predicted Altitude"}, 
        DataLogSettings<float_t>{m_observer.getPredictedVerticalVelocityRef(), "Predicted Vertical Velocity"},
        DataLogSettings<float_t>{m_observer.getPredictedVerticalAccelerationRef(), "Predicted Vertical Acceleration"},
        DataLogSettings<float_t>{m_observer.getPredictedAngleRef(), "Predicted Angle to Horizontal"},
        DataLogSettings<float_t>{m_observer.getMeasuredAltitudeRef(), "Measured Altitude"},
        DataLogSettings<float_t>{m_observer.getMeasuredPressureRef(), "Measured Pressure"},
        DataLogSettings<float_t>{m_observer.getMeasuredTemperatureRef(), "Measured Temperature"},
        DataLogSettings<float_t>{m_observer.getMeasuredLinearAccelerationRef().x, "Measured Acceleration x"},
        DataLogSettings<float_t>{m_observer.getMeasuredLinearAccelerationRef().y, "Measured Acceleration y"},
        DataLogSettings<float_t>{m_observer.getMeasuredLinearAccelerationRef().z, "Measured Acceleration z"},
        DataLogSettings<float_t>{m_observer.getMeasuredRotationRef().x, "Measured Rotation x"},
        DataLogSettings<float_t>{m_observer.getMeasuredRotationRef().y, "Measured Rotation y"},
        DataLogSettings<float_t>{m_observer.getMeasuredRotationRef().z, "Measured Rotation z"},
        DataLogSettings<float_t>{m_observer.getMeasuredGravityRef().x, "Measured Gravity x"},
        DataLogSettings<float_t>{m_observer.getMeasuredGravityRef().y, "Measured Gravity y"},
        DataLogSettings<float_t>{m_observer.getMeasuredGravityRef().z, "Measured Gravity z"},
        DataLogSettings<float_t>{m_observer.getMeasuredOrientationRef().r, "Measured Orientation real part"},
        DataLogSettings<float_t>{m_observer.getMeasuredOrientationRef().i, "Measured Orientation i part"},
        DataLogSettings<float_t>{m_observer.getMeasuredOrientationRef().j, "Measured Orientation j part"},
        DataLogSettings<float_t>{m_observer.getMeasuredOrientationRef().k, "Measured Orientation k part"},
        DataLogSettings<float_t>{m_observer.getMeasuredAngleRef(), "Measured Angle to Horizontal"},
        DataLogSettings<float_t>{m_controller.getErrorRef(), "Controller error"},
        DataLogSettings<float_t>{m_controller.getFlightPathRef(), "Flight path"},
        DataLogSettings<float_t>{m_controller.getVPartialRef(), "Flght path velocity partial derivative"},
        DataLogSettings<float_t>{m_controller.getAnglePartialRef(), "Flght path angle partial derivative"},
        DataLogSettings<float_t>{m_controller.getUpdateRuleDragRef(), "Update rule drag area"},
        DataLogSettings<float_t>{m_controller.getAdjustedDragRef(), "Adjusted drag area"},
        DataLogSettings<float_t>{m_controller.getRequestedDragRef(), "Requested drag area"},
        DataLogSettings<float_t>{m_controller.getCurrentDragRef(), "Current drag area"},
        DataLogSettings<bool>{m_controller.getClampFlagRef(), "Update rule shutdown"},
        DataLogSettings<bool>{m_controller.getSaturationFlagRef(), "Controller saturation"},
        DataLogSettings<bool>{m_controller.getFaultFlagRef(), "Controller fault"}
    ),
    m_log("log", m_sdCard, logBuffer, logBufferSize, Airbrakes_CFG_DefaultLogFile),
    m_bufferFlightTelemetry(false),
    //persistent systems
    m_persistent("persistent",
        EEPROMSettings<uint_t>{m_controller.getClockPeriodRef(), Airbrakes_CFG_ControllerPeriod_us, "controller clock period"},
        EEPROMSettings<bool>{m_controller.getActiveFlagRef(), false, "controller flag"},
        EEPROMSettings<bool>{m_bufferFlightTelemetry, false, "buffer mode flag"},
        EEPROMSettings<bool>{m_telemetry.getOverrideRef(), false, "telemetry override"},
        EEPROMSettings<bool>{m_log.getOverrideFlagRef(), false, "log override"},
        EEPROMSettings<bool>{m_bufferFlightTelemetry, false, "buffer flight telemetry flag"},
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
        EEPROMSettings<uint_t>{m_imu.getSPIFrequencyRef(), Airbrakes_CFG_IMU_SPIFrequency, "imu spi speed"},
        EEPROMSettings<uint32_t>{m_imu.getAccelerationSamplePeriodRef(), Airbrakes_CFG_IMU_SamplePeriod_us, "imu acceleration sample period"},
        EEPROMSettings<uint32_t>{m_imu.getAngularVelocitySamplePeriodRef(), Airbrakes_CFG_IMU_SamplePeriod_us, "imu angular velocity sample period"},
        EEPROMSettings<uint32_t>{m_imu.getOrientationSamplePeriodRef(), Airbrakes_CFG_IMU_SamplePeriod_us, "imu oreintation sample period"},
        EEPROMSettings<uint32_t>{m_imu.getGravitySamplePeriodRef(), Airbrakes_CFG_IMU_SamplePeriod_us, "imu gravity sample period"},
        EEPROMSettings<float_t>{m_actuator.getActuatorLimitRef(), Airbrakes_CFG_MotorDefaultLimit, "actuator range limit"},
        EEPROMSettings<uint_t>{m_actuator.getEncoderStepsRef(), Airbrakes_CFG_MotorFullStrokeNumEncoderPositions, "actuator number of encoder positions"},
        EEPROMSettings<uint_t>{m_actuator.getMotorStepsRef(), Airbrakes_CFG_MotorFullStrokeNumSteps, "actuator number of steps"},
        EEPROMSettings<Motor::SteppingModes>{m_actuator.getSteppingModeRef(), Motor::SteppingModes::HalfStep, "actuator mode"},
        EEPROMSettings<bool>{m_actuateInFlight, true, "actuate in flight flag"},
        EEPROMSettings<EventDetection::DetectionData>{m_launchDetectionParameters.getDataRef(), {Airbrakes_CFG_LaunchMaximumAltitude_m, Airbrakes_CFG_LaunchMinimumVelocity_mPerS, Airbrakes_CFG_LaunchMinimumAcceleration_mPerS2, Airbrakes_CFG_LaunchMinimumSamples, Airbrakes_CFG_LaunchMinimumTime_ms}, "launch detection parameters"},
        EEPROMSettings<EventDetection::DetectionData>{m_burnoutDetectionParameters.getDataRef(), {Airbrakes_CFG_BurnoutMinimumAltitude_m, Airbrakes_CFG_BurnoutMinimumVelocity_mPerS, Airbrakes_CFG_BurnoutMaximumAcceleration_mPerS2, Airbrakes_CFG_BurnoutMinimumSamples, Airbrakes_CFG_BurnoutMinimumTime_ms}, "coast detection parameters"},
        EEPROMSettings<EventDetection::DetectionData>{m_apogeeDetectionParameters.getDataRef(), {Airbrakes_CFG_ApogeeMinimumAltitude_m, Airbrakes_CFG_ApogeeMaximumVelocity_mPerS, Airbrakes_CFG_ApogeeMaximumAcceleration_mPerS2, Airbrakes_CFG_ApogeeMinimumSamples, Airbrakes_CFG_ApogeeMinimumTime_ms}, "apogee detection parameters"},
        EEPROMSettings<uint_t>{m_stateTransitionSamplePeriod_ms, Airbrakes_CFG_EventDetectionSamplePeriod_ms, "event detection sample period"}
    ),

    //serial systems
    m_inputBuffer(115200),

    //simulation systems
#ifndef NO_TX_HIL
    m_TxHIL(
        m_controller.getCurrentDragRef(),
        m_controller.getRequestedDragRef(),
        m_controller.getFlightPathRef(),
        m_controller.getErrorRef(),
        m_controller.getUpdateRuleDragRef(),
        m_controller.getAdjustedDragRef(),
        m_observer.getPredictedAltitudeRef(),
        m_observer.getPredictedVerticalVelocityRef(),
        m_observer.getPredictedVerticalAccelerationRef(),
        m_observer.getPredictedAngleRef()
    ),
#endif
#ifndef NO_RX_HIL
    m_RxHIL(m_inputBuffer, 
        m_observer.getPredictedAltitudeRef(),
        m_observer.getPredictedVerticalVelocityRef(),
        m_observer.getPredictedVerticalAccelerationRef(),
        m_observer.getPredictedAngleRef()
    ),
#endif
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
    if(m_imu.initialize() != error_t::GOOD){
        Serial.println("Failed to detect the IMU");
        anyError = error_t::ERROR;
    }
    else Serial.println("Initialized the IMU");
    //init motor
    m_actuator.initialize();
    //initialize control syatem
    m_controller.resetInit();
    Serial.println("Initialized the controller");
    if(m_observer.setMode(ObserverModes::Sensor) != error_t::GOOD){
        Serial.println("Failed to place the observer into sensor mode");
        anyError = error_t::ERROR;
    }
    else Serial.println("Placed the observer into sensor mode");
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
    //shutdown motor
    m_actuator.sleep();
}

void Application::updateBackground(){
    //handle HIL updates and command inputs
    if(m_serialRefresh > m_HILRefreshPeriod){
#ifndef NO_TX_HIL
        if(m_HILEnabled) m_TxHIL.sendUpdate();
#endif
        m_inputBuffer.update();
         while(m_inputBuffer.hasData()){
            noInterrupts();
            m_interpreter.readLine();
            interrupts();
#ifndef NO_RX_HIL
            if(m_HILEnabled) m_RxHIL.readLine();
#endif
            m_inputBuffer.clear();
            m_inputBuffer.update();
         }
         m_serialRefresh = 0;
    }
    //update IMU
    m_imu.updateBackground();
    //do tasks for the current state
    switch(m_state){
        case ProgramStates::Standby:
        default:
            standbyTasks();
        break;
        case ProgramStates::Armed:
        case ProgramStates::ReArmed:
            armedTasks();
        break;
        case ProgramStates::Boost:
            boostTasks();
        break;
        case ProgramStates::Coast:
            coastTasks();
        break;
        case ProgramStates::Recovery:
            recoveryTasks();
        break;
    }
}


// ====== standby state =======
void Application::initStandby(){
    m_controller.stop();
    m_actuator.sleep();
}

void Application::standbyTasks(){
    if(m_armFlag) gotoState(ProgramStates::Armed);
}



// ====== armed state ======
void Application::initArmed(){
    //initialize logic variables
    m_stateTransitionCounter = 0;
    m_stateTransitionSampleTimer = 0;
    m_stateTransitionTimer = 0;
    //setup log
    if(!m_log.overrideEnabled()){
        if(m_log.setMode(RocketOS::Telemetry::SDFileModes::Record) != error_t::GOOD) logPrint("Error: failed to place log into recording mode");
        if(m_log.newFile() != error_t::GOOD) logPrint("Error: failed to create a new log file");
    }
    else logPrint("Warning: Log is in override mode");
    logPrint("Info: Beginning airbrakes arming sequence");
    //setup telemetry
    if(!m_telemetry.overrideEnabled()){
        if(m_telemetry.setFileMode(RocketOS::Telemetry::SDFileModes::Record)) logPrint("Error: failed to place telemetry into recording mode");
        if(m_telemetry.newFile() != error_t::GOOD) logPrint("Error: failed to create a new telemetry file");
    }
    else logPrint("Warning: Telemetry is in override mode");
    if(m_bufferFlightTelemetry) logPrint("Info: Telemetry buffer mode is enabled");
    else logPrint("Info: Telemetry record mode is enabled");
    //setup observer
    if(!m_HILEnabled){
        if(m_observer.setMode(ObserverModes::Sensor) != error_t::GOOD){
            logPrint("Error: Failed to place the observer into sensor mode");
        }
    }
    else logPrint("Warning: System is in simulation mode");
    //setup motor
    if(!m_actuateInFlight) logPrint("Warning: Actuation is disabled");
    m_actuator.sleep();
    //setup controller
    m_controller.stop();
    //setup flight plan
    if(!m_flightPlan.isLoaded()){
        if(m_flightPlan.loadFromFile() != error_t::GOOD) logPrint("Error: Unable to load a flight plan");
        else logPrint("Warning: Flight plan had to be reloaded");
    }
    //log state transition
    logPrint("Info: Airbrakes arming sequence complete");
}

void Application::initReArmed(){
    //initialize logic variables
    m_stateTransitionCounter = 0;
    m_stateTransitionSampleTimer = 0;
    m_stateTransitionTimer = 0;
    logPrint("Info: Begining airbrakes re-arming sequence");
    //switch log mode
    if(!m_log.overrideEnabled()){
        if(m_log.setMode(RocketOS::Telemetry::SDFileModes::Record) != error_t::GOOD) logPrint("Error: failed to place log into recording mode");
    }
    //switch log mode
    if(!m_telemetry.overrideEnabled()){
        if(m_telemetry.setFileMode(RocketOS::Telemetry::SDFileModes::Record)) logPrint("Error: failed to place telemetry into recording mode");
    }
    //setup motor
    m_actuator.sleep();
    //setup controller
    m_controller.stop();
    //log state transition
    logPrint("Info: Airbrakes re-arming sequence complete");
}

void Application::armedTasks(){
    //log telemetry
    if(m_telemetry.ready()){
        m_telemetry.logLine();
        m_telemetry.clearReady();
    }
    //check for launch
    if(m_stateTransitionSampleTimer >= m_stateTransitionSamplePeriod_ms){
        m_stateTransitionSampleTimer = 0;
        if(m_observer.getVeritcalVelocity() > m_launchDetectionParameters.getVerticalVelocityThreshold() && 
            m_observer.getVerticalAcceleration() > m_launchDetectionParameters.getVerticalAccelerationThreshold() && 
            m_observer.getAltitude() > m_launchDetectionParameters.getAltitudeThreshold() && 
            m_stateTransitionTimer > m_launchDetectionParameters.getTimeThreshold()
        ){
            m_stateTransitionCounter++;
            if(m_stateTransitionCounter >= m_launchDetectionParameters.getConsecutiveSamplesThreshold()){
                logPrint("Info: Detected launch");
                gotoState(ProgramStates::Boost);
                return;
            }
        }
        else m_stateTransitionCounter = 0;
    }
    //check for disarm
    if(!m_armFlag){
        //save remaining telemetry
        if(!m_log.overrideEnabled()){
            m_log.close();
        }
        if(!m_log.overrideEnabled()){
            m_telemetry.close();
        }
        logPrint("Info: Airbrakes was disarmed");
        gotoState(ProgramStates::Standby);
        return;
    }
}



// ====== Boost State ======
void Application::initBoost(){
    //initialize logic variables
    m_stateTransitionCounter = 0;
    m_stateTransitionSampleTimer = 0;
    m_stateTransitionTimer = 0;
    //log start of state transition
    logPrint("Info: Initializing boost mode");
    //switch log and telemetry mode
    if(m_bufferFlightTelemetry){
        if(m_log.setMode(RocketOS::Telemetry::SDFileModes::Buffer) != error_t::GOOD) logPrint("Error: Failed to switch log to buffer mode");
        if(m_telemetry.setFileMode(RocketOS::Telemetry::SDFileModes::Buffer) != error_t::GOOD) logPrint("Error: Failed to switch telemetry to buffer mode");
    }
    //disable controller if coming from false burnout
    m_controller.stop();
    //enable motor
    m_actuator.setTargetDeployment(0);
    if(m_actuateInFlight) m_actuator.wake();
    //log state transition
    logPrint("Info: Boost mode initialized");
}

void Application::boostTasks(){
    //log telemetry
    if(m_telemetry.ready()){
        error_t error = m_telemetry.logLine();
        //flush buffer if overflow occurs
        if(error == RocketOS::Telemetry::SDFile::ERROR_BufferOverflow){
            m_telemetry.flush();
            m_telemetry.logLine();
            logPrint("Info: Telemetry buffer overflow detected");
        }
        m_telemetry.clearReady();
    }
    //check for state transition
    if(m_stateTransitionSampleTimer >= m_stateTransitionSamplePeriod_ms){
        m_stateTransitionSampleTimer = 0;
        //check for false boost
        if(m_observer.getVeritcalVelocity() < m_launchDetectionParameters.getVerticalVelocityThreshold() && 
            m_observer.getVerticalAcceleration() < m_launchDetectionParameters.getVerticalAccelerationThreshold() && 
            m_observer.getAltitude() < m_launchDetectionParameters.getAltitudeThreshold()
        ){
            m_stateTransitionCounter++;
            if(m_stateTransitionCounter >= m_launchDetectionParameters.getConsecutiveSamplesThreshold()){
                logPrint("Info: False launch detected");
                gotoState(ProgramStates::ReArmed);
                return;
            }
        }
        //check for coast
        else if(m_observer.getVeritcalVelocity() > m_burnoutDetectionParameters.getVerticalVelocityThreshold() && 
            m_observer.getVerticalAcceleration() < m_burnoutDetectionParameters.getVerticalAccelerationThreshold() && 
            m_observer.getAltitude() > m_burnoutDetectionParameters.getAltitudeThreshold() && 
            m_stateTransitionTimer > m_burnoutDetectionParameters.getTimeThreshold()
        ){
            m_stateTransitionCounter++;
            if(m_stateTransitionCounter >= m_burnoutDetectionParameters.getConsecutiveSamplesThreshold()){
                logPrint("Info: Burnout detected");
                gotoState(ProgramStates::Coast);
                return;
            }
        }
        else m_stateTransitionCounter = 0;
    }
    //check for disarm
    if(!m_armFlag){
        //save remaining telemetry
        if(m_log.overrideEnabled()){
            m_log.close();
        }
        if(m_log.overrideEnabled()){
            m_telemetry.close();
        }
        logPrint("Info: Airbrakes was disarmed");
        gotoState(ProgramStates::Standby);
        return;
    }
}



// ====== Coast State ======

void Application::initCoast(){
    //initialize logic variables
    m_stateTransitionCounter = 0;
    m_stateTransitionSampleTimer = 0;
    m_stateTransitionTimer = 0;
    //log start of state transition
    logPrint("Info: Initializing coast mode");
    //switch log and telemetry mode if coming from false apogee
    if(m_bufferFlightTelemetry){
        if(m_log.setMode(RocketOS::Telemetry::SDFileModes::Buffer) != error_t::GOOD) logPrint("Error: Failed to switch log to buffer mode");
        if(m_telemetry.setFileMode(RocketOS::Telemetry::SDFileModes::Buffer) != error_t::GOOD) logPrint("Error: Failed to switch telemetry to buffer mode");
    }
    //re-enable motor if coming from false apogee
    if(m_actuateInFlight) m_actuator.wake();
    //enable the controller
    if(m_controller.start() != error_t::GOOD) logPrint("Error: Failed to start the controller");
    //log state transition
    logPrint("Info: Coast mode initialized");
}

void Application::coastTasks(){
    //log telemetry
    if(m_telemetry.ready()){
        error_t error = m_telemetry.logLine();
        //flush buffer if overflow occurs
        if(error == RocketOS::Telemetry::SDFile::ERROR_BufferOverflow){
            m_telemetry.flush();
            m_telemetry.logLine();
            logPrint("Info: Telemetry buffer overflow detected");
        }
        m_telemetry.clearReady();
    }
    //check for state transition
    if(m_stateTransitionSampleTimer >= m_stateTransitionSamplePeriod_ms){
        m_stateTransitionSampleTimer = 0;
        //check for false burnout
        if(m_observer.getVeritcalVelocity() < m_burnoutDetectionParameters.getVerticalVelocityThreshold() && 
            m_observer.getVerticalAcceleration() > m_burnoutDetectionParameters.getVerticalAccelerationThreshold() && 
            m_observer.getAltitude() < m_burnoutDetectionParameters.getAltitudeThreshold()
        ){
            m_stateTransitionCounter++;
            if(m_stateTransitionCounter >= m_burnoutDetectionParameters.getConsecutiveSamplesThreshold()){
                logPrint("Info: False burnout detected");
                gotoState(ProgramStates::Boost);
                return;
            }
        }
        //check for apogee
        else if(m_observer.getVeritcalVelocity() < m_apogeeDetectionParameters.getVerticalVelocityThreshold() && 
            m_observer.getVerticalAcceleration() < m_apogeeDetectionParameters.getVerticalAccelerationThreshold() && 
            m_observer.getAltitude() > m_apogeeDetectionParameters.getAltitudeThreshold() && 
            m_stateTransitionTimer > m_apogeeDetectionParameters.getTimeThreshold()
        ){
            m_stateTransitionCounter++;
            if(m_stateTransitionCounter >= m_apogeeDetectionParameters.getConsecutiveSamplesThreshold()){
                logPrint("Info: Apogee detected");
                gotoState(ProgramStates::Recovery);
                return;
            }
        }
        else m_stateTransitionCounter = 0;
    }
    //check for disarm
    if(!m_armFlag){
        //save remaining telemetry
        if(!m_log.overrideEnabled()){
            m_log.close();
        }
        if(!m_log.overrideEnabled()){
            m_telemetry.close();
        }
        logPrint("Info: Airbrakes was disarmed");
        gotoState(ProgramStates::Standby);
        return;
    }
}



// ====== Recovery State ======
void Application::initRecovery(){
    //initialize logic variables
    m_stateTransitionCounter = 0;
    m_stateTransitionSampleTimer = 0;
    m_stateTransitionTimer = 0;
    //log start of state transition
    logPrint("Info: Initializing recovery mode");
    //switch log and telemetry mode
    if(m_bufferFlightTelemetry){
        if(m_log.setMode(RocketOS::Telemetry::SDFileModes::Record) != error_t::GOOD) logPrint("Error: Failed to switch log to recording mode");
        if(m_telemetry.setFileMode(RocketOS::Telemetry::SDFileModes::Record) != error_t::GOOD) logPrint("Error: Failed to switch telemetry to recording mode");
    }
    //disable the controller
    m_controller.stop();
    //initiate airbrakes retraction
    m_actuator.setTargetDeployment(0);
    //log state transition
    logPrint("Info: Recovery mode initialized");
}

void Application::recoveryTasks(){
    //log telemetry
    if(m_telemetry.ready()){
        error_t error = m_telemetry.logLine();
        //flush buffer if overflow occurs
        if(error == RocketOS::Telemetry::SDFile::ERROR_BufferOverflow){
            m_telemetry.flush();
            m_telemetry.logLine();
            logPrint("Info: Telemetry buffer overflow detected");
        }
        m_telemetry.clearReady();
    }
    //shutdown motor if retraction is complete
    if(m_actuator.onTarget()) m_actuator.sleep();
    //check for state transition
    if(m_stateTransitionSampleTimer >= m_stateTransitionSamplePeriod_ms){
        m_stateTransitionSampleTimer = 0;
        //check for false apogee
        if(m_observer.getVeritcalVelocity() > m_apogeeDetectionParameters.getVerticalVelocityThreshold() && 
            m_observer.getVerticalAcceleration() > m_apogeeDetectionParameters.getVerticalAccelerationThreshold() && 
            m_observer.getAltitude() < m_apogeeDetectionParameters.getAltitudeThreshold()
        ){
            m_stateTransitionCounter++;
            if(m_stateTransitionCounter >= m_apogeeDetectionParameters.getConsecutiveSamplesThreshold()){
                logPrint("Info: False apogee detected");
                gotoState(ProgramStates::Coast);
                return;
            }
        }
        else m_stateTransitionCounter = 0;
    }
    //check for disarm
    if(!m_armFlag){
        //save remaining telemetry
        if(!m_log.overrideEnabled()){
            m_log.close();
        }
        if(!m_log.overrideEnabled()){
            m_telemetry.close();
        }
        logPrint("Info: Airbrakes was disarmed");
        gotoState(ProgramStates::Standby);
        return;
    }
}



//state transition
void Application::gotoState(ProgramStates newState){
    switch(newState){
        case ProgramStates::Standby:
        default:
            initStandby();
            m_state = ProgramStates::Standby;
            m_stateName = APP_STANDBY_STATE_NAME;
        break;
        case ProgramStates::Armed:
            initArmed();
            m_state = ProgramStates::Armed;
            m_stateName = APP_ARMED_STATE_NAME;
        break;
        case ProgramStates::ReArmed:
            initReArmed();
            m_state = ProgramStates::ReArmed;
            m_stateName = APP_ARMED_STATE_NAME;
        break;
        case ProgramStates::Boost:
            initBoost();
            m_state = ProgramStates::Boost;
            m_stateName = APP_BOOST_STATE_NAME;
        break;
        case ProgramStates::Coast:
            initCoast();
            m_state = ProgramStates::Coast;
            m_stateName = APP_COAST_STATE_NAME;
        break;
        case ProgramStates::Recovery:
            initRecovery();
            m_state = ProgramStates::Recovery;
            m_stateName = APP_RECOVERY_STATE_NAME;
        break;
    }
}


//helper functions
void Application::logPrint(const char* message){
    if(m_log.logLine(message) == RocketOS::Telemetry::SDFile::ERROR_BufferOverflow){
        m_log.flush();
        m_log.logLine("Info: Log buffer overflow detected");
        Serial.println("Info: Log buffer overflow detected");
        m_log.logLine(message);
    }
    Serial.println(message);
}