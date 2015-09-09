AVRDUDE=avrdude
AVRDUDE_OPTS=-F -V
OBJCOPY=avr-objcopy
RM=rm -f
CC=avr-gcc

MCU=atmega328p
BIN_FORMAT=ihex
F_CPU=16000000UL
PORT=/dev/ttyACM0
BAUD=115200
PROTOCOL=arduino
PART=ATMEGA328P
PROG=arduino_display
CFLAGS=-Wall -Os -DF_CPU=$(F_CPU) -mmcu=$(MCU)

$(PROG).hex: $(PROG)
	$(OBJCOPY) -O $(BIN_FORMAT) -R .eeprom $< $@

$(PROG): $(PROG).elf lcd_driver.elf
	$(CC) -mmcu=$(MCU) $(PROG).elf lcd_driver.elf -o $(PROG)

$(PROG).elf: $(PROG).c
	$(CC) $(CFLAGS) -c -o $@ $< 

lcd_driver.elf:	lcd_driver.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
.PHONY: clean
clean:
	-$(RM) $(PROG).elf $(PROG).hex

.PHONY: upload
upload: $(PROG).hex
	$(AVRDUDE) $(AVRDUDE_OPTS) -c $(PROTOCOL) -p $(PART) -P $(PORT) \
		-b $(BAUD) -U flash:w:$(PROG).hex
