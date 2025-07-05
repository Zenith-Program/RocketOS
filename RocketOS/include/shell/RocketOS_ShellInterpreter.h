#pragma once

#include "RocketOS_ShellToken.h"
#include "RocketOSSerial.h"
#include "RocketOS_ShellCommandList.h"
#include <array>

/*RocketOS Interpreter
 * The interpreter reads incoming user inputs and searches for a matching command in the command tree. 
 * It executes command callback functions when it finds a match.
 * For more info about the command tree, look at RocketOS_ShellCommandList.h or the README in this directory
 * For more info about the parsing process, look to RocketOS_ShellToken.h or the README.
 * 
*/

namespace RocketOS{
    namespace Shell{
        class Interpreter {
            /*class members
             * m_tokens - array of tokens where each part of the incoming command are placed. A porion of this array is passed to the command callback function as it's arguments
             * m_commandBuffer - string buffer used to store the current command name at each step of parsing
             * m_rootCommandList - the command list which is the root node of the command tree used to interpret user inputs
             * m_inputBuffer - serial buffer used to get and store user input for parsing
            */
			std::array<Token, RocketOS_Shell_TokenBufferSize> m_tokens;
            static constexpr uint_t c_commandBufferSize = RocketOS_Shell_InterpreterCommandNameBufferSize;
            char m_commandBuffer[c_commandBufferSize];
            const CommandList* m_rootCommandList;
            const CommandList* m_currentCommandList;
            const SerialInput& m_inputBuffer;
		public:
            /*constructor & initialization
             * Constructor requires the root command list is provided. 
             * The root command list can be changed at runtime with setRootCommandList.
             * The init function initializes serial input and output and needs to be called before input can be processed.
            */
			Interpreter(const SerialInput&, const CommandList*);
            void setRootCommandList(const CommandList*);
            
			/*parsing interface 
             * The function handleInput reads any available user input into m_inputBuffer and searches for a command in the command tree defined by the m_rootCommandList.
             * If a command is found it's callback function is executed, otherwise an error message is printed.
             *
            */
            error_t readLine();
			
		private:
            /*helper functions
             * These private helper functions are used by the parsing algorithim. If you don't mess with the parsing algorithim you dont have to worry about these.
             * readline - interprets whatever is currently stored in m_inputBuffer. executes the command if found and prints an error message otherwise.
             * interpretCommandList - recursive function used by readline to interpret each layer of the command tree.
             * reorderTokens - parsing happens in reverse for legacy reasons so this is used to reorder the tokens before passing them to the command callback.
             * getTokenTypeFromArgCharacter - converts the data type characters ('iuswf') to their associated enum
             * printEOC - prints the character signifying the end of command execution
            */
            error_t interpretCommandList(const CommandList*, Token*, uint_t);
			void reorderTokens(int_t);
			static TokenTypes getTokenTypeFromArgCharacter(char);
			static void printEOC();
		};
    }
}