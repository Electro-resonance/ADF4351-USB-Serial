//
//  morse_code.cpp
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

void appendMorseChar(char c, String& morseString) {
    switch (c) {
        case 'A':
        case 'a':
            morseString += ".- ";
            break;
        case 'B':
        case 'b':
            morseString += "-... ";
            break;
        case 'C':
        case 'c':
            morseString += "-.-. ";
            break;
        case 'D':
        case 'd':
            morseString += "-.. ";
            break;
        case 'E':
        case 'e':
            morseString += ". ";
            break;
        case 'F':
        case 'f':
            morseString += "..-. ";
            break;
        case 'G':
        case 'g':
            morseString += "--. ";
            break;
        case 'H':
        case 'h':
            morseString += ".... ";
            break;
        case 'I':
        case 'i':
            morseString += ".. ";
            break;
        case 'J':
        case 'j':
            morseString += ".--- ";
            break;
        case 'K':
        case 'k':
            morseString += "-.- ";
            break;
        case 'L':
        case 'l':
            morseString += ".-.. ";
            break;
        case 'M':
        case 'm':
            morseString += "-- ";
            break;
        case 'N':
        case 'n':
            morseString += "-. ";
            break;
        case 'O':
        case 'o':
            morseString += "--- ";
            break;
        case 'P':
        case 'p':
            morseString += ".--. ";
            break;
        case 'Q':
        case 'q':
            morseString += "--.- ";
            break;
        case 'R':
        case 'r':
            morseString += ".-. ";
            break;
        case 'S':
        case 's':
            morseString += "... ";
            break;
        case 'T':
        case 't':
            morseString += "- ";
            break;
        case 'U':
        case 'u':
            morseString += "..- ";
            break;
        case 'V':
        case 'v':
            morseString += "...- ";
            break;
        case 'W':
        case 'w':
            morseString += ".-- ";
            break;
        case 'X':
        case 'x':
            morseString += "-..- ";
            break;
        case 'Y':
        case 'y':
            morseString += "-.-- ";
            break;
        case 'Z':
        case 'z':
            morseString += "--.. ";
            break;
        case '0':
            morseString += "----- ";
            break;
        case '1':
            morseString += ".---- ";
            break;
        case '2':
            morseString += "..--- ";
            break;
        case '3':
            morseString += "...-- ";
            break;
        case '4':
            morseString += "....- ";
            break;
        case '5':
            morseString += "..... ";
            break;
        case '6':
            morseString += "-.... ";
            break;
        case '7':
            morseString += "--... ";
            break;
        case '8':
            morseString += "---.. ";
            break;
        case '9':
            morseString += "----. ";
            break;
        case ' ':
            morseString += " ";
            break;
        default:
            morseString += " "; // For unsupported characters, append a space
            break;
    }
}


String writeMorseString(const String& inputString) {
    size_t length = inputString.length();
    String morseString;

    for (size_t i = 0; i < length; i++) {
        appendMorseChar(inputString[i], morseString);
    }
    return morseString;
}

#define DOT_UNITS 60.0 //CODEX

float calculateDotDuration(int wpm) {
    return (int)(60.0 / (DOT_UNITS * (float)wpm) * 1000.0);
}


void processMorseString(const String& morseString, void (*RF_enable_Func)(), void (*RF_disable_Func)(), int wpm = 20, bool line_end=true) {
    int gap=calculateDotDuration(wpm);
    for (size_t i = 0; i < morseString.length(); i++) {
        char c = morseString.charAt(i);
        
        if (c == '.') {
            RF_enable_Func();
            delay(gap);  // Duration of dot
            RF_disable_Func();
            delay(gap);  // Inter-element gap
        } else if (c == '-') {
            RF_enable_Func();
            delay(3 * gap);  // Duration of dash
            RF_disable_Func();
            delay(gap);  // Inter-element gap
        } else if (c == ' ') {
            delay(7 * gap);  // Duration of space
        }
        Serial.print(c);
    }
    if(line_end==true){
        Serial.println();
    }
}


void interactiveMorseCode(void (*RF_enable_Func)(), void (*RF_disable_Func)(), int wpm = 20){
    bool escapKeyPressed=false;
    Serial.println("Entered Morse Code mode. Press ESC to exit...");
    // Loop until escape key is pressed
    while (!escapKeyPressed) {
        if (Serial.available()) {
            char c = Serial.read();
            if (c == 27) {  // ASCII code for escape key
                escapKeyPressed=true;
            } else {
                String command;
                // Append character to morseString
                command += c;
                // Process morseString
                String morseString = writeMorseString(command);
                processMorseString(morseString,RF_enable_Func,RF_disable_Func,wpm,false);
                // Check if character is newline, carriage return
                if (c == '\n' || c == '\r') {
                    Serial.println();  // Print newline
                }
            }
        }
    }
    // Exit the loop
    Serial.println();
    Serial.println();
    Serial.println("Escape key pressed. Exiting Morse Code mode...");
    delay(1000);
    Serial.println();
}

