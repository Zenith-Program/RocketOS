#include "RocketOS_TelemetryDataLog.h"
#include "shell/RocketOS_Shell.h"
#include <Arduino.h> //millis

namespace RocketOS{
    namespace Telemetry{
        class FileCommands{
        private:
            SDFile& m_obj;
            const char* const m_name;

            // === FILE COMMAND LIST ===
            // children command lists ---------
                // === NAME SUBCOMMAND ===
                //list of commands
                const std::array<Shell::Command, 2> c_nameCommands{
                    Shell::Command{"", "", [this](Shell::arg_t){
                        Serial.println(m_obj.getFileName());
                    }},
                    Shell::Command{"set", "s", [this](Shell::arg_t args){
                        char messageBuffer[RocketOS_Telemetry_CommandInternalBufferSize];
                        args[0].copyStringData(messageBuffer, RocketOS_Telemetry_CommandInternalBufferSize);
                        m_obj.setFileName(messageBuffer);
                    }}
                };
                // ======================
                
                // === MODE SUBCOMMAND ===
                const std::array<Shell::Command, 3> c_modeCommands{
                    Shell::Command{"", "", [this](Shell::arg_t){
                        if(m_obj.getMode() == SDFileModes::Buffer) Serial.println("Buffer");
                        else Serial.println("Record");
                    }},
                    Shell::Command{"buffer", "", [this](Shell::arg_t){
                        m_obj.setMode(SDFileModes::Buffer);
                    }},
                    Shell::Command{"record", "", [this](Shell::arg_t){
                        m_obj.setMode(SDFileModes::Record);
                    }}
                };
                //========================
            //---------------------------------
            //list of subcommands
            const std::array<Shell::CommandList, 2> c_rootChildren{
                Shell::CommandList{"name", c_nameCommands.data(), c_nameCommands.size(), nullptr, 0},
                Shell::CommandList{"mode", c_modeCommands.data(), c_modeCommands.size(), nullptr, 0}
            };
            //list of commands
            const std::array<Shell::Command, 2> c_rootCommands{
                Shell::Command{"", "s", [this](Shell::arg_t args){
                    char messageBuffer[RocketOS_Telemetry_CommandInternalBufferSize ];
                    args[0].copyStringData(messageBuffer, RocketOS_Telemetry_CommandInternalBufferSize);
                    m_obj.log("[");
                    m_obj.log(millis());
                    m_obj.log("] ");
                    m_obj.log(messageBuffer);
                    m_obj.log("\n");
                    m_obj.flush();
                }},
                Shell::Command{"new", "", [this](Shell::arg_t){
                    m_obj.newFile();
                }}
            };
            // =========================

        public:
            FileCommands(SDFile& obj, const char* name) : m_obj(obj), m_name(name) {}

            Shell::CommandList getCommands() const{
                return {m_name, c_rootCommands.data(), c_rootCommands.size(), c_rootChildren.data(), c_rootChildren.size()};
            }
        };
    }
}