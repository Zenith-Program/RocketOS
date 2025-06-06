#include "Airbrakes/AirbrakesApplication.h"
using namespace Airbrakes;
using namespace RocketOS;

static Application& App(){
  static Application app;
  return app;
}

void setup(){
  App().initialize();
}

void loop(){
  App().updateBackground();
}