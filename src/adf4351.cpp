/*!

   @file adf4351.cpp

   @mainpage ADF4351 Arduino library driver for Wideband Frequency Synthesizer

   @section intro_sec Introduction

   The ADF4351 chip is a wideband freqency synthesizer integrated circuit that can generate frequencies
   from 35 MHz to 4.4 GHz. It incorporates a PLL (Fraction-N and Integer-N modes) and VCO, along with
   prescalers, dividers and multipiers.  The users add a PLL loop filter and reference frequency to
   create a frequency generator with a very wide range, that is tuneable in settable frequency steps.

   The ADF4351 chip provides an I2C interface for setting the device registers that control the
   frequency and output levels, along with several IO pins for gathering chip status and
   enabling/disabling output and power modes.

   The ADF4351 library provides an Arduino API for accessing the features of the ADF chip.

   The basic PLL equations for the ADF4351 are:

   \f$ RF_{out} = f_{PFD} \times (INT +(\frac{FRAC}{MOD})) \f$

   where:

   \f$ f_{PFD} = REF_{IN} \times \left[ \frac{(1 + D)}{( R \times (1 + T))} \right]  \f$

   \f$ D = \textrm{RD2refdouble, ref doubler flag}\f$

   \f$ R = \textrm{RCounter, ref divider}\f$

   \f$ T = \textrm{RD1Rdiv2, ref divide by 2 flag}\f$




   @section dependencies Dependencies

   This library uses the BigNumber library (included) from Nick Gammon

   @section author Author

   David Fannin, KK6DF

   @section modifed By

   Martin Timms, 14th July 2023 with additional frequency setting methods and use of BitBangedSPI for STM32F103 

   @section license License

   MIT License

*/

#include "adf4351.h"
#include "brd_ltdz_stm32f103cb.h"
#include "BitBangedSPI.h"

//uint32_t steps[] = { 10 , 100, 1000, 5000, 10000, 50000, 100000 , 500000, 1000000 }; ///< Array of Allowed Step Values (Hz)
//uint32_t steps[] = { 7 , 97, 997, 4999, 9973, 49999, 99991 , 500000, 1000000 }; ///< Array of Allowed Step Values (Hz)
uint16_t freq_step_count = 16;
uint32_t steps[] = { 1, 5, 8, 10, 20, 50, 100, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000, 500000 }; ///< Array of Allowed Step Values (Hz)

bitBangedSPI spi1=bitBangedSPI((uint32_t)PIN_MOSI, (uint32_t)PIN_MISO, (uint32_t)PIN_SCK, 1);

/*!
   single register constructor
*/
Reg::Reg()
{
  whole = 0 ;
}

uint32_t Reg::get()
{
  return whole ;
}

void Reg::set(uint32_t value)
{
  whole = value  ;
}

void Reg::setbf(uint8_t start, uint8_t len, uint32_t value)
{
  uint32_t bitmask =  ((1UL  << len) - 1UL) ;
  value &= bitmask  ;
  bitmask <<= start ;
  whole = ( whole & ( ~bitmask)) | ( value << start ) ;
}

uint32_t  Reg::getbf(uint8_t start, uint8_t len)
{
  uint32_t bitmask =  ((1UL  << len) - 1UL) << start ;
  uint32_t result = ( whole & bitmask) >> start  ;
  return ( result ) ;
}

// ADF4351 settings
ADF4351::ADF4351(byte pin, uint8_t mode, unsigned long  speed, BitOrder order )
{
  spi_settings = SPISettings(speed, order, mode) ;
  pinSS = pin ;
  // settings for 25 MHz internal
  reffreq = REF_FREQ_DEFAULT ;
  enabled = false ;
  cfreq = 0 ;
  ChanStep = steps[0] ;
  RD2refdouble = 0 ;
  RCounter = 25 ;
  RD1Rdiv2 = 0 ;
  BandSelClock = 80 ;
  ClkDiv = 150 ;
  Prescaler = 0 ;
  pwrlevel = 0 ;
  SPIspeed=speed;
  SPImode=mode;
  SPIorder=order;
}

void ADF4351::init()
{
  pinMode(pinSS, OUTPUT) ;
  digitalWrite(pinSS, LOW) ;
  pinMode(PIN_CE, OUTPUT) ;
  pinMode(PIN_LD, INPUT) ; 
  spi1.begin(); 
} ;


int  ADF4351::setf(uint32_t freq, uint16_t phase, uint32_t chan_steps)
{
  ChanStep = steps[chan_steps];
  //  calculate settings from freq
  if ( freq > ADF_FREQ_MAX ) return 1 ;

  if ( freq < ADF_FREQ_MIN ) return 1 ;

  int localosc_ratio =   2200000000UL / freq ;
  outdiv = 1 ;
  int RfDivSel = 0 ;

  // select the output divider
  while (  outdiv <=  localosc_ratio   && outdiv <= 64 ) {
    outdiv *= 2 ;
    RfDivSel++  ;
  }

  if ( freq > 3600000000UL/outdiv )
    Prescaler = 1 ;
  else
    Prescaler = 0 ;

  PFDFreq = (float) reffreq  * ( (float) ( 1.0 + RD2refdouble) / (float) (RCounter * (1.0 + RD1Rdiv2)));  // find the loop freq
  BigNumber::begin(10) ;
  char tmpstr[20] ;
  // kludge - BigNumber doesn't like leading spaces
  // so you need to make sure the string passed doesnt
  // have leading spaces.
  int cntdigits = 0 ;
  uint32_t num = (uint32_t) ( PFDFreq / 10000 ) ;

  while ( num != 0 )  {
    cntdigits++ ;
    num /= 10 ;
  }

  dtostrf(PFDFreq, cntdigits + 8 , 3, tmpstr) ;
  // end of kludge
  BigNumber BN_PFDFreq = BigNumber(tmpstr) ;
  BigNumber BN_N = ( BigNumber(freq) * BigNumber(outdiv) ) / BN_PFDFreq ;
  N_Int =  (uint16_t) ( (uint32_t)  BN_N ) ;
  BigNumber BN_Mod = BN_PFDFreq / BigNumber(ChanStep) ;
  Mod = BN_Mod ;
  BN_Mod = BigNumber(Mod) ;
  BigNumber BN_Frac = ((BN_N - BigNumber(N_Int)) * BN_Mod)  + BigNumber("0.5")  ;
  Frac = (int) ( (uint32_t) BN_Frac);
  BN_N = BigNumber(N_Int) ;

  if ( Frac != 0  ) {
    uint32_t gcd = gcd_iter(Frac, Mod) ;

    if ( gcd > 1 ) {
      Frac /= gcd ;
      BN_Frac = BigNumber(Frac) ;
      Mod /= gcd ;
      BN_Mod = BigNumber(Mod) ;
    }
  }

  BigNumber BN_cfreq ;

  if ( Frac == 0 ) {
    BN_cfreq = ( BN_PFDFreq  * BN_N) / BigNumber(outdiv) ;

  } else {
    BN_cfreq = ( BN_PFDFreq * ( BN_N + ( BN_Frac /  BN_Mod) ) ) / BigNumber(outdiv) ;
  }

  cfreq = BN_cfreq ;

  if ( cfreq != freq ) Serial.println(F("output freq diff than requested")) ;

  BigNumber::finish() ;

  if ( Mod < 2 || Mod > 4095) {
    Serial.println(F("Mod out of range")) ;
    return 1 ;
  }

  if ( (uint32_t) Frac > (Mod - 1) ) {
    Serial.println(F("Frac out of range")) ;
    return 1 ;
  }

  if ( Prescaler == 0 && ( N_Int < 23  || N_Int > 65535)) {
    Serial.println(F("N_Int out of range")) ;
    return 1;

  } else if ( Prescaler == 1 && ( N_Int < 75 || N_Int > 65535 )) {
    Serial.println(F("N_Int out of range")) ;
    return 1;
  }

  // setting the registers to default values
  // R0
  R[0].set(0UL) ;
  // (0,3,0) control bits
  R[0].setbf(3, 12, Frac) ; // fractonal
  R[0].setbf(15, 16, N_Int) ; // N integer
  // R1
  R[1].set(0UL) ;
  R[1].setbf(0, 3, 1) ; // control bits
  R[1].setbf(3, 12, Mod) ; // Mod
  R[1].setbf(15, 12, phase); // phase
  R[1].setbf(27, 1, Prescaler); //  prescaler
  // (28,1,0) phase adjust
  // R2
  R[2].set(0UL) ;
  R[2].setbf(0, 3, 2) ; // control bits
  // (3,1,0) counter reset
  // (4,1,0) cp3 state
  // (5,1,0) power down
  R[2].setbf(6, 1, 1) ; // pd polarity

  if ( Frac == 0 )  {
    R[2].setbf(7, 1, 1) ; // LDP, int-n mode
    R[2].setbf(8, 1, 1) ; // ldf, int-n mode

  } else {
    R[2].setbf(7, 1, 0) ; // LDP, frac-n mode
    R[2].setbf(8, 1, 0) ; // ldf ,frac-n mode
  }

  R[2].setbf(9, 4, 7) ; // charge pump
  // (13,1,0) dbl buf
  R[2].setbf(14, 10, RCounter) ; //  r counter
  R[2].setbf(24, 1, RD1Rdiv2)  ; // RD1_RDiv2
  R[2].setbf(25, 1, RD2refdouble)  ; // RD2refdouble
  // R[2].setbf(26,3,0) ; //  muxout, not used
  R[2].setbf(26,3,6) ; //  muxout, digital lock detect
  //R[2].setbf(26,3,1) ; //  muxout, VDD
  // (29,2,0) low noise and spurs mode
  // R3
  R[3].set(0UL) ;
  R[3].setbf(0, 3, 3) ; // control bits
  R[3].setbf(3, 12, ClkDiv) ; // clock divider

  // (15,2,0) clk div mode
  // (17,1,0) reserved
  // (18,1,0) CSR
  //R[3].setbf(18,1,1); //Cycle slip reduction CSR
  // (19,2,0) reserved
  if ( Frac == 0 )  {
    R[3].setbf(21, 1, 1); //  charge cancel, reduces pfd spurs
    R[3].setbf(22, 1, 1); //  ABP, int-n

  } else  {
    R[3].setbf(21, 1, 0) ; //  charge cancel
    R[3].setbf(22, 1, 0); //  ABP, frac-n
  }

  R[3].setbf(23, 1, 1) ; // Band Select Clock Mode
  // (24,8,0) reserved
  // R4
  R[4].set(0UL) ;
  R[4].setbf(0, 3, 4) ; // control bits
  R[4].setbf(3, 2, pwrlevel) ; // output power 0-3 (-4dbM to 5dbM, 3db steps)
  R[4].setbf(5, 1, 1) ; // rf output enable
  //R[4].setbf(5, 1, 0) ; // rf output disable
  // (6,2,0) aux output power
  // (8,1,0) aux output enable
  // (9,1,0) aux output select
  // (10,1,0) mtld
  R[4].setbf(11, 1, 0) ; // vco power up
  // (11,1,1) vco power down
  R[4].setbf(12, 8, BandSelClock) ; // band select clock divider
  R[4].setbf(20, 3, RfDivSel) ; // rf divider select
  R[4].setbf(23, 1, 1) ; // feedback select
  // (24,8,0) reserved
  // R5
  R[5].set(0UL) ;
  R[5].setbf(0, 3, 5) ; // control bits
  // (3,16,0) reserved
  R[5].setbf(19, 2, 3) ; // Reserved field,set to 11
  // (21,1,0) reserved
  R[5].setbf(22, 2, 1) ; // LD Pin Mode Digital lock detect
  //R[5].setbf(22, 2, 3) ; // LD Pin Mode On
  //R[5].setbf(22, 2, 0) ; // LD Pin Mode Off
  // (24,8,0) reserved
  return writeRegisters();  
}


//Gretest common divisor
uint32_t gcd(uint32_t a, uint32_t b) {
    // Ensure a is always greater than or equal to b
    if (b > a) {
        uint32_t temp = a;
        a = b;
        b = temp;
    }
    while (b != 0 && a > 1 ) {
        uint32_t remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

int ADF4351::lock_freq(bool debug){
  R[3].setbf(0, 3, 3); // control bits
  R[3].setbf(18, 1, 1); // Enable cycle slip reduction
  return writeRegisters(debug);  
}

int  ADF4351::optimise_f_only(uint32_t freq, bool debug,bool log_info, bool gcd_method)
{
  bool freq_set=false;
  if(gcd_method==true){
    uint32_t s=gcd(freq,reffreq);
    if(setf_only(freq,s,debug)==0){
      if(log_info==true){
        Serial.print("Common divisor: ");
        Serial.println(s);
        Serial.print("Step Frequency set to: ");
        Serial.println(freq);
      }
      freq_set=true;
    }
  }
  if(freq_set==false){
    //Check for frequencies which are multiplies of step sizes
    for(int s=(freq_step_count-1); s>=0; s--){
      if (freq % steps[s]==0){
        if(setf_only(freq,s,debug)==0){
          if(log_info==true){
            Serial.print("Step Frequency set to: ");
            Serial.println(freq);
          }
          freq_set=true;
          break;
        }
      }
    }
  }
  if(freq_set==false){
    for(int s=0; s<freq_step_count; s++){
      if(setf_only(freq,s,debug)==0){
        if(log_info==true){
          Serial.print("Step Frequency set to: ");
          Serial.println(freq);
        }
        freq_set=true;
        break;
      }
    }
  }
  if(freq_set==false && log_info==true){
      Serial.println("Frequency not set");
  }
  return 0;
}

int  ADF4351::setf_only(uint32_t freq, uint32_t chan_steps, bool debug)
{
  ChanStep = steps[chan_steps];
  //  calculate settings from freq
  if ( freq > ADF_FREQ_MAX ) return 1 ;

  if ( freq < ADF_FREQ_MIN ) return 1 ;

  int localosc_ratio =   2200000000UL / freq ;
  outdiv = 1 ;
  int RfDivSel = 0 ;

  // select the output divider
  while (  outdiv <=  localosc_ratio   && outdiv <= 64 ) {
    outdiv *= 2 ;
    RfDivSel++  ;
  }

  if ( freq > 3600000000UL/outdiv )
    Prescaler = 1 ;
  else
    Prescaler = 0 ;

  PFDFreq = (float) reffreq  * ( (float) ( 1.0 + RD2refdouble) / (float) (RCounter * (1.0 + RD1Rdiv2)));  // find the loop freq
  BigNumber::begin(10) ;
  char tmpstr[20] ;
  // kludge - BigNumber doesn't like leading spaces
  // so you need to make sure the string passed doesnt
  // have leading spaces.
  int cntdigits = 0 ;
  uint32_t num = (uint32_t) ( PFDFreq / 10000 ) ;

  while ( num != 0 )  {
    cntdigits++ ;
    num /= 10 ;
  }

  dtostrf(PFDFreq, cntdigits + 8 , 3, tmpstr) ;
  // end of kludge
  BigNumber BN_PFDFreq = BigNumber(tmpstr) ;
  BigNumber BN_N = ( BigNumber(freq) * BigNumber(outdiv) ) / BN_PFDFreq ;
  N_Int =  (uint16_t) ( (uint32_t)  BN_N ) ;
  BigNumber BN_Mod = BN_PFDFreq / BigNumber(ChanStep) ;
  Mod = BN_Mod ;
  BN_Mod = BigNumber(Mod) ;
  BigNumber BN_Frac = ((BN_N - BigNumber(N_Int)) * BN_Mod)  + BigNumber("0.5")  ;
  Frac = (int) ( (uint32_t) BN_Frac);
  BN_N = BigNumber(N_Int) ;

  if ( Frac != 0  ) {
    uint32_t gcd = gcd_iter(Frac, Mod) ;

    if ( gcd > 1 ) {
      Frac /= gcd ;
      BN_Frac = BigNumber(Frac) ;
      Mod /= gcd ;
      BN_Mod = BigNumber(Mod) ;
    }
  }

  BigNumber BN_cfreq ;

  if ( Frac == 0 ) {
    BN_cfreq = ( BN_PFDFreq  * BN_N) / BigNumber(outdiv) ;

  } else {
    BN_cfreq = ( BN_PFDFreq * ( BN_N + ( BN_Frac /  BN_Mod) ) ) / BigNumber(outdiv) ;
  }

  cfreq = BN_cfreq ;

  if ( cfreq != freq ) {
    if(debug){
      Serial.println(F("output freq diff than requested")) ;
    }
  }

  BigNumber::finish() ;

  if ( Mod < 2 || Mod > 4095) {
    if(debug){
      Serial.print(F("Mod out of range: ")) ;
      Serial.println(Mod) ;
    }
    return 1 ;
  }

  if ( (uint32_t) Frac > (Mod - 1) ) {
    if(debug){
        Serial.println(F("Frac out of range")) ;
    }
    return 1 ;
  }

  if ( Prescaler == 0 && ( N_Int < 23  || N_Int > 65535)) {
    if(debug){
      Serial.println(F("N_Int out of range")) ;
    }
    return 1;

  } else if ( Prescaler == 1 && ( N_Int < 75 || N_Int > 65535 )) {
    if(debug){
      Serial.println(F("N_Int out of range")) ;
    }
    return 1;
  }

  // (0,3,0) control bits
  R[0].setbf(0, 3, 0) ; // control bits
  R[0].setbf(3, 12, Frac) ; // fractonal
  R[0].setbf(15, 16, N_Int) ; // N integer
  // R1
  R[1].setbf(0, 3, 1) ; // control bits
  R[1].setbf(3, 12, Mod) ; // Mod
  R[1].setbf(27, 1, Prescaler); //  prescaler
  // (28,1,0) phase adjust
  // R2
  R[2].setbf(0, 3, 2) ; // control bits
  R[2].setbf(6, 1, 1) ; // pd polarity
  if ( Frac == 0 )  {
    R[2].setbf(7, 1, 1) ; // LDP, int-n mode
    R[2].setbf(8, 1, 1) ; // ldf, int-n mode
  } else {
    R[2].setbf(7, 1, 0) ; // LDP, frac-n mode
    R[2].setbf(8, 1, 0) ; // ldf ,frac-n mode
  }
  R[2].setbf(9, 4, 7) ; // charge pump
  // (13,1,0) dbl buf
  R[2].setbf(14, 10, RCounter) ; //  r counter
  R[2].setbf(24, 1, RD1Rdiv2)  ; // RD1_RDiv2
  R[2].setbf(25, 1, RD2refdouble)  ; // RD2refdouble
  // R[2].setbf(26,3,0) ; //  muxout, not used
  R[2].setbf(26,3,6) ; //  muxout, digital lock detect
  //R[2].setbf(26,3,1) ; //  muxout, VDD
  // (29,2,0) low noise and spurs mode
  // R3
  R[3].setbf(0, 3, 3) ; // control bits
  R[3].setbf(3, 12, ClkDiv) ; // clock divider

  R[3].setbf(18, 1, 0); //disable slip reduction
  // (15,2,0) clk div mode
  // (17,1,0) reserved
  // (18,1,0) CSR
  // (19,2,0) reserved
  if ( Frac == 0 )  {
    R[3].setbf(21, 1, 1); //  charge cancel, reduces pfd spurs
    R[3].setbf(22, 1, 1); //  ABP, int-n

  } else  {
    R[3].setbf(21, 1, 0) ; //  charge cancel
    R[3].setbf(22, 1, 0); //  ABP, frac-n
  }

  R[3].setbf(23, 1, 1) ; // Band Select Clock Mode
  // (24,8,0) reserved
  // R4
  R[4].setbf(0, 3, 4) ; // control bits
  R[4].setbf(5, 1, 1) ; // rf output enable
  //R[4].setbf(5, 1, 0) ; // rf output disable
  // (6,2,0) aux output power
  // (8,1,0) aux output enable
  // (9,1,0) aux output select
  // (10,1,0) mtld
  R[4].setbf(11, 1, 0) ; // vco power up
  // (11,1,1) vco power down
  R[4].setbf(12, 8, BandSelClock) ; // band select clock divider
  R[4].setbf(20, 3, RfDivSel) ; // rf divider select
  R[4].setbf(23, 1, 1) ; // feedback select
  // (24,8,0) reserved
  // R5
  R[5].setbf(0, 3, 5) ; // control bits
  // (3,16,0) reserved
  R[5].setbf(19, 2, 3) ; // Reserved field,set to 11
  // (21,1,0) reserved
  R[5].setbf(22, 2, 1) ; // LD Pin Mode Digital lock detect
  //R[5].setbf(22, 2, 3) ; // LD Pin Mode On
  //R[5].setbf(22, 2, 0) ; // LD Pin Mode Off
  // (24,8,0) reserved
  return writeRegisters(debug);  
}

int ADF4351::writeRegisters(bool debug)
{
  int i;
  if(debug){
    Serial.println("writing to ADF") ;
  }
  for (i = 5 ; i > -1 ; i--) {
    writeDev(i, R[i]) ;
    //delayMicroseconds(2500) ;
  }
  if(debug){
    Serial.println("Written to ADF") ;
  }

  return 0 ;  // ok
}

void ADF4351::regInfo(){
  int i;
  Serial.println("Reg Info") ;
  for (i = 0 ; i < 6 ; i++) {
    Serial.print("Register ");
    Serial.print(i);
    Serial.print(" = 0b");
    // Get the register value
    uint32_t regValue = R[i].get();
    // Create a padded binary representation
    String binaryStr = String(regValue, BIN);
    while (binaryStr.length() < 32)
    {
      binaryStr = "0" + binaryStr;
    }
    Serial.println(binaryStr);
  }
}

int ADF4351::setrf(uint32_t f)
{
  if ( f > ADF_REFIN_MAX ) return 1 ;

  if ( f < 100000UL ) return 1 ;

  float newfreq  =  (float) f  * ( (float) ( 1.0 + RD2refdouble) / (float) (RCounter * (1.0 + RD1Rdiv2)));  // check the loop freq

  if ( newfreq > ADF_PFD_MAX ) return 1 ;

  if ( newfreq < ADF_PFD_MIN ) return 1 ;

  reffreq = f ;
  return 0 ;
}

void ADF4351::enable()
{
  enabled = true ;
  digitalWrite(PIN_CE, HIGH) ;
  //R[2].setbf(0, 3, 2) ; // control bits
  //R[2].setbf(26,3,1); //VDD

  R[4].setbf(0, 3, 4) ; // control bits
  R[4].setbf(5,1,1); //RF Main on
  R[4].setbf(8,1,1); //RF Aux on

  R[5].setbf(0, 3, 5) ; // control bits
  R[5].setbf(22,2,1); //Lock detect mode
  writeRegisters(); 
}

void ADF4351::disable()
{
  enabled = false ;
  digitalWrite(PIN_CE, LOW) ;
  //R[2].setbf(0, 3, 2) ; // control bits
  //R[2].setbf(26,3,2); //DGND

  R[4].setbf(0, 3, 4) ; // control bits
  R[4].setbf(5,1,0); //RF Main off
  R[4].setbf(8,1,0); //RF Aux off

  R[5].setbf(0, 3, 5) ; // control bits
  R[5].setbf(22,2,0); //Lock detect LOW
  writeRegisters();  
}

void ADF4351::setPhase(uint16_t phase)
{
  R[1].setbf(0, 3, 1) ; // control bits
  R[1].setbf(15, 12, phase); // phase
  writeRegisters(); 
}

double ADF4351::setPhaseAngle(double phaseAngle)
{
   if(phaseAngle>360 || phaseAngle<0){
    Serial.println("Phase Angle range is 0-360");
    phaseAngle=fmodf(phaseAngle,360.0f);

  }
  double phase=phaseAngle/360.0f*4096.0f;
  phase=fmodf(phase,4096.0f);
  setPhase(uint16_t(phase));
  return phaseAngle;
}

uint16_t ADF4351::setAmplitude(uint16_t pwrlevel)
{ 
  if(pwrlevel>3){
    Serial.println("Amplitude range is 0-3");
    pwrlevel=3;
  } else if(pwrlevel<0){
    Serial.println("Amplitude range is 0-3");
    pwrlevel=0;
  }
  R[4].setbf(0, 3, 4) ; // control bits
  R[4].setbf(3, 2, pwrlevel) ; // output power 0-3 (-4dbM to 5dbM, 3db steps)
  writeRegisters(); 
  return pwrlevel;
}

void ADF4351::setSigmaDeltaAmplitude(uint16_t pwrlevel)
{
  static float targetLevel = 0.0f;         // Target amplitude level
  static float integratedLevel = 0.0f;     // Integrated amplitude level

  // Check if the target level has changed
  targetLevel = static_cast<float>(pwrlevel) / 16384.0f;

  // Calculate the step size for dithering
  float stepSize = targetLevel - integratedLevel;

  // Threshold the step size
  if (stepSize < -0.5) {
    stepSize = -1.0;
  } else if (stepSize > +0.5) {
    stepSize = +1.0;
  } else {
    stepSize = 0.0;
  }

  // Perform sigma-delta dithering
  int currentLevel = static_cast<int>(integratedLevel + stepSize);
  if (currentLevel > 3) {
    currentLevel = 3;
  } else if (currentLevel < 0) {
    currentLevel = 0;
  }

  // Update the integrated level
  integratedLevel = (integratedLevel + (float)currentLevel)/2.0;

  // Update the amplitude level
  R[4].setbf(0, 3, 4);                       // Control bits
  R[4].setbf(3, 2, currentLevel);            // Output power 0-3 (-4dBm to 5dBm, 3dB steps)
  writeRegisters();
}

void ADF4351::writeDev(int n, Reg r)
{
  //Serial.println("writeDev") ;
  byte  txbyte ;
  int i ;
  digitalWrite(pinSS, LOW) ;
  delayMicroseconds(2) ;
  i=n ; // not used 
  for ( i = 3 ; i > -1 ; i--) {
    txbyte = (byte) (r.whole >> (i * 8)) ;
    //Serial.println("writeDev Transfer") ;
    spi1.transfer(txbyte) ;
  }
  digitalWrite(pinSS, HIGH) ;
  delayMicroseconds(1) ;
  digitalWrite(pinSS, LOW) ;
  //Serial.println("writeDev Complete") ;
}


  void ADF4351::freqInfo(){
    Serial.print("Freq:");
    Serial.println(cfreq) ;
    Serial.print("PLL INT:");
    Serial.println(N_Int);
    Serial.print("PLL FRAC:");
    Serial.println(Frac);
    Serial.print("PLL MOD:");
    Serial.println(Mod);
    Serial.print("PLL PFD:");
    Serial.println(PFDFreq);
    Serial.print("PLL output divider:");
    Serial.println(outdiv);
    Serial.print("PLL prescaler:");
    Serial.println(Prescaler);
    Serial.print("Lock Detect:");
    Serial.println(digitalRead(PIN_LD));
    Serial.print("RF Enable:");
    Serial.println(enabled);
  }


uint32_t   ADF4351::getReg(int n)
{
  return R[n].whole ;
}

uint32_t ADF4351::gcd_iter(uint32_t u, uint32_t v)
{
  uint32_t t;

  while (v) {
    t = u ;
    u = v ;
    v = t % v ;
  }

  return u ;
}
