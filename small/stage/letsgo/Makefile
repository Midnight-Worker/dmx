ARDUINO_CLI := arduino-cli

FQBN := arduino:avr:nano:cpu=atmega328

UPLOAD_PORT  := COM20
MONITOR_PORT := COM13
MONITOR_BAUD := 9600

SKETCH_DIR := .
BUILD_DIR  := build

.PHONY: all compile upload run monitor ports clean help

all: compile

compile:
	$(ARDUINO_CLI) compile \
		--fqbn $(FQBN) \
		--build-path $(BUILD_DIR) \
		$(SKETCH_DIR)

upload: compile
	$(ARDUINO_CLI) upload \
		--fqbn $(FQBN) \
		--port $(UPLOAD_PORT) \
		--input-dir $(BUILD_DIR) \
		$(SKETCH_DIR)

monitor:
	$(ARDUINO_CLI) monitor \
		--port $(MONITOR_PORT) \
		--config baudrate=$(MONITOR_BAUD)

run: upload monitor

ports:
	$(ARDUINO_CLI) board list

clean:
	rm -rf "$(BUILD_DIR)"

help:
	@echo "make              Projekt kompilieren"
	@echo "make compile      Projekt kompilieren"
	@echo "make upload       Kompilieren und auf COM13 hochladen"
	@echo "make monitor      Debug-Monitor auf COM20 starten"
	@echo "make run          Hochladen und Monitor starten"
	@echo "make ports        Serielle Ports anzeigen"
	@echo "make clean        Build-Verzeichnis entfernen"
