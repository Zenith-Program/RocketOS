#pragma once
#include "RocketOS_PersistentEEPROM.h"
#include "Shell/RocketOS_ShellGeneral.h"

namespace RocketOS{
    namespace Persistent{
        template<class... T>
        class EEPROMCommands{
        private:
            EEPROMBackup<T...>& m_obj;
            const char* const m_name;
            // === PERSISTENT COMMAND LIST ===
            // --- children command lists ---------
                //=== RESTORE SUBCOMMAND ===
                //list of local commands
                const std::array<Shell::Command, 2> c_persistentResotreCommands{
                    Shell::Command{"", "", [this](Shell::arg_t){
                        this->m_obj.restore();
                    }},
                    Shell::Command{"defaults", "", [this](Shell::arg_t){
                        this->m_obj.restoreDefaults();
                    }},
                };
                //==========================
            // ------------------------------------
            //list of subcommands
            const std::array<Shell::CommandList, 1> c_persistentChildren{
                Shell::CommandList{"restore", c_persistentResotreCommands.data(), c_persistentResotreCommands.size(), nullptr, 0}
            };
            //list of local commands
            const std::array<Shell::Command, 1> c_persistentCommands{
                Shell::Command{"save", "", [this](Shell::arg_t){
                    this->m_obj.save();
                }}
            };
            //================================
        public:
            EEPROMCommands(EEPROMBackup<T...>& obj, const char* name) : m_obj(obj), m_name(name){}

            Shell::CommandList getCommands() const{
                return {m_name, c_persistentCommands.data(), c_persistentCommands.size(), c_persistentChildren.data(), c_persistentChildren.size()};
            } 
        };
    }
}
