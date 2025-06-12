#include "airbrakes/AirbrakesFlightPlan.h"

using namespace Airbrakes;
using namespace Airbrakes::Controls;

FlightPlan::FlightPlan(SdFat& sd, float_t* memory, uint_t size, const char* file) : m_sd(sd), m_memory(memory), m_memorySize(size), m_fileName(file), m_isLoaded(false){}

void FlightPlan::setFileName(const char* file){
    m_fileName = file;
}

const char* FlightPlan::getFileName() const{
    return m_fileName;
}

error_t FlightPlan::loadFromFile(){
    auto errorOut = [this](uint_t code){m_file.close(); m_isLoaded = false; return error_t(code);};
    m_file = m_sd.open(m_fileName, FILE_READ);
    if(!m_file) return errorOut(4); //file failed to open
    //read target apogee
    result_t<float_t> readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(2);
    m_targetApogee = readValue.data;
    //read max veocity
    readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(2);
    m_maxVelocity = readValue.data;
    //read num velocity samples
    readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(2);
    m_numVelocitySamples = readValue.data;
    //read num angle samples
    readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(2);
    m_numAngleSamples = readValue.data;
    //read mesh
    for(uint_t i=0; i<m_numVelocitySamples; i++){
        for(uint_t j=0; j<m_numAngleSamples; j++){
            readValue = readNextFloat();
            if(readValue.error != error_t::GOOD) return errorOut(2);
            result_t<float_t&> mem = indexMem(i,j);
            if(mem.error != error_t::GOOD) return errorOut(3);
            mem.data = readValue.data;
        }
    }
    m_isLoaded = true;
    return error_t::GOOD;
}

bool FlightPlan::isLoaded() const{
    return m_isLoaded;

}

result_t<float_t> FlightPlan::getAltitude(float_t velocity, float_t angle) const{
    result_t<float_t> truncated = indexMem(velocityIndex(velocity), angleIndex(angle));
    result_t<float_t> velocityLeg = indexMem(velocityIndex(velocity) + 1, angleIndex(angle));
    result_t<float_t> angleLeg = indexMem(velocityIndex(velocity), angleIndex(angle) + 1);
    if(truncated.error != error_t::GOOD || velocityLeg.error != error_t::GOOD && velocityLeg.error != error_t(3) || angleLeg.error != error_t::GOOD && angleLeg.error != error_t(3)) return error_t::ERROR;
    if(velocityLeg.error == error_t(3)){ //velocity leg was out of bounds
        if(angleLeg.error == error_t(3)){ //angle leg was out of bounds
            //corner of the mesh case
            return truncated.data;
        }
        //edge of mesh velocity side case
        return (angleLeg.data - truncated.data)/angleIncrement() * (angle - angleIndex(angle) * angleIncrement()) + truncated.data;
    }
    if(angleLeg.error == error_t(3)){ //angle leg was out of bounds
        //edge of mesh angle side case
        return (velocityLeg.data - truncated.data)/velocityIncrement() * (velocity - velocityIndex(velocity) * velocityIncrement()) + truncated.data;
    }
    // center of the mesh case
    return (angleLeg.data - truncated.data)/angleIncrement() * (angle - angleIndex(angle) * angleIncrement()) + (velocityLeg.data - truncated.data)/velocityIncrement() * (velocity - velocityIndex(velocity) * velocityIncrement()) + truncated.data;
}

result_t<float_t> FlightPlan::getTargetApogee() const{
    if(!isLoaded()) return error_t::ERROR;
    return m_targetApogee;
}

result_t<float_t&> FlightPlan::indexMem(uint_t velocityIndex, uint_t angleIndex){
    if(!isLoaded()) return error_t(2);
    if(velocityIndex >= m_numVelocitySamples || angleIndex >= m_numAngleSamples) return error_t(3);
    uint_t index = m_numVelocitySamples * velocityIndex + angleIndex;
    if(index >= m_memorySize) return error_t(4);
    return m_memory[index];
}

result_t<float_t> FlightPlan::indexMem(uint_t velocityIndex, uint_t  angleIndex) const{
    if(!isLoaded()) return error_t(2);
    if(velocityIndex >= m_numVelocitySamples || angleIndex >= m_numAngleSamples) return error_t(3);
    uint_t index = m_numVelocitySamples * velocityIndex + angleIndex;
    if(index >= m_memorySize) return error_t(4);
    return m_memory[index];
}

uint_t FlightPlan::velocityIndex(float_t velocity) const{
    return static_cast<uint_t>(velocity / velocityIncrement());
}

uint_t FlightPlan::angleIndex(float_t angle) const{
    return static_cast<uint_t>(angle / angleIncrement());
}

float_t FlightPlan::velocityIncrement() const{
    return m_maxVelocity / m_numVelocitySamples;
}

float_t FlightPlan::angleIncrement() const{
    return c_maxAngle / m_numAngleSamples;
}

result_t<float_t> FlightPlan::readNextFloat(){
    if(skipToStartOfNextFloat() != error_t::GOOD) return error_t::ERROR;
    result_t<char> readChar = readNextCharacter();
    if(readChar.error != error_t::GOOD) return readChar.error;
    bool negative = false;
    if(readChar.data == '-'){
        negative = true;
        readNextCharacter();
        if(readChar.error != error_t::GOOD) return readChar.error;
    }
    float_t value = 0;
    while(isNumeric(readChar.data)){
        value *= 10;
        // this is where u stoped
    }

}

result_t<char> FlightPlan::readNextCharacter(bool allowEOF){

}

error_t FlightPlan::skipToStartOfNextFloat(){
    int val = m_file.peek();
    while(!isNumeric(static_cast<char>(val)) && static_cast<char>(val) != '-'){
        if(val == -1) return error_t::ERROR;
        m_file.read();
        val = m_file.peek();
    }
    return error_t::GOOD;
}

bool FlightPlan::isNumeric(char c){
    if (c >= '0' && c <= '9') return true;
	return false;
}