//
//  morse_code.h
//  
//  Author:  Martin Timms
//  Date:    14th July 2023.
//  Contributors:
//  Version: 1.0
//
//  Released into the public domain.
//  
//  License: MIT License
//
// Description: Functions to generate morse code from text and to then call 
// RF on/off functions to key a signal generator
//

#include <Arduino.h>

//Append one character as a morse code sequence to a string
void appendMorseChar(char c, String& morseString);

//Form an output string of morse code from an input ascii string
String writeMorseString(const String& inputString);

//Parse a morse string of . - and space at a given wpm rate and call the two functions to control morse key operation
void processMorseString(const String& morseString, void (*RF_enable_Func)(), void (*RF_disable_Func)(), int gap = 20);