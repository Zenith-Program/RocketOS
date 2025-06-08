#include "Airbrakes/AirbrakesApplication.h"
using namespace Airbrakes;
using namespace RocketOS;



static Application& App(){
  DMAMEM static std::array<char, Airbrakes_CFG_TelemetryBufferSize> telemetryBuffer;
  static Application app(telemetryBuffer.data(), telemetryBuffer.size());
  return app;
}

void setup(){
  App().initialize();
}

void loop(){
  App().updateBackground();
}