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

#include "USBSerial.h"
//#include <SPI.h>
//#include "SimpleFOCDebug.h"
//#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SH110X.h>


void keyboard_test(int loop_num){
  pinMode(KEY1BIT, INPUT_PULLUP);
  //pinMode(KEY2BIT, INPUT_PULLUP); //Now reused for hardware serial
  pinMode(KEY3BIT, INPUT_PULLUP);
  //pinMode(KEY4BIT, INPUT_PULLUP); //Now reused for hardware serial
  pinMode(KEY5BIT, INPUT_PULLUP);

  for(int i=loop_num;i--;i>0){
      delay(100);
      Serial_print(i);
      Serial_print(" ");
      Serial_print(digitalRead(KEY1BIT));
      Serial_print(" ");
      //Serial_print(digitalRead(KEY2BIT));
      //Serial_print(" ");
      Serial_print(digitalRead(KEY3BIT));
      Serial_print(" ");
      //Serial_print(digitalRead(KEY4BIT));
      //Serial_print(" ");
      Serial_println(digitalRead(KEY5BIT));
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

void setupSerial(uint32_t baud) {
#ifdef USE_USB_SERIAL
  SerialUSB.begin(baud); // Use USB Serial (Serial)
#endif
#ifdef USE_HARDWARE_SERIAL
  Serial2.begin(baud); // Use Hardware Serial (Serial2)
#endif
}

int Serial_available(){
 int count=0;
#ifdef USE_HARDWARE_SERIAL
 count+=Serial2.available();
#endif
#ifdef USE_USB_SERIAL
  count+=SerialUSB.available();
#endif
  return (count>0);
}


int readSerialData() {
  int data=0;
  // Check for available data
#ifdef USE_HARDWARE_SERIAL
  if (Serial2.available()>0) {
    data = Serial2.read();
    return data;
  }
#endif
#ifdef USE_USB_SERIAL
  if (SerialUSB.available()>0) {
      data = SerialUSB.read();
      return data;
  }
#endif
  return data;
}