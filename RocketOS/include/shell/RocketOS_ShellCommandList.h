#pragma once
#include "RocketOS_ShellToken.h"
#include <inplace_function.h>

/*Command
*/

namespace RocketOS{
    namespace Shell{
        using commandCallback_t = teensy::inplace_function<void(const Token*), RocketOS_Shell_CommandCallbackCaptureSize>;
        struct Command{
            const char* name;
            const char* args;
            const commandCallback_t callback;
        };

        class CommandList{
        private:
            const char* m_name;
            const Command* m_commands;
            uint_t m_numCommands;
            const CommandList* m_children;
            uint_t m_numChildren;
        public:
            constexpr CommandList(const char* name, const Command* commands, uint_t numCommands, const CommandList* children, uint_t numChildren) : m_name(name), m_commands(commands), m_numCommands(numCommands), m_children(children), m_numChildren(numChildren){}
            const CommandList* getCommandListWithName(const char*) const;
            const Command* getCommandWithName(const char*) const;
            const char* getName() const;
            void printLocalCommands() const;
            void printAllCommands() const;
        };
    }
}