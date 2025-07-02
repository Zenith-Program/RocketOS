#include "airbrakes/AirbrakesFlightPlan.h"

using namespace Airbrakes;
using namespace Airbrakes::Controls;

FlightPlan::FlightPlan(const char* name, SdFat& sd, float_t* memory, uint_t size, const char* file) : m_name(name), m_sd(sd), m_memory(memory), m_memorySize(size), m_isLoaded(false){
    strncpy(m_fileName.data(), file, m_fileName.size()-1);
}

RocketOS::Shell::CommandList FlightPlan::getCommands(){
    return CommandList{m_name, c_rootCommands.data(), c_rootCommands.size(), nullptr, 0};
}

const char* FlightPlan::getFileName() const{
    return m_fileName.data();
}

FileName_t& FlightPlan::getFileNameRef(){
    return m_fileName;
}

error_t FlightPlan::loadFromFile(){
    auto errorOut = [this](error_t error){
        m_file.close(); 
        m_isLoaded = false; 
        return error;
    };
    m_file = m_sd.open(m_fileName.data(), FILE_READ);
    if(!m_file) return errorOut(ERROR_File); //file failed to open
    //read target apogee
    result_t<float_t> readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(ERROR_Formating);
    m_targetApogee = readValue.data;
    //read minimum drag area
    readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(ERROR_Formating);
    m_minimumDragArea = readValue.data;
    //read maximim drag area
    readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(ERROR_Formating);
    m_maximumDragArea = readValue.data;
    //read dry mass
    readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(ERROR_Formating);
    m_dryMass = readValue.data;
    //read temperature
    readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(ERROR_Formating);
    m_groundLevelTemperature = readValue.data;
    //read pressure
    readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(ERROR_Formating);
    m_groundLevelPressure = readValue.data;
    //read max veocity
    readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(ERROR_Formating);
    m_maxVelocity = readValue.data;
    //read num velocity samples
    readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(ERROR_Formating);
    m_numVelocitySamples = readValue.data;
    //read num angle samples
    readValue = readNextFloat();
    if(readValue.error != error_t::GOOD) return errorOut(ERROR_Formating);
    m_numAngleSamples = readValue.data;
    //read mesh
    for(uint_t i=0; i<m_numAngleSamples; i++){
        for(uint_t j=0; j<m_numVelocitySamples; j++){
            readValue = readNextFloat();
            if(readValue.error != error_t::GOOD) return errorOut(ERROR_Formating);
            error_t memErr = setValueInMesh(readValue.data, m_numVelocitySamples - 1 - j, m_numAngleSamples - 1 - i);
            if(memErr == error_t(3)) return errorOut(ERROR_Memory);
            if(memErr != error_t::GOOD) return errorOut(ERROR_Formating);
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
    if(!isLoaded()) return ERROR_NotLoaded;
    result_t<float_t> truncated = getValueInMesh(velocityIndex(velocity), angleIndex(angle));
    result_t<float_t> velocityLeg = getValueInMesh(velocityIndex(velocity) + 1, angleIndex(angle));
    result_t<float_t> angleLeg = getValueInMesh(velocityIndex(velocity), angleIndex(angle) + 1);
    return pointSlopeInterpolate(truncated, velocityLeg, angleLeg, velocity, angle);
}

result_t<float_t> FlightPlan::getVelocityPartial(float_t velocity, float_t angle) const{
    if(!isLoaded()) return ERROR_NotLoaded;
    result_t<float_t> truncated = getVelocityPartialInMesh(velocityIndex(velocity), angleIndex(angle));
    result_t<float_t> velocityLeg = getVelocityPartialInMesh(velocityIndex(velocity) + 1, angleIndex(angle));
    result_t<float_t> angleLeg = getVelocityPartialInMesh(velocityIndex(velocity), angleIndex(angle) + 1);
    return pointSlopeInterpolate(truncated, velocityLeg, angleLeg, velocity, angle);
}

result_t<float_t> FlightPlan::getAnglePartial(float_t velocity, float_t angle) const{
    if(!isLoaded()) return ERROR_NotLoaded;
    result_t<float_t> truncated = getAnglePartialInMesh(velocityIndex(velocity), angleIndex(angle));
    result_t<float_t> velocityLeg = getAnglePartialInMesh(velocityIndex(velocity) + 1, angleIndex(angle));
    result_t<float_t> angleLeg = getAnglePartialInMesh(velocityIndex(velocity), angleIndex(angle) + 1);
    return pointSlopeInterpolate(truncated, velocityLeg, angleLeg, velocity, angle);
}

result_t<float_t> FlightPlan::getTargetApogee() const{
    if(!isLoaded()) return error_t::ERROR;
    return m_targetApogee;
}

result_t<float_t> FlightPlan::getMinDragArea() const{
    if(!isLoaded()) return error_t::ERROR;
    return m_minimumDragArea;
}

result_t<float_t> FlightPlan::getMaxDragArea() const{
    if(!isLoaded()) return error_t::ERROR;
    return m_maximumDragArea;
}

result_t<float_t> FlightPlan::getDryMass() const{
    if(!isLoaded()) return error_t::ERROR;
    return m_dryMass;
}

result_t<float_t> FlightPlan::getGroundTemperature() const{
    if(!isLoaded()) return error_t::ERROR;
    return m_groundLevelTemperature;
}

result_t<float_t> FlightPlan::getGroundPressure() const{
    if(!isLoaded()) return error_t::ERROR;
    return m_groundLevelPressure;
}

error_t FlightPlan::setValueInMesh(float_t val, uint_t velocityIndex, uint_t angleIndex){
    if(velocityIndex >= m_numVelocitySamples || angleIndex >= m_numAngleSamples) return ERROR_OutOfBounds;
    uint_t index = m_numVelocitySamples * velocityIndex + angleIndex;
    if(index >= m_memorySize) return ERROR_Logic;
    m_memory[index] = val;
    return error_t::GOOD;
}

result_t<float_t> FlightPlan::getValueInMesh(uint_t velocityIndex, uint_t  angleIndex) const{
    if(velocityIndex >= m_numVelocitySamples || angleIndex >= m_numAngleSamples) return ERROR_OutOfBounds;
    uint_t index = m_numVelocitySamples * velocityIndex + angleIndex;
    if(index >= m_memorySize) return ERROR_Logic;
    return m_memory[index];
}

uint_t FlightPlan::velocityIndex(float_t velocity) const{
    return min(max(0, static_cast<uint_t>(velocity / velocityIncrement())), m_numVelocitySamples-1);
}

uint_t FlightPlan::angleIndex(float_t angle) const{
    return  min(max(0, static_cast<uint_t>(angle / angleIncrement())), m_numAngleSamples-1);
}

float_t FlightPlan::velocityIncrement() const{
    return m_maxVelocity / m_numVelocitySamples;
}

float_t FlightPlan::angleIncrement() const{
    return c_maxAngle / m_numAngleSamples;
}

result_t<float_t> FlightPlan::getVelocityPartialInMesh(uint_t velocityIndex, uint_t angleIndex) const{
    if(velocityIndex >= m_numVelocitySamples || angleIndex >= m_numAngleSamples) return ERROR_OutOfBounds;
    if(velocityIndex == 0){
        return (getValueInMesh(velocityIndex + 1, angleIndex) - getValueInMesh(velocityIndex, angleIndex))/velocityIncrement();
    }
    if(velocityIndex == m_numVelocitySamples - 1){
        return (getValueInMesh(velocityIndex, angleIndex) - getValueInMesh(velocityIndex - 1, angleIndex))/velocityIncrement();
    }
    return (getValueInMesh(velocityIndex + 1, angleIndex) - getValueInMesh(velocityIndex - 1, angleIndex))/(2 * velocityIncrement());
}

result_t<float_t> FlightPlan::getAnglePartialInMesh(uint_t velocityIndex, uint_t angleIndex) const{
    if(velocityIndex >= m_numVelocitySamples || angleIndex >= m_numAngleSamples) return ERROR_OutOfBounds;
    if(angleIndex == 0){
        return (getValueInMesh(velocityIndex, angleIndex+1) - getValueInMesh(velocityIndex, angleIndex))/angleIncrement();
    }
    if(angleIndex == m_numAngleSamples - 1){
        return (getValueInMesh(velocityIndex, angleIndex) - getValueInMesh(velocityIndex, angleIndex-1))/angleIncrement();
    }
    return (getValueInMesh(velocityIndex, angleIndex+1) - getValueInMesh(velocityIndex, angleIndex-1))/(2 * angleIncrement());
}

result_t<float_t> FlightPlan::pointSlopeInterpolate(result_t<float_t> root, result_t<float_t> vLeg, result_t<float_t> angleLeg, float_t velocity, float_t angle) const{
    if(root.error != error_t::GOOD || (vLeg.error != error_t::GOOD && vLeg.error != ERROR_OutOfBounds) || (angleLeg.error != error_t::GOOD && angleLeg.error != ERROR_OutOfBounds)) return error_t(2);
    if(vLeg.error == ERROR_OutOfBounds){ //velocity leg was out of bounds
        if(angleLeg.error == ERROR_OutOfBounds){ //angle leg was out of bounds
            //corner of the mesh case
            return root.data;
        }
        //edge of mesh velocity side case
        return (angleLeg.data - root.data)/angleIncrement() * (angle - angleIndex(angle) * angleIncrement()) + root.data;
    }
    if(angleLeg.error == ERROR_OutOfBounds){ //angle leg was out of bounds
        //edge of mesh angle side case
        return (vLeg.data - root.data)/velocityIncrement() * (velocity - velocityIndex(velocity) * velocityIncrement()) + root.data;
    }
    // center of the mesh case
    return (angleLeg.data - root.data)/angleIncrement() * (angle - angleIndex(angle) * angleIncrement()) + (vLeg.data - root.data)/velocityIncrement() * (velocity - velocityIndex(velocity) * velocityIncrement()) + root.data;
}


//sd card read functions-------------------------------------------------------
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