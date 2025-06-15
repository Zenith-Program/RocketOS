#include "Airbrakes/AirbrakesApplication.h"
using namespace Airbrakes;
using namespace RocketOS;



static Application& App(){
  //allocate memory in RAM2 for telemetry buffering and flight plan
  DMAMEM static std::array<char, Airbrakes_CFG_TelemetryBufferSize> telemetryBuffer;
  DMAMEM static std::array<char, Airbrakes_CFG_LogBufferSize> logBuffer;
  DMAMEM static std::array<float_t, Airbrakes_CFG_FlightPlanMemorySize> flightPlanMem;
  //create application
  static Application app(telemetryBuffer.data(), telemetryBuffer.size(), logBuffer.data(), logBuffer.size(), flightPlanMem.data(), flightPlanMem.size());
  return app;
}

void setup(){
  App().initialize();
}

void loop(){
  App().updateBackground();
}