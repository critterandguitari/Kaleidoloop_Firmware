/******************************************************************************/
/*  Long Recording                                                            */
/*  (c)2006 GPL, Owen Osborn, Critter and Guitari, Dearraindrop               */
/******************************************************************************/
/*                                                                            */
/*  :  Sampler for the Critter Board                                          */
/*                                                                            */
/******************************************************************************/

#include "LPC21xx.h"
#include "system.h"
#include "tlv320.h"
#include "recorder.h"
#include "interface.h"
#include "adc.h"
#include "simple_fat.h"
#include "printf.h"
#include "sd_raw.h"
#include <math.h>
#include "waves.h"


extern unsigned int software_index;
extern unsigned int hardware_index;
extern short playBuf[];
extern short recBuf[];
extern unsigned int mode;
extern unsigned int audio_data_max_space;

extern unsigned char sdBuf[];

extern char disk_full;

unsigned short sTest1;
unsigned short sTest2;

unsigned int playMode = 0;

// in sound codec IC driver (ssp interrupt) this counts up
extern unsigned int buttonEventCounter;

// the behaviors
static void play_fwd(void);
static void play_fwd_stepped(void);
static void play_rev(void);
static void play_rev_stepped(void);
static void play_warble(void);
static void play_warble_chop(void);
static void play_throw(void);
static void play_randomizer_1(void);
static void play_fwd_rev_stepped(void);
static void play_fwd_rev_continuous(void);
static void play_stutter(unsigned int current_time);
static void play_drunken_line(unsigned int t);

unsigned int knob = 0;


int main (void) {

    int i = 0;

    unsigned int current_time = 0;
    unsigned int last_time = 0;
 
    unsigned int used_space = 0;
    unsigned int quater_size = 0;

    // Initialize the MCU
    Initialize();

    // test mode led
//    IODIR1 |= ((1<<27) | (1<<28) | (1<<29));
    
    led_board(0);
    delay_ms(50);              // flash LEDs
    led_board(1);
    delay_ms(50);
    led_board(0);
    delay_ms(50); 
    led_board(0);  

    // for the mode LED
    IODIR1 |= ((1<<27) | (1<<28) | (1<<29));

    play_mode_led(playMode);

    IODIR0 |=  (1 << 27);
    PINSEL1 |= ((1 << 22));  // p0.27 adc0.0  

    PLED_INIT;
    RLED_INIT;
    RLED_OFF;
    PLED_OFF;
    delay_ms(100);  

    fat_locate_boot_sector();
  	fat_get_partition_info();
   
    // if all three buttons pressed
    if ( (!REC) && (!FF) && (!RW)) {
        // and pressed for 1second
        delay_ms(1000);
        if ( (!REC) && (!FF) && (!RW)) {
            RLED_ON;PLED_ON;delay_ms(100);RLED_OFF;PLED_OFF;delay_ms(100);
            RLED_ON;PLED_ON;delay_ms(100);RLED_OFF;PLED_OFF;delay_ms(100);
            RLED_ON;PLED_ON;delay_ms(100);RLED_OFF;PLED_OFF;delay_ms(100);
            
            // initialize the disk 
            fat_initialize_disk();
            fat_make_sounds_directory();
            
            RLED_ON;PLED_ON;delay_ms(100);RLED_OFF;PLED_OFF;delay_ms(100);
            RLED_ON;PLED_ON;delay_ms(100);RLED_OFF;PLED_OFF;delay_ms(100);
            RLED_ON;PLED_ON;delay_ms(100);RLED_OFF;PLED_OFF;delay_ms(100);
        }
    }

    fat_check_for_sounds_directory();
    fat_clean_root_directory();
 
    // this will create a file if disk was just erased...
    printf("%d files found \r\n", fat_count_files());

    // list files
    for (i = 0; i < fat_get_num_files(); i++){
        printf("file %d start: %d end %d \r\n", i, fat_get_file_start_address(i), fat_get_file_end_address(i));
    }
   
    recorder_init();

    printf(" total space: %d\r\n", fat_get_total_space());
    printf(" used space: %d\r\n", fat_get_used_space() ); 
 
    quater_size = fat_get_total_space() / 4;
    used_space = fat_get_used_space();

    // if full, or even near the end, (it auto stops at 1000000)
    if (((fat_get_total_space() - used_space) < 1000000) || (fat_get_total_space() < used_space)) {
        RLED_ON; PLED_ON;
        disk_full = 1;
    }
    
    // flash depending how full it is
    if (used_space > (quater_size * 3))
        fill_flash(4);
    else if (used_space > (quater_size * 2))
        fill_flash(3);
    else if (used_space > quater_size)
        fill_flash(2);
    else 
        fill_flash(1);
  
    // INITIALIZE TLV CODEC
    tlv_init();
    // enable interrupt
    enableIRQ();   

    for (;;){  
        
        // check buttons, etc ...
        interface_check_for_events();

        // do other stuff, but only during play mode
        if (!mode){   // dispatcher to behavirs
            current_time = buttonEventCounter;
            
            if (current_time > (last_time + 50)) {
                
                knob = get_adc0(0);
               // printf("x: %d\r\n", i);
                // dispatch  behaviors
                if (playMode == 0)      play_fwd_rev_stepped();
                else if (playMode == 1) play_fwd_rev_continuous();
                else if (playMode == 2) play_warble();
                else if (playMode == 3) play_warble_chop();
                else if (playMode == 4) play_stutter(current_time);
                else if (playMode == 5) play_rev();
                
                last_time = current_time;
            } 
        } // if playing do this 
        
        // maintain audio
        if (software_index != hardware_index){

            playBuf[software_index] = recorder_perform(recBuf[software_index]);            
            software_index++;
            software_index &= 0xfff;

        }
        // printf("%d\n\r", get_adc0(0));
	  }// forr 
} // main()

void play_fwd_rev_stepped(void) {
    if (knob > 512)
        recorder_direction(1);
    else
        recorder_direction(0);

    knob &= 0x3e0;
    recorder_speed(abs(1024 - (knob * 2)) & 0x3ff);
}

void play_fwd_rev_continuous(void) {
    if (knob > 512)
        recorder_direction(1);
    else
        recorder_direction(0);

    recorder_speed(abs(1024 - (knob * 2)) & 0x3ff);
}

void play_stutter(unsigned int current_time){

    static unsigned int count = 0;
    static unsigned int next_time = 0;
    
    recorder_speed(256);
    
    if (current_time > next_time){
        count = knob;
        next_time = current_time + 8184;
        printf("%d,\r\n", count);
        /*count++;
        if (count == 4) {
            count = 0;
            recorder_direction(0);
        }
        else {
            recorder_direction(1);
        }*/
    }

    if ((next_time - current_time) > (count * 8)) recorder_direction(0);
    else recorder_direction(1);
}

void play_warble(void){

    static unsigned int phase;
    static unsigned int phase_delta;

    unsigned int depth;
    int rate;

    depth = ((knob >> 8) + 1); // 4 depth settings

    phase_delta = (knob & 0xff) * 4;
    phase += phase_delta;

    //TODO:  make only 3 zones
    if (depth == 1) {
        rate = ((sin_table[(phase >> 8) & 0xff]) >> 3) + 240;
    }
    else if (depth == 2) {
        rate = ((sin_table[(phase >> 8) & 0xff]) >> 2) + 224;
    }
    else if (depth == 3) {
        rate = ((sin_table[(phase >> 8) & 0xff]) >> 1) + 192;
    }
    else if (depth == 4) {
        rate = (sin_table[(phase >> 8) & 0xff]) + 128;
    }
    
    recorder_speed(rate);
}

void play_warble_chop(void){

    static unsigned int phase;
    static unsigned int phase_delta;

    unsigned int depth;
    int rate;

    depth = ((knob >> 8) + 1); // 4 depth settings

    phase_delta = (knob & 0xff) * 32;
    phase += phase_delta;
    //TODO:  make only 3 zones
    if (depth == 1) {
        rate = ((square_table[(phase >> 8) & 0xff]) >> 3) + 240;
    }
    else if (depth == 2) {
        rate = ((square_table[(phase >> 8) & 0xff]) >> 2) + 224;
    }
    else if (depth == 3) {
        rate = ((square_table[(phase >> 8) & 0xff]) >> 1) + 192;
    }
    else if (depth == 4) {
        rate = (square_table[(phase >> 8) & 0xff]) + 128;
    }
    
    recorder_speed(rate);
}


// testers  // 
void play_drunken_line(unsigned int current_time){
    static unsigned int next_time = 0;
    static unsigned int speed = 0x3ffff;
    unsigned int delta = 0;
    static int slope;
    static unsigned int count = 0;

    count++;

    if (current_time > next_time){

        delta = rand() & 0x3fff;
        next_time = current_time + delta;
        slope = (128 - (rand() & 0xff)) * knob;
    }
    
        speed += (slope >> 6);
        printf("%d, %d\r\n", slope, ((speed >> 4) & 0x3ff));
      // printf("%d\r\n", speed);
         recorder_speed(speed >> 6);
}

void play_rev(void){ 
    recorder_direction(0);
    recorder_speed(knob & 0x3ff);
}

void play_rev_stepped(void) {

}

void play_fwd(void){
    recorder_direction(1);
    recorder_speed(knob & 0x3ff);
}

void play_fwd_stepped(void){

}

void play_throw(void) {

    static int knob_last;
    int knob_prime;

    knob_prime = knob - knob_last;
    knob_last = knob;

    recorder_speed(256 + knob_prime);

}


void play_randomizer_1(void) {

    unsigned int p;
    unsigned int ll;
    unsigned int ls;
    unsigned int a;
    unsigned int qll;
    unsigned int zone;
    static unsigned int zone_last;

    ls = recorder_current_loop_start();
    ll = recorder_current_track_length();
    a = recorder_current_address();
    qll = ll / 4;

    if (a < (ls + qll)) {
        zone = 1;
    }
    else if (a < (ls + (qll * 2))) {
        zone = 2;
    }
    else if (a < (ls + (qll * 3))) {
        zone = 3;
    }
    else  {
        zone = 4;
    }

    recorder_speed(256);
    
    if (zone != zone_last){
        printf("%d\n\r", zone);
        zone_last = zone;
        p = rand() & 0x3ff;
        if (knob > p) {
            recorder_direction_toggle();        
        }
    }
}


