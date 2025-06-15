#pragma once
#include "RocketOS.h"
#include "AirbrakesGeneral.h"

namespace Airbrakes{
    namespace Controls{
        class FlightPlan{
        private:
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
            FlightPlan(SdFat&, float_t*, uint_t, const char*);

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
            
        };
    }
}