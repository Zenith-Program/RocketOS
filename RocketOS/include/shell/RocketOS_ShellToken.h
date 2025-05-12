#pragma once
#include "RocketOS_ShellGeneral.h"
#include "RocketOS_ShellSerial.h"

namespace RocketOS{
    namespace Shell{
        /*Token Helper Structures------------------
	* Description:
	* These structures are used by the internals of Token and other Intrpreter classes to store incoming data from the console
	* 
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
	* Description:
	* The token class is used when parsing commands sent to the OS. They first check what types they could be, and then based on the request of the current command store the type of data it expects.
	* Commands recive an array of tokens as their argument that will be of the types they specify in their expected types string.
	* 
	* Members:
	* data - stores the data for command arguments.
	* interpretations - bitmask of the possible interpretations on the characters assigned to the token. used to check that arguments are valid before stroing token data
	* buffer - static member that points to the console's incoming character buffer
	*/

	enum class TokenTypes : uint_t {
		Empty = 0, unsignedData = 1, signedData = (1 << 1), FloatData = (1 << 2), StringData = (1 << 3), WordData = (1 << 4), Invalid = (1 << 5)
	};

	class Token {
		TokenData m_data;
		uint_t m_interpretations;
		static SerialInput* s_buffer;
	public:
		/*Command Interface----------
		* Description:
		* Use these functions to get argument data from the token
		*/
		error_t copyStringData(char*, uint_t) const;
		uint_t getUnsignedData() const;
		int_t getSignedData() const;
		float_t getFloatData() const;

		/*Interpreter Interface------
		* Description:
		* These functions are used by Interpreter objects
		*/
		bool extract(int_t&);
		bool hasInterpretation(TokenTypes) const;
		error_t setInterpretation(TokenTypes);
		static void setTokenBuffer(SerialInput*);
		
	private:
		/*Implementation details
		* Description:
		* These functions are used by token to decode incoming characters and save the argument data
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

	using arg_t = const Token*;
    }
}