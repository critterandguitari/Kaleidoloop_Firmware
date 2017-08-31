NAME   = LPC2138_flash

CC      = arm-elf-gcc
LD      = arm-elf-ld -v
AR      = arm-elf-ar
AS      = arm-elf-as
CP      = arm-elf-objcopy
OD		= arm-elf-objdump

CFLAGS  = -I./ -c -fno-common -O0 -g -mcpu=arm7tdmi -nostartfiles -Wall
AFLAGS  = -ahls -mapcs-32 -o crt.o
LFLAGS  =  -Map main.map -T LPC2138_flash.cmd -L /home/owen/ARM/gnuarm-3.4.3/arm-elf/lib -L /home/owen/ARM/gnuarm-3.4.3/lib/gcc/arm-elf/3.4.3
CPFLAGS = -O ihex
ODFLAGS	= -x --syms

all: test

clean:
	-rm crt.lst *.o

test: main.out
	@ echo "...copying"
	$(CP) $(CPFLAGS) main.out main.hex
	$(OD) $(ODFLAGS) main.out > main.dmp

main.out: crt.o main.o system.o adc.o printf.o tlv320.o interface.o recorder.o simple_fat.o sd_raw.o LPC2138_flash.cmd 
	@ echo "..linking"
	$(LD) $(LFLAGS) -o main.out crt.o main.o adc.o tlv320.o system.o printf.o interface.o recorder.o simple_fat.o sd_raw.o -lc -lm -lgcc

crt.o: crt.s
	@ echo ".assembling"
	$(AS) $(AFLAGS) crt.s > crt.lst

# Compile: create object files from C source files.
%.o : %.c
	@echo
	@echo $(MSG_COMPILING) $<
	$(CC) -c $(CFLAGS) $< -o $@

