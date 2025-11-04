# ESP8266 Integrated Control System

## Overview

This project integrates three major functionalities into a single ESP8266 program:

1. **Part 1**: Environmental sensor logging with Slack/Google Sheets integration
2. **Part 2A**: Remote LED control (ON/OFF status)
3. **Part 2B**: Remote RGB LED control (color values)

## Hardware Requirements

### Components
- ESP8266 NodeMCU (or compatible)
- DHT11 Temperature/Humidity sensor
- 2× Standard LEDs (any color)
- 1× RGB LED (common cathode)
- 2× Momentary push button switches
- 2× 10kΩ resistors (for switches)
- 5× 220-330Ω resistors (for LEDs)
- Breadboard and jumper wires
- USB cable for programming
- Optional: Battery pack for standalone operation

### Pin Connections

```
ESP8266 GPIO    Function            Component
─────────────────────────────────────────────────
GPIO0  (D3)     Switch 1 Input      Push button → GND (with 10kΩ pull-up)
GPIO16 (D0)     Switch 2 Input      Push button → GND (with 10kΩ pull-up)
GPIO14 (D5)     DHT11 Data          DHT11 sensor
GPIO12 (D6)     LED1 Output         LED → 220Ω → GND
GPIO13 (D7)     LED2 Output         LED → 220Ω → GND
GPIO15 (D8)     RGB Red             RGB Red pin → 330Ω
GPIO4  (D2)     RGB Green           RGB Green pin → 330Ω
GPIO5  (D1)     RGB Blue            RGB Blue pin → 330Ω
GND             Ground              Common ground
3.3V/VIN        Power               DHT11 VCC
```

### Wiring Diagram

```
                    ESP8266 NodeMCU
                    ┌─────────────┐
    [Switch 1] ──►─┤ GPIO0  (D3) │
    [Switch 2] ──►─┤ GPIO16 (D0) │
    [DHT11]    ──►─┤ GPIO14 (D5) │
    [LED1]     ◄───┤ GPIO12 (D6) │
    [LED2]     ◄───┤ GPIO13 (D7) │
    [RGB-R]    ◄───┤ GPIO15 (D8) │
    [RGB-G]    ◄───┤ GPIO4  (D2) │
    [RGB-B]    ◄───┤ GPIO5  (D1) │
                    └─────────────┘
```

## Software Setup

### 1. Configure WiFi Credentials

Edit `config.h` and update your WiFi credentials:

```cpp
#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASS "Your_WiFi_Password"
```

### 2. Configure Backend URLs

Verify the URLs in `config.h` point to your server:

```cpp
#define DB_BASE_URL "https://your-domain.com/sensor_dashboard.php"
#define LED_CONTROL_URL "https://your-domain.com/led_control.php"
#define RGB_CONTROL_URL "https://your-domain.com/rgb_proxy.php"
```

### 3. Configure Slack Webhook (Optional)

Edit `messaging.cpp` and add your Slack webhook URL:

```cpp
const char* SLACK_WEBHOOK = "https://hooks.slack.com/services/YOUR/WEBHOOK/URL";
```

### 4. Upload Code

Using PlatformIO:
```bash
cd ESP8266_Integrated_System
pio run --target upload
pio device monitor
```

Using Arduino IDE:
1. Open `main.cpp` in Arduino IDE
2. Select Board: "NodeMCU 1.0 (ESP-12E Module)"
3. Select Port: Your ESP8266 COM port
4. Click Upload
5. Open Serial Monitor (9600 baud)

## Operation

### Switch Functions

**Switch 1 (GPIO0) - Part 1: Sensor Logging**
- Triggers DHT11 reading
- Logs data to Google Sheets
- Sends notification to Slack/SMS
- Blinks LED1 for 2 seconds

**Switch 2 (GPIO16) - Part 2: Status Check**
- Polls LED control status from server
- Polls RGB values from server
- Updates local LED/RGB states
- Sends status notification
- Blinks LED2 for 2 seconds

### Serial Commands

- Press **'T'** or **'t'**: Change timezone
  - Enter IANA timezone string (e.g., America/New_York)
  - Saved to EEPROM for persistence

### LED Feedback

- **LED1**: Blinks when Part 1 completes successfully
- **LED2**: Blinks when Part 2 completes successfully
- **RGB LED**: Shows current color values from remote control

## Features

### Robust Operation
- ✓ Non-blocking event-driven architecture
- ✓ Simultaneous switch handling (both can be pressed rapidly)
- ✓ Message buffering and retry logic
- ✓ Visual feedback for all operations
- ✓ Battery operation capable
- ✓ Automatic WiFi reconnection
- ✓ NTP time synchronization with timezone support

### Error Handling
- WiFi connection retry
- Sensor reading validation
- HTTP request timeout handling
- Message queue with retry logic
- Graceful degradation on failures

## Web Interface

### Part 2A: LED Control Page (`led_control.php`)

The LED control page provides:
- GET: Returns current LED states
- PUT/POST: Updates LED states
- JSON format: `{"led1": "ON/OFF", "led2": "ON/OFF", "timestamp": "..."}`

### Part 2B: RGB Control (`rgb_proxy.php`)

The RGB control provides:
- Returns comma-separated RGB values
- Format: `R,G,B` (e.g., `255,128,0`)
- Controlled via `mySlider.php` web interface

## File Structure

```
ESP8266_Integrated_System/
├── platformio.ini          # PlatformIO configuration
├── config.h                # System-wide configuration
├── main.cpp                # Main program (event loop)
├── switches.h/cpp          # Dual switch input handling
├── sensors.h/cpp           # DHT11 sensor interface
├── leds.h/cpp              # LED and RGB control
├── control.h/cpp           # Remote LED/RGB polling
├── messaging.h/cpp         # Slack/SMS notifications
├── time_client.h/cpp       # NTP time synchronization
├── tx.h/cpp                # Data transmission to server
├── net.h/cpp               # WiFi connectivity helpers
└── README.md               # This file
```

## Troubleshooting

### WiFi Issues
- Verify SSID and password in `config.h`
- Check router signal strength
- Ensure router is 2.4GHz (ESP8266 doesn't support 5GHz)

### Sensor Issues
- Check DHT11 wiring (VCC, GND, Data)
- Verify 3.3V or 5V power supply
- Try different GPIO pin if needed
- DHT11 requires 2-second delay between readings

### LED Not Blinking
- Check LED polarity (long leg = anode = +)
- Verify resistor values (220-330Ω)
- Test with digitalWrite directly
- Check GPIO pin number in config.h

### Messages Not Sent
- Verify Slack webhook URL in messaging.cpp
- Check internet connectivity
- Monitor Serial output for error codes
- Verify message queue not full (MAX 10 messages)

### NTP Sync Fails
- Check internet connection
- Verify firewall allows NTP (UDP port 123)
- Try different NTP server in time_client.cpp
- Check timezone string format

### RGB LED Issues
- Verify common cathode type (not common anode)
- Check PWM-capable pins (GPIO4, 5, 15)
- Verify resistor values (330Ω recommended)
- Test each color channel individually

## Power Consumption

- Idle (WiFi on): ~80mA
- Active transmission: 150-300mA
- Deep sleep (future feature): <1mA

Battery recommendations:
- 2000mAh: ~6-8 hours continuous operation
- 5000mAh: ~16-20 hours continuous operation
- With sleep mode: Days to weeks possible

## Serial Monitor Output

Example successful operation:

```
╔═══════════════════════════════════════════════════════╗
║     ESP8266 INTEGRATED CONTROL SYSTEM v2.0           ║
╚═══════════════════════════════════════════════════════╝

MAC Address: AA:BB:CC:DD:EE:FF
[SWITCHES] Initialized
[SENSORS] DHT11 initialized
[TIME] Timezone loaded: America/Los_Angeles
[LEDS] Initialized
[CONTROL] Remote control module initialized
[MESSAGING] Module initialized

[READY] Waiting for switch events...

[SWITCH 1] Pressed -> Part 1 Triggered

╔═══════════════════════════════════════════════════════╗
║           PART 1: SENSOR LOGGING EVENT                ║
╚═══════════════════════════════════════════════════════╝
[1/5] Getting timestamp from NTP...
✓ Time: 2025-11-03T14:30:45-08:00
[2/5] Reading DHT11 sensor...
✓ Temperature: 23.5°C
✓ Humidity: 65.2%
[3/5] Transmitting to Google Sheets...
✓ Data logged successfully
[4/5] Sending Slack notification...
✓ Notification queued
[5/5] Visual confirmation...
✓ LED1 blinking

╔═══════════════════════════════════════════════════════╗
║  PART 1 COMPLETE - Activity Count: 1                 ║
╚═══════════════════════════════════════════════════════╝
```

## Future Enhancements

- [ ] Deep sleep mode for battery optimization
- [ ] SD card logging for offline operation
- [ ] OTA (Over-The-Air) firmware updates
- [ ] Web server for configuration
- [ ] MQTT support for IoT platforms
- [ ] Additional sensor support (BME280, etc.)
- [ ] SMS notifications via Twilio
- [ ] Push notifications via mobile app

## License

This project is provided as-is for educational purposes.

## Author

Huy Nguyen
Date: November 3, 2025
Version: 2.0

## Support

For issues and questions:
1. Check Serial Monitor output (9600 baud)
2. Verify all connections match pin assignments
3. Test each component individually
4. Review error codes in console output
