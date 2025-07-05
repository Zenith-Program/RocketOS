#include "shell\RocketOS_ShellCommandList.h"
#include <cstring>
#include <Arduino.h> //for Serial

using namespace RocketOS;
using namespace Shell;


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
    for(uint_t i=0; i<m_numCommands; i++){
        //print each command except for the default command
        if(std::strcmp("", m_commands[i].name) != 0){
            Serial.print(m_commands[i].name);
            Serial.print(" {");
            Serial.print(m_commands[i].args);
            Serial.println("}");
        }
    }
    for(uint_t i=0; i<m_numChildren; i++){
        //print each subcommand name
        Serial.print(m_children[i].getName());
        //print the arg list as would be for a command if a default command exists
        const Command* defaultCommand = m_children[i].getCommandWithName("");
        if(defaultCommand != nullptr){
            Serial.print(" {");
            Serial.print(defaultCommand->args);
            Serial.print("}");
        }
        Serial.println(" [...]");
    }
}

void CommandList::printAllCommands(uint_t depth) const{
    //print each command except for the default command
    for(uint_t i=0; i<m_numCommands; i++){
        if(std::strcmp("", m_commands[i].name) != 0){
            printIndent(depth);
            Serial.print(m_commands[i].name);
            Serial.print(" {");
            Serial.print(m_commands[i].args);
            Serial.println("}");
        }
    }
    //print each commandList
    for(uint_t i=0; i<m_numChildren; i++){
        printIndent(depth);
        Serial.print(m_children[i].getName());
        //print the arg list as would be for a command if a default command exists
        const Command* defaultCommand = m_children[i].getCommandWithName("");
        if(defaultCommand != nullptr){
            Serial.print(" {");
            Serial.print(defaultCommand->args);
            Serial.print("}");
        }
        Serial.println(" [");
        //recursively print all commands of children
        m_children[i].printAllCommands(depth+1);
        printIndent(depth);
        Serial.println("]");
    }
}

void CommandList::printIndent(uint_t num){
    for(uint_t i=0; i<num; i++)
        Serial.print("    ");
}