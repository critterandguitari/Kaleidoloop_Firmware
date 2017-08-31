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
#include "tlv320.h"

static void sspISR(void) __attribute__ ((interrupt ("IRQ")));

// Global variables used in main program
unsigned int buttonEvent = 0;
unsigned int buttonEventCounter = 0;
unsigned int software_index = 0;
unsigned int hardware_index = 0;
short playBuf[4096];
short recBuf[4096];

unsigned int mode = 0;

// holds data for codec
static int tlv_data[] = {0, 0, 0, 0};

void tlv_init(void){

    PINSEL1 |= ((1<<3) | (1<<5) | (1<<7) | (1<<9));  // ssp 
     
    // PWM used to generate clock for codec
    PINSEL0 |= (1<<19);      // P0.9 is PWM5
    PWMTCR = 0x02;              // reset PWM counter
    PWMPR = 0;                  // reset prescaler
    //PWMMR0 = 5;                 //  about 46 Khz
    PWMMR0 = 10;                 //  about 23 Khz
    //PWMMR6 = 2;                 // about 46 Khz
    PWMMR6 = 4;                 // about 46 Khz
    PWMMCR = 0x00000002;        // reset MR0 on match
    PWMPCR = 0x4000;            // enable PWM6 
    PWMTCR = 0x09;              // enable PWM
    
    delay_ms(5);           
    tlv_spi_init();
    
    delay_ms(1); 

    tlv_spi_send_command(TLV_RST, 0);
    tlv_spi_send_command(TLV_PWRCTL, 0);        // everything on
    tlv_spi_send_command(TLV_APC, 0x015);       // mic input, 20db boost
    //tlv_spi_send_command(TLV_APC, 0x014);       // mic input, no boost
    tlv_spi_send_command(TLV_SRCTL, 0x000);     // 44.1 khz, 
    tlv_spi_send_command(TLV_DAFMT, 0x001);     // slave, left justified
    tlv_spi_send_command(TLV_DAPC , 0);         // mute off
    tlv_spi_send_command(TLV_DIA, 1);           // active
    
    delay_ms(1); 
    
    // setup for timer 1
    PINSEL0 &= 0xfccfffff;  // clear bits 21,20 and 25,24
    PINSEL0 |= 0x02200000;  // select funcion 2 CAP1.0 for P0.10 and MAT1.0 for P0.12
    T1TCR = 0x02;   // reset timer 1
    T1PR = 0;
    // VERY STRANGE ::::: ON SOME DEVICES THE BELOW LINE SHOULD BE 2, ON OTHERS 3, SKRATCHY RECORDING IF SET WRONG
    T1CTCR = 0x02;  // counter mode
    
    T1MR0 = 1;
    T1MCR = 0x0002; // was 2 reset on MR0
    T1EMR = 0x0030; // togle MAT1.0 on match
    T1TCR = 0x01;   // start timer 1
   
    // setup arm side ssi  
    SSPCR0 = 0x1f;
    // SSPCPSR = 40;               // 46 khz
    SSPCPSR = 80;               // 23 khz
    SSPIMSC = 0x08; // enable TXIM
    VICIntSelect &= ~0x800;   // SSP selected as IRQ (bit 11)
    VICIntEnable = 0x800;     // enable SSP interrupt
    //VICIntEnClr = 0x800;    // disable SSP interrupt
    VICVectCntl4 = 0x2b;      // Use slot 4 for ssp interrupt 
    VICVectAddr4 = (unsigned int)sspISR; // address of the ISR
    //VICIntEnClr = 0x800;    // disable SSP interrupt
    SSPICR = 0x03;  // reset INT errors
    
    SSPDR = 0xaaaa;
    SSPDR = 0xaaaa;
    SSPDR = 0xaaaa;
    SSPDR = 0xaaaa;
    SSPDR = 0xaaaa;
    SSPDR = 0xaaaa;
    SSPDR = 0xaaaa;
    SSPDR = 0xaaaa;
    
    SSPCR1 |= (1 << 1); // enable
}



void tlv_spi_init(void){
    IODIR0 |= (TLV_MOSI | TLV_CS | TLV_SCK);
    IOSET0 |= TLV_CS;
    IOSET0 |= TLV_SCK;
}


void tlv_spi_send_command(unsigned int address, unsigned int data){
    
    unsigned int i = 0;
    unsigned int spi_data = 0;
    
    spi_data = address & 0x7f;
    spi_data <<= 9;
    spi_data |= (data & 0x1ff);
    
    // cs low
    IOCLR0 |= TLV_CS;
    delay_ticks(60);  
    IOCLR0 |= TLV_SCK;
    

    
    // do it
    for (i = 0; i < 16; i ++){
        
        if (spi_data & (1 << (15 - i)) )
            IOSET0 |= TLV_MOSI;
        else
            IOCLR0 |= TLV_MOSI;
 
        delay_ticks(60);           
        IOSET0 |= TLV_SCK;
        delay_ticks(60);
        IOCLR0 |= TLV_SCK;
  
    
    }
        delay_ticks(60);
    // cs high
    IOSET0 |= TLV_CS;
    IOSET0 |= TLV_SCK;
    delay_ticks(160);
}

static void sspISR(void){
    unsigned char c;
    c = SSPMIS;
    SSPMIS = 0x1f;  // try
    SSPICR = 0x03;  // reset INT errors
    
    if (!(buttonEventCounter & 0x3f)){
        buttonEvent = 1;
    }
    buttonEventCounter++;


    // check to see if record or play
    if (mode){ // record
        tlv_data[0] = SSPDR;
        tlv_data[1] = SSPDR;
        tlv_data[2] = SSPDR;
        tlv_data[3] = SSPDR;
    
        // frame 1
        recBuf[hardware_index] = tlv_data[0];
        hardware_index++;
        // and 2
        recBuf[hardware_index] = tlv_data[2];
        hardware_index++;
        hardware_index &= 0xfff;

        SSPDR = 0;//tlv_data[0];
        SSPDR = 0;
        SSPDR = 0;//tlv_data[2];
        SSPDR = 0;
    }
  
    else {  // play
    
        tlv_data[0] = playBuf[hardware_index];
        hardware_index++;
        tlv_data[2] = playBuf[hardware_index];
        hardware_index++;
        hardware_index &= 0xfff;

        SSPDR = 0;//tlv_data[0];
        SSPDR = tlv_data[0];
        SSPDR = 0;//tlv_data[2];
        SSPDR = tlv_data[2];  // end play
    }


  /*  doing both
    // record
    tlv_data[0] = SSPDR;
    tlv_data[1] = SSPDR;
    tlv_data[2] = SSPDR;
    tlv_data[3] = SSPDR;

    // frame 1
    recBuf[hardware_index] = tlv_data[0];
    // and 2
    recBuf[hardware_index + 1] = tlv_data[2];

    tlv_data[0] = playBuf[hardware_index];
    tlv_data[2] = playBuf[hardware_index + 1];
    hardware_index += 2;
    hardware_index &= 0x7ff;

    SSPDR = 0;//tlv_data[0];
    SSPDR = tlv_data[0];
    SSPDR = 0;//tlv_data[2];
    SSPDR = tlv_data[2];  // end play
    */
    VICVectAddr = 0;  // Update VIC priorities 
}
