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
    m_launchDetectionParameters("launch", Airbrakes_CFG_LaunchMinimumVelocity_mPerS, Airbrakes_CFG_LaunchMinimumAcceleration_mPerS2, Airbrakes_CFG_LaunchMinimumSamples, Airbrakes_CFG_LaunchMinimumTime_ms),
    m_coastDetectionParameters("coast", Airbrakes_CFG_CoastMinimumVelocity_mPerS, Airbrakes_CFG_CoastMaximumAcceleration_mPerS2, Airbrakes_CFG_CoastMinimumSamples, Airbrakes_CFG_CoastMinimumTime_ms),
    m_apogeeDetectionParameters("apogee", Airbrakes_CFG_ApogeeMaximumVelocity_mPerS, Airbrakes_CFG_ApogeeMaximumAcceleration_mPerS2, Airbrakes_CFG_ApogeeMinimumSamples, Airbrakes_CFG_ApogeeMinimumTime_ms),
    //peripherals
    m_altimeter("altimeter", Airbrakes_CFG_AltimeterNominalGroundTemperature, Airbrakes_CFG_AltimeterNominalGroundPressure, Airbrakes_CFG_AltimeterSPIFrequency, TeensyTimerTool::TMR1),
    m_imu("imu", Airbrakes_CFG_IMU_SPIFrequency, Airbrakes_CFG_IMU_SamplePeriod_us),
    m_actuator("motor"),
    //control syatems
    m_controller("controller", 100000, m_flightPlan, m_observer, Airbrakes_CFG_DecayRate),
    m_flightPlan("plan", m_sdCard, flightPlanMem, flightPlanMemSize, Airbrakes_CFG_DefaultFlightPlanFileName),
    m_observer(m_imu, m_altimeter),
    m_simulationType(ObserverModes::Sensor),
    //telemetry systems
    m_telemetry("telemetry", m_sdCard, telemetryBuffer, telemetryBufferSize, Airbrakes_CFG_DefaultTelemetryFile, Airbrakes_CFG_TelemetryRefreshPeriod_ms,
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
        EEPROMSettings<uint_t>{m_imu.getSPIFrequencyRef(), Airbrakes_CFG_IMU_SPIFrequency, "imu spi speed"},
        EEPROMSettings<uint32_t>{m_imu.getAccelerationSamplePeriodRef(), Airbrakes_CFG_IMU_SamplePeriod_us, "imu acceleration sample period"},
        EEPROMSettings<uint32_t>{m_imu.getAngularVelocitySamplePeriodRef(), Airbrakes_CFG_IMU_SamplePeriod_us, "imu angular velocity sample period"},
        EEPROMSettings<uint32_t>{m_imu.getOrientationSamplePeriodRef(), Airbrakes_CFG_IMU_SamplePeriod_us, "imu oreintation sample period"},
        EEPROMSettings<uint32_t>{m_imu.getGravitySamplePeriodRef(), Airbrakes_CFG_IMU_SamplePeriod_us, "imu gravity sample period"},
        EEPROMSettings<float_t>{m_actuator.getActuatorLimitRef(), Airbrakes_CFG_MotorDefaultLimit, "actuator range limit"},
        EEPROMSettings<uint_t>{m_actuator.getEncoderStepsRef(), Airbrakes_CFG_MotorFullStrokeNumEncoderPositions, "actuator number of encoder positions"},
        EEPROMSettings<uint_t>{m_actuator.getMotorStepsRef(), Airbrakes_CFG_MotorFullStrokeNumSteps, "actuator number of steps"},
        EEPROMSettings<Motor::SteppingModes>{m_actuator.getSteppingModeRef(), Motor::SteppingModes::HalfStep, "actuator mode"},
        EEPROMSettings<EventDetection::DetectionData>{m_launchDetectionParameters.getDataRef(), {Airbrakes_CFG_LaunchMinimumVelocity_mPerS, Airbrakes_CFG_LaunchMinimumAcceleration_mPerS2, Airbrakes_CFG_LaunchMinimumSamples, Airbrakes_CFG_LaunchMinimumTime_ms}, "launch detection parameters"},
        EEPROMSettings<EventDetection::DetectionData>{m_coastDetectionParameters.getDataRef(), {Airbrakes_CFG_CoastMinimumVelocity_mPerS, Airbrakes_CFG_CoastMaximumAcceleration_mPerS2, Airbrakes_CFG_CoastMinimumSamples, Airbrakes_CFG_CoastMinimumTime_ms}, "coast detection parameters"},
        EEPROMSettings<EventDetection::DetectionData>{m_apogeeDetectionParameters.getDataRef(), {Airbrakes_CFG_ApogeeMaximumVelocity_mPerS, Airbrakes_CFG_ApogeeMaximumAcceleration_mPerS2, Airbrakes_CFG_ApogeeMinimumSamples, Airbrakes_CFG_ApogeeMinimumTime_ms}, "apogee detection parameters"}
    ),

    //serial systems
    m_inputBuffer(115200),

    //simulation systems
#ifndef NO_TX_HIL
    m_TxHIL(
        m_observer.getPredictedAltitudeRef(),
        m_observer.getPredictedVerticalVelocityRef(),
        m_observer.getPredictedVerticalAccelerationRef(),
        m_observer.getPredictedAngleRef(),
        m_observer.getMeasuredPressureRef(),
        m_observer.getMeasuredTemperatureRef(),
        m_observer.getMeasuredAltitudeRef()
    ),
#endif
#ifndef NO_RX_HIL
    m_RxHIL(m_inputBuffer, 
        m_observer.getPredictedAltitudeRef(),
        m_observer.getPredictedVerticalVelocityRef(),
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
            armedTasks();
        break;
        case ProgramStates::Boost:
            boostTasks();
        break;
        case ProgramStates::Coast:
            coastTasks();
        break;
        case ProgramStates::PostApogee:
            postApogeeTasks();
        break;
    }
}

//state tasks
void Application::standbyTasks(){

}

void Application::armedTasks(){
    //log telemetry
    if(m_telemetry.ready()){
        m_telemetry.logLine();
        m_telemetry.clearReady();
    }
    //check for state transition
    if(m_stateTransitionSampleTimer >= m_stateTransitionSamplePeriod){
        if(m_observer.getVeritcalVelocity() > m_launchDetectionParameters.getVerticalVelocityThreshold() && m_observer.getVerticalAcceleration() > m_launchDetectionParameters.getVerticalAccelerationThreshold() && m_stateTransitionTimer > m_launchDetectionParameters.getTimeThreshold()){
            m_stateTransitionCounter++;
            if(m_stateTransitionCounter >= m_launchDetectionParameters.getConsecutiveSamplesThreshold()) gotoState(ProgramStates::Boost);
        }
        else m_stateTransitionCounter = 0;
    }
}

void Application::boostTasks(){

}

void Application::coastTasks(){

}

void Application::postApogeeTasks(){

}

//state transitions
void Application::gotoState(ProgramStates newState){
    switch(newState){
        case ProgramStates::Standby:
        default:
            initStandby();
            m_state = ProgramStates::Standby;
        break;
        case ProgramStates::Armed:
            initArmed();
            m_state = ProgramStates::Armed;
        break;
        case ProgramStates::Boost:
            initBoost();
            m_state = ProgramStates::Boost;
        break;
        case ProgramStates::Coast:
            initCoast();
            m_state = ProgramStates::Coast;
        break;
        case ProgramStates::PostApogee:
            initPostApogee();
            m_state = ProgramStates::PostApogee;
        break;
    }
}

void Application::initStandby(){
    m_controller.stop();
    m_actuator.sleep();
}

void Application::initArmed(){

}

void Application::initBoost(){

}

void Application::initCoast(){

}

void Application::initPostApogee(){
    m_controller.stop();
    m_actuator.setTargetDeployment(0);

}