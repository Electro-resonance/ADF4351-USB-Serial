/*!
   @file ADF4351.h

   This is part of the Arduino Library for the ADF4351 PLL wideband frequency synthesier

   @section modifed By

   Martin Timms, 14th July 2023 with additional frequency setting methods and use of BitBangedSPI for STM32F103 

*/

#ifndef ADF4351_H
#define ADF4351_H
#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>
#include <BigNumber.h>


extern uint32_t steps[];  ///< Array of Frequency Step Values

#define NSTEPS 7  ///< Number of Freq Step Values defined

// need to use max unsigned long on arduino , oh well
#define ADF_FREQ_MAX  4294967295UL    ///< Maximum Generated Frequency = value of MAX Unsigned Long
#define ADF_FREQ_MIN  34385000UL      ///< Minimum Generated Frequency
#define ADF_PFD_MAX   32000000.0      ///< Maximum Frequency for Phase Detector
#define ADF_PFD_MIN   125000.0        ///< Minimum Frequency for Phase Detector
#define ADF_REFIN_MAX   250000000UL   ///< Maximum Reference Frequency
#define REF_FREQ_DEFAULT 25000000L ///< Default Reference Frequency


/*!
   @brief Stores a device register value

   This class is used to store and manage a single ADF4351 register value
   and provide bit field manipulations.
*/
class Reg
{
  public:
    /*!
       Constructor
    */
    Reg();
    /*!
       get the current register value
       returns same value as getbf(0,32)
       @return unsigned long value of the register
    */
    uint32_t get()  ;
    /*!
       sets the register value
       @param value unsigned long
    */
    void set(uint32_t value);
    /*!
       current register value

       returns same value as getbf(0,32)
    */
    uint32_t whole ;
    /*!
       modifies the register value based on a value and bitfield (mask)
       @param start index of the bit to start the modification
       @param len length number of bits to modify
       @param value value to modify (value is truncated if larger than len bits)
    */
    void setbf(uint8_t start, uint8_t len , uint32_t value) ;
    /*!
       gets the current register bitfield value, based on the start and length mask
       @param start index of the bit of where to start
       @param len length number of bits to get
       @return bitfield value
    */
    uint32_t getbf(uint8_t start, uint8_t len) ;

};

/*!
   @brief ADF4351 chip device driver

   This class provides the overall interface for ADF4351 chip. It is used
   to define the SPI connection, initalize the chip on power up, disable/enable
   frequency generation, and set the frequency and reference frequency.

   The PLL values and register values can also be set directly with this class,
   and current settings for the chip and PLL can be read.

   As a simple frequency generator, once the target frequency and desired channel step
   value is set, the library will perform the required calculations to
   set the PLL and other values, and determine the mode (Frac-N or Int-N)
   for the PLL loop. This greatly simplifies the use of the ADF4351 chip.

   The ADF4351 datasheet should be consulted to understand the correct
   register settings. While a number of checks are provided in the library,
   not all values are checked for allowed settings, so YMMV.

*/
class ADF4351
{
  public:
    /*!
       Constructor
       creates an object and sets the SPI parameters.
       see the Arduino SPI library for the parameter values.
       @param pin the SPI Slave Select Pin to use
       @param mode the SPI Mode (see SPI mode define values)
       @param speed the SPI Serial Speed (see SPI speed values)
       @param order the SPI bit order (see SPI bit order values)
    */
    ADF4351(byte pin, uint8_t mode, unsigned long  speed, BitOrder order );
    /*!
       initialize and start the SPI interface. Call this once after instanciating
       the object
    */
    void init() ;
    /*!
       sets the output frequency
       automatically calculates the PLL and other parameters
       returns false if the desired frequency does not match the
       calculated frequency or is out of range.

       @param freq target frequency
       @return success (True or False)
    */
    int  setf(uint32_t freq, uint16_t phase=1, uint32_t chan_steps=0); // set freq
    /*!
       sets the reference frequency
       sets the incoming reference frequency to the ADF4351 chip,
       based on your OXCO or other freqeuncy reference source value.
       Checks for min and max settings

       @param f reference frequency
       @return success (True or False)
    */

    int lock_freq(bool debug=false);

    int optimise_f_only(uint32_t freq, bool debug=false, bool loginfo=false, bool gcd_method=false);
   /*!
      sets the reference frequency trying to optimise the channel setting for stability
      and retrying with different settings if a setting would have otherwise failed.
    */

    int setf_only(uint32_t freq, uint32_t chan_steps=0,bool debug=false); // set reference freq
    /*!
      sets the reference frequency changing minimum number of registers
    */

    int setrf(uint32_t f) ;  // set reference freq
    /*!
       turns on the output frequency (enables the CE pin)
       The chip is still powered up, so all settings are
       maintained in this mode.
    */

   int writeRegisters(bool debug=false); //Write R to the registers
   /*!
      write complete set of registers via SPI
   */

    void enable();
    /*!
       turns off the output frequency (disables the CE pin)
       The chip is still powered up, so all settings are
       maintained in this mode.
    */
    void disable();
    /*!
       writes the register value to the device
       normally used as an internal helper function
       @param n nth register to write to
       @param r the register value to write
    */

   void setPhase(uint16_t phase);
   /*!
       set phase as a value 0-4095
    */

   double setPhaseAngle(double phaseAngle);
   /*!
       set phase as a value 0-360 degrees
    */

   uint16_t setAmplitude(uint16_t pwrlevel);
   /*!
       set amplitude as a value 0-3
    */

   void setSigmaDeltaAmplitude(uint16_t pwrlevel);
   /*!
       set sigma delta value 0-65535
    */

    void freqInfo();

    void regInfo();

    void writeDev(int n, Reg r) ;

    /*!
       gets the value of the device register
       @param n nth register on the device
       @return the current value of the register
    */
    uint32_t getReg(int n) ;
    /*!
       calculates the greatest common denominator for two values
       helper function to calculate PLL values
       @param u value 1
       @param v value 2
       @return the greatest common denominator
    */
    uint32_t gcd_iter(uint32_t u, uint32_t v) ;
    /*!
       stores the SPI settings
    */
    SPISettings spi_settings;
    /*!
       stores the SPI Slave Select Pin
    */
    uint8_t pinSS ;
    /*!
       array for storing the working register values (used for writing)
    */
    Reg R[6] ;
    /*!
       stores the reference frequency
    */
    uint32_t reffreq;
    /*!
       stores the current frequency generation on/off status
    */
    byte enabled ;
    /*!
       stores the calculated frequency (vs the desired frequency)
       used to check for issues in the setf() function.
       this value is overwritten each time sef() is called.
    */
    uint32_t cfreq ;
    /*!
       stores the PLL INT value for the current frequency
       this value is overwritten each time sef() is called.
    */
    uint16_t N_Int ;
    /*!
       The PLL Frac value for the current frequency
       this value is overwritten each time sef() is called.
    */
    int Frac ;
    /*!
       The PLL Mod value for the current frequency
       this value is overwritten each time sef() is called.
    */
    uint32_t Mod ;
    /*!
       The PLL Phase Detect Freq  value for the current frequency
       can be changed if a new reference frequency is used.
    */
    float PFDFreq ;
    /*!
       the channel step value
       can be directly changed to set a new frequency step
       from the defined steps[] array. You should not use
       an arbitrary value for this.
    */
    uint32_t ChanStep;
    /*!
       the PLL output divider value for the current frequency
       this value is overwritten each time sef() is called.
    */
    int outdiv ;
    /*!
       the PLL ref freq doubler flag for the current frequency
       it is used to double the incoming ref frequency.
       this should used to when setting up the reference frequency
       set to 0 or 1
    */
    uint8_t RD2refdouble ;
    /*!
       the PLL R counter value for the current frequency
       10bit counter used to divide the ref freq for the PFD
    */
    int RCounter ;
    /*!
       the PLL ref freq Divider by 2
       sets a divide-by-2 between the Rcounter and PFD
    */
    uint8_t RD1Rdiv2 ;
    /*!
       the PLL Band Select Clock Value
    */
    uint8_t BandSelClock ;
    /*!
       the PLL Clock Divider value
       the 12bit timeout counter for activation of phase resync and fast lock.
    */
    int ClkDiv ;
    /*!
       The PLL ref freq prescaler (divider) flag
       set to 0 or 1
    */
    uint8_t Prescaler ;
    /*!
       The power output level settings
       allowed values 0-4
    */
    byte pwrlevel ;

    uint8_t SPImode;
    unsigned long SPIspeed;
    uint8_t SPIorder;

};


#endif