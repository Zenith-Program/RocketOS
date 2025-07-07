#pragma once
#include "AirbrakesGeneral.h"
#include "RocketOS.h"
#include <Encoder.h>
#include <IntervalTimer.h>

namespace Airbrakes{
    namespace Motor{
        enum class SteppingModes{
            FullStep, HalfStep, QuarterStep, MicroStep
        };

        class Actuator{
        private:
            enum class Directions{
                Extend, Retract
            };

        private:
            static constexpr uint_t c_numFullStrokeSteps = Airbrakes_CFG_MotorFullStrokeNumSteps;
            static constexpr uint_t c_numFullStrokeEncoderPositionns = Airbrakes_CFG_MotorFullStrokeNumEncoderPositions;
            const char* const m_name;
            Encoder m_encoder;
            uint_t m_currentEncoderEndPosition;
            bool m_active;
            uint_t m_targetEncoderPosition;
            SteppingModes m_mode;
            uint_t m_stepPeriod_us;
            IntervalTimer m_timer;

        public:
            Actuator(const char*);
            void initialize();
            void sleep();
            void wake();
            void setSteppingCharacteristics(float_t, SteppingModes);
            void setSteppingSpeed(float_t);
            void setSteppingMode(SteppingModes);
            error_t setTargetDeployment(float_t);
            float_t getCurrentDeployment();
            RocketOS::Shell::CommandList getCommands();

        private:
            void stepISR();
            uint_t getStepPeriod_us(float_t, SteppingModes) const;
            uint_t getEncoderPositionFromUnitDeployment(float_t) const;
            float_t getUnitDeploymentFromEncoderPosition(uint_t) const;
            void applySteppingMode(SteppingModes);
            void setDirectionPin(Directions) const;

        private:
            // ######### command structure #########
            using Command = RocketOS::Shell::Command;
            using CommandList = RocketOS::Shell::CommandList;
            using arg_t = RocketOS::Shell::arg_t;

            // === ROOT COMMAND LIST ===
                //commands
                const std::array<Command, 7> c_rootCommands{
                    Command{"position", "", [this](arg_t){
                        Serial.println(getCurrentDeployment());
                    }},
                    Command{"zero", "", [this](arg_t){
                       m_encoder.write(0);
                    }},
                    Command{"target", "f", [this](arg_t args){
                        if(setTargetDeployment(args[0].getFloatData()) != error_t::GOOD)
                        Serial.println("Invalid position");
                    }},
                    Command{"start", "", [this](arg_t){
                        wake();
                    }},
                    Command{"stop", "", [this](arg_t){
                        sleep();
                    }},
                    Command{"debug", "", [this](arg_t){
                        Serial.print("period: ");
                        Serial.println(m_stepPeriod_us);
                        Serial.print("targetEP: ");
                        Serial.println(m_targetEncoderPosition);
                        Serial.print("maxEP: ");
                        Serial.println(m_currentEncoderEndPosition);
                        Serial.print("EP: ");
                        Serial.println(abs(m_encoder.read()));
                    }},
                    Command{"speed", "f", [this](arg_t args){
                        setSteppingSpeed(args[0].getFloatData());
                    }}
                };
            // =========================
        };
    }
}