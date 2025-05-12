#pragma once
#include "RocketOS_ShellGeneral.h"

/*SerialInput class
 *This class is used to handle serial inputs for the shell. It buffers inputs from Serial so they can be interpreted in their entirety by the command interpreter 
 *
 * 
*/

namespace RocketOS{
    class SerialInput{
        static constexpr int_t Size = RocketOS_Shell_SerialRxBufferSize;
        char m_rxBuffer[Size];
        bool m_hasData;
        uint_t m_baud;
    public:
        SerialInput(uint_t);

        int_t size() const;
        char& operator[](int_t);
        char& at(int_t);
        char operator[](int_t) const;
        char at(int_t) const;
		void copy(char*, int_t, int_t) const;

		error_t init();
		error_t update();
		bool hasData() const;
		void clear();
    };
}