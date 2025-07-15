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

            enum class States{
                Sleep, Active, Tare, Zero
            };

        private:
            const char* const m_name;
            Encoder m_encoder;

            uint_t m_numLimitedEncoderPositions;
            uint_t m_numFullStrokeSteps = Airbrakes_CFG_MotorFullStrokeNumSteps;
            uint_t m_numFullStrokeEncoderPositionns = Airbrakes_CFG_MotorFullStrokeNumEncoderPositions;

            States m_state;
            int_t m_targetEncoderPosition;
            SteppingModes m_mode;
            Directions m_currentDirection;

            uint_t m_stepPeriod_us;
            IntervalTimer m_timer;
            
            int_t m_currentEncoderPosition;
            int_t m_currentEncoderError;

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


            const int_t& getEncoderPosRef() const;
            const int_t& getTargetEncoderRef() const;
            const int_t& getErrorRef() const;

        private:
            void stepISR();
            uint_t getStepPeriod_us(float_t, SteppingModes) const;
            int_t getEncoderPositionFromUnitDeployment(float_t) const;
            float_t getUnitDeploymentFromEncoderPosition(int_t) const;
            void applySteppingMode(SteppingModes);
            void setDirection(Directions);
            static int_t getPeriodConversionPower(SteppingModes, SteppingModes);

        private:
            // ######### command structure #########
            using Command = RocketOS::Shell::Command;
            using CommandList = RocketOS::Shell::CommandList;
            using arg_t = RocketOS::Shell::arg_t;

            // === ROOT COMMAND LIST ===
                //children command lists
                // === MODE COMMAND LIST ===
                    //commands
                    const std::array<Command, 5> c_modeCommands{
                        Command{"", "", [this](arg_t){
                            if(m_mode == SteppingModes::MicroStep) Serial.println("micro");
                            if(m_mode == SteppingModes::QuarterStep) Serial.println("quarter");
                            if(m_mode == SteppingModes::HalfStep) Serial.println("half");
                            Serial.println("full");
                        }},
                        Command{"full", "", [this](arg_t){
                            setSteppingMode(SteppingModes::FullStep);
                        }},
                        Command{"half", "", [this](arg_t){
                            setSteppingMode(SteppingModes::HalfStep);
                        }},
                        Command{"quarter", "", [this](arg_t){
                            setSteppingMode(SteppingModes::QuarterStep);
                        }},
                        Command{"micro", "", [this](arg_t){
                            setSteppingMode(SteppingModes::MicroStep);
                        }}
                    };
                // =========================
                //command list
                const std::array<CommandList, 1> c_rootCommandList{
                    CommandList{"mode", c_modeCommands.data(), c_modeCommands.size(), nullptr, 0}
                };

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
                        Serial.println(m_numLimitedEncoderPositions);
                        Serial.print("EP: ");
                        Serial.println(m_encoder.read());
                    }},
                    Command{"speed", "f", [this](arg_t args){
                        setSteppingSpeed(args[0].getFloatData());
                    }}
                };
            // =========================
        };
    }
}