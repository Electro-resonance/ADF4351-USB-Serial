//
//  brd_ltdz_stm32f103cb.h
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

//Schematic
//https://img.elecbee.com/ic/download/pdf/20190731013337STM32-ADF4351.pdf
//OLED:
//https://www.google.com/url?sa=i&rct=j&q=&esrc=s&source=web&cd=&ved=0CAIQw7AJahcKEwjwkMHlqf3_AhUAAAAAHQAAAAAQAw&url=https%3A%2F%2Fwww.crystalfontz.com%2Fcontrollers%2FSinoWealth%2FSH1106%2F468%2F&psig=AOvVaw3VD3veFE1-kBqfVLacjLLL&ust=1688844399634853&opi=89978449
//STM32F103CB
//https://www.researchgate.net/figure/Pin-diagram-of-STM32F103_fig2_343718149
//ADF4351
//https://www.analog.com/media/en/technical-documentation/data-sheets/ADF4351.pdf

//Reference software:
//https://github.com/dfannin/siggen4351

//https://github.com/rpakdel/stm32_bluepill_arduino_prep
//Check the resistance between pin A12 and 3.3v.
//Soldered a regular 1.5k resistor on above the board between A12 and 3.3v.


#ifndef BRD_LTDZ_H
#define BRD_LTDZ_H

//OLED display
#define OLED_MOSI     PA7
#define OLED_CLK      PA6
#define OLED_DC       PA4
#define OLED_CS           PB9
#define OLED_RST      PA5

//ADF4351
#define PIN_LD   PA8    ///< Ard Pin for Lock Detect **Not Used - LED only**

#define PIN_CE   PB12    ///< Ard Pin for Chip Enable
#define PIN_SS   PB13    ///< Ard Pin for SPI ADF Select **PIN_LE**
#define PIN_MOSI  PB14  ///< Ard Pin for SPI MOSI
#define PIN_MISO  PB11  ///< Ard Pin for SPI MISO
#define PIN_SCK  PB15   ///< Ard Pin for SPI CLK

//KEYPAD PB0,PB1, PA1, PA2, PA3
#define KEY1BIT PA1 //LEFT 
#define KEY2BIT PA3 //DOWN
#define KEY3BIT PB0 //RIGHT
#define KEY4BIT PA2 //Select
#define KEY5BIT PB1 //UP

//HardwareSerial Serial1(PA10,PA9);
//HardwareSerial Serial2(PA3,PA2);    // PA3  (RX)  PA2  (TX)
//HardwareSerial Serial3(PB11,PB10);

void keyboard_test(int loop_num);
void oled_setup();


#define USE_USB_SERIAL
#define USE_HARDWARE_SERIAL

void setupSerial(uint32_t baud);
int readSerialData();

int Serial_available();

// Custom print function using a macro to redirect to the appropriate Serial print function
//#ifdef USE_USB_SERIAL
    #define Serial_print(...) SerialUSB.print(__VA_ARGS__);Serial2.print(__VA_ARGS__)
    #define Serial_println(...) SerialUSB.println(__VA_ARGS__);Serial2.println(__VA_ARGS__)
//#endif
//#ifdef USE_HARDWARE_SERIAL
//   #define Serial_print(...) Serial2.print(__VA_ARGS__)
//    #define Serial_println(...) Serial2.println(__VA_ARGS__)
//#endif

#endif