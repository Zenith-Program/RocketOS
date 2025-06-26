#pragma once
#include "RocketOS.h"
#include "AirbrakesGeneral.h"
#include <TeensyTimerTool.h>

namespace Airbrakes{
    namespace Sensors{
        class BarometerSPI{
        private:
            static constexpr uint_t c_numCalibrationCoefficients = 8;
            const char* const m_name;
            std::array<uint16_t, c_numCalibrationCoefficients> m_calibrationCoeffieicents;
            uint32_t m_temperatureADC;
            uint32_t m_pressureADC;
            uint_t m_SPIFrequency;
            TeensyTimerTool::OneShotTimer m_timer;
            bool m_inAsyncUpdate;
            bool m_newData;
            float_t m_pressure_pa;
            float_t m_temperature_k;
            float_t m_altitude_m;
        public:
            BarometerSPI(const char*, uint_t, TeensyTimerTool::TimerGenerator*);
            error_t initialize();
            bool initialized() const;
            error_t updateBlocking();
            void updateAsync();
            result_t<float_t> getPressure();
            result_t<float_t> getTemperature();
            float_t getPressureAsync();
            float_t getTemperatureAsync();
            RocketOS::Shell::CommandList getCommands();
            uint_t& getSPIFrequencyRef();
        private:
            void resetDevice();
            result_t<uint16_t> getCalibrationCoefficient(uint_t n);
            void markAsUninitialized();

            void beginPressureConversion();
            void beginTemperatureConversion();
            error_t readPressureVal();
            error_t readTemperatureVal();
            void asyncStep1();
            void asyncStep2();

            void updateOutputValues();


        private:
            // ######### command structure #########
            using Command = RocketOS::Shell::Command;
            using CommandList = RocketOS::Shell::CommandList;
            using arg_t = RocketOS::Shell::arg_t;

            // === ROOT COMMAND LIST ===
                //child command lists
                // === INIT COMMAND LIST === 
                    //commands
                    const std::array<Command, 2> c_initCommands{
                        Command{"get", "", [this](arg_t){
                            if(initialized()) Serial.println("Initialized");
                            else Serial.println("Uninitialized");
                        }},
                        Command{"", "", [this](arg_t){
                            if(initialize() != error_t::GOOD) Serial.println("Failed to initialize");
                            else Serial.println("Sucesfully initialized");
                        }}
                    };
                // =========================

                // === SPEED COMMAND LIST ===
                    //commands
                    const std::array<Command, 2> c_speedCommands{
                        Command{"", "", [this](arg_t){
                            Serial.print(m_SPIFrequency);
                            Serial.println("Hz");
                        }},
                        Command{"set", "u", [this](arg_t args){
                            uint_t newPeriod = args[0].getUnsignedData();
                            m_SPIFrequency = newPeriod;
                        }}
                    };
                // ==========================
                //command list
                const std::array<CommandList, 2> c_rootCommandList{
                    CommandList{"init", c_initCommands.data(), c_initCommands.size(), nullptr, 0},
                    CommandList{"speed", c_speedCommands.data(), c_speedCommands.size(), nullptr, 0}
                };
                //commands
                const std::array<Command, 2> c_rootCommands{
                    Command{"pressure", "", [this](arg_t){
                        result_t<float_t> result = getPressure();
                        if(result.error == error_t::GOOD){ 
                            Serial.print(result.data);
                            Serial.println("pa");
                        }
                        else if(result.error == error_t(2)) Serial.println("Altimeter is not initialized");
                        else Serial.println("Error reading from altimeter");
                    }},
                    Command{"temperature", "", [this](arg_t){
                        result_t<float_t> result = getTemperature();
                        if(result.error == error_t::GOOD){ 
                            Serial.print(result.data);
                            Serial.println("K");
                        }
                        else if(result.error == error_t(2)) Serial.println("Altimeter is not initialized");
                        else Serial.println("Error reading from altimeter");
                    }}
                };
            // =========================


        };
    }
}