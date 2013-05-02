#define F_CPU 8000000UL
#define USART_BAUDRATE 9600

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "macros.h"
#include "sinetest.h"

void setShift(int value) 
{
	int x;
	// Shift in 8 bits, LSB first
	for(x = 0; x < 8; x++) {
		int bit = (value >> x) & 1;
		bit_write(bit, SR_A_PORT, SR_A_PIN);
		bit_set(SR_CLK_PORT, SR_CLK_PIN);
		bit_clear(SR_CLK_PORT, SR_CLK_PIN);
	}     
}

void configureLEDs(int columns, int rows) 

{
#ifdef OLDBADGE
        setShift(((rows & 0xF) << 4) + (0xF ^ (columns & 0xF)));
#else
        // Invert columns data
        columns = (columns & 8) >> 3 | (columns & 4) >> 1 | ( columns & 2) << 1 | (columns & 1) << 3 | (columns & 0x10);
        int shiftData = (0xF ^ (columns & 0xF))<<2;
	shiftData |= (rows & 1) << 6;
	shiftData |= (rows & 2) << 6;
	shiftData |= (rows & 4) >> 2;
	shiftData |= (rows & 8) >> 2;
	setShift(shiftData);
#endif

        if(columns & 0x10) {
          bit_clear(LED_C5_PORT,LED_C5_PIN);
        } else {
          bit_set(LED_C5_PORT,LED_C5_PIN);
        }
}
