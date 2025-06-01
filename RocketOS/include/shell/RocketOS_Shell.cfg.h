#pragma once
/*RocketOS Shell Configuration File----------------------------
 * This file is used to configure the Shell of RocketOS.
 * The Shell is the user interface layer of RocketOS that processes commands sent by the user.
 * Look at the README file in this directory for more info.
*/

/*Interpreter Parameters
 * These macros parameterize the input interpreter of the shell. Look in the Shell README for more info about how these work.
 * Macros:
 * RocketOS_Shell_TokenBufferSize - Determines the size of the intepreter's token buffer and thus the maximum number of tokens that can be interpreted.
 * RocketOS_Shell_InterpreterCommandNameBufferSize - Determines the size of the buffer that stores command and subcommand names while parsing input. This determines the max size of command and subcommand names.
 * 
*/
#define RocketOS_Shell_TokenBufferSize 32
#define RocketOS_Shell_InterpreterCommandNameBufferSize 64

/*Command List Parameters
 * These macros parameterize the commands and command lists of the shell. Look in the Shell README for more info about how these work.
 * Macros:
 * RocketOS_Shell_CommandCallbackCaptureSize - Determines the max size of the data (in bytes) command callbacks can capture. 
 * 
*/
#define RocketOS_Shell_CommandCallbackCaptureSize 4
