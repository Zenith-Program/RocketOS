#pragma once
#include "RocketOS_Persistent.cfg.h"
#include "RocketOSGeneral.h"

/*Configuration Validity Checks
 * These are compile time checks that ensure all configuration macros are exist and are valid.
 * If a macro is missing or invalid a compilation error will be thrown.
 *
*/

/*
#define RocketOS_Persistent_EEPROMBaseAdress 0
#define RocketOS_Persistent_EEPROMAlignment 4
#define RocketOS_Persistent_EEPROMMaxSize 1024
*/

//RocketOS_Persistent_EEPROMMaxSize check
#ifndef RocketOS_Persistent_EEPROMMaxSize
    static_assert(false, "RocketOS_Persistent_EEPROMMaxSize must be defined in the file RocketOS_Persistent.cfg.h");
#else
    static_assert(RocketOS_Persistent_EEPROMMaxSize > 0 , "RocketOS_Persistent_EEPROMMaxSize must be positive");
#endif

//RocketOS_Persistent_EEPROMBaseAdress check
#ifndef RocketOS_Persistent_EEPROMBaseAdress
    static_assert(false, "RocketOS_Persistent_EEPROMBaseAdress must be defined in the file RocketOS_Persistent.cfg.h");
#else
    static_assert(RocketOS_Persistent_EEPROMBaseAdress >= 0 && RocketOS_Persistent_EEPROMBaseAdress < RocketOS_Persistent_EEPROMMaxSize, "RocketOS_Persistent_EEPROMBaseAdress must be non-negative and within the bounds of EEPROM adressing");
#endif

//RocketOS_Persistent_EEPROMAlignment check
#ifndef RocketOS_Persistent_EEPROMAlignment
    static_assert(false, "RocketOS_Persistent_EEPROMAlignment must be defined in the file RocketOS_Persistent.cfg.h");
#else
    static_assert(RocketOS_Persistent_EEPROMAlignment > 0, "RocketOS_Persistent_EEPROMAlignment must be positive");
#endif