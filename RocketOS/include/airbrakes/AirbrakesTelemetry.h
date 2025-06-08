#include "RocketOS.h"
#include <Arduino.h> //millis

namespace Airbrakes{
    template<std::size_t t_bufferSize, std::size_t t_nameSize>
    class SDFileWithCommands : public RocketOS::Telemetry::SDFile{
    private:
        const char* const m_name;
        std::array<char, t_bufferSize> m_buffer;
        std::array<char, t_nameSize> m_fileName;

        using Command = RocketOS::Shell::Command;
        using CommandList = RocketOS::Shell::CommandList;
        using arg_t = RocketOS::Shell::arg_t;

        // === ROOT COMMAND LIST ===
            // === NAME SUBCOMMAND ===
            //list of local commands
            const std::array<Command, 2> c_nameCommands{
                Command{"", "", [this](arg_t){
                    Serial.println(m_fileName.data());
                }},
                Command{"set", "s", [this](arg_t args){
                    args[0].copyStringData(m_fileName.data(), m_fileName.size());
                }}
            };
            // ======================
            
            // === MODE SUBCOMMAND ===
            //list of local commands
            const std::array<Command, 3> c_modeCommands{
                Command{"", "", [this](arg_t){
                    if(this->getMode() == RocketOS::Telemetry::SDFileModes::Buffer) Serial.println("Buffer");
                    else Serial.println("Record");
                }},
                Command{"buffer", "", [this](arg_t){
                    this->setMode(RocketOS::Telemetry::SDFileModes::Buffer);
                }},
                Command{"record", "", [this](arg_t){
                    this->setMode(RocketOS::Telemetry::SDFileModes::Record);
                }}
            };
            //========================
        //list of subcommands
        const std::array<CommandList, 2> c_rootChildren{
            CommandList{"name", c_nameCommands.data(), c_nameCommands.size(), nullptr, 0},
            CommandList{"mode", c_modeCommands.data(), c_modeCommands.size(), nullptr, 0}
        };
        //list of local commands
        const std::array<Command, 2> c_rootCommands{
            Command{"", "s", [this](arg_t args){
                char messageBuffer[RocketOS_Telemetry_CommandInternalBufferSize ];
                args[0].copyStringData(messageBuffer, RocketOS_Telemetry_CommandInternalBufferSize);
                this->logLine(messageBuffer);
            }},
            Command{"new", "", [this](arg_t){
                this->newFile();
            }}
        };
        // =========================

    public:
        SDFileWithCommands(const char* name, SdFat& sd, const char* file) : RocketOS::Telemetry::SDFile(sd, m_buffer.data(), m_buffer.size(), m_fileName.data()), m_name(name) {
            std::strncpy(m_fileName.data(), file, m_fileName.size());
        }

        RocketOS::Shell::CommandList getCommands() const{
            return {m_name, c_rootCommands.data(), c_rootCommands.size(), c_rootChildren.data(), c_rootChildren.size()};
        }

        void logLine(const char* message){
            this->log("[");
            this->log(millis());
            this->log("] ");
            this->log(message);
            this->log("\n");
            this->flush();
        }

        //refrence acessors for persistent storage
        auto& getNameBufferRef(){
            return m_fileName;
        }
    };


    template<std::size_t t_nameSize, class... T>
    class DataLogWithCommands : public RocketOS::Telemetry::DataLog<T...>{
    private:
        const char* const m_name;
        std::array<char, t_nameSize> m_fileName;
        uint_t m_refreshPeriod;
        elapsedMillis m_refresh;

        using Command = RocketOS::Shell::Command;
        using CommandList = RocketOS::Shell::CommandList;
        using arg_t = RocketOS::Shell::arg_t;

        // === ROOT COMMAND LIST ===
            // === NAME SUBCOMMAND ===
            //list of local commands
            const std::array<Command, 2> c_nameCommands{
                Command{"", "", [this](arg_t){
                    Serial.println(m_fileName.data());
                }},
                Command{"set", "s", [this](arg_t args){
                    args[0].copyStringData(m_fileName.data(), m_fileName.size());
                }}
            };
            // ======================
            
            // === MODE SUBCOMMAND ===
            //list of local commands
            const std::array<Command, 3> c_modeCommands{
                Command{"", "", [this](arg_t){
                    if(this->getFileMode() == RocketOS::Telemetry::SDFileModes::Buffer) Serial.println("Buffer");
                    else Serial.println("Record");
                }},
                Command{"buffer", "", [this](arg_t){
                    this->setFileMode(RocketOS::Telemetry::SDFileModes::Buffer);
                }},
                Command{"record", "", [this](arg_t){
                    this->setFileMode(RocketOS::Telemetry::SDFileModes::Record);
                }}
            };
            // =======================

            // === REFRESH SUBCOMMAND ===
            //list of commands
            const std::array<Command, 2> c_refreshCommands{
                Command{"", "", [this](arg_t){
                    Serial.print(m_refreshPeriod);
                    Serial.println(" ms");
                }},
                Command{"set", "u", [this](arg_t args){
                    this->m_refreshPeriod = args[0].getUnsignedData();
                }}
            };
            // ==========================
        //list of subcommands
        const std::array<CommandList, 3> c_rootChildren{
            CommandList{"name", c_nameCommands.data(), c_nameCommands.size(), nullptr, 0},
            CommandList{"mode", c_modeCommands.data(), c_modeCommands.size(), nullptr, 0},
            CommandList{"refresh", c_refreshCommands.data(), c_refreshCommands.size(), nullptr, 0}
        };
        //list of commands
        const std::array<Command, 1> c_rootCommands{
            Command{"new", "", [this](arg_t){
                this->newFile();
            }}
        };
        // =========================

    public:
        DataLogWithCommands(const char* name, SdFat& sd, char* fileBuffer, uint_t fileBufferSize, const char* file, uint_t refreshPeriod, RocketOS::Telemetry::DataLogSettings<T>... settings) : RocketOS::Telemetry::DataLog<T...>(sd, fileBuffer, fileBufferSize, m_fileName.data(), settings...), m_name(name), m_refreshPeriod(refreshPeriod), m_refresh(0){
            std::strncpy(m_fileName.data(), file, m_fileName.size());
        }

        RocketOS::Shell::CommandList getCommands() const{
            return {m_name, c_rootCommands.data(), c_rootCommands.size(), c_rootChildren.data(), c_rootChildren.size()};
        }

        bool isRefreshed() const{
            return m_refresh > m_refreshPeriod;
        }

        void clearRefresh(){
            m_refresh = 0;
        }

        //refrence acessors for persistent storage
        auto& getNameBufferRef(){
            return m_fileName;
        }

        auto& getRefreshPeriodRef(){
            return m_refreshPeriod;
        }
    };
}