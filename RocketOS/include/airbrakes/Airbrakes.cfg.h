#pragma once

#define Airbrakes_CFG_TelemetryBufferSize 0x20000 //128Kb
#define Airbrakes_CFG_LogBufferSize 512

#define Airbrakes_CFG_TelemetryRefreshPeriod_ms 100
#define Airbrakes_CFG_SerialRefreshPeriod_ms 10

#define Airbrakes_CFG_CommandLocalStringBufferSize 64
#define Airbrakes_CFG_FileNameBufferSize 64

#define Airbrakes_CFG_DefaultLogFile "log.txt"
#define Airbrakes_CFG_DefaultTelemetryFile "telemetry.csv"

#define Airbrakes_CFG_FlightPlanMemorySize 0x4000 // 2^14 entries or 64Kb
#define Airbrakes_CFG_DefaultFlightPlanFileName "flightPath.csv"

#define Airbrakes_CFG_ControllerPeriod_us 100000
#define Airbrakes_CFG_DecayRate -5

#define Airbrakes_CFG_HILRefresh_ms 10

#define Airbrakes_CFG_AltimeterSPIFrequency 4000000

#define Airbrakes_CFG_IMU_SPIFrequency 500000
#define Airbrakes_CFG_IMUBufferSize 512
