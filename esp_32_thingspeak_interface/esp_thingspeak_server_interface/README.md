| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 | Linux |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- | ----- |

# ESP32 ThingSpeak Weather Station Interface

A comprehensive IoT weather station application that collects local environmental data using a DHT11 sensor and global weather data from OpenWeatherMap API, then uploads all readings to the ThingSpeak cloud platform via secure HTTPS connections.

---

## Project Overview

This project implements a complete weather monitoring system that:
- Reads temperature and humidity from a local DHT11 sensor
- Fetches global weather data from OpenWeatherMap API
- Uploads data to ThingSpeak cloud server for remote monitoring
- Calculates and tracks temperature differences between global and local readings
- Updates data every 30 seconds

---

## Hardware Components

| Component | Model | Purpose |
| --- | --- | --- |
| **Microcontroller** | ESP32-C3 WROOM | Main processor and WiFi connectivity |
| **Temperature/Humidity Sensor** | DHT11 | Local environmental data collection |
| **Server Platform** | ThingSpeak | Cloud data storage and visualization |
| **Weather API** | OpenWeatherMap | Global weather data source |

---

## Key Features

✅ **Dual Data Source**: Combines local sensor readings with global API data
✅ **Secure Communication**: HTTPS/TLS encrypted data transmission
✅ **WiFi Connectivity**: Automatic WiFi connection with reconnection handling
✅ **Real-time Monitoring**: 30-second update cycle
✅ **Cloud Integration**: Direct ThingSpeak server integration with API key authentication
✅ **Data Analysis**: Automatic temperature difference calculation between global and local readings
✅ **JSON Parsing**: Efficient OpenWeatherMap API response parsing using cJSON library

---

## How It Works

### Program Flow

1. **WiFi Initialization**: Connects to WiFi network with automatic reconnection capability
2. **Main Loop** (executes every 30 seconds):
   - Fetches global weather data from OpenWeatherMap API (for Mumbai, Maharashtra, India)
   - Reads local temperature and humidity from DHT11 sensor
   - Calculates temperature difference (global - local)
   - Sends all data to ThingSpeak cloud server via secure HTTPS

### Data Fields Sent to ThingSpeak

| Field | Data | Unit |
| --- | --- | --- |
| **Field 1** | Global Temperature | °C |
| **Field 2** | Local Temperature (DHT11) | °C |
| **Field 3** | Global Humidity | % |
| **Field 4** | Local Humidity (DHT11) | % |
| **Field 5** | Temperature Difference | °C (float)| 

### Technical Details

- **Temperature Conversion**: OpenWeatherMap API returns Kelvin; program converts to Celsius (Celsius = Kelvin - 273.15)
- **Sensor Interface**: DHT11 connected to GPIO5
- **HTTP Client**: ESP-IDF HTTP client with event handler for data reception
- **Update Interval**: 30 seconds delay between each data collection cycle
- **Error Handling**: Graceful error logging and recovery for WiFi and sensor failures

---

## Dependencies

- **ESP-IDF Components**:
  - FreeRTOS: Real-time operating system
  - WiFi (esp_wifi): WiFi connectivity
  - HTTP Client (esp_http_client): HTTPS communication
  - NVS Flash: Non-volatile storage
  
- **External Libraries**:
  - **cJSON**: JSON parsing for OpenWeatherMap API responses
  - **DHT Library**: DHT11 sensor communication
  - **Certificate Bundle**: TLS/SSL certificate validation for HTTPS

---

## Configuration

### WiFi Credentials
Update the following defines in `main/esp_thingspeak_server_main.c`:
```c
#define WIFI_SSID      "Your_WiFi_SSID"
#define WIFI_PASS      "Your_WiFi_Password"
```

### ThingSpeak API Key
```c
#define THINGSPEAK_API_KEY "Your_ThingSpeak_API_Key"
```

### DHT11 Sensor GPIO
```c
#define DHT_GPIO GPIO_NUM_5  // Adjust if using different GPIO
```

### Weather Location
```c
#define WEATHER_URL "http://api.openweathermap.org/data/2.5/weather?q=Mumbai,Maharashtra,IN&APPID=Your_API_Key"
```

---

## Build & Flash

```bash
# Configure project
idf.py menuconfig

# Build the project
idf.py build

# Flash to ESP32
idf.py flash

# Monitor serial output
idf.py monitor
```

---

## Supported Targets

- ESP32
- ESP32-C2
- ESP32-C3 ⭐ (Recommended for this project)
- ESP32-C5
- ESP32-C6
- ESP32-C61
- ESP32-H2
- ESP32-P4
- ESP32-S2
- ESP32-S3
- Linux

---

## Troubleshooting

| Issue | Solution |
| --- | --- |
| WiFi connection fails | Verify SSID, password, and WiFi credentials in code |
| DHT11 read errors | Check sensor wiring and GPIO configuration |
| ThingSpeak upload fails | Verify API key and HTTPS certificate bundle |
| Poor sensor readings | Ensure DHT11 is properly connected with pull-up resistor |

