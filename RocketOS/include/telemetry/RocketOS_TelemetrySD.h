#pragma once
#include "RocketOS_TelemetryGeneral.h"
#include <SdFat.h>

namespace RocketOS{
    namespace Telemetry{

        template<class T>
        result_t<char*> printToBuffer(char*, uint_t, const T&){
            static_assert(sizeof(T) == 0, "printToBuffer<T> not implemented for this type");
            return error_t::ERROR;
        }

        enum class SDFileModes : uint_t{
            Record, Buffer
        };


        class SDFile{
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
                    if(!m_file) return error_t::ERROR;
                    m_file.print(value);
                    return error_t::GOOD;
                } 
                uint_t remainingSpace = (m_buffer + m_bufferSize) - m_currentBufferPos;
                auto result = printToBuffer(m_currentBufferPos, remainingSpace, value);
                m_currentBufferPos = result.data;
                return result.error;
            }

        private:

            error_t flushRecord();
            error_t flushBuffer();

            error_t switchToRecord();
            error_t switchToBuffer();
        };
    }
}