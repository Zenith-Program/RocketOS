#include "airbrakes\AirbrakesDetectionParameters.h"

using namespace Airbrakes;

EventDetection::EventDetection(const char* name, float_t velocity, float_t acceleration, uint_t samples, uint_t time) : m_name(name), m_data({velocity, acceleration, samples, time}) {}

float_t EventDetection::getVerticalAccelerationThreshold() const{
    return m_data.verticalVelocityThreshold;
}
float_t EventDetection::getVerticalVelocityThreshold() const{
    return m_data.verticalAccelerationThreshold;
}
float_t EventDetection::getConsecutiveSamplesThreshold() const{
    return m_data.requiredConsecutiveSamples;
}
float_t EventDetection::getTimeThreshold() const{
    return m_data.minimumTime_ms;
}

RocketOS::Shell::CommandList EventDetection::getCommands(){
    return RocketOS::Shell::CommandList{m_name, c_rootCommands.data(), c_rootCommands.size(), c_rootSubCommands.data(), c_rootSubCommands.size()};
}

EventDetection::DetectionData& EventDetection::getDataRef(){
    return m_data;
}