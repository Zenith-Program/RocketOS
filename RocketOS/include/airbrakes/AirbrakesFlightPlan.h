#pragma once
#include "RocketOS.h"
#include "AirbrakesGeneral.h"

namespace Airbrakes{
    namespace Controls{
        class FlightPlan{
        private:
            //error codes
            static constexpr error_t ERROR_OutOfBounds = error_t(6);
            static constexpr error_t ERROR_Logic = error_t(7);
        public:
            static constexpr error_t ERROR_Formating = error_t(2);
            static constexpr error_t ERROR_Memory = error_t(3);
            static constexpr error_t ERROR_File = error_t(4);
            static constexpr error_t ERROR_NotLoaded = error_t(5);
        
        private:
            //data
            const char* const m_name;
            SdFat& m_sd;
            FsFile m_file;
            float_t* const m_memory;
            const uint_t m_memorySize;
            float_t m_maxVelocity;
            uint_t m_numVelocitySamples;
            static constexpr float_t c_maxAngle = PI/2;
            uint_t m_numAngleSamples;

            //launch parameters
            float_t m_targetApogee;
            float_t m_minimumDragArea;
            float_t m_maximumDragArea;
            float_t m_dryMass;
            float_t m_groundLevelTemperature;
            float_t m_groundLevelPressure;
            FileName_t m_fileName;
            bool m_isLoaded;
        public:
            //interface
            FlightPlan(const char*, SdFat&, float_t*, uint_t, const char*);

            RocketOS::Shell::CommandList getCommands();

            const char* getFileName() const;
            FileName_t& getFileNameRef();

            error_t loadFromFile();
            bool isLoaded() const;

            result_t<float_t> getAltitude(float_t, float_t) const;
            result_t<float_t> getVelocityPartial(float_t, float_t) const;
            result_t<float_t> getAnglePartial(float_t, float_t) const;
            result_t<float_t> getTargetApogee() const;
            result_t<float_t> getMinDragArea() const;
            result_t<float_t> getMaxDragArea() const;
            result_t<float_t> getDryMass() const;
            result_t<float_t> getGroundTemperature() const;
            result_t<float_t> getGroundPressure() const;
            
        private:
            //helpers
            result_t<float_t> pointSlopeInterpolate(result_t<float_t>, result_t<float_t>, result_t<float_t>, float_t, float_t) const;
            error_t setValueInMesh(float_t, uint_t, uint_t);
            result_t<float_t> getValueInMesh(uint_t, uint_t) const;
            result_t<float_t> getVelocityPartialInMesh(uint_t, uint_t) const;
            result_t<float_t> getAnglePartialInMesh(uint_t, uint_t) const;
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
                             Serial.printf("Target apogee: %.2fm\n", m_targetApogee);
                             Serial.printf("Effective drag area range: %.4fm^2 - %.4fm^2\n", m_minimumDragArea, m_maximumDragArea);
                             Serial.printf("Dry mass: %.2fkg\n", m_dryMass);
                             Serial.printf("Launch site conditions: %.2fC at %.2fpa\n", m_groundLevelTemperature - 273.15, m_groundLevelPressure);
                             Serial.printf("Vertical velocity range: 0m/s - %.2fm/s with %d samples\n", m_maxVelocity, m_numVelocitySamples);
                             Serial.printf("Angle with horizontal range: 0 degrees - %.2f degrees with %d samples\n", c_maxAngle * 180 / PI, m_numAngleSamples);
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
                    Command{"altitude", "ff", [this](arg_t args){
                        float_t velocity = args[0].getFloatData();
                        float_t angle_deg = args[1].getFloatData();
                        if(!isLoaded()) Serial.println("No flight plan is loaded");
                        else{
                            result_t<float_t> result = getAltitude(velocity, angle_deg * PI /180);
                            if(result.error != error_t::GOOD) Serial.println("Value is out of range");
                            else Serial.printf("%.2fm\n", result.data);
                        }
                    }},
                    Command{"gradient", "ff", [this](arg_t args){
                        float_t velocity = args[0].getFloatData();
                        float_t angle_deg = args[1].getFloatData();
                        if(!isLoaded()) Serial.println("No flight plan is loaded");
                        else{
                            float_t velocityPartial = getVelocityPartial(velocity, angle_deg * PI /180);
                            float_t anglePartial = getAnglePartial(velocity, angle_deg * PI /180);
                            Serial.printf("<%.2f, %.2f>\n", velocityPartial, anglePartial);
                        }
                    }}
                };
        };
    }
}