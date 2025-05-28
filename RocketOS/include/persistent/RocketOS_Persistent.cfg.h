#pragma once
/*RocketOS Persistent Configuration File-----------------------
 * This file is used to configure the persistent data system of RocketOS.
 * The persistent data system sotres program variables into non-volatile emulated EEPROM memory allowing for settings and critical flight variables to persist accross resets. 
 * Look at the README file in this directory for more info.
*/

/*EEPROM adressing parameters
 * These macros parameterize the adressing scheme that is used when storing multiple variables into EEPROM
 * Macros:
 * RocketOS_Persistent_EEPROMBaseAdress - Starting adress in EEPROM for non-volatile storage. A single word hash value is stored here to compare program versions, followed by program variables.
 * RocketOS_Persistent_EEPROMAlignment - Determines what variable allignment to use. All variables will be stored at an adress that is a multiple of this value. For Teensy4.1, the optimal allignmnet for the hardware is 4.
 * RocketOS_Persistent_EEPROMMaxSize - Stores the maximum number of bytes that are available for sotrage. This value is used for compile time checks.
*/
#define RocketOS_Persistent_EEPROMBaseAdress 0
#define RocketOS_Persistent_EEPROMAlignment 4
#define RocketOS_Persistent_EEPROMMaxSize 1024