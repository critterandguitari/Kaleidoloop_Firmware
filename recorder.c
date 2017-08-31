/******************************************************************************/
/*  Long Recording                                                            */
/*  (c)2006-2009 GPL, Owen Osborn, Critter and Guitari, Dearraindrop          */
/******************************************************************************/
/*                                                                            */
/*  :  Sampler for the Critter Board                                          */
/*                                                                            */
/******************************************************************************/

#include "interface.h"
#include "sd_raw.h"
#include "system.h"
#include "LPC21xx.h"
#include "simple_fat.h"
#include "printf.h"

// in codec IC driver
extern unsigned int software_index;
extern unsigned int hardware_index;
extern unsigned int mode;
extern unsigned int buttonEventCounter;

// global
unsigned char sdBuf[512];      // data read out of SD card
char disk_full = 0;

static unsigned int writeTime = 0;

static unsigned int next = 0;
static unsigned int prev = 0;

static unsigned int ready = 0;
static unsigned int   phasor = 0;
static int recordButtonEvent;

static int sdBufOldLast = 0; 
static int sdBufOldFirst = 0; 
static char justGrabbedBuffer = 0; 
static char needNewBuffer = 1; 


// these have to be shorts, otherwise if int data read back from SD buffer looses sign
// really ??
static short sample_1 = 0; 
static short sample_0 = 0;   
static int sample = 0; 

static unsigned int sdInBufIndex = 0;    
static unsigned int diskBufIndex = 0; 

static int gainIn = 0;   
static int gain = 0; 
static unsigned int address = 0;

static unsigned int loopStart = 0;
static unsigned int loopEnd = 0;

static unsigned int direction = 1;
static unsigned int directionTarget = 1;
static unsigned int track = 0;
static unsigned int speed = 3;
static unsigned int play_all = 1;

// initialize recording
int recorder_init(void){

    printf(" total tracks: %d\r\n", fat_get_num_files()); 
    track = 0;

	loopStart = fat_get_file_start_address(0);
    address = loopStart;
	loopEnd = fat_get_file_end_address(0);
	mode = 0;
	needNewBuffer = 1;
    play_all = 1;  // start with playing all tracks
    return 1;
}

void recorder_set_play_all(unsigned int p) {
    play_all = p;
}

unsigned int recorder_current_track_length(void) {
    return loopEnd - loopStart;
}

unsigned int recorder_current_address(void) {
    return address; 
}

unsigned int recorder_current_loop_start(void) {
    return loopStart;
}

void recorder_toggle_mode(void){
    recordButtonEvent = 1;
}

void recorder_next(void){
    next = 1;
}

void recorder_prev(void){
    prev = 1;
}

void recorder_speed(unsigned int s){
    speed = s & 0x3ff;
}

void recorder_direction(unsigned int d){
    directionTarget = d;
}

void recorder_direction_toggle(void){
    directionTarget++;
    directionTarget &= 1;
}

// do play and record stuff
short recorder_perform(short input){

    if (mode) {             // REC
        PLED_OFF;
        ready = 0;   // only change mode at sector boundry
        sample = input;
        
        // ramp up gain
        if (gainIn < 1024) {
            gainIn++;
            sample *= gainIn;
            sample >>= 10;
        }
        
        sdBuf[sdInBufIndex] = sample & 0xff;
        sdInBufIndex++;
        sdBuf[sdInBufIndex] = sample >> 8;
        sdInBufIndex++;
        sdInBufIndex &= 0x1ff;

        if (!sdInBufIndex) {  
           
            // see if its time to stop (less then mega byte remain)
            if ((fat_get_total_space() - fat_get_used_space()) < 1000000) {
                // toggle to play mode 
                recordButtonEvent = 1;
                ready = 1;
            }
            // see if its near end of disk, 5 minutes
            else if ((fat_get_total_space() - fat_get_used_space()) < 13230000){
                // blink fast if so
                if (fat_get_used_space() & 0x1000) RLED_ON;
                else RLED_OFF;
            } 
            // 10 minutes
            else if ((fat_get_total_space() - fat_get_used_space()) < 26460000){
                // blink slower
                if (fat_get_used_space() & 0x4000) RLED_ON;
                else RLED_OFF;
            }
            // otherwise, always turn it on
            else RLED_ON;
            
            
            // write sector and time how long it takes
            writeTime = buttonEventCounter;                       
            //sd_raw_write(address + (sfs_audio_start * 512), sdBuf, 512);
            fat_write_file_data();
  	    RLED_OFF;

            // finish writing it, then allow mode change
            ready = 1;
        }
        //return input;  // for playthru
        sample = 0;
    }
    //play
    else {              

        ready = 1;  // change mode anytime
        // get a new buffer 1 sample early (254 not 255) since an s+1 is needed for interpolation
       	if (needNewBuffer){    // get a new sector if needed
            
            justGrabbedBuffer = 1;
            needNewBuffer = 0;

            // save the last value
            sdBufOldLast = sdBuf[511] << 8;
            sdBufOldLast |= sdBuf[510];
            
            // save the first value
            sdBufOldFirst = sdBuf[1] << 8;
            sdBufOldFirst |= sdBuf[0];

            // get a new one
            // flash red too , if disk is full (both lights on if disk full)
		    play_led_on(); if(disk_full) RLED_ON;
            sd_raw_read(address, sdBuf, 512);
		    play_led_off(); if(disk_full) RLED_OFF;

            if (direction){
	            address += 512;
                if (address > loopEnd){
                    address = loopStart;
                    // go to next track if play_all
                    if (play_all) next = 1;
                }  //forward
			}
		    else {
			    address -= 512;
			    if (address < loopStart){ 
                    address = loopEnd;
                    // go to previous track if play_all
                    if (play_all) prev = 1;
                }  // reverse
	        }   
        }

        // if Forward
        if (direction) {
            // if its the last sample in buffer, grab it from last buffer since buffer is already updated (in order to have s+1)
            if (diskBufIndex == 255) {
                sample_0 = sdBufOldLast; 
                sample_1 = sdBuf[1] << 8;
                sample_1 |= sdBuf[0];
            }
            else {
                sample_0 = sdBuf[(diskBufIndex * 2) + 1] << 8;
                sample_0 |= sdBuf[(diskBufIndex * 2)];
                sample_1 = sdBuf[((diskBufIndex + 1) * 2) + 1] << 8;
                sample_1 |= sdBuf[((diskBufIndex + 1) * 2)];
            }
            // do linear interpolation
            sample = sample_0 + ( ((phasor & 0xff) * (sample_1 - sample_0)) >> 8 );
        } else {
            // if its the first sample in buffer, grab it from last buffer since buffer is already updated (in order to have s+1)
            if (diskBufIndex == 255) {
                sample_1 = sdBufOldFirst; 
                sample_0 = sdBuf[511] << 8;
                sample_0 |= sdBuf[510];
            }
            else {
                sample_0 = sdBuf[(diskBufIndex * 2) + 1] << 8;
                sample_0 |= sdBuf[(diskBufIndex * 2)];
                sample_1 = sdBuf[((diskBufIndex + 1) * 2) + 1] << 8;
                sample_1 |= sdBuf[((diskBufIndex + 1) * 2)], 512;
            }
            // do linear interpolation
            sample = sample_0 + ( ((phasor & 0xff) * (sample_1 - sample_0)) >> 8 );
        }	    
        
        // advance phasor
        if (direction)
            phasor += speed;
		    else
			      phasor -= speed;  // reverse
 
        // see if new buffer is needed
       	if (((phasor >> 8) > 254) && !justGrabbedBuffer){
            needNewBuffer = 1;
        }
        // if we are in the middle of sector, its safe to say we can get a new buffer, also safe to change direction
        if (((phasor >> 8) > 100) && ((phasor >> 8) < 150)) {
            justGrabbedBuffer = 0;
            direction = directionTarget;
        }
        // phasor is 16 bits
        phasor &= 0xffff;
        // address within sector of disk
        diskBufIndex = (phasor >> 8) & 0xff; 
       
        PLED_OFF;              
    
    }  // end play
    
    // if at a sector boundry
    if (ready){
        // toggle mode
        if (recordButtonEvent && !disk_full){
            mode++;
            mode &= 0x1;
            recordButtonEvent = 0;  
            // if mode = 1 we get ready for recording
            if (mode) { 
                software_index = 0;
                hardware_index = 2;   // reset buffers, give the hardware buffer index a head start, since software index gets incremented in main this
                sdInBufIndex = 0; 
			    gainIn = 0;
            }
            else{ // end recording
                
                fat_write_file_entry();
                track = fat_get_num_files() - 1;
                loopStart = fat_get_file_start_address(fat_get_num_files() - 1) + 512;  // first 512 for wav header and dummy bytes  
                loopEnd = fat_get_file_end_address(fat_get_num_files() - 1) - 512;  //  the looper actually plays 1 sector past the loop end

                //printf("s: %d,   e: %d,   total tracks: %d\r\n", loopStart, loopEnd, track);
                
                // mark disk full, disabling any more recording
                if (((fat_get_total_space() - fat_get_used_space()) < 1000000) || (fat_get_num_files() > 2000)) {
                    disk_full = 1;
                    PLED_ON;RLED_ON;
                }

                needNewBuffer = 1;
			    address = loopStart;
                software_index = 0;
                hardware_index = 2;   // reset buffers, hardware buffer a head start
			    gain = 1024;
            }
        }
    }
    // ff
    if (next){
        // only do this if playing
        if (!mode){
            next = 0;
            track++;
            if (track == fat_get_num_files())
                track = 0;

            // if there are more then 1 track (first track is dummy track), skip the dummy track 
            if (fat_get_num_files() > 1) {
                if (track == 0)
                    track = 1;
            }


            loopStart = fat_get_file_start_address(track) + 512;  // first 512 for wav header and dummy bytes  
            loopEnd = fat_get_file_end_address(track) - 512;  // the looper actually plays 1 sector past the loop end
            address = loopStart;
            needNewBuffer = 1;
            printf("p: %d, s: %d e: %d\r\n", track, loopStart, loopEnd);  
        }
    }
    // rw
    if (prev){
        // only do this if playing
        if (!mode){
            prev = 0;
            if (track == 0)
                track = fat_get_num_files() - 1;
            else 
                track--;

            // if there are more then 1 track (first track is dummy track), skip the dummy track 
            if (fat_get_num_files() > 1) {
                if (track == 0)
                    track = fat_get_num_files() - 1;
            }

            loopStart = fat_get_file_start_address(track) + 512;  // first 512 for wav header and dummy bytes  
            loopEnd = fat_get_file_end_address(track) - 512;  // for some reason , this returns 1 sector past last of file ?
            
            // if play_all, set address to loopEnd (because we are playing in reverse)
            if (play_all) address = loopEnd;
            else address = loopStart;
            
            needNewBuffer = 1;
            printf("p: %d\r\n", track);        
        }
    }
    return sample;
}

