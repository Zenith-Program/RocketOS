#pragma once
#include "RocketOS.h"

namespace Airbrakes{
    template<class... T>
    class EEPROMWithCommands : public RocketOS::Persistent::EEPROMBackup<T...>{
    private:
        const char* const m_name;

        using Command = RocketOS::Shell::Command;
        using CommandList = RocketOS::Shell::CommandList;
        using arg_t = RocketOS::Shell::arg_t;
        
        // === ROOT COMMAND LIST ===
        // --- children command lists ---------
            //=== RESTORE SUBCOMMAND ===
            //list of local commands
            const std::array<Command, 2> c_persistentResotreCommands{
                Command{"", "", [this](arg_t){
                    this->restore();
                }},
                Command{"defaults", "", [this](arg_t){
                    this->restoreDefaults();
                }},
            };
            //======================
        // ------------------------------------
        //list of subcommands
        const std::array<CommandList, 1> c_persistentChildren{
            CommandList{"restore", c_persistentResotreCommands.data(), c_persistentResotreCommands.size(), nullptr, 0}
        };
        //list of local commands
        const std::array<Command, 1> c_persistentCommands{
            Command{"save", "", [this](arg_t){
                this->save();
            }}
        };
        //================================
    public:
        EEPROMWithCommands(const char* name, RocketOS::Persistent::EEPROMSettings<T>... settings) : RocketOS::Persistent::EEPROMBackup<T...>(settings...), m_name(name){}

        RocketOS::Shell::CommandList getCommands() const{
            return {m_name, c_persistentCommands.data(), c_persistentCommands.size(), c_persistentChildren.data(), c_persistentChildren.size()};
        } 
    };
}
