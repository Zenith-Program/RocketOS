#include "shell\RocketOS_ShellToken.h"

using namespace RocketOS;
using namespace RocketOS::Shell;

//implementation of static TokenBuffer interface---------------
SerialInput* Token::s_buffer = nullptr;
void Token::setTokenBuffer(SerialInput* buffer) {
	Token::s_buffer = buffer;
}

//implementation of command interface--------------------------
error_t Token::copyStringData(char* commandBuffer, uint_t size) const {
	uint_t start = m_data.strData.start, end = m_data.strData.end;
	if (end - start >= size) {
		if(size > 0) commandBuffer[0] = '\0';
		return error_t::ERROR;
	}
	s_buffer->copy(commandBuffer, start, end);
	commandBuffer[end - start] = '\0';
	return error_t::GOOD;
}

float_t Token::getFloatData() const {
	return m_data.floatData;
}

uint_t Token::getUnsignedData() const {
	return m_data.unsignedData;
}

int_t Token::getSignedData() const {
	return m_data.signedData;
}

//implementation of console interface--------------------------
bool Token::hasInterpretation(TokenTypes type) const{
	return static_cast<uint_t>(type) & m_interpretations && type != TokenTypes::Invalid;
}

error_t Token::setInterpretation(TokenTypes type) {
	if (static_cast<uint_t>(type) & m_interpretations && type != TokenTypes::Invalid) {
		switch (type) {
		case TokenTypes::unsignedData:
			interpretUnsignedData();
			break;
		case TokenTypes::signedData:
			interpretSignedData();
			break;
		case TokenTypes::FloatData:
			interpretFloatData();
			break;
		case TokenTypes::StringData:
			interpretStringData();
			break;
		case TokenTypes::WordData:
			interpretWordData();
			break;
        default:
            break;
		}
		return error_t::GOOD;
	}
	else
		return error_t::ERROR;
}

bool Token::extract(int_t& pos) {
	//clear white space
	for (; isWhiteSpace(s_buffer->at(pos)); pos--);
	//save end of token
	int_t end = pos + 1;
	//check for newline (\n) or command start (>)
	if (isStopCharacter(s_buffer->at(pos))) return true;
	int_t start;
	m_interpretations = static_cast<uint_t>(TokenTypes::Empty);
	if (s_buffer->at(pos) == '"') { //string token
		end = pos;
		pos--;
		for (; s_buffer->at(pos) != '"'; pos--);
		if (end == (pos % s_buffer->size() + s_buffer->size()) % s_buffer->size()) return true;
		start = pos + 1;
		pos--;
		setInterpretationRange(start, end);
		checkStringData();
	}
	else { //non-string token
		//find begining of token
		for (; !isEndCharacter(s_buffer->at(pos)); pos--);
		start = pos + 1;
		setInterpretationRange(start, end);
		checkUnsignedData();
		checkSignedData();
		checkFloatData();
		checkWordData();
	}
	return false;
}

//private implementation functions-----------------------------

void Token::setInterpretationRange(uint_t start, uint_t end) {
	m_data.strData.start = start;
	m_data.strData.end = end;
}

//interpretations for each type----------------

/* unsigned formats
* Decimal: 12345
* Hex: 0x43F2Ea02
* Binary 0b01001011
*/
void Token::interpretUnsignedData() {
	m_interpretations = static_cast<uint_t>(TokenTypes::Empty);
	int_t start = m_data.strData.start;
	int_t end = m_data.strData.end;
	uint_t value = 0;
	//Decimal Case
	int_t i;
	for (i = start; i < end && isNumeric(s_buffer->at(i)); i++)
		value += powerOfTen_u(end - start - 1 - (i - start)) * numberValue_u(s_buffer->at(i));
	if (i == end) {
		m_data.unsignedData = value;
		m_interpretations |= static_cast<uint_t>(TokenTypes::unsignedData);
		return;
	}
	//Hex case
	value = 0;
	if (end - start > 1 && s_buffer->at(start) == '0' && s_buffer->at(start + 1) == 'x') {
		for (i = start + 2; i < end && isHex(s_buffer->at(i)); i++)
			value += numberValue_u(s_buffer->at(i)) << ((end - (start + 2) - 1 - (i - (start + 2))) << 2);
		if (i == end) {
			m_data.unsignedData = value;
			m_interpretations |= static_cast<uint_t>(TokenTypes::unsignedData);
			return;
		}
	}
	//Binary
	value = 0;
	if (end - start > 1 && s_buffer->at(start) == '0' && s_buffer->at(start + 1) == 'b') {
		for (i = start + 2; i < end && isBinary(s_buffer->at(i)); i++)
			value += numberValue_u(s_buffer->at(i)) << (end - (start + 2) - 1 - (i - (start + 2)));
		if (i == end) {
			m_data.unsignedData = value;
			m_interpretations |= static_cast<uint_t>(TokenTypes::unsignedData);
			return;
		}
	}
}
void Token::interpretSignedData() {
	m_interpretations = static_cast<uint_t>(TokenTypes::Empty);
	int_t start = m_data.strData.start;
	int_t end = m_data.strData.end;
	int_t i;
	int_t value = 0;
	//negative case
	if (s_buffer->at(start) == '-') {
		for (i = start + 1; i < end && isNumeric(s_buffer->at(i)); i++) {
			value += powerOfTen_s(end - start - 2 - (i - start - 1)) * numberValue_s(s_buffer->at(i));
		}
		if (i == end) {
			m_data.signedData = -value;
			m_interpretations |= static_cast<int_t>(TokenTypes::signedData);
			return;
		}
	}
	else {
		//positive case
		value = 0;
		for (i = start; i < end && isNumeric(s_buffer->at(i)); i++)
			value += powerOfTen_s(end - start - 1 - (i - start)) * numberValue_s(s_buffer->at(i));
		if (i == end) {
			m_data.signedData = value;
			m_interpretations |= static_cast<int_t>(TokenTypes::signedData);
			return;
		}
	}
}
void Token::interpretWordData() {
	m_interpretations = static_cast<uint_t>(TokenTypes::WordData);
}
void Token::interpretStringData() {
	m_interpretations = static_cast<uint_t>(TokenTypes::StringData);
}
void Token::interpretFloatData() {
	m_interpretations = static_cast<uint_t>(TokenTypes::Empty);
	int_t start = m_data.strData.start;
	int_t end = m_data.strData.end;
	float_t value = 0;
	bool neg = false;
	if (s_buffer->at(start) == '-') {
		neg = true;
		start++;
	}
	int_t i;
	for (i = start; i < end && isNumeric(s_buffer->at(i)); i++);
	if (i == end) {//no decimal point case
		for (i = start; i < end; i++)
			value += powerOfTen_f(end - start - 1 - (i - start)) * numberValue_s(s_buffer->at(i));
		if (neg) value = -value;
		m_data.floatData = value;
		m_interpretations |= static_cast<uint_t>(TokenTypes::FloatData);
		return;
	}
	else if (s_buffer->at(i) == '.') {//decimal point case
		uint_t decimalPoint = i;
		//before decimal point
		for (uint_t j = start; j < decimalPoint; j++)
			value += powerOfTen_f(decimalPoint - start - 1 - (j - start)) * numberValue_s(s_buffer->at(j));
		//after decimal point
		for (i++; i < end && isNumeric(s_buffer->at(i)); i++)
			value += powerOfTen_f(decimalPoint - i) * numberValue_s(s_buffer->at(i));
		if (i == end) {
			if (neg) value = -value;
			m_data.floatData = value;
			m_interpretations |= static_cast<uint_t>(TokenTypes::FloatData);
			return;
		}
	}
}

//checks for each type-------------------------
void Token::checkUnsignedData() {
	int_t start = m_data.strData.start;
	int_t end = m_data.strData.end;
	//decimal case
	int_t i;
	for (i = start; i < end && isNumeric(s_buffer->at(i)); i++);
	if (end == i) {
		m_interpretations |= static_cast<uint_t>(TokenTypes::unsignedData);
		return;
	}
	//hex case
	if (end - start > 1 && s_buffer->at(start) == '0' && s_buffer->at(start + 1) == 'x') {
		for (i = start + 2; i < end && isHex(s_buffer->at(i)); i++);
		if (i == end) {
			m_interpretations |= static_cast<uint_t>(TokenTypes::unsignedData);
			return;
		}
	}
	//binary case
	if (end - start > 1 && s_buffer->at(start) == '0' && s_buffer->at(start + 1) == 'b') {
		for (i = start + 2; i < end && isBinary(s_buffer->at(i)); i++);
		if (i == end) {
			m_interpretations |= static_cast<uint_t>(TokenTypes::unsignedData);
			return;
		}
	}
}
void Token::checkSignedData() {
	int_t start = m_data.strData.start;
	int_t end = m_data.strData.end;
	int_t i;
	//negative case
	if (s_buffer->at(start) == '-') {
		for (i = start + 1; i < end && isNumeric(s_buffer->at(i)); i++);
		if (i == end) {
			m_interpretations |= static_cast<int_t>(TokenTypes::signedData);
			return;
		}
	}
	else {
		//positive case
		for (i = start; i < end && isNumeric(s_buffer->at(i)); i++);
		if (i == end) {
			m_interpretations |= static_cast<int_t>(TokenTypes::signedData);
			return;
		}
	}
}
void Token::checkWordData() {
	m_interpretations |= static_cast<uint_t>(TokenTypes::WordData);
}
void Token::checkStringData() {
	m_interpretations |= static_cast<uint_t>(TokenTypes::StringData);
}
void Token::checkFloatData() {
	int_t start = m_data.strData.start;
	int_t end = m_data.strData.end;
	if (s_buffer->at(start) == '-') {
		start++;
	}
	int_t i;
	for (i = start; i < end && isNumeric(s_buffer->at(i)); i++);
	if (i == end) {
		m_interpretations |= static_cast<uint_t>(TokenTypes::FloatData);
		return;
	}
	else if (s_buffer->at(i) == '.') {
		for (i++; i < end && isNumeric(s_buffer->at(i)); i++);
		if (i == end) {
			m_interpretations |= static_cast<uint_t>(TokenTypes::FloatData);
			return;
		}
	}
}

//statics--------------------------------------
bool Token::isEndCharacter(char c) {
	if (isWhiteSpace(c) || c == '>' || c == '\n' || c == '"') return true;
	return false;
}

bool Token::isStopCharacter(char c) {
	if (c == '\n' || c == '>') return true;
	return false;
}

bool Token::isWhiteSpace(char c) {
	if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f') return true;
	return false;
}

bool Token::isNumeric(char c) {
	if (c >= '0' && c <= '9') return true;
	return false;
}

bool Token::isHex(char c) {
	if (isNumeric(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) return true;
	return false;
}

bool Token::isBinary(char c) {
	if (c == '1' || c == '0') return true;
	return false;
}

uint_t Token::powerOfTen_u(uint_t n) {
	uint_t value = 1;
	for (uint_t i = 0; i < n; i++)
		value *= 10;
	return value;
}

int_t Token::powerOfTen_s(uint_t n) {
	int_t value = 1;
	for (uint_t i = 0; i < n; i++)
		value *= 10;
	return value;
}
float_t Token::powerOfTen_f(int_t n) {
	float_t value = 1;
	if (n < 0) {
		for (int_t i = 0; i < (-1 * n); i++)
			value *= 1.0 / 10;
	}
	else {
		for (int_t i = 0; i < n; i++)
			value *= 10;
	}
	return value;
}

uint_t Token::numberValue_u(char c) {
	uint_t value;
	if (c >= '0' && c <= '9')
		value = c - '0';
	else if (c >= 'a' && c <= 'f')
		value = c - 'a' + 10;
	else
		value = c - 'A' + 10;
	return value;
}

int_t Token::numberValue_s(char c) {
	return c - '0';
}