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
                Sleep, Active, TareRetract, TareExtend, Zero
            };

        private:
            const char* const m_name;
            Encoder m_encoder;
            //actuator characteristics
            float_t m_ActuatorLimit;
            uint_t m_numFullStrokeSteps = Airbrakes_CFG_MotorFullStrokeNumSteps;
            uint_t m_numFullStrokeEncoderPositions = Airbrakes_CFG_MotorFullStrokeNumEncoderPositions;
            //motor state
            States m_state;
            int_t m_targetEncoderPosition;
            SteppingModes m_mode;
            Directions m_currentDirection;
            //timing
            uint_t m_stepPeriod_us;
            IntervalTimer m_timer;
            //motion control data
            int_t m_currentEncoderPosition;
            float_t m_currentEncoderDerivative;
            RocketOS::Processing::Differentiator<Airbrakes_CFG_MotorEncoderDifferentiatorOrder> m_encoderDifferentiator;
            uint_t m_calibrationCycleCount;
            elapsedMicros m_difPd;
            uint_t m_calibrationStepCount;

        public:
            Actuator(const char*);
            void initialize();
            void sleep();
            void wake();
            error_t setSteppingCharacteristics(float_t, SteppingModes);
            error_t setSteppingSpeed(float_t);
            error_t setSteppingMode(SteppingModes);
            error_t setTargetDeployment(float_t);
            error_t setActuatorLimit(float_t);
            float_t getCurrentDeployment();
            float_t getTarget() const;
            void beginTare();
            void beginZero();
            RocketOS::Shell::CommandList getCommands();


            float_t& getActuatorLimitRef();
            uint_t& getEncoderStepsRef();
            uint_t& getMotorStepsRef();
            SteppingModes& getSteppingModeRef();

        private:
            void stepISR();
            uint_t getStepPeriod_us(float_t, SteppingModes) const;
            int_t getEncoderPositionFromUnitDeployment(float_t) const;
            float_t getUnitDeploymentFromEncoderPosition(int_t) const;
            void applySteppingMode(SteppingModes);
            void setDirection(Directions);
            static int_t getPeriodConversionPower(SteppingModes, SteppingModes);
            uint_t getNumLimitedEncoderPositions() const;

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
                            if(m_mode == SteppingModes::FullStep) Serial.println("full");
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

                // === TARGET COMMAND LIST ===
                    //commands
                    const std::array<Command, 2> c_targetCommands{
                        Command{"", "", [this](arg_t){
                            Serial.println(getTarget());
                        }},
                        Command{"set", "f", [this](arg_t args){
                            if(setTargetDeployment(args[0].getFloatData()) != error_t::GOOD) Serial.println("Invalid target position");
                        }}
                    };
                // ===========================

                // === LIMIT COMMAND LIST ===
                    //commands
                    const std::array<Command, 2> c_limitCommands{
                        Command{"", "", [this](arg_t){
                            Serial.println(m_ActuatorLimit);
                        }},
                        Command{"set", "f", [this](arg_t args){
                            if( setActuatorLimit(args[0].getFloatData()) != error_t::GOOD) Serial.println("Invalid actuator limit");
                        }}
                    };
                // ==========================
                //command list
                const std::array<CommandList, 3> c_rootCommandList{
                    CommandList{"target", c_targetCommands.data(), c_targetCommands.size(), nullptr, 0},
                    CommandList{"limit", c_limitCommands.data(), c_limitCommands.size(), nullptr, 0},
                    CommandList{"mode", c_modeCommands.data(), c_modeCommands.size(), nullptr, 0}
                };

                //commands
                const std::array<Command, 6> c_rootCommands{
                    Command{"position", "", [this](arg_t){
                        Serial.println(getCurrentDeployment());
                    }},
                    Command{"start", "", [this](arg_t){
                        wake();
                    }},
                    Command{"stop", "", [this](arg_t){
                        sleep();
                    }},
                    Command{"zero", "", [this](arg_t){
                       beginZero();
                    }},
                    Command{"tare", "", [this](arg_t){
                        beginTare();
                    }},
                    Command{"speed", "f", [this](arg_t args){
                        setSteppingSpeed(args[0].getFloatData());
                    }}
                };
            // =========================
        };
    }
}