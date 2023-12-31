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

#include "usbd_if.c" //Arduino USB detatch



#define SWVERSION "2.0"


ADF4351  vfo(PIN_SS, SPI_MODE0, 1000000UL , MSBFIRST) ;

int32_t deltaAmplitude=-1;
int32_t mod_speed=2;
int32_t linearRamp=0;
int32_t sineWave=0;
int32_t triangle=0;
int32_t randomMod=0;
int32_t randomDither=0;
int32_t exp_glide=0;
int32_t constant_glide=0;
int32_t glide=0;
bool lock_enable=false;


void setup()
{
  delay(500);
  setupSerial(115200);
  USBD_reenumerate(); //Only if USBD_ATTACH_PIN or USBD_DETACH_PIN are defined to rtrigger USB reenumeration

  delay(10);
  Serial_print("Adf4351 demo v") ;
  Serial_println(SWVERSION) ;
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
  {
    Serial_println("ref freq set to 10 Mhz") ;
  } else {
    Serial_println("ref freq set error") ;
  }
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
double freq_step=1;
bool calc_freq_step=false;
bool modulation_enable;
unsigned long currentTime=micros();
unsigned long startTime=currentTime;

void enableRF() {
    vfo.enable(); // Code to enable the ADF4351 RF
} 

void disableRF() {
    vfo.disable(); // Code to disable the ADF4351 RF
}

void processSerialInput()
{
  static String command = "";
  while (Serial_available())
  {
    char c = readSerialData();
    // Echo back the received character
    Serial_print(c);
    // Convert the received character to uppercase
    c = toupper(c);

    // Check if the received character is a newline or carriage return
    if (c == '\n' || c == '\r')
    {
      Serial_println();
      // Process the command if it's not empty
      if (command.length() > 0)
      {
        char firstChar = command[0];
        command.remove(0, 1);  // Remove the first character
        switch (firstChar)
        {
          case 'A':
          {
            uint16_t pwrlevel = command.toInt();
            uint16_t pwrSet = vfo.setAmplitude(pwrlevel);
            Serial_print("Amplitude set to: ");
            Serial_println(pwrSet);
            deltaAmplitude=-1;
            break;
          }
          case 'B':
          {
            int32_t sleep_time = command.toInt();
            if(sleep_time<0){
              sleep_time=0;
            } else if (sleep_time>120000){
              sleep_time=120000;
            }
            Serial_print("Waiting for: ");
            Serial_print(sleep_time);
            Serial_println("ms");
            delay(sleep_time);
            Serial_println("Wait completed");
            break;
          }
          case 'D':
          {
            vfo.disable();
            Serial_println("Disabled RF");
            linearRamp=0;
            sineWave=0;
            triangle=0;
            randomMod=0;
            randomDither=0;
            deltaAmplitude=-1;
            break;
          }
          case 'E':
          {
            vfo.enable();
            Serial_println("Enabled RF");
            break;
          }
          case 'F':
          {
            uint32_t f = command.toInt();
            last_f=f;
            setpoint_freq=f;
            if(glide==0 && exp_glide==0 && constant_glide==0){
              vfo.optimise_f_only(f, true, true);
              current_freq=f;
              vfo.lock_freq();
              lock_enable=true;
            } else {
              Serial_print("Frequency setpoint set to: ");
              Serial_println(f); 
              startpoint_freq=current_freq;
              calc_freq_step=true;
            }
            linearRamp=0;
            sineWave=0;
            triangle=0;
            randomMod=0;
            break;
          }
          case 'G':
          {
            glide = command.toInt();
            if(glide<0){
              glide=0;
            }
            Serial_print("Glide set to: ");
            Serial_println(glide);
            exp_glide=0;
            constant_glide=0;
            break;
          }
          case 'H':
          {
            Serial_println("H: ADF4351 STM32F103CB Help->");
            Serial_println("A: Set amplitude                     (0-4)");
            Serial_println("B: Time delay in milliseconds        (0-120000)");
            Serial_println("D: Disable RF");
            Serial_println("E: Enable RF");
            Serial_println("F: Set frequency                     (35000000 - 4400000000 Hz)");
            Serial_println("G: Glide Time                        (0-2000 ms)");
            Serial_println("I: Frequency information");
            Serial_println("J: Exponential Glide Time            (0-2000 ms)");
            Serial_println("K: Constant Glide Time               (0-2000 ms)");
            Serial_println("L: Set linear frequency ramp         (0=stop, or: -/+____ Hz)");
            Serial_println("M: Morse Code                        (string)");
            Serial_println("Morse: enter morse only mode         (ESC to exit)");
            Serial_println("O: Set triangle frequency modulation (0=stop, or: -/+____ Hz)");
            Serial_println("P: Set phase angle                   (0.0-360.0 deg.)");
            Serial_println("R: Register information");
            Serial_println("S: Set sinewave frequency modulation (0=stop, or: -/+____ Hz)");
            Serial_println("V: Set random dither frequency width (0=stop, or: -/+____ Hz)");
            Serial_println("W: Morse Code words per minute       (5-120 WPM)");
            Serial_println("X: Modulation LFO Speed              (1-1024)");
            Serial_println("Y: Set sigma-delta amplitude         (-1=stop, or: 0-65535)");
            Serial_println("Z: Set random frequency modulation   (0=stop, or: -/+____ Hz)");
            break;
          }
          case 'I':
          {
            vfo.freqInfo();
            Serial_println();
            Serial_println("Mod options:");
            Serial_print("G: Linear glide: ");
            Serial_println(glide);
            Serial_print("J: Expontential glide: ");
            Serial_println(exp_glide);
            Serial_print("K: Constant glide: ");
            Serial_println(constant_glide);
            Serial_print("L: Linear ramp: ");
            Serial_println(linearRamp);
            Serial_print("S: Sinewave: ");
            Serial_println(sineWave);
            Serial_print("T: Triangle: ");
            Serial_println(triangle);
            Serial_print("V: Random Dither:");
            Serial_println(randomDither*2);
            Serial_print("X: Modulation Speed: ");
            Serial_println(mod_speed);
            Serial_print("Y: Sigma delta Amplitude: ");
            Serial_println(deltaAmplitude);
            Serial_print("Z: Random Modulation: ");
            Serial_println(randomMod);
            Serial_print("Lock Enable: ");
            Serial_println(lock_enable);
            Serial_print("Freq step: ");
            Serial_println(freq_step);
            break;
          }
          case 'J':
          {
            exp_glide = command.toInt();
            if(exp_glide<0){
              exp_glide=0;
            }
            Serial_print("Exponential Glide set to: ");
            Serial_println(exp_glide);
            glide=0;
            constant_glide=0;
            break;
          }
          case 'K':
          {
            constant_glide = command.toInt();
            if(constant_glide<0){
              constant_glide=0;
            }
            Serial_print("Constant Glide set to: ");
            Serial_println(constant_glide);
            calc_freq_step=true;
            glide=0;
            exp_glide=0;
            break;
          }
          case 'L':
          {
            linearRamp = command.toInt();
            Serial_print("Linear ramp sweep set to: ");
            Serial_println(linearRamp);
            sineWave=0;
            triangle=0;
            randomMod=0;
            break;
          }
          case'M':
          {
            if (command.startsWith("ORSE")) {
              //Interactive Morse Code mode
              interactiveMorseCode(enableRF,disableRF, wpm);
            } else {
              String morseString = writeMorseString(command);
              //Send only current string as Morse Code
              processMorseString(morseString,enableRF,disableRF,wpm);
            }
            break;
          }
          case 'O':
          {
            triangle = command.toInt();
            Serial_print("Triangle sweep set to: ");
            Serial_println(triangle);
            sineWave=0;
            linearRamp=0;
            randomMod=0;
            break;
          }
          case 'P':
          {
            double phaseAngle = command.toFloat();
            double phaseSet=vfo.setPhaseAngle(phaseAngle);
            Serial_print("Phase angle set to: ");
            Serial_println(phaseSet);
            break;
          }
          case 'R':
          {
            vfo.regInfo();
            break;
          }
          case 'S':
          {
            sineWave = command.toInt();
            Serial_print("Sinewave sweep set to: ");
            Serial_println(sineWave);
            linearRamp=0;
            triangle=0;
            randomMod=0;
            break;
          }
          case 'V':
          {
            randomDither = command.toInt();
            Serial_print("Random diter frequency width set to: ");
            Serial_println(randomDither);
            randomDither/=2; //Divide by two as amplitude spread equally either side of carrier
            break;
          }
          case 'W':
          {
            wpm = command.toInt();
            if(wpm<5){
              wpm=5;
            } else if (wpm>120){
              wpm=120;
            }
            Serial_print("Morse Code speed set to: ");
            Serial_print(wpm);
            Serial_println(" words per minute");
            break;
          }
          case 'X':
          {
            mod_speed = command.toInt();
            if(mod_speed<1){
              mod_speed=1;
            } else if (mod_speed>1024){
              mod_speed=1024;
            }
            Serial_print("Modulation speed set to: ");
            Serial_println(mod_speed);
            break;
          }
          case 'Y':
          {
            int32_t pwrlevel = command.toInt();
            if(pwrlevel!=-1){
              vfo.setSigmaDeltaAmplitude(pwrlevel);
              Serial_print("Sigma-delta amplitude set to: ");
              Serial_println(pwrlevel);
            } else {
              vfo.setAmplitude(0);
              Serial_print("Sigma-delta amplitude: disabled ");
            }
            deltaAmplitude=pwrlevel;
            break;
          }
          case 'Z':
          {
            randomMod = command.toInt();
            Serial_print("Random modulation set to: ");
            Serial_println(randomMod);
            linearRamp=0;
            sineWave=0;
            triangle=0;
            break;
          }
          default:
            // Invalid command
            Serial_println("Invalid command");
            break;
        }
        modulation_enable=(linearRamp!=0 | sineWave!=0 | triangle!=0 | randomMod!=0 | glide>0 | exp_glide>0 | constant_glide>0 | randomDither>0);
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
  if (Serial_available() == 0)
  {
    if(deltaAmplitude>=0){
      vfo.setSigmaDeltaAmplitude(deltaAmplitude);
    }
    currentTime = micros(); // Get the end time
    unsigned long elapsedTime = currentTime - startTime; // Calculate the elapsed time
      
    if(modulation_enable==true){
      freq_loop+=mod_speed;
      if(freq_loop>=sin2048Size){
        freq_loop=0;
      }
      uint32_t freq=0;
      if(linearRamp!=0){
        setpoint_freq=last_f+(double)freq_loop/(double)sin2048Size*(double)linearRamp;
        calc_freq_step=true;
        startpoint_freq=current_freq;
      } else if(sineWave!=0){
        setpoint_freq=last_f+(double)sin2048[freq_loop]/65536.0f*(double)sineWave;
        calc_freq_step=true;
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
        calc_freq_step=true;
        startpoint_freq = current_freq;
      } else if(randomMod!=0)
      {
        setpoint_freq=last_f+random(0, randomMod);
        calc_freq_step=true;
        startpoint_freq=current_freq;
      }

      if( calc_freq_step==true){
        freq_step = int32_t(fabs((double)setpoint_freq - (double)(current_freq)) / (double)constant_glide);
        calc_freq_step=false;
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
      } else if(constant_glide>0){
        freq=current_freq;
        if(freq != setpoint_freq){
          int32_t adjusted_freq_step = freq_step * (double)elapsedTime / 1000000.0; // Adjust for iteration time in secs
          if(adjusted_freq_step<1){
            adjusted_freq_step=1;
          }
          if (freq < setpoint_freq) {
            freq += adjusted_freq_step;
            if (freq > setpoint_freq) {
                freq = setpoint_freq;
            }
          } else {
            freq -= adjusted_freq_step;
            if (freq < setpoint_freq) {
                freq = setpoint_freq;
            }
          }
        }
      } else {
        freq=setpoint_freq; //Default with no glide
      }
      if(randomDither!=0){
        freq+=random(-randomDither, +randomDither);
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
  Serial_println("Adf4351") ;

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
  {
    Serial_println("ref freq set to 25 Mhz") ;
  } else {
    Serial_println("ref freq set error") ;
  }

  //initialize the chip
  vfo.init() ;
  //enable frequency output
  vfo.enable() ;

  delay(1000); 

  keyboard_test(2);

  vfo.setf_only(last_f);

  //disable frequency output
  vfo.disable() ;

  while(true){
    processSerialInput();
  }

}