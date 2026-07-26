.PHONY: all esp32-build esp32-monitor esp32-reset

ESP32_SRC ?= ./firmware
ESP32_TARGET ?= main_controller
ESP32_PORT ?= /dev/ttyUSB0
ESP32_FQBN ?= esp32:esp32:esp32
ESP32_BAUDRATE ?= 115200


all: esp32-build esp32-monitor

esp32-build:
	@echo -n "[Makefile]: "

	arduino-cli compile \
		--fqbn $(ESP32_FQBN) \
		--output-dir $(ESP32_SRC)/build \
		$(ESP32_SRC)/

	arduino-cli upload \
		--fqbn $(ESP32_FQBN) \
		--port $(ESP32_PORT) \
		--build-path $(ESP32_SRC)/build


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
	if [ -d "$(ESP32_SRC)/build" ]; then rm -rf "$(ESP32_SRC)/build"; fi