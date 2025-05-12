#include "shell\RocketOS_ShellCommandList.h"
#include <cstring>

using namespace RocketOS;
using namespace Shell;



//constexpr CommandList::CommandList(const char* name, const Command* commands, uint_t numCommands, const CommandList* children, uint_t numChildren) : m_name(name), m_commands(commands), m_numCommands(numCommands), m_children(children), m_numChildren(numChildren){}

const CommandList* CommandList::getCommandListWithName(const char* name) const{
    for(uint_t i=0; i<m_numChildren; i++)
        if(std::strcmp(name, m_children[i].getName()) == 0) return m_children+i;
    return nullptr;
}

const Command* CommandList::getCommandWithName(const char* name) const{
    for(uint_t i=0; i<m_numCommands; i++)
        if(std::strcmp(name, m_commands[i].name) == 0) return m_commands+i;
    return nullptr;
}

const char* CommandList::getName() const{
    return m_name;
}

void CommandList::printLocalCommands() const{

}

void CommandList::printAllCommands() const{

}