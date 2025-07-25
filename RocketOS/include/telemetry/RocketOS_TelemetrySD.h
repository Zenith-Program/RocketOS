#pragma once
#include "RocketOS_TelemetryGeneral.h"
#include <SdFat.h>

namespace RocketOS{
    namespace Telemetry{

        struct PrintFunctions{
            template<std::size_t t_size>
            static result_t<char*> printToBuffer(char* buffer, uint_t size, const char (&value)[t_size]){
                uint_t targetSize = snprintf(buffer, size, "%s", value);
                if(targetSize>size) return {buffer + size, error_t::ERROR};
                return {buffer + targetSize, error_t::GOOD};
            }

            template<std::size_t t_size>
            static result_t<char*> printToBuffer(char* buffer, uint_t size, char (&value)[t_size]){
                uint_t targetSize = snprintf(buffer, size, "%s", value);
                if(targetSize>size) return {buffer + size, error_t::ERROR};
                return {buffer + targetSize, error_t::GOOD};
            }

            static result_t<char*> printToBuffer(char* buffer, uint_t size, char* const& value);
            static result_t<char*> printToBuffer(char* buffer, uint_t size, const char* const& value);
            static result_t<char*> printToBuffer(char* buffer, uint_t size, const int_t& value);
            static result_t<char*> printToBuffer(char* buffer, uint_t size, const uint_t& value);
            static result_t<char*> printToBuffer(char* buffer, uint_t size, const float_t& value);
            static result_t<char*> printToBuffer(char* buffer, uint_t size, const bool& value);
        };
        
        
        enum class SDFileModes : uint_t{
            Record, Buffer
        };


        class SDFile{
        public:
            static constexpr error_t ERROR_FileAcess = error_t(2);
            static constexpr error_t ERROR_BufferOverflow = error_t(3);
        private:
            SdFat& m_sd;
            FsFile m_file;
            const char* m_fileName;
            SDFileModes m_mode;
            char* const m_buffer;
            const uint_t m_bufferSize;
            char* m_currentBufferPos;
        public:
            SDFile(SdFat&, char*, uint_t);
            SDFile(SdFat&, char*, uint_t, const char*);

            void setFileName(const char*);
            const char* getFileName() const;

            error_t newFile();
            
            error_t setMode(SDFileModes);
            SDFileModes getMode() const;

            error_t flush();

            error_t close();

            template<class T>
            error_t log(const T& value){
                if(m_mode == SDFileModes::Record){
                    if(!m_file.isOpen()) m_file = m_sd.open(m_fileName, O_WRITE | O_CREAT | O_AT_END);
                    if(!m_file) return ERROR_FileAcess;
                    m_file.print(value);
                    return error_t::GOOD;
                } 
                uint_t remainingSpace = (m_buffer + m_bufferSize) - m_currentBufferPos;
                auto result = PrintFunctions::printToBuffer(m_currentBufferPos, remainingSpace, value);
                if(result.error != error_t::GOOD) return ERROR_BufferOverflow;
                m_currentBufferPos = result.data;
                return error_t::GOOD;
            }

        private:

            error_t flushRecord();
            error_t flushBuffer();

            error_t switchToRecord();
            error_t switchToBuffer();
        };
    }
}