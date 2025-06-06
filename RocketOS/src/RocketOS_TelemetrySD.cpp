#include "telemetry\RocketOS_TelemetrySD.h"
#include <cstring>

using namespace RocketOS;
using namespace Telemetry;

#define SD_OpenNew O_WRITE | O_CREAT | O_TRUNC
#define SD_OpenAppend O_WRITE | O_CREAT | O_AT_END

SDFile::SDFile(SdFat& sd, char* buffer, uint_t bufferSize) : m_sd(sd), m_fileName(RocketOS_Telemetry_SDDefaultFileName), m_mode(SDFileModes::Record), m_buffer(buffer), m_bufferSize(bufferSize), m_currentBufferPos(buffer){}

SDFile::SDFile(SdFat& sd, char* buffer, uint_t bufferSize, const char* name) : m_sd(sd),  m_fileName(name), m_mode(SDFileModes::Record), m_buffer(buffer), m_bufferSize(bufferSize), m_currentBufferPos(buffer){}

void SDFile::setFileName(const char* name){
    close();
    m_fileName = name;
}
const char* SDFile::getFileName() const{
    return m_fileName;
}

error_t SDFile::setMode(SDFileModes newMode){
    if(m_mode != newMode){
        if(newMode == SDFileModes::Buffer) return switchToBuffer();
        return switchToRecord();
    }
    return error_t::GOOD;
}
SDFileModes SDFile::getMode() const{
    return m_mode;
}

error_t SDFile::flush(){
    if(m_mode == SDFileModes::Record) return flushRecord();
    error_t error = flushBuffer();
    m_file.close();
    return error;
}

error_t SDFile::close(){
    if(m_mode == SDFileModes::Buffer) return flush();
    m_file.close();
    return error_t::GOOD;
}

error_t SDFile::newFile(){
    m_file.close();
    m_file = m_sd.open(m_fileName, SD_OpenNew);
    error_t error = static_cast<bool>(m_file) == false;
    if(m_mode == SDFileModes::Buffer) m_file.close();
    return error;
}

//private mode specific implementations

error_t SDFile::flushRecord(){
    m_file.flush();
    return error_t::GOOD;
}

error_t SDFile::flushBuffer(){
    *m_currentBufferPos = '\0';
    m_file = m_sd.open(m_fileName, SD_OpenAppend);
    if(!m_file) return error_t::ERROR;
    m_file.print(m_buffer);
    m_currentBufferPos = m_buffer;
    m_file.flush();
    return error_t::GOOD;
}

error_t SDFile::switchToRecord(){
    error_t error = flushBuffer();
    m_mode = SDFileModes::Record;
    return error;
}

error_t SDFile::switchToBuffer(){
    m_file.close();
    m_mode = SDFileModes::Buffer;
    return error_t::GOOD;
}

//implementation of printFunctions
result_t<char*> PrintFunctions::printToBuffer(char* buffer, uint_t size, char* const& value){
    uint_t targetSize = snprintf(buffer, size, "%s", value);
    if(targetSize>size) return {buffer + size, error_t::ERROR};
    return {buffer + targetSize, error_t::GOOD};
}

result_t<char*> PrintFunctions::printToBuffer(char* buffer, uint_t size, const char* const& value){
    uint_t targetSize = snprintf(buffer, size, "%s", value);
    if(targetSize>size) return {buffer + size, error_t::ERROR};
    return {buffer + targetSize, error_t::GOOD};
}

result_t<char*> PrintFunctions::printToBuffer(char* buffer, uint_t size, const int_t& value){
    uint_t targetSize = snprintf(buffer, size, "%d", static_cast<int>(value));
    if(targetSize>size) return {buffer + size, error_t::ERROR};
    return {buffer + targetSize, error_t::GOOD};
}

result_t<char*> PrintFunctions::printToBuffer(char* buffer, uint_t size, const uint_t& value){
    uint_t targetSize = snprintf(buffer, size, "%d", static_cast<int>(value));
    if(targetSize>size) return {buffer + size, error_t::ERROR};
    return {buffer + targetSize, error_t::GOOD};
}

result_t<char*> PrintFunctions::printToBuffer(char* buffer, uint_t size, const float_t& value){
    uint_t targetSize = snprintf(buffer, size, "%f", value);
    if(targetSize>size) return {buffer + size, error_t::ERROR};
    return {buffer + targetSize, error_t::GOOD};
}