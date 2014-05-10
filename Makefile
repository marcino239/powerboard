controller.elf:  controller.c
	msp430-gcc -Os -mmcu=msp430g2231 -o controller.elf controller.c

prog:	controller.elf
	mspdebug rf2500 "prog controller.elf"

