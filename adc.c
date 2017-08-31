/******************************************************************************/
/*  Flash Sampler Tutorial                                                    */
/*  (c)2006 GPL, Owen Osborn, Critter and Guitari, Dearraindrop               */
/******************************************************************************/
/*                                                                            */
/*  :  Sampler for the Critter Board with on-board flash or flash cards       */
/*                                                                            */
/******************************************************************************/

#include "adc.h"
#include "LPC21xx.h"

/*
    There are 2 analog to digital convertes on the LPC2138.  There are 
    several channels (pins) attached to each ADC.  Check the LPC2138 
    documentation to see which pins are assigned to which ADC.
    These are 2 functions for reading each ADC.  Each function takes
    the channel number as an argument.
*/
int get_adc0(unsigned int channel){
  int val;
  AD0CR = 0;
  AD0DR = 0;
  ADGSR = 0;
		
  AD0CR = 0x00202000 | (1 << channel);  // select channel
  AD0CR |= 0x01000000;                  // start conversion
  
 // do {
    val = AD0DR;                        // Read A/D Data Register 
  //} while ((val & 0x80000000) == 0);    //Wait for the conversion to complete
  val = ((val >> 6) & 0x03FF);          //Extract the A/D result 

  return val;
}

int get_adc1(unsigned int channel){
  int val;
  AD1CR = 0;
  AD1DR = 0;
  ADGSR = 0;
		
  AD1CR = 0x00202000 | (1 << channel);  // select channel
  AD1CR |= 0x01000000;                  // start conversion
  
  do {
    val = AD1DR;                        // Read A/D Data Register 
  } while ((val & 0x80000000) == 0);    //Wait for the conversion to complete
  val = ((val >> 6) & 0x03FF);          //Extract the A/D result 

  return val;
}

int get_adc1_fast(unsigned int channel){
  int val;
  AD1CR = 0;
  AD1DR = 0;
  ADGSR = 0;
		
  AD1CR = 0x00202000 | (1 << channel);  // select channel
  AD1CR |= 0x01000000;                  // start conversion
  
  //do {
    val = AD1DR;                        // Read A/D Data Register 
 // } while ((val & 0x80000000) == 0);    //Wait for the conversion to complete
  val = ((val >> 6) & 0x03FF);          //Extract the A/D result 

  return val;
}
