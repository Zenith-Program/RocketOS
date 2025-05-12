#pragma once

#include "RocketOS_ShellToken.h"
#include "RocketOS_ShellSerial.h"
#include "RocketOS_ShellCommandList.h"
#include <array>

namespace RocketOS{
    namespace Shell{
        class Interpreter {
            /*class members
             * m_tokens - 
             * m_commandBuffer - 
             * m_rootCommandList - 
             * m_inputBuffer - 
            */
			std::array<Token, RocketOS_Shell_TokenBufferSize> m_tokens;
            static constexpr uint_t c_commandBufferSize = RocketOS_Shell_InterpreterCommandNameBufferSize;
            char m_commandBuffer[c_commandBufferSize];
            const CommandList* m_rootCommandList;
            SerialInput m_inputBuffer;
		public:
            //constructor
			Interpreter(const CommandList*);
            error_t init();
            
			//parsing interface 
            error_t handleInput();
			
			//helper functions
		private:
            error_t readLine();
            error_t interpretCommandList(const CommandList*, Token*, uint_t);
			void reorderTokens(int_t);
			static TokenTypes getTokenTypeFromArgCharacter(char);
			static void printEOC();
		};
    }
}