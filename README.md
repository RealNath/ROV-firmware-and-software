### Underwater ROV (Remote Operated Vehicle)

## How to run

Makefile are used for firmware management, to use it you need **esptool** and **arduino-cli**. Recipes are:
```bash
esp32-build     # Compile all firmware
esp32-upload    # Upload compiled firmware
esp32-monitor   # Attach into esp32 and see output from serial monitor
esp32-clean     # Clean previous compilation result
esp32-reset     # Pin reset the esp32
```

You can specify specific firmware main build target by **ESP32_SRC** Makefile variable (by default, the value is "**main**"),
```bash
esp32-build ESP32_SRC=main
esp32-build ESP32_SRC=test-thruster
```

To launch the ground control GUI web-app, use **docker compose**, no other dependency is needed,
```bash
cd firmware
docker compose up --build
```

<img width="1010" height="704" alt="image" src="https://github.com/user-attachments/assets/780208ba-5d46-4cd9-95bf-a6a4f21f5a2e" />