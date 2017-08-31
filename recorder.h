/******************************************************************************/
/*  Long Recording                                                            */
/*  (c)2006-2009 GPL, Owen Osborn, Critter and Guitari, Dearraindrop          */
/******************************************************************************/
/*                                                                            */
/*  :  Sampler for the Critter Board                                          */
/*                                                                            */
/******************************************************************************/


int recorder_init(void);

short recorder_perform(short input);

void recorder_toggle_mode(void);

void recorder_next(void);

void recorder_prev(void);

void recorder_speed(unsigned int s);

void recorder_direction(unsigned int d);

void recorder_direction_toggle(void);

unsigned int recorder_current_track_length(void);

unsigned int recorder_current_address(void);

unsigned int recorder_current_loop_start(void);

void recorder_set_play_all(unsigned int p);
