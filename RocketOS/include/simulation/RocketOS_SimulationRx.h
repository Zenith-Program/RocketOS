#pragma once 
#include "RocketOS_SimulationGeneral.h"
#include "RocketOSSerial.h"
#include <tuple>
#include <array>

namespace RocketOS{
    namespace Simulation{
        template<class... T_types>
        class RxHIL{
        private:
            static constexpr uint_t c_size = sizeof...(T_types);
            std::tuple<T_types&...> m_values;
            const SerialInput& m_inputBuffer;

        public:
            template<std::enable_if_t<(std::is_convertible_v<float_t, T_types> && ...), bool> = true>
            RxHIL(const SerialInput& buffer, T_types&... values) : m_values(values...), m_inputBuffer(buffer) {}

            void readLine(){
                std::array<float_t, c_size> readValues = {};
                error_t error = error_t::GOOD;
                if(m_inputBuffer[0] != '#') return;
                uint_t bufferIndex = 1;
                for(uint_t i=0; i<readValues.size() && error == error_t::GOOD; i++){
                    result_t<float_t> result = parseFloat(bufferIndex);
                    if(result.error != error_t::GOOD) error = result.error;
                    readValues[i] = result.data;
                }
                if(error != error_t::GOOD) return;
                copyFromArray(readValues, std::make_index_sequence<c_size>());
            }

        private:
            template<size_t... t_indexSeq>
            void copyFromArray(const std::array<float_t, c_size>& arr, std::index_sequence<t_indexSeq...>){
                ((std::get<t_indexSeq>(m_values) = static_cast<T_types>(std::get<t_indexSeq>(arr))), ...);
            }

            result_t<float_t> parseFloat(uint_t& bufferLocation){
                const uint_t bufferSize = static_cast<uint_t>(m_inputBuffer.size());
                skipWhitespace(bufferLocation);
                bool negative = false;
                if(m_inputBuffer[bufferLocation] == '-'){
                    negative = true;
                    bufferLocation++;
                }
                auto result = parseInteger(bufferLocation);
                if(result.error != error_t::GOOD) return result.error;
                float_t value = result.data;
                //add value of decimal part if it exists
                if(bufferLocation < bufferSize && m_inputBuffer[bufferLocation] == '.'){
                    bufferLocation++; //skip decimal point
                    uint_t i;
                    for(i=0; i + bufferLocation<bufferSize && isNumeric(m_inputBuffer[i + bufferLocation]); i++){
                        value += powTen(-(i+1)) * getNumber(m_inputBuffer[bufferLocation + i]);
                    }
                    if(i == 0) return error_t::ERROR; //no decimal part after the decimal point (no numeric characters after the decimal)
                    bufferLocation += i;
                }
                //handle scientific notation case
                if(bufferLocation < bufferSize && m_inputBuffer[bufferLocation] == 'e'){
                    bufferLocation++; //skip e character
                    if(bufferLocation >= bufferSize) return error_t::ERROR; //empty after e character
                    bool negativeExponent = false;
                    if(m_inputBuffer[bufferLocation] == '-'){
                         negativeExponent = true;
                         bufferLocation++;
                    }
                    auto result = parseInteger(bufferLocation);
                    if(result.error != error_t::GOOD) return result.error;
                    int_t exponentValue = result.data;
                    if(negativeExponent) exponentValue *= -1;
                    value *= pow10(exponentValue);
                }
                if(negative) value = -value;
                return value;
            }

            result_t<uint_t> parseInteger(uint_t& bufferLocation){
                const uint_t bufferSize = static_cast<uint_t>(m_inputBuffer.size());
                //find the ones place
                uint_t endOfWholePart;
                for(endOfWholePart = bufferLocation; endOfWholePart < bufferSize && isNumeric(m_inputBuffer[endOfWholePart]); endOfWholePart++);
                if(endOfWholePart == bufferLocation) return error_t::ERROR; //empty whole part (no numeric characters)
                //find value of whole part
                uint_t value = 0;
                for(uint_t i=0; i<(endOfWholePart - bufferLocation); i++){
                    value += powTenU(i) * getNumberU(m_inputBuffer[endOfWholePart - i - 1]);
                }
                bufferLocation = endOfWholePart;
                return value;
            }

            error_t skipWhitespace(uint_t& bufferLocation){
                const uint_t bufferSize = static_cast<uint_t>(m_inputBuffer.size());
                while(isWhitespace(m_inputBuffer[bufferLocation]) && bufferLocation < bufferSize) bufferLocation++;
                if(bufferLocation == bufferSize) return error_t::ERROR;
                return error_t::GOOD;
            }

            static bool isWhitespace(const char c){
                if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f') return true;
	            return false;
            }

            static bool isNumeric(const char c){
                if (c >= '0' && c <= '9') return true;
	            return false;
            }

            static float_t powTen(int_t n){
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

            static float_t getNumber(const char c){
                return static_cast<float_t>(c - '0');
            }

            static uint_t powTenU(uint_t n){
                float_t value = 1;
                for (uint_t i = 0; i < n; i++)
                    value *= 10;
                return value;
            }

            static uint_t getNumberU(const char c){
                return static_cast<uint_t>(c - '0');
            }

            
        };
    }
}