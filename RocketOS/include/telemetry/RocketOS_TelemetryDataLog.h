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
        

        template<class... T_types>
        class DataLog{
        private:
            static constexpr std::size_t c_size = sizeof...(T_types);
            std::tuple<DataLogValue<T_types>...> m_values;
            SDFile m_file;
        public:
            DataLog(SdFat& sd, char* fileBuffer, uint_t fileBufferSize, DataLogSettings<T_types>... settings) : m_values(DataLogValue<T_types>(settings)...), m_file(sd, fileBuffer, fileBufferSize, RocketOS_Telemetry_DefaultTelemetryFileName){}
            DataLog(SdFat& sd, char* fileBuffer, uint_t fileBufferSize, const char* file, DataLogSettings<T_types>... settings) : m_values(DataLogValue<T_types>(settings)...), m_file(sd, fileBuffer, fileBufferSize, file){}

            error_t newFile(){
                error_t error1 = m_file.newFile();
                error_t error2 = logAllHeaders(std::make_index_sequence<c_size>());
                if(error1 != error_t::GOOD) return error1;
                if(error2 != error_t::GOOD) return error2;
                return error_t::GOOD;
            }

            error_t logLine(){
                return logAllLines(std::make_index_sequence<c_size>());
            }

            error_t setFileMode(SDFileModes mode){
                return m_file.setMode(mode);
            }

            SDFileModes getFileMode() const{
                return m_file.getMode();
            }

            error_t flush(){
                return m_file.flush();
            }

            error_t close(){
                return m_file.close();
            }

        private:
            template<std::size_t... tt_indexSeq>
            error_t logAllLines(std::index_sequence<tt_indexSeq...>){
                error_t error = error_t::GOOD;
                ((error = logDataCSV<tt_indexSeq>(error)), ...);
                return error;
            }

            template<std::size_t... tt_indexSeq>
            error_t logAllHeaders(std::index_sequence<tt_indexSeq...>){
                error_t error = error_t::GOOD;
                ((error = logHeaderCSV<tt_indexSeq>(error)), ...);
                return error;
            }

            template<std::size_t tt_index>
            error_t logDataCSV(error_t prevError){
                error_t newError = std::get<tt_index>(m_values).logValue(m_file);
                if(tt_index == c_size-1) m_file.log("\n");
                else m_file.log(",");
                return (prevError != error_t::GOOD )? prevError : newError;
            }

            template<std::size_t tt_index>
            error_t logHeaderCSV(error_t prevError){
                error_t newError = std::get<tt_index>(m_values).logName(m_file);
                if(tt_index == c_size-1) m_file.log("\n");
                else m_file.log(",");
                return (prevError != error_t::GOOD )? prevError : newError;
            }
            
        };
    }
}