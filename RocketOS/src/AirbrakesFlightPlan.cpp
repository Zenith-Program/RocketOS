#include "airbrakes/AirbrakesFlightPlan.h"

using namespace Airbrakes;
using namespace Airbrakes::Controls;

FlightPlan::FlightPlan(SdFat& sd, float_t* memory, uint_t size, const char* file) : m_sd(sd), m_memory(memory), m_memorySize(size), m_isLoaded(false){
    std::strncpy(m_fileName.data(), file, m_fileName.size()-1);
}

const char* FlightPlan::getFileName() const{
    return m_fileName.data();
}

FileName_t& FlightPlan::getFileNameRef(){
    return m_fileName;
}

error_t FlightPlan::loadFromFile(){
    auto errorOut = [this](uint_t code){m_file.close(); m_isLoaded = false; return error_t(code);};
    m_file = m_sd.open(m_fileName.data(), FILE_READ);
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
    for(uint_t i=0; i<m_numAngleSamples; i++){
        for(uint_t j=0; j<m_numVelocitySamples; j++){
            readValue = readNextFloat();
            if(readValue.error != error_t::GOOD) return errorOut(2);
            error_t memErr = setValueInMesh(readValue.data, m_numVelocitySamples - 1 - j, m_numAngleSamples - 1 - i);
            if(memErr == error_t(3)) return errorOut(3);
            if(memErr != error_t::GOOD) return errorOut(2);
        }
    }
    m_isLoaded = true;
    m_file.close();
    return error_t::GOOD;
}

bool FlightPlan::isLoaded() const{
    return m_isLoaded;

}

result_t<float_t> FlightPlan::getAltitude(float_t velocity, float_t angle) const{
    const error_t OUT_OF_BOUNDS = error_t(2);
    result_t<float_t> truncated = getValueInMesh(velocityIndex(velocity), angleIndex(angle));
    result_t<float_t> velocityLeg = getValueInMesh(velocityIndex(velocity) + 1, angleIndex(angle));
    result_t<float_t> angleLeg = getValueInMesh(velocityIndex(velocity), angleIndex(angle) + 1);
    if(truncated.error != error_t::GOOD || (velocityLeg.error != error_t::GOOD && velocityLeg.error != OUT_OF_BOUNDS) || (angleLeg.error != error_t::GOOD && angleLeg.error != OUT_OF_BOUNDS)) return error_t::ERROR;
    if(velocityLeg.error == OUT_OF_BOUNDS){ //velocity leg was out of bounds
        if(angleLeg.error == OUT_OF_BOUNDS){ //angle leg was out of bounds
            //corner of the mesh case
            return truncated.data;
        }
        //edge of mesh velocity side case
        return (angleLeg.data - truncated.data)/angleIncrement() * (angle - angleIndex(angle) * angleIncrement()) + truncated.data;
    }
    if(angleLeg.error == OUT_OF_BOUNDS){ //angle leg was out of bounds
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

error_t FlightPlan::setValueInMesh(float_t val, uint_t velocityIndex, uint_t angleIndex){
    if(velocityIndex >= m_numVelocitySamples || angleIndex >= m_numAngleSamples) return error_t(2);
    uint_t index = m_numVelocitySamples * velocityIndex + angleIndex;
    if(index >= m_memorySize) return error_t(3);
    m_memory[index] = val;
    return error_t::GOOD;
}

result_t<float_t> FlightPlan::getValueInMesh(uint_t velocityIndex, uint_t  angleIndex) const{
    if(velocityIndex >= m_numVelocitySamples || angleIndex >= m_numAngleSamples) return error_t(2);
    uint_t index = m_numVelocitySamples * velocityIndex + angleIndex;
    if(index >= m_memorySize) return error_t(3);
    return m_memory[index];
}

uint_t FlightPlan::velocityIndex(float_t velocity) const{
    return min(max(0, static_cast<uint_t>(velocity / velocityIncrement())), m_numVelocitySamples-1);
}

uint_t FlightPlan::angleIndex(float_t angle) const{
    return  min(max(0, static_cast<uint_t>(angle / angleIncrement())), m_numAngleSamples);
}

float_t FlightPlan::velocityIncrement() const{
    return m_maxVelocity / m_numVelocitySamples;
}

float_t FlightPlan::angleIncrement() const{
    return c_maxAngle / m_numAngleSamples;
}

result_t<float_t> FlightPlan::readNextFloat(){
    //skip non numeric / non -sign chars
    if(skipToStartOfNextFloat() != error_t::GOOD) return error_t::ERROR; //error skiping non-numeric chars
    //read first number or - sign
    result_t<char> readChar = readNextCharacter();
    if(readChar.error != error_t::GOOD) return readChar.error; //error reading first character
    bool negative = false;
    if(readChar.data == '-'){
        //set negative flag and read next char
        negative = true;
        readChar = readNextCharacter();
        if(readChar.error != error_t::GOOD || !isNumeric(readChar.data)) return readChar.error; //invalid char after - sign 
    }
    //read whole part
    float_t value = 0;
    while(isNumeric(readChar.data)){
        value *= 10;
        value += getNumberVersion(readChar.data);
        readChar = readNextCharacter(true/*no error for eof*/);  
        if(readChar.error != error_t::GOOD) return readChar.error; //error reading next char
    }
    //check for decimal point
    if(readChar.data != '.') return negative ? -value : value;
    readChar = readNextCharacter();
    if(readChar.error != error_t::GOOD || !isNumeric(readChar.data)) return readChar.error; //decimal point followed by invalid character
    float_t decimalPart = 0;
    //read decimal part
    for(int_t i=1; isNumeric(readChar.data); i++){
        decimalPart += getNumberVersion(readChar.data) * pow10(-i); 
        readChar = readNextCharacter(true/*no error for eof*/);
        if(readChar.error != error_t::GOOD) return readChar.error; //error reading next char
    }
    float_t combinedVal = value + decimalPart;
    return negative? -combinedVal : combinedVal;
}

result_t<char> FlightPlan::readNextCharacter(bool allowEOF){
    int value = m_file.read();
    if(value != -1) return static_cast<char>(value);
    if(allowEOF && m_file.available() == 0) return '\0';
    return error_t::ERROR;
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

uint_t FlightPlan::getNumberVersion(char c){
    return static_cast<uint_t>(c - '0');
}

float_t FlightPlan::pow10(int_t n){
    float_t value = 1;
    if (n < 0) {
        for (int_t i = 0; i < (-1 * n); i++)
            value *= 1.0 / 10;
    }
    else {
        for (int_t i = 0; i < n; i++)
            value *= 10;
    }
    return value;
}