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
// Title: ADF4351-USB-Serial
// Description: USB serial driven ADF4351 signal generator for Physics and Ham Radio
// Firmware for an STM32F103CBT6 with ADF4351 phase locked loop signal generator to provide
// A simple text based set of commands to control the amplitude, frequency, phase with
// additional features for generating LFO frerquency modulations (sine, triangle, ramp, stochastic noise),
// with linear or exponential frequency glide to allow smoother transitions.
// A Sigma delta mode is used to extend the 4 amplitude settings to simulated 16 bit (0-65535).
// As an added bonus for Ham Radio and Amateur Radio enthusiasts, the firmware includes a morse code encoder 
// which can directly encode text and key the output of the synthesizer at a set number of words-per-minute.
// (With the right license and expertise, just add an amplifier, antenna filter and antenna for CW in the HF band..)
//

#include <Arduino.h>
#include "brd_ltdz_stm32f103cb.h"
#include "adf4351.h"
#include <math.h>
#include "sine_16bit_2048.h"
#include "morse_code.h"

#define SWVERSION "2.0"


ADF4351  vfo(PIN_SS, SPI_MODE0, 1000000UL , MSBFIRST) ;

int32_t deltaAmplitude=-1;
int32_t mod_speed=2;
int32_t linearRamp=0;
int32_t sineWave=0;
int32_t triangle=0;
int32_t randomMod=0;
int32_t exp_glide=0;
int32_t glide=0;
bool lock_enable=false;

void setup()
{
  //Toggle the USB+ pin
  //May trigger USB detect from host
  const byte USB_Pin = PA12;
  pinMode(USB_Pin, OUTPUT);
  digitalWrite(USB_Pin, HIGH);
  delay(50);
  digitalWrite(USB_Pin, LOW);

  SerialUSB.begin(115200);
  //SimpleFOCDebug::enable(&SerialUSB);

  delay(10);
  SerialUSB.print("Adf4351 demo v") ;
  SerialUSB.println(SWVERSION) ;
  delay(10); 
}

void setupdds()
{
  /*!
      setup the chip (for a 10 mhz ref freq)
      most of these are defaults
  */
  vfo.pwrlevel = 0 ; ///< sets to -4 dBm output
  vfo.RD2refdouble = 0 ; ///< ref doubler off
  vfo.RD1Rdiv2 = 0 ;   ///< ref divider off
  vfo.ClkDiv = 150 ;
  vfo.BandSelClock = 80 ;
  vfo.RCounter = 1 ;  ///< R counter to 1 (no division)
  vfo.ChanStep = steps[2] ;  ///< set to 10 kHz steps
  /*!
      sets the reference frequency to 10 Mhz
  */
  if ( vfo.setrf(10000000UL) ==  0 )
    SerialUSB.println("ref freq set to 10 Mhz") ;
  else
    SerialUSB.println("ref freq set error") ;
  /*!
    initialize the chip
  */
  vfo.init() ;
  /*!
      enable frequency output
  */
  vfo.enable() ;
}


uint16_t freq_loop=0;
uint32_t last_f=102500000; 
uint32_t setpoint_freq=last_f;
uint32_t startpoint_freq=last_f;  
uint32_t current_freq=last_f; 
uint16_t wpm=20;

void enableRF() {
    vfo.enable(); // Code to enable the ADF4351 RF
} 

void disableRF() {
    vfo.disable(); // Code to disable the ADF4351 RF
}

void processSerialInput()
{
  static String command = "";

  while (Serial.available())
  {
    char c = Serial.read();
    // Echo back the received character
    Serial.print(c);
    // Convert the received character to uppercase
    c = toupper(c);

    // Check if the received character is a newline or carriage return
    if (c == '\n' || c == '\r')
    {
      Serial.println();
      // Process the command if it's not empty
      if (command.length() > 0)
      {
        char firstChar = command[0];
        switch (firstChar)
        {
          case 'A':
          {
            command.remove(0, 1);  // Remove the first character
            uint16_t pwrlevel = command.toInt();
            uint16_t pwrSet = vfo.setAmplitude(pwrlevel);
            Serial.print("Amplitude set to: ");
            Serial.println(pwrSet);
            deltaAmplitude=-1;
            break;
          }
          case 'D':
          {
            vfo.disable();
            Serial.println("Disabled RF");
            linearRamp=0;
            sineWave=0;
            triangle=0;
            randomMod=0;
            deltaAmplitude=-1;
            break;
          }
          case 'E':
          {
            vfo.enable();
            Serial.println("Enabled RF");
            break;
          }
          case 'F':
          {
            command.remove(0, 1);  // Remove the first character
            uint32_t f = command.toInt();
            last_f=f;
            setpoint_freq=f;
            if(glide==0 && exp_glide==0){
              vfo.optimise_f_only(f, true, true);
              current_freq=f;
            } else {
              Serial.print("Frequency setpoint set to: ");
              Serial.println(f); 
              startpoint_freq=current_freq;
            }
            linearRamp=0;
            sineWave=0;
            triangle=0;
            randomMod=0;
            break;
          }
          case 'G':
          {
            command.remove(0, 1);  // Remove the first character
            glide = command.toInt();
            if(glide<0){
              glide=0;
            }
            Serial.print("Glide set to: ");
            Serial.println(glide);
            exp_glide=0;
            break;
          }
          case 'H':
          {
            Serial.println("H: ADF4351 STM32F103CB Help->");
            Serial.println("A: Set amplitude                     (0-4)");
            Serial.println("D: Disable RF");
            Serial.println("E: Enable RF");
            Serial.println("F: Set frequency                     (35000000 - 4400000000 Hz)");
            Serial.println("G: Glide Time                        (0-2000 ms)");
            Serial.println("J: Exponential Glide Time            (0-2000 ms)");
            Serial.println("I: Frequency information");
            Serial.println("L: Set linear frequency ramp         (0=stop, or: -/+____ Hz)");
            Serial.println("M: Morse Code                        (string)");
            Serial.println("O: Set triangle frequency modulation (0=stop, or: -/+____ Hz)");
            Serial.println("P: Set phase angle                   (0.0-360.0 deg.)");
            Serial.println("R: Register information");
            Serial.println("S: Set sinewave frequency modulation (0=stop, or: -/+____ Hz)");
            Serial.println("W: Morse Code words per minute       (5-120 WPM)");
            Serial.println("X: Modulation LFO Speed              (1-1024)");
            Serial.println("Y: Set sigma-delta amplitude         (-1=stop, or: 0-65535)");
            Serial.println("Z: Set random frequency modulation   (0=stop, or: -/+____ Hz)");
            break;
          }
          case 'I':
          {
            vfo.freqInfo();
            break;
          }
          case 'J':
          {
            command.remove(0, 1);  // Remove the first character
            exp_glide = command.toInt();
            if(exp_glide<0){
              exp_glide=0;
            }
            Serial.print("Exponential Glide set to: ");
            Serial.println(exp_glide);
            glide=0;
            break;
          }
          case 'L':
          {
            command.remove(0, 1);  // Remove the first character
            linearRamp = command.toInt();
            Serial.print("Linear ramp sweep set to: ");
            Serial.println(linearRamp);
            sineWave=0;
            triangle=0;
            randomMod=0;
            break;
          }
          case'M':
          {
            command.remove(0, 1);  // Remove the first character
            String morseString = writeMorseString(command);
            processMorseString(morseString,enableRF,disableRF,wpm);
            break;
          }
          case 'O':
          {
            command.remove(0, 1);  // Remove the first character
            triangle = command.toInt();
            Serial.print("Triangle sweep set to: ");
            Serial.println(triangle);
            sineWave=0;
            linearRamp=0;
            randomMod=0;
            break;
          }
          case 'P':
          {
            command.remove(0, 1);  // Remove the first character
            double phaseAngle = command.toFloat();
            double phaseSet=vfo.setPhaseAngle(phaseAngle);
            Serial.print("Phase angle set to: ");
            Serial.println(phaseSet);
            break;
          }
          case 'R':
          {
            vfo.regInfo();
            break;
          }
          case 'S':
          {
            command.remove(0, 1);  // Remove the first character
            sineWave = command.toInt();
            Serial.print("Sinewave sweep set to: ");
            Serial.println(sineWave);
            linearRamp=0;
            triangle=0;
            randomMod=0;
            break;
          }
          case 'W':
          {
            command.remove(0, 1);  // Remove the first character
            wpm = command.toInt();
            if(wpm<5){
              wpm=5;
            } else if (wpm>120){
              wpm=120;
            }
            Serial.print("Morse Code speed set to: ");
            Serial.print(wpm);
            Serial.println(" words per minute");
            break;
          }
          case 'X':
          {
            command.remove(0, 1);  // Remove the first character
            mod_speed = command.toInt();
            if(mod_speed<1){
              mod_speed=1;
            } else if (mod_speed>1024){
              mod_speed=1024;
            }
            Serial.print("Modulation speed set to: ");
            Serial.println(mod_speed);
            break;
          }
          case 'Y':
          {
            command.remove(0, 1);  // Remove the first character
            int32_t pwrlevel = command.toInt();
            if(pwrlevel!=-1){
              vfo.setSigmaDeltaAmplitude(pwrlevel);
              Serial.print("Sigma-delta amplitude set to: ");
              Serial.println(pwrlevel);
            } else {
              vfo.setAmplitude(0);
              Serial.print("Sigma-delta amplitude: disabled ");
            }
            deltaAmplitude=pwrlevel;
            break;
          }
          case 'Z':
          {
            command.remove(0, 1);  // Remove the first character
            randomMod = command.toInt();
            Serial.print("Random modulation set to: ");
            Serial.println(randomMod);
            linearRamp=0;
            sineWave=0;
            triangle=0;
            break;
          }
          default:
            // Invalid command
            Serial.println("Invalid command");
            break;
        }
      }

      // Clear the command string for the next command
      command = "";
    }
    else
    {
      // Add the received character to the command string
      command += c;
    }
  }
  // Check if no data is available
  if (Serial.available() == 0)
  {
    if(deltaAmplitude>=0){
      vfo.setSigmaDeltaAmplitude(deltaAmplitude);
    }
    if(linearRamp!=0 | sineWave!=0 | triangle!=0 | randomMod!=0 | glide>0 | exp_glide>0){
      freq_loop+=mod_speed;
      if(freq_loop>=sin2048Size){
        freq_loop=0;
      }
      uint32_t freq=0;
      if(linearRamp!=0){
        setpoint_freq=last_f+(double)freq_loop/(double)sin2048Size*(double)linearRamp;
        startpoint_freq=current_freq;
      } else if(sineWave!=0){
        setpoint_freq=last_f+(double)sin2048[freq_loop]/65536.0f*(double)sineWave;
        startpoint_freq=current_freq;
      } else if(triangle!=0){
        double time_period = (double)triangle;
        //double freq_range = (double)sin1024Size / time_period;
        double time_offset = ((double)freq_loop / (double)sin2048Size) * triangle;
        if (time_offset <= time_period / 2.0)
        {
          setpoint_freq = last_f + time_offset * 2;
        }
        else
        {
          setpoint_freq = last_f + (time_period - time_offset) * 2;
        }
        startpoint_freq = current_freq;
      } else if(randomMod!=0)
      {
        setpoint_freq=last_f+random(0, randomMod);
        startpoint_freq=current_freq;
      }
      if(exp_glide>0){
        double freq_diff=(double)setpoint_freq-(double)current_freq;
        freq=current_freq+(int32_t)(freq_diff/(double)exp_glide);
      } else if(glide>0){
        double freq_step=((double)setpoint_freq-(double)startpoint_freq)/(double)glide;
        double freq_diff=(double)setpoint_freq-(double)current_freq;
        if(fabsf(freq_diff)>fabsf(freq_step)){
          freq=(int32_t)((double)current_freq + freq_step);
        } else {
          freq=setpoint_freq; //When the setpoint is reached
        }
      } else {
        freq=setpoint_freq; //Default with no glide
      }
      if(current_freq!=freq){
        vfo.optimise_f_only(freq);
        current_freq=freq;
        lock_enable=false;
      } else {
        if(lock_enable==false){
          vfo.lock_freq();
          lock_enable=true;
        }
      }
    }
  }
}


void loop()
{
  delay(10);
  SerialUSB.println("Adf4351") ;

  //Setup ADF4351 defaults
  vfo.pwrlevel = 0 ; ///< sets to -4 dBm output
  vfo.RD2refdouble = 0 ; ///< ref doubler off
  vfo.RD1Rdiv2 = 0 ;   ///< ref divider off
  vfo.ClkDiv = 150 ;
  vfo.BandSelClock = 80 ;
  vfo.RCounter = 1 ;  ///< R counter to 1 (no division)
  vfo.ChanStep = steps[2] ;  ///< set to 10 kHz steps
  vfo.ChanStep = steps[0] ;  ///< set to 1 Hz steps

  // sets the reference frequency to 10 Mhz
  if ( vfo.setrf(25000000UL) ==  0 )
    SerialUSB.println("ref freq set to 25 Mhz") ;
  else
    SerialUSB.println("ref freq set error") ;

  //initialize the chip
  vfo.init() ;
  //enable frequency output
  vfo.enable() ;

  keyboard_test(2);

  vfo.setf_only(last_f);

  while(true){
    processSerialInput();
  }

}