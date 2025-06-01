#pragma once
#include "RocketOS_ShellGeneral.h"
#include "RocketOSSerial.h"

namespace RocketOS{
    namespace Shell{

    /*Token Helper Structures------------------
	* StrPos - Sotres the start and end of the substring within the input buffer that corrisponds to the data assigned to the token by the interprter. 
	* TokenData - Union that stores each of the supported token data types in the same block of memory. 
	* Only one member of the union will be valid after interpretation so it is important that command callback functions extract the same data types as are in their argument list.
	*/
	struct StrPos {
		int_t start;
		int_t end;
	};
	union TokenData {
		uint_t unsignedData;
		int_t signedData;
		float_t floatData;
		StrPos strData;
	};
	

	/*Token Class------------------------------
	 * Token objects are used by the interpreter to parse incomming commands from the user and to pass argument data to command callback functions.
	 * Each substring of characters seperated by whitespace in a command is considered a seperate token.
	 * 
	 * There are 5 types of tokens:
	 * unisgned number - whole number in decimal form (ex. 1, 46, etc), hexadecimal form (ex. 0xFA7b, 0x01, etc), or binary form (ex. 0b0100110, 0b001, etc).
	 * signed number - integer number in decimal form (ex. 1, -5, etc).
	 * floating point number - positive or negative number with an optional decimal point (ex. -5, 2.0, 5, 12.75, -16.39).
	 * string data - string of characters enclosed in quotation marks that can include spaces (ex. "hello world", "2.575", etc).
	 * word data - string of characters not containing whitespace, quotation marks, or the command character '>' (ex. hello, helloWorld, 2.75 0x000, etc)
     * 
	 * The command interpreter contains an array of tokens objects. Durring parsing, each token within the incoming string is assigned to a token within the array for interpretation.
	 * Tokens which were assigned to the arguments of a command are passed as parameters to the command's callback function.
	*/

	enum class TokenTypes : uint_t {
		Empty = 0, unsignedData = 1, signedData = (1 << 1), FloatData = (1 << 2), StringData = (1 << 3), WordData = (1 << 4), Invalid = (1 << 5)
	};

	class Token {
		/*cass members
		 * m_data - stores the data for command arguments.
		 * m_interpretations - bitmask of the possible interpretations on the characters assigned to the token. used to check that arguments are valid before stroing token data
		 * s_buffer - static member that points to the console's incoming character buffer
		*/
		TokenData m_data;
		uint_t m_interpretations;
		static const SerialInput* s_buffer;
	public:
		/*Command Interface----------
		 * Use these functions to get argument data from the token.
		 * String data must be copied from the token to avoid it being overwrittn by the next command.
		 * All other data types just return the value from the user.
		*/
		error_t copyStringData(char*, uint_t) const;
		uint_t getUnsignedData() const;
		int_t getSignedData() const;
		float_t getFloatData() const;

		/*Interpreter Interface------
		 * These functions are used by the interpreter to deduce argument types and extract data.
		 * If you dont mess with the interpreter, you dont have to worry about these.
		 * 
		*/
		bool extract(int_t&);
		bool hasInterpretation(TokenTypes) const;
		error_t setInterpretation(TokenTypes);
		static void setTokenBuffer(const SerialInput*);
		
	private:
		/*Implementation details
		* These functions are used by token to decode incoming characters and save the argument data.
		* If you dont mess with the parsing algorithim, you dont have to worry about these.
		*/
		static bool isEndCharacter(char);
		static bool isStopCharacter(char);
		static bool isWhiteSpace(char);
		static bool isNumeric(char);
		static bool isHex(char);
		static bool isBinary(char);
		static uint_t powerOfTen_u(uint_t);
		static int_t powerOfTen_s(uint_t);
		static float_t powerOfTen_f(int_t);
		static uint_t numberValue_u(char);
		static int_t numberValue_s(char);

		void setInterpretationRange(uint_t, uint_t);

		void interpretUnsignedData();
		void interpretSignedData();
		void interpretWordData();
		void interpretStringData();
		void interpretFloatData();

		void checkUnsignedData();
		void checkSignedData();
		void checkWordData();
		void checkStringData();
		void checkFloatData();
	};

	/*arg_t alias
	 * arg_t is a type alias for command callbacks to use.
	 * An array of tokens is passed to command callbacks as a pointer for them extract each their arguments from.
	 * A single parameter of type arg_t should be used for all command callbacks regardless of how many arguments they have.
	*/
	using arg_t = const Token*;
    }
}