#pragma once 
#include "RocketOS_SimulationGeneral.h"
#include <array>
#include <tuple>
#include <Arduino.h>//for serial

namespace RocketOS{
    namespace Simulation{
        template<class... T_types>
        class TxHIL{
        private:
            static constexpr uint_t c_size = sizeof...(T_types);
            static constexpr uint_t c_formatStringSize = (3 + ((c_size>0) ? (2 + (c_size-1)*3) : 2));
            std::tuple<const T_types&...> m_values;
            const std::array<char, c_formatStringSize> m_formatString;
        public:
            template<std::enable_if_t<(std::is_convertible_v<T_types, float_t> && ...), bool> = true>
            TxHIL(const T_types&... values) : m_values(values...), m_formatString(makeFormatString()){}

            void sendUpdate() const{
                sendAll(std::make_index_sequence<c_size>());
            }

        private:
            template<std::size_t... tt_indexSeq>
            void sendAll(std::index_sequence<tt_indexSeq...>) const{
                Serial.printf(m_formatString.data(), static_cast<float_t>(std::get<tt_indexSeq>(m_values))...);
            }

            std::array<char, c_formatStringSize> makeFormatString(){
                std::array<char, c_formatStringSize> format;
                format[0] = '#';
                char* location = format.data()+1;
                if(c_size>0){
                    *(location++) = '%';
                    *(location++) = 'f';
                }
                for(uint_t i=1; i<c_size; i++){
                    *(location++) = ' ';
                    *(location++) = '%';
                    *(location++) = 'f';
                }
                format[format.size()-2] = '\n';
                format[format.size()-1] = '\0';
                return format;
            }
        };
    }
}