/******************************************************************************/
/*  Flash Sampler Tutorial                                                    */
/*  (c)2006 GPL, Owen Osborn, Critter and Guitari, Dearraindrop               */
/******************************************************************************/
/*                                                                            */
/*  :  Sampler for the Critter Board with on-board flash or flash cards       */
/*                                                                            */
/******************************************************************************/

#define RED_BUTTON      !(IOPIN0 & (1 << 4))
#define SWITCH_UP       !(IOPIN0 & (1 << 3))
#define SWITCH_DOWN     !(IOPIN0 & (1 << 5))

#define L0 {IOSET0 |= ((1 << 12)|(1<<11)|(1<<10)|(1<<7)|(1<<8)|(1<<9)); }
#define L1 {IOCLR0 |= ((1 << 12)); }
#define L2 {IOCLR0 |= ((1 << 12)|(1<<11)); }
#define L3 {IOCLR0 |= ((1 << 12)|(1<<11)|(1<<10)); }
#define L4 {IOCLR0 |= ((1 << 12)|(1<<11)|(1<<10)|(1<<7)); }
#define L5 {IOCLR0 |= ((1 << 12)|(1<<11)|(1<<10)|(1<<7)|(1<<8)); }
#define L6 {IOCLR0 |= ((1 << 12)|(1<<11)|(1<<10)|(1<<7)|(1<<8)|(1<<9)); }

// size of sfs track listing
#define SFS_OFFSET 2048

/// LED error codes
#define NO_CARD_ERROR 0
#define NO_FAT_ERROR 1
#define NO_SFS_ERROR 2

// initialization
void Initialize(void);
void feed(void);

// serial stuff
void put_char(char c);
char get_char(void);
 
// timing stuff
void delay_ms(unsigned int ms);
void delay_ticks(unsigned int count);

// led stuff
void led_board_init();
void led_card_init();
inline void led_board(int stat);
inline void led_card(int stat);

// isr stuff:
// stock interrupt stubs
void IRQ_Routine (void) ;
void FIQ_Routine (void) ;
void SWI_Routine (void);
void UNDEF_Routine (void) ;

unsigned enableIRQ(void);
static inline unsigned __get_cpsr(void);
static inline void __set_cpsr(unsigned val);
unsigned disableIRQ(void);


#define IRQ_MASK 0x00000080
#define FIQ_MASK 0x00000040
#define INT_MASK (IRQ_MASK | FIQ_MASK)

#define ISR_ENTRY() asm volatile(" sub   lr, lr,#4\n" \
                                 " stmfd sp!,{r0-r12,lr}\n" \
                                 " mrs   r1, spsr\n" \
                                 " stmfd sp!,{r1}")
                                 
#define ISR_EXIT()  asm volatile(" ldmfd sp!,{r1}\n" \
                                 " msr   spsr_c,r1\n" \
                                 " ldmfd sp!,{r0-r12,pc}^")
