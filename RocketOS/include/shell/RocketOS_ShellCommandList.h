#pragma once
#include "RocketOS_ShellToken.h"
#include <inplace_function.h>

/*Command Tree (Command & CommandList)
 * Commands are structured as a series of tokens seperated by spaces. For more info on how tokens work look at the README or the file RocketOS_ShellToken.h
 * The first few tokens of a command contain command and subcommand names, then the arguments for the command.
 * EX. >commandName subcommand1Name subCommand2name 5 "hello world" 3.75
 * A tree structure can be used to describe the subcommands available to each command
 * EX.
 *                     |-------------Root----------------|
 *                     |               |                 |
 *             |----Command1---|     Command2    |----Command3---|--------------|
 *             |               |                 |               |              |
 *       SubCommand1  |--Subcommand2--|       SubCommand1  |--Subcommand2--|  SubCommand3
 *                    |               |                    |               |
 *               SubCommand1     SubCommand2          SubCommand1     SubCommand2
 * 
 * 
 * In the above tree, The command ">Command1 SubCommand2 SubCommand1" is available while ">Command2 SubCommand1" is not.
 * 
 * Within the program, the command tree is formed by a collection of command lists and commands. 
 * The root command list for the tree is given to the interpreter to decode user commands.
 * Each branch of the command tree is defined by a command list that has a command name. 
 * Each of these command lists has a list of subCommands that can be imidiately executed. These represent the 'leaves' of the command tree.
 * Each command list also has a list of subCommandLists that describe commands which may themselfs have subcommands.
 * Any sequence of tokens which represent a command must eventually terminate at a leaf of the command tree.
 * 
 * The following lists the structure of each commandList in the above command tree structure:A0
 * Root: Commands - Command2, | ChildrenCommandLists - Command1, Command3
 *      Command1: Commands - SubCommand1 | ChildrenCommandLists - SubCommand2
 *          SubCommand2: Commands - SubCommand1, SubCommand2 | ChildrenCommandLists - NONE
 *      Command3: Commands - SubCommand1, SubCommand3 | ChildrenCommandLists - SubCommand2
 *          SubCommand2: Commands - SubCommand1, SubCommand2 | ChildrenCommandLists - NONE
*/

namespace RocketOS{
    namespace Shell{
        /*Command Callback
         * Command callbacks store the a function to implement a command. 
         * They can be function pointers, non-capturing lambdas, or capturing lambdas with a sufficiently small size (max size can be changed in the shell config file).
         * const Shell::Token* must be the type of the sole parameter for a command callback function. The type alias Shell::arg_t can also be used. 
         * An array of Tokens is passed to the command callback function through this single parameter. This array can be indexed by the command callback to acess whatever parameters the command expects.
        */
        using commandCallback_t = teensy::inplace_function<void(const Shell::Token*), RocketOS_Shell_CommandCallbackCaptureSize>;

        /*Command
         * Commands are defined by a name, argument list, and callback function. 
         * The name "" is used to define a default inplementation for the parent command list. Names of commands should be unique within the same command list and cannot contain spaces or reserved characters.
         * The command interpreter will only call a command's callback function if the number and types of the remaining tokens match its argument list.
         * The argument list is a string where the nth letter represents the type of the nth argument. 
         * The number of arguments is deduced by the length of the string. An empty string represents no arguments.
         * List of argument type characters:
         * word - 'w'
         * string - 's'
         * unsigned number - 'u'
         * signed number - 'i'
         * floating point number - 'f'
        */
        struct Command{
            const char* name;
            const char* args;
            const commandCallback_t callback;
        };

        /*CommandList
         * CommandLists store a list of commands and children command lists.
         * Both lists are stored as pointers to arrays with size variables. 
         * This means that the arrays themselfs need to be created externally to the command list and then passed to it's constructor. See the README for an example
        */
        class CommandList{
        private:
            /*class members
             * m_name - string represnting the name of the subcommand the list represents. The name of the root command list is ignored by the interpreter.
             * m_commands - pointer to the array of commands.
             * m_numCommands - size of the array of commands.
             * m_children - pointer to the array of children command lists.
             * m_numChildren - size of the array of command lists.
            */
            const char* m_name;
            const Command* m_commands;
            uint_t m_numCommands;
            const CommandList* m_children;
            uint_t m_numChildren;
        public:
            /*Constructor
             * The name and both command and child command list arrays must be passed into the constructor.
             * The arrays need to be declared externally to the commandList object itself.
             * 
            */
            constexpr CommandList(const char* name, const Command* commands, uint_t numCommands, const CommandList* children, uint_t numChildren) : m_name(name), m_commands(commands), m_numCommands(numCommands), m_children(children), m_numChildren(numChildren){}
            
            /*Interpreter interface functions
             * These functions are used by the command interpreter to locate commands and subcommands within the command list.
             * They are also used by the interpreter to print the command streucture as a reference for the user.
             *
            */
            const CommandList* getCommandListWithName(const char*) const;
            const Command* getCommandWithName(const char*) const;
            const char* getName() const;
            void printLocalCommands() const;
            void printAllCommands(uint_t=0) const;

            /*Helper functions
             * printIndent - prints n tabs. Used for recursively printing the command tree
             *
             * 
            */
            static void printIndent(uint_t);
        };
    }
}