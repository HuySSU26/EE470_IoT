# ESP8266 Integrated System - Complete Setup Guide

## Table of Contents
1. [Hardware Assembly](#hardware-assembly)
2. [Software Configuration](#software-configuration)
3. [Web Server Setup](#web-server-setup)
4. [Testing Procedures](#testing-procedures)
5. [Battery Operation](#battery-operation)

---

## Hardware Assembly

### Required Components Checklist

- [ ] ESP8266 NodeMCU board
- [ ] DHT11 sensor module
- [ ] 2× Standard LEDs (5mm, any color)
- [ ] 1× RGB LED (common cathode, 5mm)
- [ ] 2× Momentary push buttons
- [ ] 2× 10kΩ resistors (brown-black-orange)
- [ ] 5× 220-330Ω resistors (red-red-brown or orange-orange-brown)
- [ ] Breadboard (full size recommended)
- [ ] Male-to-male jumper wires (20+ pieces)
- [ ] USB Micro cable
- [ ] Optional: 5V battery pack with USB output

### Step 1: Prepare the Breadboard

1. Place the ESP8266 NodeMCU in the center of the breadboard
2. Ensure pins on both sides have accessible rows
3. Connect power rails:
   - Connect 3V3 pin to breadboard positive rail (red)
   - Connect GND pin to breadboard negative rail (blue)

### Step 2: Wire the Switches

**Switch 1 (Part 1 - Sensor Logging)**
```
1. Insert push button across breadboard center gap
2. Connect one leg to GPIO0 (D3)
3. Connect same leg to 3V3 via 10kΩ resistor (pull-up)
4. Connect other leg to GND
```

**Switch 2 (Part 2 - Status Check)**
```
1. Insert push button across breadboard center gap
2. Connect one leg to GPIO16 (D0)
3. Connect same leg to 3V3 via 10kΩ resistor (pull-up)
4. Connect other leg to GND
```

### Step 3: Wire the DHT11 Sensor

```
DHT11 Pin    →    ESP8266 Pin
─────────────────────────────
VCC          →    3V3 (or VIN for 5V modules)
GND          →    GND
DATA         →    GPIO14 (D5)
```

**Note**: Some DHT11 modules have built-in pull-up resistors. If using a bare DHT11 sensor, add a 10kΩ pull-up resistor between DATA and VCC.

### Step 4: Wire LED1 and LED2

**LED1 (GPIO12)**
```
GPIO12 (D6) → 220Ω resistor → LED anode (long leg)
LED cathode (short leg) → GND
```

**LED2 (GPIO13)**
```
GPIO13 (D7) → 220Ω resistor → LED anode (long leg)
LED cathode (short leg) → GND
```

**LED Polarity Check**:
- Long leg = Anode (+) = Connect to GPIO via resistor
- Short leg = Cathode (-) = Connect to GND
- Flat edge on LED = Cathode side

### Step 5: Wire the RGB LED

**RGB LED Pinout** (Common Cathode - looking at flat side):
```
    Longest leg (2nd from left) = Common Cathode (GND)
    
    Pin 1 (leftmost)  = Red
    Pin 2 (longest)   = Cathode (GND)
    Pin 3             = Green
    Pin 4 (rightmost) = Blue
```

**Connections**:
```
GPIO15 (D8) → 330Ω resistor → RGB Red pin
GPIO4  (D2) → 330Ω resistor → RGB Green pin
GPIO5  (D1) → 330Ω resistor → RGB Blue pin
Common Cathode (longest leg) → GND
```

### Complete Wiring Table

| Component      | Component Pin | Wire Color | ESP8266 Pin |
|----------------|---------------|------------|-------------|
| Switch 1       | One side      | Black      | GPIO0 (D3)  |
| Switch 1       | Other side    | Black      | GND         |
| Switch 1       | Pull-up       | Red        | 3V3         |
| Switch 2       | One side      | Black      | GPIO16 (D0) |
| Switch 2       | Other side    | Black      | GND         |
| Switch 2       | Pull-up       | Red        | 3V3         |
| DHT11          | VCC           | Red        | 3V3         |
| DHT11          | GND           | Black      | GND         |
| DHT11          | DATA          | Yellow     | GPIO14 (D5) |
| LED1           | Anode         | Red        | GPIO12 (D6) |
| LED1           | Cathode       | Black      | GND         |
| LED2           | Anode         | Red        | GPIO13 (D7) |
| LED2           | Cathode       | Black      | GND         |
| RGB LED        | Red           | Red        | GPIO15 (D8) |
| RGB LED        | Green         | Green      | GPIO4 (D2)  |
| RGB LED        | Blue          | Blue       | GPIO5 (D1)  |
| RGB LED        | Cathode       | Black      | GND         |

### Visual Verification Checklist

Before powering on:
- [ ] All GND connections share common ground
- [ ] No GPIO pins are shorted to VCC or GND
- [ ] All LEDs have current-limiting resistors
- [ ] LED polarities are correct (long leg to GPIO)
- [ ] RGB LED cathode (longest leg) connected to GND
- [ ] Switch pull-up resistors connected properly
- [ ] DHT11 VCC to 3V3 (not 5V if using NodeMCU)

---

## Software Configuration

### Step 1: Install Development Environment

**Option A: PlatformIO (Recommended)**
```bash
# Install VSCode and PlatformIO extension
# Or use PlatformIO CLI:
pip install platformio
```

**Option B: Arduino IDE**
1. Download Arduino IDE from arduino.cc
2. Install ESP8266 board support:
   - File → Preferences → Additional Board URLs
   - Add: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
   - Tools → Board → Boards Manager → Search "ESP8266" → Install

### Step 2: Configure WiFi Credentials

Edit `config.h`:
```cpp
#define WIFI_SSID "YourNetworkName"      // Replace with your WiFi SSID
#define WIFI_PASS "YourNetworkPassword"  // Replace with your WiFi password
```

**Important**: 
- ESP8266 only supports 2.4GHz WiFi
- Avoid special characters in SSID if possible
- Use WPA2-PSK encryption

### Step 3: Configure Server URLs

Edit `config.h` to point to your server:
```cpp
#define DB_BASE_URL "https://your-domain.com/sensor_dashboard.php"
#define LED_CONTROL_URL "https://your-domain.com/led_control.php"
#define RGB_CONTROL_URL "https://your-domain.com/rgb_proxy.php"
```

### Step 4: Optional - Configure Slack Notifications

Edit `messaging.cpp` line ~46:
```cpp
const char* SLACK_WEBHOOK = "https://hooks.slack.com/services/YOUR/WEBHOOK/URL";
```

To get a Slack webhook URL:
1. Go to https://api.slack.com/apps
2. Create New App → From scratch
3. Add features: Incoming Webhooks
4. Activate Incoming Webhooks
5. Add New Webhook to Workspace
6. Copy the webhook URL

### Step 5: Upload Code

**Using PlatformIO**:
```bash
cd ESP8266_Integrated_System
pio run --target upload
pio device monitor
```

**Using Arduino IDE**:
1. Open `main.cpp`
2. Tools → Board → ESP8266 Boards → NodeMCU 1.0 (ESP-12E Module)
3. Tools → Port → Select your COM port
4. Tools → Upload Speed → 115200
5. Sketch → Upload
6. Tools → Serial Monitor → Set to 9600 baud

### First Boot Checklist

After upload, open Serial Monitor (9600 baud):
- [ ] "ESP8266 INTEGRATED CONTROL SYSTEM v2.0" appears
- [ ] MAC address displayed
- [ ] All modules initialize successfully
- [ ] "SYSTEM READY" message appears
- [ ] No error messages

---

## Web Server Setup

### Required PHP Files

Upload these files to your web server:
1. `led_control.php` - LED ON/OFF control
2. `rgb_proxy.php` - RGB value provider
3. `sensor_dashboard.php` - Sensor data logger (from Part 1)

### File Permissions

```bash
chmod 644 led_control.php
chmod 644 rgb_proxy.php
chmod 666 result.txt        # LED control data file
chmod 666 rgb_value.txt     # RGB values file
```

### Test Web Interface

**Test LED Control**:
```bash
curl https://your-domain.com/led_control.php
# Should return: {"status":"success","led1":"OFF","led2":"OFF",...}
```

**Test RGB Proxy**:
```bash
curl https://your-domain.com/rgb_proxy.php
# Should return: 0,0,0
```

### Create Control Web Pages

**LED Control HTML** (`control_leds.html`):
```html
<!DOCTYPE html>
<html>
<head>
    <title>LED Control</title>
    <script>
        function setLED(led, state) {
            fetch('led_control.php', {
                method: 'PUT',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({[led]: state})
            })
            .then(r => r.json())
            .then(data => {
                document.getElementById('status').innerText = 
                    'LED1: ' + data.led1 + ', LED2: ' + data.led2;
            });
        }
    </script>
</head>
<body>
    <h1>LED Control Panel</h1>
    <button onclick="setLED('led1', 'ON')">LED1 ON</button>
    <button onclick="setLED('led1', 'OFF')">LED1 OFF</button>
    <br><br>
    <button onclick="setLED('led2', 'ON')">LED2 ON</button>
    <button onclick="setLED('led2', 'OFF')">LED2 OFF</button>
    <div id="status"></div>
</body>
</html>
```

**RGB Control HTML** (`mySlider.php` from Part 2B) should already exist.

---

## Testing Procedures

### Test 1: Basic Connectivity

1. Power on ESP8266
2. Check Serial Monitor shows "WiFi OK, IP: xxx.xxx.xxx.xxx"
3. Verify all modules initialized

Expected output:
```
[SWITCHES] Initialized
[SENSORS] DHT11 initialized
[TIME] Timezone loaded: America/Los_Angeles
[LEDS] Initialized
[CONTROL] Remote control module initialized
[MESSAGING] Module initialized
```

### Test 2: Switch 1 (Sensor Logging)

1. Press Switch 1 briefly
2. Observe Serial Monitor

Expected sequence:
```
[SWITCH 1] Pressed -> Part 1 Triggered
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
```

3. LED1 should blink for 2 seconds
4. Check Google Sheets for new data entry
5. Check Slack for notification

### Test 3: Switch 2 (Status Check)

1. Set LED states via web interface:
   - Visit control_leds.html
   - Click "LED1 ON"
   - Click "LED2 OFF"

2. Set RGB values via mySlider.php:
   - Adjust R, G, B sliders
   - Note the values

3. Press Switch 2 on ESP8266

Expected sequence:
```
[SWITCH 2] Pressed -> Part 2 Triggered
[1/4] Polling LED control status...
[CONTROL] LED1 -> ON
[CONTROL] LED2 -> OFF
✓ LED states updated
[2/4] Polling RGB values...
[CONTROL] RGB updated: R=255, G=128, B=0
✓ RGB values updated
[3/4] Sending status notification...
  LED1:ON, LED2:OFF
  RGB(255,128,0)
✓ Notification queued
[4/4] Visual confirmation...
✓ LED2 blinking
```

4. Physical LEDs should match web interface settings
5. RGB LED should display the color from sliders
6. LED2 should blink for 2 seconds
7. Check Slack for status notification

### Test 4: Simultaneous Switch Press

1. Quickly press Switch 1, then Switch 2 (within 1 second)
2. Observe both operations complete successfully
3. Both LED1 and LED2 should blink
4. All messages should be queued and sent

Expected behavior:
- Part 1 executes first
- Part 2 executes second
- No data loss
- All confirmations visible
- Both Slack messages received

### Test 5: Rapid Repeated Presses

1. Press Switch 1 five times rapidly (1-second intervals)
2. Observe message queue handling

Expected:
- Each press is detected
- Messages queued properly
- Activity counter increments
- No buffer overflow

### Test 6: Network Failure Recovery

1. Disconnect WiFi (turn off router)
2. Press Switch 1
3. Observe error handling

Expected:
```
✗ Failed to transmit data
```

4. Reconnect WiFi
5. Press Switch 1 again
6. Should work normally

---

## Battery Operation

### Battery Selection

Recommended battery packs:
- **2000mAh**: 6-8 hours continuous operation
- **5000mAh**: 16-20 hours continuous operation
- **10000mAh**: 30-40 hours continuous operation

Use 5V USB output battery pack with:
- Minimum 1A output current
- USB Micro connector or adapter
- Power button optional

### Power Consumption

| Mode                    | Current Draw | Duration (5000mAh) |
|-------------------------|--------------|---------------------|
| Idle (WiFi connected)   | ~80mA        | ~60 hours           |
| Sensor reading          | ~100mA       | ~50 hours           |
| HTTPS transmission      | ~150-300mA   | Peak only           |
| Both LEDs + RGB at max  | +100mA       | When active         |

### Battery Operation Procedure

1. Upload and test code with USB connection
2. Disconnect USB
3. Connect battery pack via USB cable
4. Power on battery pack
5. Wait for startup (30 seconds for NTP sync on first boot)
6. System is now standalone

### Indicators for Battery Operation

- LED1/LED2 will blink when switches are pressed
- RGB LED will show status
- No Serial Monitor available
- All data still logged to server
- Notifications still sent to Slack

### Battery Safety

⚠️ **Important**:
- Do not short circuit battery terminals
- Use quality branded battery packs
- Monitor temperature (should stay cool)
- Disconnect if any burning smell detected
- Do not expose to water
- Do not leave unattended for extended periods

---

## Troubleshooting Guide

### Issue: ESP8266 Won't Connect to WiFi

**Symptoms**: Serial shows repeated dots, "WiFi connect failed"

**Solutions**:
1. Verify SSID and password in config.h
2. Check router is 2.4GHz (not 5GHz)
3. Move closer to router
4. Check router allows new device connections
5. Try different WiFi network

### Issue: DHT11 Reading Error

**Symptoms**: "✗ Failed to read sensor", temperature shows -10

**Solutions**:
1. Check wiring: VCC to 3V3, GND to GND, DATA to GPIO14
2. Verify DHT11 module has power LED lit
3. Wait 2 seconds between readings
4. Try different GPIO pin and update config.h
5. Replace DHT11 sensor if damaged

### Issue: LEDs Don't Light Up

**Symptoms**: No visual feedback when switches pressed

**Solutions**:
1. Check LED polarity (long leg to GPIO)
2. Verify resistors present (220-330Ω)
3. Test LED with 3V battery directly
4. Check GPIO pins in config.h match wiring
5. Measure voltage at GPIO pins (should be 3.3V when HIGH)

### Issue: RGB LED Wrong Colors

**Symptoms**: Colors don't match sliders, or one color missing

**Solutions**:
1. Verify common cathode type (not common anode)
2. Check longest leg connected to GND
3. Verify pin assignments: R=GPIO15, G=GPIO4, B=GPIO5
4. Test each color individually via web interface
5. Check all three resistors present and correct

### Issue: Switches Not Detected

**Symptoms**: No response when pressing switches

**Solutions**:
1. Check pull-up resistors present (10kΩ)
2. Verify switch wiring: one leg to GPIO, other to GND
3. Test with multimeter: GPIO should read 3.3V when not pressed, 0V when pressed
4. Check debounce not too aggressive
5. For GPIO16: verify pull-up resistor (internal pull-up weak)

### Issue: Data Not Logging to Server

**Symptoms**: "✗ Failed to transmit data"

**Solutions**:
1. Verify server URL in config.h is correct and accessible
2. Check PHP files uploaded and have correct permissions
3. Test PHP endpoint with curl from computer
4. Check server logs for PHP errors
5. Verify SSL certificate valid (or use setInsecure())

### Issue: Slack Notifications Not Received

**Symptoms**: "✓ Notification queued" but no Slack message

**Solutions**:
1. Verify Slack webhook URL in messaging.cpp
2. Test webhook with curl:
   ```bash
   curl -X POST -H 'Content-type: application/json' \
   --data '{"text":"Test"}' YOUR_WEBHOOK_URL
   ```
3. Check Slack app permissions
4. Verify message queue not full (MAX 10 messages)
5. Check Serial Monitor for transmission errors

### Issue: System Hangs or Crashes

**Symptoms**: ESP8266 stops responding, requires reset

**Solutions**:
1. Check power supply adequate (min 500mA)
2. Add delay between rapid switch presses
3. Reduce message queue size if memory low
4. Check for infinite loops in code
5. Monitor Serial for stack overflow errors
6. Verify breadboard connections solid (no loose wires)

---

## Performance Optimization

### Reduce Power Consumption

1. Lower WiFi transmission power:
   ```cpp
   WiFi.setOutputPower(0);  // Range: 0-20.5 dBm
   ```

2. Reduce polling frequency (in control.cpp)

3. Implement deep sleep between operations (future enhancement)

### Improve Response Time

1. Use faster WiFi connection (closer to router)
2. Reduce HTTP timeout values
3. Use connection reuse for HTTPS
4. Pre-connect WiFi before first operation

### Increase Reliability

1. Add watchdog timer
2. Implement brownout detection
3. Store critical data in EEPROM
4. Add retry logic with exponential backoff
5. Implement heartbeat monitoring

---

## Appendix A: Pin Reference

### ESP8266 NodeMCU Pin Mapping

| Label  | GPIO | Function        | Notes                |
|--------|------|-----------------|----------------------|
| D0     | 16   | Switch 2        | No PWM, no I2C       |
| D1     | 5    | RGB Blue (PWM)  | I2C SCL              |
| D2     | 4    | RGB Green (PWM) | I2C SDA              |
| D3     | 0    | Switch 1        | Boot mode (pull-up)  |
| D4     | 2    | Built-in LED    | Boot mode (pull-up)  |
| D5     | 14   | DHT11 Data      | SPI SCLK             |
| D6     | 12   | LED1 Output     | SPI MISO             |
| D7     | 13   | LED2 Output     | SPI MOSI             |
| D8     | 15   | RGB Red (PWM)   | SPI CS, boot mode    |
| D9/RX  | 3    | -               | Serial RX (reserved) |
| D10/TX | 1    | -               | Serial TX (reserved) |

### Safe GPIO Pins

✅ **Safe for general use**:
- GPIO4 (D2)
- GPIO5 (D1)
- GPIO12 (D6)
- GPIO13 (D7)
- GPIO14 (D5)

⚠️ **Use with caution** (boot mode pins):
- GPIO0 (D3) - Must be HIGH at boot
- GPIO2 (D4) - Must be HIGH at boot
- GPIO15 (D8) - Must be LOW at boot

❌ **Avoid**:
- GPIO1 (TX) - Serial communication
- GPIO3 (RX) - Serial communication
- GPIO6-11 - Flash memory (internal)

---

## Appendix B: Error Codes

| Code | Module    | Meaning                    | Solution                      |
|------|-----------|----------------------------|-------------------------------|
| -100 | Network   | WiFi timeout               | Check WiFi credentials        |
| -1   | Time      | NTP sync failed            | Check internet connection     |
| -2   | Time      | Local time conversion fail | Check timezone string         |
| -10  | Sensor    | DHT11 read error           | Check DHT11 wiring           |
| -20  | Transmit  | No WiFi connection         | Reconnect WiFi               |
| -21  | Transmit  | HTTP client init failed    | Restart ESP8266              |
| 404  | HTTP      | Endpoint not found         | Check server URL             |
| 409  | Database  | Duplicate timestamp        | Wait before retry            |

---

## Appendix C: Maintenance Schedule

| Task                          | Frequency  |
|-------------------------------|------------|
| Check breadboard connections  | Weekly     |
| Clean dust from components    | Monthly    |
| Update firmware               | As needed  |
| Check battery health          | Monthly    |
| Verify server connectivity    | Weekly     |
| Review log files              | Weekly     |
| Test emergency switches       | Monthly    |
| Backup configuration          | Monthly    |

---

## Support and Resources

- ESP8266 Arduino Core: https://github.com/esp8266/Arduino
- DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
- PlatformIO Docs: https://docs.platformio.org/
- ESP8266 Community: https://www.esp8266.com/

**Document Version**: 1.0
**Last Updated**: November 3, 2025
