#pragma once
#include "RocketOS.h"
#include "AirbrakesGeneral.h"

namespace Airbrakes{
    class EventDetection{
    public:
        struct DetectionData{
            float_t altitudeThreshold;
            float_t verticalVelocityThreshold;
            float_t verticalAccelerationThreshold;
            uint_t requiredConsecutiveSamples;
            uint_t minimumTime_ms;
        };
    private:
        const char* const m_name;
        DetectionData m_data;
    public:
        EventDetection(const char*, float_t, float_t, float_t, uint_t, uint_t);

        float_t getAltitudeThreshold() const;
        float_t getVerticalVelocityThreshold() const;
        float_t getVerticalAccelerationThreshold() const;
        float_t getConsecutiveSamplesThreshold() const;
        float_t getTimeThreshold() const;

        RocketOS::Shell::CommandList getCommands();

        DetectionData& getDataRef();

    private:
        // ######### command structure #########
        using Command = RocketOS::Shell::Command;
        using CommandList = RocketOS::Shell::CommandList;
        using arg_t = RocketOS::Shell::arg_t;

        // === ROOT COMMAND LIST ===
            //children command lists
            // === ALTITUDE COMMAND LIST ===
                //commands
                const std::array<Command, 2> c_altitudeCommands{
                    Command{"", "", [this](arg_t){
                        Serial.print(m_data.altitudeThreshold);
                        Serial.println("m");
                    }},
                    Command{"set", "f", [this](arg_t args){
                        m_data.altitudeThreshold = args[0].getFloatData();
                        if(m_data.altitudeThreshold < 0) Serial.println("Warning: Negative altitude value");
                    }}
                };
            // =================================

            // === VELOCITY COMMAND LIST ===
                //commands
                const std::array<Command, 2> c_velocityCommands{
                    Command{"", "", [this](arg_t){
                        Serial.print(m_data.verticalVelocityThreshold);
                        Serial.println("m/s");
                    }},
                    Command{"set", "f", [this](arg_t args){
                        float_t newValue = args[0].getFloatData();
                        if((newValue < 0 && m_data.verticalVelocityThreshold > 0) || (newValue >0 && m_data.verticalVelocityThreshold < 0)) Serial.println("Warning: Signs differ between old and new values");
                        m_data.verticalVelocityThreshold = newValue;
                    }}
                };
            // =============================

            // === ACCELERATION COMMAND LIST ===
                //commands
                const std::array<Command, 2> c_accelerationCommands{
                    Command{"", "", [this](arg_t){
                        Serial.print(m_data.verticalAccelerationThreshold);
                        Serial.println("m/s^2");
                    }},
                    Command{"set", "f", [this](arg_t args){
                        float_t newValue = args[0].getFloatData();
                        if((newValue < 0 && m_data.verticalAccelerationThreshold > 0) || (newValue >0 && m_data.verticalAccelerationThreshold < 0)) Serial.println("Warning: Signs differ between old and new values");
                        m_data.verticalAccelerationThreshold = newValue;
                    }}
                };
            // =================================

            // === SAMPLES COMMAND LIST ===
                //commands
                const std::array<Command, 2> c_samplesCommands{
                    Command{"", "", [this](arg_t){
                        Serial.print(m_data.requiredConsecutiveSamples);
                        Serial.println(" samples");
                    }},
                    Command{"set", "u", [this](arg_t args){
                        m_data.requiredConsecutiveSamples = args[0].getUnsignedData();
                    }}
                };
            // ============================

            // === TIME COMMAND LIST ===
                //commands
                //commands
                const std::array<Command, 2> c_timeCommands{
                    Command{"", "", [this](arg_t){
                        Serial.print(m_data.minimumTime_ms);
                        Serial.println("ms");
                    }},
                    Command{"set", "u", [this](arg_t args){
                        m_data.minimumTime_ms = args[0].getUnsignedData();
                    }}
                };
            // =========================
            //----------------------
            //sub commmans
            const std::array<CommandList, 5> c_rootSubCommands{
                CommandList{"altitude", c_altitudeCommands.data(), c_altitudeCommands.size(), nullptr, 0},
                CommandList{"velocity", c_velocityCommands.data(), c_velocityCommands.size(), nullptr, 0},
                CommandList{"acceleration", c_accelerationCommands.data(), c_accelerationCommands.size(), nullptr, 0},
                CommandList{"samples", c_samplesCommands.data(), c_samplesCommands.size(), nullptr, 0},
                CommandList{"time", c_timeCommands.data(), c_timeCommands.size(), nullptr, 0}
            };
            //commands
            const std::array<Command, 1> c_rootCommands{
                Command{"properties", "", [this](arg_t){
                    Serial.printf("%s event detection parameters:\n", m_name);
                    Serial.printf("Vertical velocity threshold: %.2fm/s\n", m_data.verticalVelocityThreshold);
                    Serial.printf("Vertical acceleration threshold: %.2fm/s^2\n", m_data.verticalAccelerationThreshold);
                    Serial.printf("Minimum consecutive samples: %d samples\n", m_data.requiredConsecutiveSamples);
                    Serial.printf("Minimum state time: %dms\n", m_data.minimumTime_ms);
                }}
            };
        // =========================
    };
}