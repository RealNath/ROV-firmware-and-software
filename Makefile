.PHONY: all esp32-build esp32-monitor esp32-reset

ESP32_ROOT ?= firmware
ESP32_SRC ?= main
ESP32_PORT ?= /dev/ttyUSB0
ESP32_FQBN ?= esp32:esp32:esp32
ESP32_BAUDRATE ?= 115200


all: esp32-build esp32-upload esp32-monitor

esp32-build:
	@echo -n "[Makefile]: "

	arduino-cli compile \
		--fqbn $(ESP32_FQBN) \
		--build-property "compiler.cpp.extra_flags=-I$(ESP32_ROOT)/include" \
		--output-dir $(ESP32_ROOT)/build \
		$(ESP32_ROOT)/$(ESP32_SRC)

esp32-upload:
	@echo -n "[Makefile]: "

	arduino-cli upload \
		--fqbn $(ESP32_FQBN) \
		--port $(ESP32_PORT) \
		--build-path $(ESP32_ROOT)/build


esp32-monitor:
	@echo -n "[Makefile]: "
	arduino-cli monitor \
		--port $(ESP32_PORT) \
		--fqbn $(ESP32_FQBN) \
		--config baudrate=$(ESP32_BAUDRATE)


esp32-reset:
	@echo -n "[Makefile]: "
	@set -e; \
	if lsof $(ESP32_PORT) >/dev/null 2>&1 || fuser $(ESP32_PORT) >/dev/null 2>&1; then \
		echo "Port $(ESP32_PORT) is busy, close serial monitor or other process which uses this port first before resetting."; \
		exit 1; \
	fi; \
	esptool --chip esp32 \
		--port $(ESP32_PORT) \
		--no-stub flash_id


esp32-clean:
	@echo -n "[Makefile]: "
	if [ -d "$(ESP32_ROOT)/build" ]; then rm -rf "$(ESP32_ROOT)/build"; fi