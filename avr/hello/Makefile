MCU := atmega328p
F_CPU := 16000000UL

CC := /ucrt64/bin/avr-gcc
OBJCOPY := /ucrt64/bin/avr-objcopy
SIZE := /ucrt64/bin/avr-size
AVRDUDE := /ucrt64/bin/avrdude

PROGRAMMER := arduino
UPLOAD_PORT := COM20
UPLOAD_BAUD := 115200
MONITOR_PORT := COM13
MONITOR_BAUD := 9600

TARGET := main

CFLAGS := -mmcu=$(MCU)
CFLAGS += -DF_CPU=$(F_CPU)
CFLAGS += -Os
CFLAGS += -std=c11
CFLAGS += -Wall
CFLAGS += -Wextra

LDFLAGS := -mmcu=$(MCU)

.PHONY: all compile upload monitor run size clean help

all: compile

compile: $(TARGET).hex
	$(SIZE) --mcu=$(MCU) --format=avr $(TARGET).elf

$(TARGET).elf: $(TARGET).c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

upload: $(TARGET).hex
	$(AVRDUDE) \
		-c $(PROGRAMMER) \
		-p m328p \
		-P $(UPLOAD_PORT) \
		-b $(UPLOAD_BAUD) \
		-D \
		-U flash:w:$(TARGET).hex:i

monitor:
	arduino-cli monitor \
		--port $(MONITOR_PORT) \
		--config baudrate=$(MONITOR_BAUD)

run: upload monitor

size: $(TARGET).elf
	$(SIZE) --mcu=$(MCU) --format=avr $(TARGET).elf

clean:
	rm -f $(TARGET).elf $(TARGET).hex $(TARGET).o

help:
	@echo "make           Programm kompilieren"
	@echo "make upload    Kompilieren und hochladen"
	@echo "make monitor   Seriellen Monitor starten"
	@echo "make run       Hochladen und Monitor starten"
	@echo "make size      Speicherverbrauch anzeigen"
	@echo "make clean     Build-Dateien entfernen"
