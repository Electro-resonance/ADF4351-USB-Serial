# ADF4351-USB-Serial
<img src="./images/modulations.jpg" height ="250", width=100%>>
USB serial driven ADF4351 signal generator for Physics, Electronics Lab Test, RF Signal Experimentation and also (CW / HF / Morse Code) Ham Radio.

## Recently added or updated 🆕 ✨
- [Example glide modulation](images/exponential_glides.jpg)[14th July,2023]

## Introduction
This project arose from a requirement to run multiple frequency generators in parallel for a quantum physics experiment without resorting to a rack of signal generators. The low cost LTDZ ADF4351-PLL board was chosen as a source however, the default firmware did not provide the required USB-serial control, only keypad and OLED manual control. Some hardware mods were required to fix the USB interface and upgrade the microcontroller to one with same pin out and larger Flash memory. Extensive sets of modulation functions were written to provide dynamic control of the synthesizer output. 

The usb serial interface implements a very simple set of text commands which can be used either manually or programmatically from a script.

To make the firmware also useful for HAM radio and amateur radio enthusiasts, a morse code mode is added to directly key the RF output of the signal generator with text provided over the USB-serial interface.


## Hardware mods to the LTDZ board
<img src="./images/STM32_removal.jpg" height ="250">

1. An extra 1.5kohm resistor was required as a pull-up on the D+ USB line to 3.3V. 
2. A 4 lead pin header is soldered for the ST-Link firmware updates.
3. The STM32F1038t requires reflow work to swap out the stm32f103c6t6 (32 kBytes) with the larger flash memory STM32F103CBT6 chip (128kBytes) 

## Use on other hardware
It is suspected that this firmware could be easily be adapted for use on the devices sold as "Spectrum Analyzer USB 35-4400M Signal Source with Tracking Source Module RF Frequency Analysis Tool Support NWT4" as these have the ADF4351 as the output RF signal generator and also incorporate a larger ST32F103 microcontroller with USB interface. Further testing is required. It may be just as simple as modifying the SPI pins.


## Features
+ Triangle, Ramp, Sine wave and stochastic noise frequency modulation with a low frequency oscillator (LFO)
+ Optional linear or exponential frequency glide
+ Amplitude and phase control
+ Simulated 16 bit sigma delta amplitude modulation
+ Enable/disable RF output
+ Algorithms for selecting frequencies using either highest common dividor or by searching multiple channel space tables
+ Direct text to morse code keying of the signal generator at 5 WPM up to 120 WPM! 

## Commands 
The usb serial commands are very simple in that they are each a letter followed by a number or a set of words. The commands are as follows:

```console
H: ADF4351 STM32F103CB Help->
A: Set amplitude                     (0-4)
D: Disable RF
E: Enable RF
F: Set frequency                     (35000000 - 4400000000 Hz)
G: Glide Time                        (0-2000 ms)
J: Exponential Glide Time            (0-2000 ms)
I: Frequency information
L: Set linear frequency ramp         (0=stop, or: -/+____ Hz)
M: Morse Code                        (string)
O: Set triangle frequency modulation (0=stop, or: -/+____ Hz)
P: Set phase angle                   (0.0-360.0 deg.)
R: Register information
S: Set sinewave frequency modulation (0=stop, or: -/+____ Hz)
W: Morse Code words per minute       (5-120 WPM)
X: Modulation LFO Speed              (1-1024)
Y: Set sigma-delta amplitude         (-1=stop, or: 0-65535)
Z: Set random frequency modulation   (0=stop, or: -/+____ Hz)
```

# Compilation
The code is compiled with Visual Studio Code with Platform.IO

The compiled firmware is supplied for use with ST-LINK tools


## References

+ [siggen4351 Arduino Signal Generator using ADF4351](https://github.com/dfannin/siggen4351) by David Fannin
+ [Big Number Arduino Library](https://github.com/nickgammon/BigNumber) by Nick Gammon
+ [bitBangedSPI Lbrary](https://github.com/nickgammon/bitBangedSPI) by Nick Gammon
+ [ADF4351 Product Page](https://goo.gl/tkMjw6) Analog Devices
+ [SV1AFN ADF4351 Board](https://www.sv1afn.com/adf4351m.html) by Makis Katsouris, SV1AFN
+ [Schematics of a similar board](https://img.elecbee.com/ic/download/pdf/20190731013337STM32-ADF4351.pdf) Elecbee PCB
+ [STM32 Bluepill Setup](https://github.com/rpakdel/stm32_bluepill_arduino_prep) Reza Pakdel 



