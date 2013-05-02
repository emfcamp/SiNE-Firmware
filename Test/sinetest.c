#define F_CPU 8000000UL
#define USART_BAUDRATE 9600

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "macros.h"
#include "sinetest.h"

#include "display.h"

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
int badgeID = 0;
int idTimeout = 0;
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

	badgeID = eeprom_read_byte(4) | ((eeprom_read_byte(5) & 1)<<8);

        idTimeout = 5;
}

void loop()
{
	if(!bit_get(BTN1_PORT, BTN1_PIN)) {
          mode = BEACONS;
	}
	if(!bit_get(BTN2_PORT, BTN2_PIN)) {
          mode = DEBUG;
	}
        
        // Process potentially new beacons.
        if((latchIR >> 10) == 0) {
          int badgeCode = latchIR & 0x1F;
          int badgeConfirm = 0x1f^((latchIR >> 5) & 0x1F);
          if(badgeCode == badgeConfirm) {
            if(badgeCode < 10) {
              if((foundBeacons & (1<<badgeCode)) == 0) {
                foundBeacons |= (1<<badgeCode);
                eeprom_write_byte(0,foundBeacons);
                eeprom_write_byte(1,foundBeacons>>8);
              }
            }
            else if(badgeCode < 20) {
              if((foundBeacons2 & (1<<(badgeCode-10))) == 0) {
                foundBeacons2 |= (1<<(badgeCode-10));
                eeprom_write_byte(2,foundBeacons2);
                eeprom_write_byte(3,foundBeacons2>>8);
              }
            }
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
	

        if(idTimeout > 0) {
          colData[0] = badgeID & 0x1F;
          colData[1] = (badgeID >> 5) & 0x1F;
          idTimeout -= 1;
        }


	  
        int columns = colData[row%4];
        int rows = 1<<(row % 4);

        row += 1;


        configureLEDs(columns, rows);

	if(mode==BEACONS) {
		_delay_ms(500);
                configureLEDs(0,0);
		if(row%4==0) {
			_delay_ms(5000);
			// send ID...
			cli();
			int i;
      			for(i=0;i<5;i++) {
                          int transmitCode = badgeID;
                          int checkCode = badgeID & 3;
				transmit(transmitCode | checkCode << 9 | 1 << 11);
				_delay_ms(100);
			}
			sei();

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
