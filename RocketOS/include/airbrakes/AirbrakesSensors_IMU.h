#pragma once
#include "AirbrakesGeneral.h"
#include "RocketOS.h"
#include <SPI.h>
#include <array>
#include <TeensyTimerTool.h>
//
namespace Airbrakes{
    namespace Sensors{

        enum class IMUStates{
            Uninitialized, Reseting, Configuring, Operational
        };

        enum class IMUSensorStatus{
            Disabled, Unreliable, LowAccuracy, ModerateAccuracy, HighAccuracy
        };

        struct Vector3{
            float_t x; 
            float_t y; 
            float_t z;

            void print() const;
            void println() const;
        };

        struct Quaternion{
            float_t r;
            float_t i;
            float_t j;
            float_t k;

            void print() const;
            void println() const;
        };

        class BNO085_SPI{
        private:
            //structures

            enum class IMUData{
                Orientation, AngularVelocity, LinearAcceleration, Gravity
            };

            struct SHTPHeader{
                uint16_t length;
                uint8_t channel;
                bool continuation;
            };

            using txCallback_t = RocketOS::inplaceFunction_t<SHTPHeader(void), Airbrakes_CFG_IMUTxCallbackCaptureSize>;

        private:
            //error codes
            static constexpr error_t ERROR_HeaderLength = error_t(5);
            static constexpr error_t ERROR_HeaderChannel = error_t(6);
            static constexpr error_t ERROR_RxBufferOverflow = error_t(7);
            static constexpr error_t ERROR_ContinuationPacket = error_t(8);
        public:
            static constexpr error_t ERROR_ResetTimeout = error_t(2);
            static constexpr error_t ERROR_ConfigurationTimeout = error_t(3);
            static constexpr error_t ERROR_WakeTimeout = error_t(4);

        private:
            //constants
            static constexpr uint_t c_numSHTPChannels = 6;

            //data
            const char* const m_name;
            uint_t m_SPIFrequency;
            IMUStates m_state;
            bool m_resetComplete, m_hubInitialized, m_waking;
            std::array<uint8_t, Airbrakes_CFG_IMUBufferSize> m_rxBuffer;
            std::array<uint8_t, Airbrakes_CFG_IMUBufferSize> m_txBuffer;
            std::array<uint_t, c_numSHTPChannels> m_sequenceNumbers;
            RocketOS::Utilities::Queue<txCallback_t, Airbrekes_CFG_IMUTxQueueSize> m_txQueue;
            TeensyTimerTool::OneShotTimer m_timer;

            Vector3 m_currentLinearAcceleration;
            Vector3 m_currentAngularVelocity;
            Vector3 m_currentGravity;
            Quaternion m_currentOrientation;
            IMUSensorStatus m_linearAccelerationStatus, m_angularVelocityStatus, m_gravityStatus, m_orientationStatus;
            uint32_t m_linearAccelerationSamplePeriod_us, m_angularVelocitySamplePeriod_us, m_gravitySamplePeriod_us, m_orientationSamplePeriod_us;

        public:
            //interface
            BNO085_SPI(const char*, uint_t, uint32_t, TeensyTimerTool::TimerGenerator*);
            error_t initialize();
            IMUStates getState() const;
            RocketOS::Shell::CommandList getCommands();
            void setSamplePeriod_us(uint32_t, IMUData);
            void setSamplePeriod_us(uint32_t);
        private:

            //helpers
            void resetAsync();
            void wakeAsync();
            void serviceInterrupt();
            void serviceTimer();
            void debugPrintRx(SHTPHeader, bool = true);
            void debugPrintTx(SHTPHeader, bool = true);

            //SHTP communication
            result_t<SHTPHeader> doSHTP(SHTPHeader);

            //packet handling
            void respondToPacket(SHTPHeader);   
            bool handleInitializeResponse(SHTPHeader);
            bool handleResetComplete(SHTPHeader);
            bool handleFeatureResponse(IMUData, SHTPHeader);
            bool handleVectorReport(IMUData, SHTPHeader);
            bool handleOrientationReport(SHTPHeader);

            //configuration
            SHTPHeader generateFeatureCommand(IMUData);
            txCallback_t makeFeatureCallback(IMUData);
            SHTPHeader generateFeatureResponseCommand(IMUData);
            txCallback_t makeFeatureResponseCallback(IMUData);

            static uint8_t getReportID(IMUData);
            uint_t& getSamplePeriod(IMUData);
            IMUSensorStatus& getStatus(IMUData);
            static uint_t getQPoint(IMUData);
            Vector3& getVector(IMUData);

            uint_t getMaxSamplePeriod() const;

        private:
            // ######### command structure #########
            using Command = RocketOS::Shell::Command;
            using CommandList = RocketOS::Shell::CommandList;
            using arg_t = RocketOS::Shell::arg_t;

            // === ROOT COMMAND LIST ===
                //child command lists
                // === PERIOD COMMAND ===
                    //child command lists
                    // === SET COMMAND ===
                        //commands
                        const std::array<Command, 5> c_periodSetCommands{
                            Command{"", "u", [this](arg_t args){
                                uint_t newPeriod = args[0].getUnsignedData();
                                setSamplePeriod_us(newPeriod);
                            }},
                            Command{"acceleration", "u", [this](arg_t args){
                                uint_t newPeriod = args[0].getUnsignedData();
                                setSamplePeriod_us(newPeriod, IMUData::LinearAcceleration);
                            }},
                            Command{"angularVelocity", "u", [this](arg_t args){
                                uint_t newPeriod = args[0].getUnsignedData();
                                setSamplePeriod_us(newPeriod, IMUData::AngularVelocity);
                            }},
                            Command{"orientation", "u", [this](arg_t args){
                                uint_t newPeriod = args[0].getUnsignedData();
                                setSamplePeriod_us(newPeriod, IMUData::Orientation);
                            }},
                            Command{"gravity", "u", [this](arg_t args){
                                uint_t newPeriod = args[0].getUnsignedData();
                                setSamplePeriod_us(newPeriod, IMUData::Gravity);
                            }},
                        };
                    // ===================
                    //sub command list
                    const std::array<CommandList, 1> c_periodCommandList = {
                        CommandList{"set", c_periodSetCommands.data(), c_periodSetCommands.size(), nullptr, 0}
                    };
                    //command list
                    const std::array<Command, 1> c_periodCommands = {
                        Command{"", "", [this](arg_t){
                            Serial.printf("Linear Acceleration - %dus\n", m_linearAccelerationSamplePeriod_us);
                            Serial.printf("Angular Velocity - %dus\n", m_angularVelocitySamplePeriod_us);
                            Serial.printf("Orientation - %dus\n", m_orientationSamplePeriod_us);
                            Serial.printf("Gravity - %dus\n", m_gravitySamplePeriod_us);
                        }}
                    };
                // ======================

                // === speed sub command ===
                    //commands
                    const std::array<Command, 2> c_speedCommands{
                        Command{"", "", [this](arg_t){
                            Serial.printf("%dHz\n", m_SPIFrequency);
                        }},
                        Command{"set", "u", [this](arg_t args){
                            uint_t newFrequency = args[0].getUnsignedData();
                            m_SPIFrequency = newFrequency;
                        }}
                    };
                // =========================
                //sub command list
                const std::array<CommandList, 2> c_rootCommandList{
                    CommandList{"period", c_periodCommands.data(), c_periodCommands.size(), c_periodCommandList.data(), c_periodCommandList.size()},
                    CommandList{"speed", c_speedCommands.data(), c_speedCommands.size(), nullptr, 0}
                };
                //commands
                const std::array<Command, 6> c_rootCommands{
                    Command{"status", "", [this](arg_t){
                        IMUStates state = getState();
                        if(state == IMUStates::Uninitialized) Serial.println("Uninitialized");
                        if(state == IMUStates::Reseting) Serial.println("Reseting");
                        if(state == IMUStates::Configuring) Serial.println("Configuring");
                        if(state == IMUStates::Operational) Serial.println("Operational");
                        auto printStatus = [](IMUSensorStatus status){
                            if(status == IMUSensorStatus::Disabled) Serial.print("Disabled");
                            if(status == IMUSensorStatus::Unreliable) Serial.print("Unreliable");
                            if(status == IMUSensorStatus::LowAccuracy) Serial.print("Low Accuracy");
                            if(status == IMUSensorStatus::ModerateAccuracy) Serial.print("Moderate Accuracy");
                            if(status == IMUSensorStatus::HighAccuracy) Serial.print("High Accuracy");
                        };
                        Serial.print("Linear Acceleration - ");
                        printStatus(m_linearAccelerationStatus);
                        Serial.printf(", %.2fHz\n", 1000000.0 / m_linearAccelerationSamplePeriod_us);
                        Serial.print("Angular Velocity - ");
                        printStatus(m_angularVelocityStatus);
                        Serial.printf(", %.2fHz\n", 1000000.0 / m_angularVelocitySamplePeriod_us);
                        Serial.print("Orientation - ");
                        printStatus(m_orientationStatus);
                        Serial.printf(", %.2fHz\n", 1000000.0 / m_orientationSamplePeriod_us);
                        Serial.print("Gravity - ");
                        printStatus(m_gravityStatus);
                        Serial.printf(", %.2fHz\n", 1000000.0 / m_gravitySamplePeriod_us);
                    }},
                    Command{"acceleration", "", [this](arg_t){
                        m_currentLinearAcceleration.print();
                        Serial.println("m/s^2");
                    }},
                    Command{"angularVelocity", "", [this](arg_t){
                        m_currentAngularVelocity.print();
                        Serial.println("rad/s");
                    }},
                    Command{"orientation", "", [this](arg_t){
                        m_currentOrientation.print();
                    }},
                    Command{"gravity", "", [this](arg_t){
                        m_currentGravity.print();
                        Serial.println("m/s^2");
                    }},
                    Command{"reset", "", [this](arg_t){
                        error_t error = initialize(); //hang
                        if(error == ERROR_ResetTimeout) Serial.println("IMU failed to respond");
                        else if(error == ERROR_ConfigurationTimeout) Serial.println("Reset the IMU. Failed to reconfigure the IMU");
                        else Serial.println("Reset and reconfigured the IMU");
                    }}
                };
        };
    }
}