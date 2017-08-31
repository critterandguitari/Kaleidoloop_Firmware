/******************************************************************************/
/*  Long Recording                                                            */
/*  (c)2006-2009 GPL, Owen Osborn, Critter and Guitari, Dearraindrop          */
/******************************************************************************/
/*                                                                            */
/*  :  Sampler for the Critter Board                                          */
/*                                                                            */
/******************************************************************************/

#define REC (IOPIN0&(1<<30))
#define RW (IOPIN0&(1<<29))
#define FF (IOPIN0&(1<<28))       
#define MODE_BUTTON (IOPIN0&(1<<25))       


#define RLED_INIT   (IODIR0|=(1<<26))
#define PLED_INIT	(IODIR0|=(1<<31)|(1<<22))
#define RLED_ON		(IOCLR0|=(1<<26))
#define RLED_OFF	(IOSET0|=(1<<26))	
#define PLED_ON		(IOCLR0|=(1<<31))
#define PLED_OFF	(IOSET0|=(1<<31)|(1<<22))		

#define DEBOUNCE 3

void error_out(unsigned int);
void get_buttons(void);
void interface_check_for_events(void);
void fill_flash(unsigned int num);
void play_mode_led(unsigned int stat);
void play_led_on(void);
void play_led_off(void);

