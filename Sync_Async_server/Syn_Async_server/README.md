# ESP32 HTTP LED Control Server

This project runs on ESP32 and starts a simple HTTP server that controls an LED using PWM brightness.

## What the main program does

- Initializes NVS and the LED PWM driver (LEDC) on `GPIO2`.
- Connects to Wi-Fi using the configured SSID and password.
- Starts an HTTP server after Wi-Fi initialization.
- Serves a web page with buttons to:
  - turn the LED on
  - turn the LED off
  - adjust LED brightness using a slider
- Handles HTTP routes for `/`, `/led/on`, `/led/off`, and `/led/brightness?v=<value>`.

## Features

- Station mode Wi-Fi connection with auto-reconnect.
- LED brightness control from 0 to 100%.
- Simple browser-based UI served directly from the ESP32.
- Status logging for IP address and Wi-Fi events.

## Files

- `CMakeLists.txt` - top-level build configuration.
- `main/CMakeLists.txt` - application build configuration.
- `main/Sync_Async_main.c` - main application source code.
- `pytest_hello_world.py` - placeholder Python test script.
- `README.md` - this project description.

## Build and flash

Use the ESP-IDF build system from the project root:

```bash
idf.py build
idf.py -p <PORT> flash monitor
```

## Notes

- Update `WIFI_SSID` and `WIFI_PASS` in `main/Sync_Async_LED_control_main.c` before flashing if needed.
- The server listens on the IP assigned by the Wi-Fi network.
- Open the ESP32 IP in a browser to access the LED control page.
