#pragma once
/*################ RocketOS Configuration File ################
 * This is the main configuration file for RocketOS. 
 * General system configurations are included in this file
 * Look at the README in this directory for more info
*/

/*included modules
 *
 *
 * 
*/
#define RocketOS_CFG_HasShell
#define RocketOS_CFG_HasPersistent
#define RocketOS_CFG_HasTelemetry
#define RocketOS_CFG_HasSimulation
#define RocketOS_CFG_HasProcessing

/*Architecure Type
 * 
*/
#define RocketOS_CFG_NativeWordWidth 32

/*Serial Parameters
 * These macros parameterize the serial IO of the shell. Look in the Shell README for more info about how these work.
 * RocketOS_CFG_SerialRxBufferSize - Determines the size of the shell's input buffer and thus the maximum size of interpretable commands.
 * 
*/
#define RocketOS_CFG_SerialRxBufferSize 256








