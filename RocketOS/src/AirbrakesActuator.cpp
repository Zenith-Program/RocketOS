#include "airbrakes\AirbrakesActuator.h"

using namespace Airbrakes;
using namespace Motor;

#define ENCODER_PIN_A 6
#define ENCODER_PIN_B 9

#define DRIVER_PIN_EN 23
#define DRIVER_PIN_MS1 22
#define DRIVER_PIN_MS2 21
#define DRIVER_PIN_I1 20
#define DRIVER_PIN_I2 19
#define DRIVER_PIN_SLP 18
#define DRIVER_PIN_STEP 17
#define DRIVER_PIN_DIR 16

Actuator::Actuator(const char* name) : m_name(name), m_encoder(ENCODER_PIN_A, ENCODER_PIN_B){}

RocketOS::Shell::CommandList Actuator::getCommands(){
    return CommandList{m_name, c_rootCommands.data(), c_rootCommands.size(), nullptr, 0};
}