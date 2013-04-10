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

char a[4] = { 0b10001, 0b11111, 0b01010, 0b00100 };
char b[4] = { 0b11111, 0b10001, 0b11110, 0b10000 };

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

ISR(PCINT0_vect)
{
	// Check the clock and figure out the time since the last change
	// If it's "long", we're on the start bit, and the current bit is 1, emit 1.
	// If it's about 889usec and interbit is set, clear interbit.
	// If it's about 889usec, emit current, set the interbit flag.
	// If it's longer, about 1778usec, current=!current and emit current.

	unsigned int time = TCNT1;
	if(time < oldTime) {
		unsigned int remain = 0xFFFF - oldTime;
		elapsed += time + remain;
	}
	else {
		elapsed += time - oldTime;
	}
	oldTime = time;

	if(elapsed < 889+tolerance && seq < 14) {
		if(interbit) {
			interbit = 0;
		} else {
			IRcode <<= 1;
			IRcode |= currentBit;
			interbit = 1;
			seq ++;
			if(seq == 14) {
				latchIR = IRcode;
			}
		}
		elapsed = 0;
	}
	else if(elapsed > 1778-tolerance && elapsed < 1778+tolerance && seq < 14) {
		currentBit = 1-currentBit;
		IRcode <<= 1;
		IRcode |= currentBit;
		seq ++;
		elapsed = 0;
		if(seq == 14) {
			latchIR = IRcode;
		}

	}
	else if(elapsed > 2000) {
		interbit = 0;
		IRcode = 1;
		currentBit = 1;
		seq=0;
		elapsed = 0;
	}
}

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

	// Configure interrupts
	PCMSK0 |= (1<<PCINT6); // Interrupt on pin 6 change (MOSI)
	GIMSK |= (1<<PCIE0); // Enable interrupts on PCINT7:0
	sei();  // Enable global interrupts.

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
	colData = a;


	bit_set(BTN1_PORT, BTN1_PIN);
	bit_set(BTN2_PORT, BTN2_PIN);

	TCNT1H = 0; // Initial TIMER1
	TCNT1L = 0; // Initial TIMER1
	// Set up timer1 as a general interval timer.
	// Ideally we should set up timer1 so it latches at TOP
	TCCR1B = 2; // Start clock (/8 prescaler = 1Mhz)
}

void loop()
{

	if(!bit_get(BTN1_PORT, BTN1_PIN)) {
		colData = a;
	}

	if(!bit_get(BTN2_PORT, BTN2_PIN)) {
		colData = b;
	}

	int command = (latchIR & 0x3F);
	int address = (latchIR >> 5) & 0x1f;

	colData[2] = command & 0x1F; 
	colData[3] = address;

	colData[0] = 1 << (seq % 5);
	colData[1] = 0x1;
	//bit_flip(MISO_PORT, MISO_PIN); // Uncomment this to flash the IR Led
	for(row=0;row<4;row++) {
		int columns = colData[row];
		int rows = 1<<(row % 4);
		
		setShift(((rows & 0xF) << 4) + (0xF ^ (columns & 0xF)));
		if(columns & 0x10) {
			bit_clear(LED_C5_PORT,LED_C5_PIN);
		} else {
			bit_set(LED_C5_PORT,LED_C5_PIN);
		}
		_delay_ms(5);
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
