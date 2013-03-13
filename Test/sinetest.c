#define F_CPU 8000000UL
#define USART_BAUDRATE 9600

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "macros.h"
#include "sinetest.h"

int x;

void setShift(int value) 
{
	// Set shift register low, then pulse in 4 bits
	for(x = 0; x < 8; x++)
	{
	  int bit = (value >> x) & 1;
	  bit_write(bit, SR_A_PORT, SR_A_PIN);
	  _delay_ms(1);	
	  bit_set(SR_CLK_PORT, SR_CLK_PIN);
	  _delay_ms(1);
	  bit_clear(SR_CLK_PORT, SR_CLK_PIN);
	}     
}

void setup()
{
	// Set shift register pins to output
	bit_set(SR_CLK_DDR, SR_CLK_PIN);
	bit_set(SR_CLK_DDR, SR_CLR_PIN);
	bit_set(SR_CLK_DDR, SR_A_PIN);

	// Set Column 5 to output and set it low
	bit_set(SR_CLK_DDR, LED_C5_PIN);
	bit_clear(SR_CLK_PORT, LED_C5_PIN);

	// Pulse shift register's !CLR pin high then low to clear the SR
	bit_clear(SR_CLR_PORT, SR_CLR_PIN);
	_delay_ms(1);	
	bit_set(SR_CLR_PORT, SR_CLR_PIN);
	_delay_ms(1);

	
}

void loop()
{
	_delay_ms(200);

	setShift(0xF0);

}

/**
 * Main to allow arduino-style setup and main functions
 */
int main(void)
{
    setup();
    while (1)
    {
        loop();
    }
}
