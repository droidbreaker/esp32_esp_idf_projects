# ESP32 Weather Station HTTP Client

This project uses an ESP32-based board to connect to Wi-Fi and fetch weather data from the OpenWeatherMap API using `esp_http_client`.

## Project structure

- `main/http_request_example_main.c` — application source code
- `main/CMakeLists.txt` — component build configuration
- `main/idf_component.yml` — component metadata and external dependencies

## What it does

- Initializes NVS flash
- Configures Wi-Fi station mode
- Connects to a WPA2 network
- Sends periodic HTTP GET requests to OpenWeatherMap
- Prints the HTTP response body and status information to the serial console every 30 seconds

## Hard-coded settings

The project currently uses hard-coded values in `main/http_request_example_main.c`:

- `WIFI_SSID` = `FirseStart`
- `WIFI_PASS` = `braker@123`
- `WEATHER_URL` = `http://api.openweathermap.org/data/2.5/weather?q=Mumbai,Maharashtra,IN&APPID=ed3d757c7d232b52e1a0b1bd5df8a30b`

> Replace these settings with your own Wi-Fi credentials and a valid OpenWeatherMap API key before flashing.

## Component dependencies

The component requires:

- `esp_wifi`
- `esp_event`
- `esp_netif`
- `nvs_flash`
- `esp_http_client`

These dependencies are declared in `main/CMakeLists.txt`.

## Build and flash

1. If needed, set the target chip:

```bash
idf.py set-target esp32
```

2. Build the project:

```bash
idf.py build
```

3. Flash and monitor the board:

```bash
idf.py -p PORT flash monitor
```

Replace `PORT` with the serial device for your board, for example `/dev/ttyUSB0`.

## Expected behavior

- The board starts, initializes Wi-Fi, and attempts to connect.
- On connection, it sends an HTTP GET request to the configured OpenWeatherMap URL.
- The response body is printed during `HTTP_EVENT_ON_DATA`.
- A log message shows HTTP status and response length.
- Requests repeat every 30 seconds.
- On Wi-Fi disconnect, the firmware automatically reconnects.

## Customization

To use a different city or API key, edit `WEATHER_URL` in `main/http_request_example_main.c`.

To use your own network credentials, update `WIFI_SSID` and `WIFI_PASS` in the same file.

## Troubleshooting

- If Wi-Fi connection fails, verify SSID and password.
- If HTTP requests fail, verify the URL and API key.
- Ensure ESP-IDF is sourced and `idf.py` is available in the terminal.
- Use the serial monitor output for debug logs.
