MCU      = atmega328p
F_CPU    = 16000000UL

PORT    ?= COM8
BAUD    ?= 115200

CXX      = avr-g++
OBJCOPY  = avr-objcopy
SIZE     = avr-size
AVRDUDE  = avrdude

CXXFLAGS  = -mmcu=$(MCU)
CXXFLAGS += -DF_CPU=$(F_CPU)
CXXFLAGS += -Os
CXXFLAGS += -Wall -Wextra
CXXFLAGS += -std=gnu++17

LDFLAGS   = -mmcu=$(MCU)

all: main.hex

main.elf: main.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<
	$(SIZE) $@

main.hex: main.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

upload: main.hex
	$(AVRDUDE) -p m328p -c arduino -P $(PORT) -b $(BAUD) -D -U flash:w:$<:i

clean:
	rm -f main.elf main.hex

.PHONY: all upload clean
