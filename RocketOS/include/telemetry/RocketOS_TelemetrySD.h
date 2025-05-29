#pragma once
#include "RocketOS_TelemetryGeneral.h"
#include <SdFat.h>

namespace RocketOS{
    namespace Telemetry{

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

            error_t log(const char*);
            
            error_t setMode(SDFileModes);
            SDFileModes getMode() const;

            error_t flush();

            error_t close();
        private:
            error_t logRecord(const char*);
            error_t logBuffer(const char*);

            error_t flushRecord();
            error_t flushBuffer();

            error_t switchToRecord();
            error_t switchToBuffer();
        };
    }
}