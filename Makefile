.PHONY: all esp32-build esp32-monitor esp32-reset

ESP32_ROOT ?= firmware
ESP32_SRC ?= main
ESP32_BUILD ?= build
ESP32_INCLUDE ?= include

ESP32_PORT ?= /dev/ttyUSB0
ESP32_BOARD ?= esp32s3
ESP32_FQBN ?= esp32:esp32:$(ESP32_BOARD)
ESP32_BAUDRATE ?= 115200


THRUSTER_FL_INV ?= 0
THRUSTER_FR_INV ?= 0
THRUSTER_ML_INV ?= 1
THRUSTER_MR_INV ?= 1
THRUSTER_BL_INV ?= 1
THRUSTER_BR_INV ?= 1


CFLAGS = -I$(ESP32_ROOT)/$(ESP32_INCLUDE)

ifeq ($(THRUSTER_FL_INV), 1)
    CFLAGS += -DTHRUSTER_FL_INV
endif

ifeq ($(THRUSTER_FR_INV), 1)
    CFLAGS += -DTHRUSTER_FR_INV
endif

ifeq ($(THRUSTER_ML_INV), 1)
    CFLAGS += -DTHRUSTER_ML_INV
endif

ifeq ($(THRUSTER_MR_INV), 1)
    CFLAGS += -DTHRUSTER_MR_INV
endif

ifeq ($(THRUSTER_BL_INV), 1)
    CFLAGS += -DTHRUSTER_BL_INV
endif

ifeq ($(THRUSTER_BR_INV), 1)
    CFLAGS += -DTHRUSTER_BR_INV
endif


all: esp32-build esp32-upload esp32-monitor

esp32-build:
	@echo "[Makefile]: "

	arduino-cli compile \
		--fqbn $(ESP32_FQBN) \
		--build-property "compiler.cpp.extra_flags=$(CFLAGS)" \
		--output-dir $(ESP32_ROOT)/$(ESP32_BUILD) \
		$(ESP32_ROOT)/$(ESP32_SRC)
	
	@echo

esp32-upload: esp32-build
	@echo "[Makefile]: "

	arduino-cli upload \
		--fqbn $(ESP32_FQBN) \
		--port $(ESP32_PORT) \
		--build-path $(ESP32_ROOT)/$(ESP32_BUILD)
	
	@echo


esp32-monitor:
	@echo "[Makefile]: "
	arduino-cli monitor \
		--port $(ESP32_PORT) \
		--fqbn $(ESP32_FQBN) \
		--config baudrate=$(ESP32_BAUDRATE)
	@echo


esp32-reset:
	@echo "[Makefile]: "
	@set -e; \
	if lsof $(ESP32_PORT) >/dev/null 2>&1 || fuser $(ESP32_PORT) >/dev/null 2>&1; then \
		echo "Port $(ESP32_PORT) is busy, close serial monitor or other process which uses this port first before resetting."; \
		exit 1; \
	fi; \
	esptool --chip $(ESP32_BOARD) \
		--port $(ESP32_PORT) \
		--no-stub flash_id
	@echo


esp32-clean:
	@echo "[Makefile]: "
	if [ -d "$(ESP32_ROOT)/$(ESP32_BUILD)" ]; then rm -rf "$(ESP32_ROOT)/$(ESP32_BUILD)"; fi
	if [ -d "$(ESP32_ROOT)/$(ESP32_SRC)/$(ESP32_BUILD)" ]; then rm -rf "$(ESP32_ROOT)/$(ESP32_SRC)/$(ESP32_BUILD)"; fi
	@echo