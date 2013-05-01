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

char colData[4];
volatile int IRcode = 0;
int latchIR = 0;
char currentBit = 1;
unsigned int oldTime = 0;
volatile int seq = 0;
int elapsed = 0;
int bit = 0;
unsigned int foundBeacons = 0;
unsigned int foundBeacons2 = 0;
const int cmdlen = 12;
#define BEACONS 0
#define DEBUG 1
int mode = BEACONS;
int frame;

ISR(PCINT0_vect)
{
        const int tolerance = 200;
        const int timebase = 600;
	unsigned int time = TCNT1;
        elapsed = 0;
	if(time < oldTime) {
		unsigned int remain = 0xFFFF - oldTime;
		elapsed += time + remain;
	}
	else {
		elapsed += time - oldTime;
	}
 	oldTime = time;

        bit = 1-bit;

        // We only care about falling edges
        if(elapsed < timebase*4 + tolerance && elapsed > timebase*4-tolerance) {
          // start pulse
          IRcode = 0;
          seq = 0;
          bit = 1;
        }
        if(bit==1) {
          if(seq < cmdlen) {
            if(elapsed < timebase*2 + tolerance && elapsed > timebase*2-tolerance) {
              IRcode |= (1<<seq);
              seq++;
            }
            else if(elapsed < timebase + tolerance && elapsed > timebase-tolerance) {
              seq++;
            }
            if(seq >=cmdlen) {
              latchIR = IRcode;
            }
          }
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


	bit_set(BTN1_PORT, BTN1_PIN);
	bit_set(BTN2_PORT, BTN2_PIN);

	TCNT1H = 0; // Initial TIMER1
	TCNT1L = 0; // Initial TIMER1
	// Set up timer1 as a general interval timer.
	// Ideally we should set up timer1 so it latches at TOP
	TCCR1B = 2; // Start clock (/8 prescaler = 1Mhz)

        foundBeacons = eeprom_read_byte(0) | (eeprom_read_byte(1)<<8);
        foundBeacons2 = eeprom_read_byte(2) | (eeprom_read_byte(3)<<8);
        frame = 0;
}

void loop()
{
	if(!bit_get(BTN1_PORT, BTN1_PIN)) {
          mode = BEACONS;
	}
	if(!bit_get(BTN2_PORT, BTN2_PIN)) {
          mode = DEBUG;
	}
        
        int command = latchIR & 0x7F;
        int device = (latchIR>> 7) & 0x1F;

        if(device == 1 && command < 10) {
          if((foundBeacons & (1<<command)) == 0) {
            foundBeacons |= (1<<command);
            eeprom_write_byte(0,foundBeacons);
            eeprom_write_byte(1,foundBeacons>>8);
          }
        }
        else if(device == 1 && command < 20) {
          if((foundBeacons2 & (1<<(command-10))) == 0) {
            foundBeacons2 |= (1<<(command-10));
            eeprom_write_byte(2,foundBeacons2);
            eeprom_write_byte(3,foundBeacons2>>8);
          }
        }

        if(mode == BEACONS) {
          colData[0] = foundBeacons & 0x1f;
          colData[1] = (foundBeacons>>5) & 0x1f;
          colData[2] = (foundBeacons2) & 0x1f;
          colData[3] = (foundBeacons2>>5) & 0x1f;
        }
        else
        {
          colData[0] = 1 << (seq % 5);
          colData[1] = (latchIR & 0x1F);
          colData[2] = (latchIR >> 5) & 0x1f;
          colData[3] = (latchIR >> 10) & 0x1f;
        }

	//bit_flip(MISO_PORT, MISO_PIN); // Uncomment this to flash the IR Led
        frame += 1;
	
        row += 1;

	  
        int columns = colData[row%4];
        int rows = 1<<(row % 4);


#ifdef OLDBADGE
        setShift(((rows & 0xF) << 4) + (0xF ^ (columns & 0xF)));
#else
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

	if(mode==BEACONS) {
		_delay_ms(500);
		setShift(0);
		if(row%4==3) {
			_delay_ms(5000);
		}
	}
	else {
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
