/******************************************************************************/
/*  Long Recording                                                            */
/*  (c)2006 GPL, Owen Osborn, Critter and Guitari, Dearraindrop               */
/******************************************************************************/
/*                                                                            */
/*  :  Sampler for the Critter Board                                          */
/*                                                                            */
/******************************************************************************/


#include "system.h"
#include "LPC21xx.h"

// define pins for the two leds
#define LED_BOARD       (1 << 31)       // LED mounted to critter board

/* 
    Functions to turn LEDs on and off.  
*/
void led_board_init()
{
 // IODIR0 |= LED_BOARD;
//  IOSET0 |= LED_BOARD;                  // start with LED off
}

void led_card_init()
{
  //IODIR0 |= LED_FLASH_CARD;
  //IOSET0 |= LED_FLASH_CARD;             // start with LED off
}

inline void led_board(int stat)
{
   /* if (stat)
        IOCLR0 |= LED_BOARD;
    else
        IOSET0 |= LED_BOARD;*/
}

inline void led_card(int stat)
{
   // if (stat)
     //   IOCLR0 |= LED_FLASH_CARD;
   // else
     //   IOSET0 |= LED_FLASH_CARD;
}


/* 
    This is a pretty accurate milli-second delay that uses 
    Timer 1.  (timer 0 is used for the audio interrupt)
    Note that the timer is stopped and restarted each time this function is called,
    which leads to slight inaccruacies.  A more accurate schedualing system 
    might have the timer running constantly in the background, but this is still a 
    good general purpose delay.  
*/
void delay_ms(unsigned int dtime){
    volatile unsigned int current_time = 0;
    T1TCR = 0x2;                        // stop and reset timer
    T1PR = 58982;                       // timer prescale for ms (58982400 clks/sec)/1000 = 58982.4,
                                        // we round down to 58982 wich is a little off. (about 24ms short in 1 hour)
    T1TCR = 0x1;                        // start timer
    while (current_time < dtime)        // wait untill time is reached
        current_time = T1TC;                                        
}

/* 
    A tick delay.  One tick is 1/58982400 seconds.  delay_ticks()
    will actually be a few clocks longer than the number specified since it doesn't 
    take into account the number of clocks taken to stop/setup/restart the timer.
*/
void delay_ticks(unsigned int count){
    volatile unsigned int current_time = 0;
    T1TCR = 0x2;                        // stop and reset timer
    T1PR = 0;                           // timer prescale for ticks
    T1TCR = 0x1;                        // start timer
    while (current_time < count)        // wait untill time is reached
        current_time = T1TC;       
}

/* 
    waits for a character on UART0. does not return untill a character 
    is recieved.  UART0 is the one that connects to the USB programmer
    so this function is convenient for communicating with a pc.
*/
char get_char(void) { 
  while (!(U0LSR & 0x01));
  return (U0RBR);
}

/* 
    sends a character out UART0. 
    UART0 is the one that connects to the USB programmer
    so this function is convenient for communicating with a pc.
*/
void put_char(char c) {
  while((U0LSR & 0x20) == 0);
  U0THR = c;
}

/* 
    Initialize the LPC2138.  first the phase lock loop is setup
    so the final clock frequency is 58.9824 MHz.  The peripheral clock
    that connects to things like the UARTs and timers is also set to this frequency.
    UART0 is then setup for serial TX and RX at 115200 baud.
*/

#define PLOCK 0x400

void Initialize(void)  {
	
	// Setting Multiplier and Divider values
  	PLLCFG=0x23;
  	feed();
	// Enabling the PLL 
	PLLCON=0x1;
	feed();
	// Wait for the PLL to lock to set frequency
	while(!(PLLSTAT & PLOCK)) ;
	// Connect the PLL as the clock source
	PLLCON=0x3;
	feed();
	// Enabling MAM and setting number of clocks used for Flash memory fetch (4 cclks in this case)
	MAMCR=0x2;
	MAMTIM=0x4;
	// Setting peripheral Clock (pclk) to System Clock (cclk)
	VPBDIV=0x1;


    led_card_init();        // init LEDs
    led_board_init();

    PINSEL0 |= 0x00000005;	//enable uart0
    //Divisor = Pclk/(16*baudrate), where Pclk = VPBclk = Cclk/VPBDIV
    U0LCR 	= 0x83;			// 8 bits, no Parity, 1 Stop bit, DLAB = 1 
    U0DLM   = 0x00;		  	// 
    U0DLL 	= 0x20;        	// 115200 Baud Rate @ 58982400 VPB Clock  
    U0LCR 	= 0x03;        	// DLAB = 0    
    U0IER   = 0x01;         // rx interrupt enable
    U0FCR	= 0x01;			// Not sure this is necessary, but the manual says it is

}

/* 
    feed the phase lock loop. used during Initialize()
*/
void feed(void)
{
  PLLFEED=0xAA;
  PLLFEED=0x55;
}

/* 
    The following are functions that provide low level interrupt
    functionality such ase enabling and disabling interrupts.
*/
unsigned enableIRQ(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();
  __set_cpsr(_cpsr & ~IRQ_MASK);
  return _cpsr;
}

unsigned disableIRQ(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();
  __set_cpsr(_cpsr | IRQ_MASK);
  return _cpsr;
}

static inline unsigned __get_cpsr(void)
{
  unsigned long retval;
  asm volatile (" mrs  %0, cpsr" : "=r" (retval) : /* no inputs */  );
  return retval;
}

static inline void __set_cpsr(unsigned val)
{
  asm volatile (" msr  cpsr, %0" : /* no outputs */ : "r" (val)  );
}

// stock interrupt stubs
void IRQ_Routine (void) {
	//ledOn();
	while (1) ;	
}

void FIQ_Routine (void)  {
	//ledOn();
	while (1) ;	
}


void SWI_Routine (void)  {
	//ledOn();
	while (1) ;	
}

void UNDEF_Routine (void) {
	//ledOn();
	while (1) {             // usually means something went wrong
	led_board(0);
		delay_ms(70);
	led_board(1);
		delay_ms(70);
	}		
}





