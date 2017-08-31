/******************************************************************************/
/*  Long Recording                                                            */
/*  (c)2006-2009 GPL, Owen Osborn, Critter and Guitari, Dearraindrop          */
/******************************************************************************/
/*                                                                            */
/*  :  Sampler for the Critter Board                                          */
/*                                                                            */
/******************************************************************************/
#include "LPC21xx.h"
#include "recorder.h"
#include "interface.h"
#include "adc.h"
#include "system.h"

static unsigned int recordButtonUp = 1;
static unsigned int recordButton = 0;

static unsigned int ffButtonUp = 1;
static unsigned int ffButton = 0;

static unsigned int rwButtonUp = 1;
static unsigned int rwButton = 0;

static unsigned int modeButtonUp = 1;
static unsigned int modeButton = 0;

extern unsigned int buttonEvent;
extern unsigned int playMode;


void error_out(unsigned int stat){
    
    stat &= 0x3;

    for(;;){
        PLED_OFF;RLED_OFF;delay_ms(800); // wait a sec in between
         // use both for yellow 
        if (stat & 1) {PLED_ON;RLED_ON;} else {RLED_ON;}
        delay_ms(300);
        PLED_OFF;RLED_OFF;delay_ms(300);

        if ((stat >> 1) & 1) {PLED_ON;RLED_ON;} else {RLED_ON;}
        delay_ms(300);
        PLED_OFF;RLED_OFF;delay_ms(300);

        if ((stat >> 2) & 1) {PLED_ON;RLED_ON;} else {RLED_ON;}
        delay_ms(300);
        PLED_OFF;RLED_OFF;delay_ms(300);
    } 

}

// green and blue depend on play mode
void play_led_on(void) {
	if (playMode) IOCLR0|=(1<<22);
    else IOCLR0|=(1<<31);
}

void play_led_off(void){
    IOSET0|=(1<<22) | (1<<31);
}

void fill_flash(unsigned int num) {
    unsigned int i = 0;
        
   for (i = 0; i < num; i++) {
        PLED_ON;delay_ms(300); // wait a sec in between
        PLED_OFF;delay_ms(300); // wait a sec in between
   }
   PLED_OFF;RLED_OFF;delay_ms(800); // wait a sec in between
}

void play_mode_led(unsigned int stat) {
/*
    IOSET1 |= ((1<<22) | (1<<26) | (1<<31));  // all off
    if (stat == 0)
        IOCLR1 |= (1<<22);
    if (stat == 1)
        IOCLR1 |= ((1<<22) | (1<<26));
    if (stat == 2)
        IOCLR1 |= (1<<26);
    if (stat == 3)
        IOCLR1 |= ((1<<26) | (1<<31));
    if (stat == 4)
        IOCLR1 |= (1<<31);
    if (stat == 5)
        IOCLR1 |= ((1<<22) | (1<<31));
        */
}

void interface_check_for_events(void){
        // check buttons
        if (buttonEvent){
            get_buttons();
            buttonEvent = 0;
        }

        // process button events and update synthesizers     
        if ((recordButton == DEBOUNCE) && recordButtonUp){
            recorder_toggle_mode();
            recorder_set_play_all(0);   // stop playing all tracks, go into loop mode
            recordButtonUp = 0;
        }
        if ((recordButton == 0) && !recordButtonUp){
            recordButtonUp = 1;
        }

        if ((ffButton == DEBOUNCE) && ffButtonUp){
            recorder_next();
            recorder_set_play_all(0); // stop playing all tracks, go into loop mode
            ffButtonUp = 0;
        }
        if ((ffButton == 0) && !ffButtonUp){
            ffButtonUp = 1;
        }

        if ((rwButton == DEBOUNCE) && rwButtonUp){
            recorder_prev();
            recorder_set_play_all(0); // stop playing all tracks, go into loop mode
            rwButtonUp = 0;
        }
        if ((rwButton == 0) && !rwButtonUp){
            rwButtonUp = 1;
        }
        if ((modeButton == DEBOUNCE) && modeButtonUp){
            playMode++;
            if (playMode == 2)
                playMode = 0;
            modeButtonUp = 0;
        }
        if ((modeButton == 0) && !modeButtonUp){
            modeButtonUp = 1;
        }
}


void get_buttons(void){
    if ((!REC) && (recordButton < DEBOUNCE))
        recordButton++;
    if ((REC) && (recordButton > 0))
        recordButton--;

    if ((!FF) && (ffButton < DEBOUNCE))
        ffButton++;
    if ((FF) && (ffButton > 0))
        ffButton--;

    if ((!RW) && (rwButton < DEBOUNCE))
        rwButton++;
    if ((RW) && (rwButton > 0))
        rwButton--;
 
    if ((!MODE_BUTTON) && (modeButton < DEBOUNCE))
        modeButton++;
    if ((MODE_BUTTON) && (modeButton > 0))
        modeButton--;
}

