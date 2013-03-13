#define F_CPU 1000000UL
#define USART_BAUDRATE 9600

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "macros.h"
#include "sinetest.h"

volatile int column;
volatile int row;

ISR(PCINT0_vect)
{
	column = 0;
	row = 0;
}

void setShift(int value) 
{
	int x;
	// Shift in 8 bits, LSB first
	for(x = 0; x < 8; x++) {
		int bit = (value >> x) & 1;
		bit_write(bit, SR_A_PORT, SR_A_PIN);
		_delay_us(10);	
		bit_set(SR_CLK_PORT, SR_CLK_PIN);
		_delay_us(10);
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

	column = 0;
	row = 0;
	
	bit_clear(MOSI_DDR, MOSI_PIN); // Make sure MOSI is an input

	// Configure interrupts
	PCMSK0 |= (1<<PCINT6); // Interrupt on pin 6 change (MOSI)
	GIMSK |= (1<<PCIE0); // Enable interrupts on PCINT7:0
	sei();  // Enable global interrupts.

	// Configure IR_OUT pwm
	// Make MISO an output and drive it low
	bit_set(MISO_DDR, MISO_PIN); 
	  bit_clear(MISO_PORT, MISO_PIN);

	// Make IR_PWM an output
	bit_set(IR_PWM_DDR, IR_PWM_PIN);

	TCNT0 = 0; // Initial clock
	int period = 13; // Gets about 37.5kHz
	OCR0B = period; // OC0B compare register
	OCR0A = period; // Same value
	
	TCCR0A |= (1<<COM0B0) | (1<<WGM01); // Toggle OC0B on compare match, CTC mode
	TCCR0B = 1; // start clock (/1 prescaler)
	// We run at 1000000Hz
	// OCR0A = 255 So we will toggle at 1000/255 kHz 
	// We want 1000/X == 76khz 
	// So 
}

void loop()
{
	_delay_ms(200);
	row++;

	if(row % 4 == 0) {
		column += 1;
	}
	
	int columns = 1<<(column % 5);
	int rows = 1<<(row % 4);

	setShift(((rows & 0xF) << 4) + (0xF ^ (columns & 0xF)));
	if(columns & 0x10) {
		bit_clear(LED_C5_PORT,LED_C5_PIN);
	} else {
		bit_set(LED_C5_PORT,LED_C5_PIN);
	}
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
