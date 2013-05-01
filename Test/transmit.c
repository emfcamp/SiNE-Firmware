#define F_CPU 8000000UL
#define USART_BAUDRATE 9600

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "macros.h"
#include "sinetest.h"

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

void transmit(int code) {
  
  bit_clear(MISO_PORT, MISO_PIN);
  _delay_us(500);
  transmit_start();
  int bit;
  for(bit=0;bit<12;bit++) {
    int b = (code>>bit) & 1;
    if(b==0) {
      transmit0();
    }
    else
    {
      transmit1();
    }
  }
  bit_clear(MISO_PORT, MISO_PIN);
}
