# SiNE Devboard AVR Test Code
# Copyright (C) 2013  Bob Clough <bob@clough.me>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

CC=avr-gcc
OBJCOPY=avr-objcopy
STRIP=avr-strip
AVRDUDE=avrdude

# Change this to change the default output name
BADGENAME=sinetest
BEACONNAME=beacon

# Change this section to change the chip type.
PART=attiny44

# Change this secton to change AVRDUDE settings
AVRDUDE_PART=t44
AVRDUDE_PROG=stk500v2
AVRDUDE_PORT=/dev/ttyACM0

# Set the default action to be compiling everything, then writing the chip
default: $(BADGENAME).hex

$(BADGENAME).o: $(BADGENAME).c
	$(CC) -g -O -mmcu=$(PART) -c $^ -o $@
	
$(BADGENAME): $(BADGENAME).o 
	$(CC) -g -O -mmcu=$(PART) $^ -o $@

$(BADGENAME)-stripped: $(BADGENAME)
	$(STRIP) $^ -o $@

$(BADGENAME).hex: $(BADGENAME)-stripped
	$(OBJCOPY) -O ihex $^ $@

$(BEACONNAME).o: $(BEACONNAME).c
	$(CC) -g -O -mmcu=$(PART) -c $^ -o $@
	
$(BEACONNAME): $(BEACONNAME).o 
	$(CC) -g -O -mmcu=$(PART) $^ -o $@

$(BEACONNAME)-stripped: $(BEACONNAME)
	$(STRIP) $^ -o $@

$(BEACONNAME).hex: $(BEACONNAME)-stripped
	$(OBJCOPY) -O ihex $^ $@


# Program the chip, using avrdude and the variables set above
program: $(BADGENAME).hex
	$(AVRDUDE) -p $(AVRDUDE_PART) -c $(AVRDUDE_PROG) -P $(AVRDUDE_PORT) -U flash:w:$(BADGENAME).hex 

# Program the chip, using avrdude and the variables set above
progbeacon: $(BEACONNAME).hex
	$(AVRDUDE) -p $(AVRDUDE_PART) -c $(AVRDUDE_PROG) -P $(AVRDUDE_PORT) -U flash:w:$(BEACONNAME).hex 

fuses:	
	$(AVRDUDE) -p $(AVRDUDE_PART) -c $(AVRDUDE_PROG) -P $(AVRDUDE_PORT) -U lfuse:w:0xc2:m -U hfuse:w:0xd7:m -U efuse:w:0xff:m

	
# Cleanup, leave only makefile and any .c/.h files
clean:
	rm -f *.o $(BADGENAME) $(BADGENAME).hex $(BADGENAME)-stripped
	rm -f *.o $(BEACONNAME) $(BEACONNAME).hex $(BEACONNAME)-stripped
