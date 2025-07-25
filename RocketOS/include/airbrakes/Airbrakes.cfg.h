#pragma once

/*Serial Configuration
*/
#define Airbrakes_CFG_SerialRefreshPeriod_ms 10


/*Telemetry Configuration
*/
#define Airbrakes_CFG_TelemetryBufferSize 0x6000 /*0x20000 //128Kb*/
#define Airbrakes_CFG_LogBufferSize 512
#define Airbrakes_CFG_TelemetryRefreshPeriod_ms 100


/*File Configuration
*/
#define Airbrakes_CFG_CommandLocalStringBufferSize 64
#define Airbrakes_CFG_FileNameBufferSize 64


/*Log Configuration
*/
#define Airbrakes_CFG_DefaultLogFile "log.txt"
#define Airbrakes_CFG_DefaultTelemetryFile "telemetry.csv"


/*Flight Plan Configuration
*/
#define Airbrakes_CFG_FlightPlanMemorySize 0x4000 // 2^14 entries or 64Kb
#define Airbrakes_CFG_DefaultFlightPlanFileName "flightPath.csv"


/*Controller Configuration
*/
#define Airbrakes_CFG_ControllerPeriod_us 100000
#define Airbrakes_CFG_DecayRate -2.5


/*Simulation Configuration
*/
#define Airbrakes_CFG_HILRefresh_ms 10


/*Altimeter Configuration
*/
#define Airbrakes_CFG_AltimeterSPIFrequency 4000000
#define Airbrakes_CFG_AltimeterNominalGroundPressure 101325
#define Airbrakes_CFG_AltimeterNominalGroundTemperature 288.15


/*IMU Configuration
*/
#define Airbrakes_CFG_IMU_SPIFrequency 1000000
#define Airbrakes_CFG_IMU_SamplePeriod_us 10000
#define Airbrakes_CFG_IMUBufferSize 64
#define Airbrekes_CFG_IMUTxQueueSize 16
#define Airbrakes_CFG_IMUTxCallbackCaptureSize 12


/*Motor Configuration
*/
#define Airbrakes_CFG_MotorFullStrokeNumEncoderPositions 15000
#define Airbrakes_CFG_MotorFullStrokeNumSteps 745
#define Airbrakes_CFG_MotorDefaultSpeed 1
#define Airbrakes_CFG_MotorDefaultLimit 1
#define Airbrakes_CFG_MotorEncoderDifferentiatorOrder 6


/*Observer Configuration
*/
#define Airbrakes_CFG_ObserverAltimeterSamplePeriod_us 25000
#define Airbrakes_CFG_ObserverIMUSamplePeriod_us 10000
#define Airbrakes_CFG_ObserverFilterDelay_us 400000


/*Detection Configuration
*/
#define Airbrakes_CFG_LaunchMaximumAltitude_m 10
#define Airbrakes_CFG_LaunchMinimumVelocity_mPerS 30
#define Airbrakes_CFG_LaunchMinimumAcceleration_mPerS2 100
#define Airbrakes_CFG_LaunchMinimumSamples 3
#define Airbrakes_CFG_LaunchMinimumTime_ms 10000

#define Airbrakes_CFG_BurnoutMinimumAltitude_m 70
#define Airbrakes_CFG_BurnoutMinimumVelocity_mPerS 100
#define Airbrakes_CFG_BurnoutMaximumAcceleration_mPerS2 -10
#define Airbrakes_CFG_BurnoutMinimumSamples 3
#define Airbrakes_CFG_BurnoutMinimumTime_ms 400

#define Airbrakes_CFG_ApogeeMinimumAltitude_m 500
#define Airbrakes_CFG_ApogeeMaximumVelocity_mPerS 5
#define Airbrakes_CFG_ApogeeMaximumAcceleration_mPerS2 0
#define Airbrakes_CFG_ApogeeMinimumSamples 3
#define Airbrakes_CFG_ApogeeMinimumTime_ms 6000

#define Airbrakes_CFG_EventDetectionSamplePeriod_ms 50

