# ThermoSense S3

Non-contact thermometer on an ESP32-S3 with an MLX90614 IR sensor, a small OLED and a status RGB LED.

It reads object and ambient temperature four times a second. The OLED shows the object temperature in large digits, the Fahrenheit value, ambient temperature and a live scrolling graph of the last 120 readings (about 30 seconds). The graph auto-scales with a minimum 1 degree range so small noise doesn't make it jump.

The onboard RGB LED breathes in a colour that matches the reading:

| Object temperature | LED |
| --- | --- |
| below 20 °C | blue (cool) |
| 20 - 37.5 °C | green (normal) |
| 37.5 - 40 °C | orange (elevated) |
| 40 °C and above | red (high) |

Readings are also printed to Serial at 115200 baud.

## Hardware

- ESP32-S3 dev board with the onboard WS2812 RGB LED on GPIO 48 (e.g. ESP32-S3-DevKitC-1)
- MLX90614 IR temperature sensor (I2C)
- 128x64 SSD1306 OLED (I2C, address 0x3C)

| Signal | ESP32-S3 GPIO |
| --- | --- |
| I2C SDA (OLED + MLX90614) | 8 |
| I2C SCL (OLED + MLX90614) | 9 |
| RGB LED data | 48 |

Both I2C devices share the same bus. Power them from 3.3 V.

If the OLED or the sensor isn't found at boot, the sketch prints `FAILED` on Serial and stops.

## Stack

- Arduino C++ on the ESP32 core
- Adafruit SSD1306, Adafruit GFX, Adafruit MLX90614, Adafruit NeoPixel

## Flash

The sketch is `S3_1.ino`, so the folder must be named `S3_1` for Arduino to open it (clone with `git clone <repo-url> S3_1`, or rename the folder).

Arduino IDE: install the "esp32 by Espressif Systems" board package and the four Adafruit libraries above from Library Manager, select "ESP32S3 Dev Module" and your port, then upload.

arduino-cli:

```bash
arduino-cli core install esp32:esp32
arduino-cli lib install "Adafruit SSD1306" "Adafruit GFX Library" "Adafruit MLX90614 Library" "Adafruit NeoPixel"
arduino-cli compile --fqbn esp32:esp32:esp32s3 S3_1
arduino-cli upload -p /dev/cu.usbmodem101 --fqbn esp32:esp32:esp32s3 S3_1
```

Replace the port with whatever your board shows up as (`arduino-cli board list`).
