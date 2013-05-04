#define F_CPU 8000000UL
#define USART_BAUDRATE 9600

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "macros.h"
#include "sinetest.h"
#include "transmit.h"
#include "display.h"
#include "common_config.h"

char colData[4];

int transmitCode = 0;

void setup()
{
        general_pin_setup();
	configure_IR_output();
}

void loop()
{
	if(!bit_get(BTN1_PORT, BTN1_PIN)) {
          transmitCode += 1;
          if(transmitCode >= 20) {
            transmitCode = 0;
          }
	}

	if(!bit_get(BTN2_PORT, BTN2_PIN)) {
          // Does nothing
	}

	// Transmit sequence

        int IRseq = (transmitCode) | (0x1F ^ transmitCode) << 5;

        int i;
        for(i=0;i<5;i++) {
          transmit(IRseq);
          _delay_ms(100);
        }

        colData[0] = 0;
        colData[1] = 0;
        colData[2] = 0;
        colData[3] = 0;

        int c = transmitCode / 5;
        colData[c] = (char) 1<<(transmitCode % 5);

        int row;
	for(row=0;row<4;row++) {
		int columns = colData[row];
		int rows = 1<<(row % 4);
                configureLEDs(columns, rows);
                _delay_ms(5);
        }

        setShift(0);
	_delay_ms(500);
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
