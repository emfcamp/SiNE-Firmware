#define F_CPU 8000000UL
#define USART_BAUDRATE 9600

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "macros.h"
#include "sinetest.h"

#include "display.h"
#include "common_config.h"

char colData[4];

// Globals used by IR receiver interrupt
volatile int IRcode = 0;
int latchIR = 0;
unsigned int oldTime = 0;
volatile int seq = 0;
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
int eraseConfirm;

ISR(PCINT0_vect)
{
        const int tolerance = 200;
        const int timebase = 600;
	unsigned int time = TCNT1;
	int elapsed; 
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
	general_pin_setup();

	configure_IR_interrupt();

	configure_IR_output();

	// Set up timer1 as a general interval timer.
	TCNT1H = 0; // Initial TIMER1
	TCNT1L = 0; // Initial TIMER1
	TCCR1B = 2; // Start clock (/8 prescaler = 1Mhz)

	// Read persistent variables from EEPROM
        foundBeacons = eeprom_read_byte((char*)0) | (eeprom_read_byte((char*)1)<<8);
        foundBeacons2 = eeprom_read_byte((char*)2) | (eeprom_read_byte((char*)3)<<8);
	badgeID = eeprom_read_byte((char*)4) | ((eeprom_read_byte((char*)5) & 1)<<8);

	// Set up other global variables
        idTimeout = 4;
        eraseConfirm = 0;
        frame = 0;
}

void transmitBadgeID()
{
  if(badgeID > 0) {
    // send ID...
    cli();
    int i;
    for(i=0;i<5;i++) {
      int transmitCode = badgeID;
      int checkCode = badgeID & 3;
      transmit(transmitCode | checkCode << 9 | 3 << 11);
      _delay_ms(100);
    }
    sei();
  }

}

void loop()
{
	if(!bit_get(BTN1_PORT, BTN1_PIN)) {
          idTimeout = 4;
	}

	if(!bit_get(BTN2_PORT, BTN2_PIN)) {
          eraseConfirm += 1;
          if(eraseConfirm > 1) {
            if(badgeID > 0) {
              eeprom_write_byte((char*)4,0);
              eeprom_write_byte((char*)5,0);
            }
            badgeID = 0;
          }

	}
        else
        {
          eraseConfirm = 0;
        }
        
        // Process potentially new beacons.
        if((latchIR >> 10) == 0) {
          int badgeCode = latchIR & 0x1F;
          int badgeConfirm = 0x1f^((latchIR >> 5) & 0x1F);
          if(badgeCode == badgeConfirm) {
            if(badgeCode < 10) {
              if((foundBeacons & (1<<badgeCode)) == 0) {
                foundBeacons |= (1<<badgeCode);
                eeprom_write_byte((char*)0,foundBeacons);
                eeprom_write_byte((char*)1,foundBeacons>>8);
              }
            }
            else if(badgeCode < 20) {
              if((foundBeacons2 & (1<<(badgeCode-10))) == 0) {
                foundBeacons2 |= (1<<(badgeCode-10));
                eeprom_write_byte((char*)2,foundBeacons2);
                eeprom_write_byte((char*)3,foundBeacons2>>8);
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
		_delay_ms(400);
                configureLEDs(0,0);
		if(row%4==0) {
                  int i;
                  for(i=0;i<5;i++) {
			_delay_ms(1000);
                        transmitBadgeID();
                  }
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
