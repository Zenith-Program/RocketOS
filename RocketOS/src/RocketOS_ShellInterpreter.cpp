#include "shell\RocketOS_ShellInterpreter.h"
#include <cstring>
#include <Arduino.h> //for Serial

using namespace RocketOS;
using namespace RocketOS::Shell;

Interpreter::Interpreter(const CommandList* root) : m_rootCommandList(root), m_inputBuffer(RocketOS_Shell_BaudRate) {}

void Interpreter::setRootCommandList(const CommandList* root){
	m_rootCommandList = root;
}

error_t Interpreter::handleInput(){
    m_inputBuffer.clear();
    if(m_inputBuffer.update() != error_t::GOOD) return error_t::ERROR;
    if(m_inputBuffer.hasData()){
        error_t code = readLine();
        m_inputBuffer.clear();
        return code;
    }
    return error_t::GOOD;
}

error_t Interpreter::readLine() {
	Token::setTokenBuffer(&m_inputBuffer);
    //check if there was a command at all
    if(m_inputBuffer.at(0) != '>'){
        return error_t::GOOD;
        //ignored
    }
	//find newline at the end of the transmission
	int_t currentCharacter;
	for (currentCharacter = 0; currentCharacter < m_inputBuffer.size() && m_inputBuffer.at(currentCharacter) != '\0'; currentCharacter++);
	if (currentCharacter >= m_inputBuffer.size()) {
		return error_t::ERROR;
		//log message (no newline)
	}
	currentCharacter--;
	//extract data into tokens
	uint_t currentToken;
	for (currentToken = 0; currentToken < m_tokens.size() && !m_tokens[currentToken].extract(currentCharacter); currentToken++);
	if (currentToken == m_tokens.size()) { 
		Serial.println("<[Cmd:error] Command has too many arguments for interpretation\n");
		return error_t::GOOD;
	}
	//check for wraparound
	if (m_inputBuffer.at(currentCharacter) != '>') return error_t::GOOD; //possible message
	//reorder tokens start to finish
	int_t numTokens = currentToken;
	reorderTokens(numTokens);
    //traverse command structure recursively
	std::strcpy(m_commandBuffer, "[root]");
    return interpretCommandList(m_rootCommandList, m_tokens.data(), numTokens);
     
}
//-----------------------------------------------------------------------------


//helper functions-------------------------------------------------------------

error_t Interpreter::interpretCommandList(const CommandList* list, Token* tokens, uint_t numTokens){
	//check that commandList is valid
    if(list == nullptr){
		Serial.printf("<[Cmd:error] Command list not found for '%s'\n", m_commandBuffer);
    	return error_t::GOOD;
	}
	const Command* command = nullptr;
    //check that there is at least one more token
    if (numTokens < 1) { 
		//look for default command if no more tokens
		if(numTokens == 0) command = list->getCommandWithName("");
		//backtrack one token to account for empty command name token if default command is found
		if(command != nullptr) {
			tokens--;
			numTokens++;
		}
		else {
			//error if no default command
			if(tokens == m_tokens.data()) Serial.println("<[Cmd:error] You must provide a command name");
			else Serial.printf("<[Cmd:error] Invalid number of arguments for '%s'. Expected at least one more argument\n", m_commandBuffer);
			return error_t::GOOD;
		}
	}
	//check that first token is a word if no command default command match before
	if (command == nullptr && m_tokens[0].setInterpretation(TokenTypes::WordData) == error_t::ERROR) {
		//check for default command if next token is not a word
		command = list->getCommandWithName("");
		//backtrack one token to account for empty command name token if default command is found
		if(command != nullptr){
			numTokens++;
			tokens--;
		} 
		else{
			//error if no default command
			Serial.println("<[Cmd:error] Command or Sub-Command name must be interpretable as a word\n");
			return error_t::GOOD;
		}
	}
	//copy command name from first token and look for a command if no default command match found before
	if(command == nullptr){
		if (tokens->copyStringData(m_commandBuffer, c_commandBufferSize) == error_t::ERROR){ 
			Serial.println("<[Cmd:error] Command name is too long for interpretation\n");
			return error_t::GOOD;
		}
		command = list->getCommandWithName(m_commandBuffer);
	}
	if(command == nullptr){
		//search for a sub-command if no commands match
		const CommandList* child = list->getCommandListWithName(m_commandBuffer);
		if(child == nullptr){
			//check for default comands if no commands or subcommands match the word argument
			command = list->getCommandWithName("");
			//backtrack one token to account for empty command name and load correct command name
			if(command != nullptr){
				numTokens++;
				tokens--;
				std::strcpy(m_commandBuffer, list->getName());
			} 
			else{
				//error if no command match, commandList match or default command exists
				Serial.printf("<[Cmd:error] Command '%s' is not available in this context\n", m_commandBuffer);
				return error_t::GOOD;
			}
		}
		else{
			//recursively search sub-command list if sub-command is found
			return interpretCommandList(child, tokens+1, numTokens-1);
		}
	}
	//found matching command case
	//check number of args
	if(numTokens-1 != std::strlen(command->args)){
		Serial.printf("<[Cmd:error] Invalid number of arguments for '%s'. Expected %d, actual was %d\n", m_commandBuffer, std::strlen(command->args), numTokens-1);
		return error_t::GOOD;
	}
	//check each arg type
	uint_t i;
	for (i = 0; i < numTokens-1; i++) {
		TokenTypes expected = getTokenTypeFromArgCharacter(command->args[i]);
		if (expected == TokenTypes::Invalid) {
			Serial.printf("<[Cmd:error] Argument types specifier for '%s' contains invalid character '%c'\n", m_commandBuffer, command->args[i]);
			return error_t::GOOD;
		}
		if (tokens[i + 1].setInterpretation(expected) != error_t::GOOD) {
			Serial.printf("<[Cmd:error] Invalid argument type for '%s'. Expected type '%c' for argument %d\n", m_commandBuffer, command->args[i], i+1);
			return error_t::GOOD;
		}
	}
	//check that callback exists
	if (command->callback == nullptr) {
		Serial.printf("<[Cmd:error] Implementation for '%s' not found\n", m_commandBuffer);
		return error_t::GOOD;
	}
	//call command implementation
	command->callback(tokens+1);
	printEOC();
	return error_t::GOOD;
}

error_t Interpreter::init(){
    return m_inputBuffer.init();
}

void Interpreter::reorderTokens(int_t numTokens) {
	Token temp;
	for (int_t i = 0; i < numTokens / 2; i++) {
		temp = m_tokens[i];
		m_tokens[i] = m_tokens[numTokens - 1 - i];
		m_tokens[numTokens - 1 - i] = temp;
	}
}

TokenTypes Interpreter::getTokenTypeFromArgCharacter(char arg) {
	switch (arg) {
	case 'u':
		return TokenTypes::unsignedData;
		break;
	case 'i':
		return TokenTypes::signedData;
		break;
	case 's':
		return TokenTypes::StringData;
		break;
	case 'w':
		return TokenTypes::WordData;
		break;
	case 'f':
		return TokenTypes::FloatData;
		break;
	default:
		return TokenTypes::Invalid;
		break;
	}
}

void Interpreter::printEOC() {
	Serial.println("<\n");
}
