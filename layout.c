





       ---1024------>
 ARM                   AUDIO   
       <--1024-------

 
   two notifications happen:  when there is data in the input buffer needing to be read, and space in the output buffer needing to be filled.
   
   i don't think two notifications is necessary because data in and data out are synchronous and both buffers share index
   
   they share both indecies really. one index incremented by the hardware, one index catches up in software.

   so there only needs to be 1 audio maintain function:
   
   audio_buffers_status();  // audio buffers need / have data 
   
      
    so like
 
audio_buffers_status() {
    
    int sample;
    
    sample = audio_input_buf[soft_index];   // get the input
    
    sample = ring_mod(sample);
    
    sample = phaser(sample);
    
    audio_output_buf[soft_index] = sample;   // put it backh  
}


    
// main program will have these functions:    
  if input_buffer_has_data()
    
    audio_in_process();
    
  
 if output_buffer_needs_data()
    
      audio_out_process();


// also these functions to shut off streams and stuff
audio_out_stop();
audio_in_stop();
audio_out_reset();
audio_in_reset();
  
 // the process functions is where the signal chain is setup (1 in 1 out)
 
audio_out_process() {
  
   int sample;
   
   
   out_buf[out_buf_index] = sample;

} 
 
 
 void audio_out_process() {
     unsigned int *stream = get_output_buf_pointer();  // or just use array access
      int sample;
   while (output_read_pointer() != output_write_pointer()){
   
    sample = sampler_process_output();
    sample = some_other_effect_process_output(sample);
 
   }
 }