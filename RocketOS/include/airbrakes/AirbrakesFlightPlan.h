#pragma once
#include "RocketOS.h"
#include "AirbrakesGeneral.h"

namespace Airbrakes{
    namespace Controls{
        class FlightPlan{
        private:
            const char* const m_name;
            SdFat& m_sd;
            FsFile m_file;
            float_t* const m_memory;
            const uint_t m_memorySize;
            float_t m_maxVelocity;
            uint_t m_numVelocitySamples;
            static constexpr float_t c_maxAngle = PI/2;
            uint_t m_numAngleSamples;
            float_t m_targetApogee;
            FileName_t m_fileName;
            bool m_isLoaded;
        public:
            FlightPlan(const char*, SdFat&, float_t*, uint_t, const char*);

            RocketOS::Shell::CommandList getCommands();

            const char* getFileName() const;
            FileName_t& getFileNameRef();

            error_t loadFromFile();
            bool isLoaded() const;

            result_t<float_t> getAltitude(float_t, float_t) const;
            result_t<float_t> getTargetApogee() const;
        private:
            error_t setValueInMesh(float_t, uint_t, uint_t);
            result_t<float_t> getValueInMesh(uint_t, uint_t) const;
            uint_t velocityIndex(float_t) const;
            uint_t angleIndex(float_t) const;
            float_t velocityIncrement() const;
            float_t angleIncrement() const;

            result_t<float_t> readNextFloat();
            error_t skipToStartOfNextFloat();
            static bool isNumeric(char);
            static uint_t getNumberVersion(char);
            static float_t pow10(int_t);
            result_t<char> readNextCharacter(bool=false);

            // ##### COMMAND LIST #####
            using Command = RocketOS::Shell::Command;
            using CommandList = RocketOS::Shell::CommandList;
            using arg_t = RocketOS::Shell::arg_t;

            // === ROOT COMMAND LIST ===
                //list of local commands
                const std::array<Command, 4> c_rootCommands = {
                    Command{"properties", "", [this](arg_t){
                        if(isLoaded()){
                             Serial.printf("Flight plan '%s':\n", m_fileName.data());
                             Serial.printf("Target apogee: %.2f m\n", m_targetApogee);
                             Serial.printf("Vertical velocity: 0 m/s - %.2f m/s with %d samples\n", m_maxVelocity, m_numVelocitySamples);
                             Serial.printf("Angle to horizontal: 0 degrees - %.2f degrees with %d samples\n", c_maxAngle * 180 / PI, m_numAngleSamples);
                             Serial.printf("Using %d kB of available %d kB storage\n", m_numAngleSamples * m_numVelocitySamples * 4 / 1024, m_memorySize * 4 / 1024);
                        }
                        else {
                            Serial.println("No flight plan is loaded");
                            Serial.printf("%d kB available for storage\n", m_memorySize * 4 / 1024);
                        }
                    }},
                    Command{"load", "s", [this](arg_t args){
                        args[0].copyStringData(m_fileName.data(), m_fileName.size());
                        error_t error = loadFromFile();
                        if(error == error_t::GOOD) Serial.printf("Sucesfully loaded flight plan from '%s'\n", getFileName());
                        else if(error == error_t(2)) Serial.printf("Formatting error encountered when loading flight plan from '%s'\n", getFileName());
                        else if(error == error_t(3)) Serial.printf("Failed to load flight plan from '%s' due to lack of allocated memory\n", getFileName());
                        else if(error == error_t(4)) Serial.printf("Failed to open flight plan with file name '%s'\n", getFileName());
                        else Serial.println("Failed to load the flight plan");
                    }},
                    Command{"target", "", [this](arg_t){
                        if(!isLoaded()) Serial.println("No flight plan is loaded");
                        else Serial.printf("%.2f m (%.2f ft)\n", m_targetApogee, m_targetApogee * 3.28);
                    }},
                    Command{"probe", "ff", [this](arg_t args){
                        float_t velocity = args[0].getFloatData();
                        float_t angle_deg = args[1].getFloatData();
                        if(!isLoaded()) Serial.println("No flight plan is loaded");
                        else{
                            result_t<float_t> result = getAltitude(velocity, angle_deg * PI /180);
                            if(result.error != error_t::GOOD) Serial.println("Value is out of range");
                            else Serial.printf("%.2f m\n", result.data);
                        }
                    }}
                };
        };
    }
}