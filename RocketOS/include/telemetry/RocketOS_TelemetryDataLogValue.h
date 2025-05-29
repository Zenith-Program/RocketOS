#pragma once
#include "RocketOS_TelemetryGeneral.h"
#include "RocketOS_TelemetrySD.h"

namespace RocketOS{
    namespace Telemetry{
        template<class T>
        struct DataLogSettings{
            const T& value;
            const char* name;
        };


        template<class T>
        class DataLogValue{
        private:
            const T& m_value;
            const char* m_name;
        public:
            DataLogValue(DataLogSettings<T> settings) : m_value(settings.value), m_name(settings.name){}

            error_t logName(SDFile& file){
                return file.log(m_name);
            }
            error_t logValue(SDFile& file){
                 return file.log(m_value);
            }
        };
    }
}