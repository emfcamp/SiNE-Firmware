#define F_CPU 8000000UL
#define USART_BAUDRATE 9600

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "macros.h"
#include "sinetest.h"

volatile int column;
volatile int row;

char* colData;
volatile int IRcode = 0;
int latchIR = 0;
char currentBit = 1;
unsigned int oldTime = 0;
char interbit = 0;
volatile int seq = 0;
const int tolerance = 200;
int receives = 0;
int elapsed = 0;
int collection = 0;
int IRseq = 0;
	int columns = 0x0001;
	int rows = 0x0001;

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

	// Configure IR_OUT pwm
	// Make MISO an output and drive it high
	bit_set(MISO_DDR, MISO_PIN); 
	bit_clear(MISO_PORT, MISO_PIN);

	// Make IR_PWM an output
	bit_set(IR_PWM_DDR, IR_PWM_PIN);

	// We run at 8MHz
	// We want period to be half the wavelength
	// We want 38kHz = 1/38000 seconds / cycle
	// = 1/76000 seconds/half cycle
	// = 8000000/76000

	TCNT0 = 0; // Initial clock
	int period = 106; // Gets about 38kHz
	OCR0B = period; // OC0B compare register
	OCR0A = period; // Same value
	
	TCCR0A |= (1<<COM0B0) | (1<<WGM01); // Toggle OC0B on compare match, CTC mode
	TCCR0B = 1; // start clock (/1 prescaler)

	bit_set(BTN1_PORT, BTN1_PIN);
	bit_set(BTN2_PORT, BTN2_PIN);
}

void transmit1()
{
	bit_set(MISO_PORT, MISO_PIN);
	_delay_us(1200);
	bit_clear(MISO_PORT, MISO_PIN); 
	_delay_us(600);
}

void transmit0()
{
	bit_set(MISO_PORT, MISO_PIN);
	_delay_us(600);
	bit_clear(MISO_PORT, MISO_PIN); 
	_delay_us(600);
}

void transmit_start()
{
	bit_set(MISO_PORT, MISO_PIN);
	_delay_us(600*4);
	bit_clear(MISO_PORT, MISO_PIN); 
	_delay_us(600);
}

void loop()
{
	if(!bit_get(BTN1_PORT, BTN1_PIN)) {
		IRseq = 0b10000001;
		rows = 0b10;
	}

	if(!bit_get(BTN2_PORT, BTN2_PIN)) {
		IRseq = 0b10000010;
		rows = 0b100;
	}

	// Transmit sequence

	bit_clear(MISO_PORT, MISO_PIN);
	_delay_us(1000);
        transmit_start();
	int bit;
	for(bit=0;bit<12;bit++) {
		int b = (IRseq>>bit) & 1;
		if(b==0) {
			transmit0();
		}
		else
		{
			transmit1();
		}
	}
	bit_clear(MISO_PORT, MISO_PIN);
	setShift(((rows & 0xF) << 4) + (0xF ^ (columns & 0xF)));
	
	_delay_ms(1000);
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