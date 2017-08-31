/******************************************************************************/
/*  Long Recording                                                            */
/*  (c)2006 GPL, Owen Osborn, Critter and Guitari, Dearraindrop               */
/******************************************************************************/
/*                                                                            */
/*  :  Sampler for the Critter Board                                          */
/*                                                                            */
/******************************************************************************/


#define TLV_MOSI        (1<<8)
#define TLV_CS          (1<<13) 
#define TLV_SCK         (1<<7)

// registers

#define TLV_LIV     0x00    // left line in  volume
#define TLV_RIV     0x01    // right line in volume
#define TLV_LHP     0x02    // left headphone 
#define TLV_RHP     0x03    // right headphone
#define TLV_APC     0x04    // analog path control
#define TLV_DAPC    0x05    // digital audio path control
#define TLV_PWRCTL  0x06    // power ctrl
#define TLV_DAFMT   0x07    // digital audio format
#define TLV_SRCTL   0x08    // sample rate control
#define TLV_DIA     0x09    // digital audio activation
#define TLV_RST     0x0F    // register reset


void tlv_spi_init(void);
void tlv_init(void);

void tlv_spi_send_command(unsigned int reg, unsigned int data);


