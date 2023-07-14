//
//  brd_ltdz_stm32f103cb.cpp
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
// Description: IO specifics for an ADF4351-PPP LTDZ board which is 
// modified and upgaraded to a STM32F103CBT6 chip with larger flash memory
//

#include <Arduino.h>
#include "brd_ltdz_stm32f103cb.h"
//#include <SPI.h>
//#include "SimpleFOCDebug.h"
//#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SH110X.h>

void keyboard_test(int loop_num){
  pinMode(KEY1BIT, INPUT_PULLUP);
  pinMode(KEY2BIT, INPUT_PULLUP);
  pinMode(KEY3BIT, INPUT_PULLUP);
  pinMode(KEY4BIT, INPUT_PULLUP);
  pinMode(KEY5BIT, INPUT_PULLUP);

  for(int i=loop_num;i--;i>0){
      delay(100);
      SerialUSB.print(i);
      SerialUSB.print(" ");
      SerialUSB.print(digitalRead(KEY1BIT));
      SerialUSB.print(" ");
      SerialUSB.print(digitalRead(KEY2BIT));
      SerialUSB.print(" ");
      SerialUSB.print(digitalRead(KEY3BIT));
      SerialUSB.print(" ");
      SerialUSB.print(digitalRead(KEY4BIT));
      SerialUSB.print(" ");
      SerialUSB.println(digitalRead(KEY5BIT));
  }
}

void oled_setup(){
    // // Create the OLED display
    // Adafruit_SH1106G display = Adafruit_SH1106G(128, 64,OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);
    // // Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, Wire, OLED_RST);
    //
    // display.begin(0, true); // we dont use the i2c address but we will reset!
    // // Show image buffer on the display hardware.
    // // Since the buffer is intialized with an Adafruit splashscreen
    // // internally, this will display the splashscreen.
    // display.display();
    // delay(5);
    // //Clear the buffer.
    // display.clearDisplay();
    // //text display tests
    // display.setTextSize(1);
    // display.setTextColor(SH110X_WHITE);
    // display.setCursor(0, 0);
    // display.println("ADF4351 Test");
}