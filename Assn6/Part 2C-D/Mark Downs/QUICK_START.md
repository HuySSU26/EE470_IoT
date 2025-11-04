# Quick Start Guide - ESP8266 Integrated System

## 5-Minute Setup

### What You Need Right Now
- [ ] ESP8266 NodeMCU board
- [ ] USB cable
- [ ] Computer with Arduino IDE or PlatformIO
- [ ] Your WiFi network name and password

### Step 1: Configure WiFi (2 minutes)

Open `config.h` and edit these two lines:
```cpp
#define WIFI_SSID "YourNetworkName"       // ← Change this
#define WIFI_PASS "YourNetworkPassword"   // ← Change this
```

### Step 2: Upload Code (2 minutes)

**Using PlatformIO:**
```bash
cd ESP8266_Integrated_System
pio run --target upload
```

**Using Arduino IDE:**
1. Open `main.cpp`
2. Select Board: "NodeMCU 1.0 (ESP-12E Module)"
3. Click Upload button

### Step 3: Open Serial Monitor (30 seconds)

Set to **9600 baud** and you should see:
```
╔═══════════════════════════════════════════════════════╗
║     ESP8266 INTEGRATED CONTROL SYSTEM v2.0           ║
╚═══════════════════════════════════════════════════════╝

[READY] Waiting for switch events...
```

**✅ If you see this, your ESP8266 is ready!**

## Without Hardware (Software Only Testing)

You can test the code without any additional hardware by:

1. Upload the code
2. Open Serial Monitor
3. Type **'T'** to test timezone configuration
4. Watch the startup sequence

The system will show errors for missing sensors/switches, but you can verify:
- WiFi connection works
- NTP time synchronization works
- Code compiles and runs correctly

## Minimum Hardware Test

To test with minimal hardware, you just need:

### Test 1: Built-in LED
Add this to `main.cpp` at the end of `setup()`:
```cpp
pinMode(LED_BUILTIN, OUTPUT);
digitalWrite(LED_BUILTIN, LOW);
delay(1000);
digitalWrite(LED_BUILTIN, HIGH);
```

You should see the built-in LED blink once at startup.

### Test 2: External LED (Simplest)
```
GPIO12 (D6) → 220Ω resistor → LED anode (+)
LED cathode (-) → GND
```
Press GPIO0 to GND momentarily (built-in FLASH button).

## Basic Wiring (5-minute setup)

If you want to test the full system quickly:

### Absolute Minimum
```
Component           Connect To          Notes
──────────────────────────────────────────────────────────
Switch 1            GPIO0 → GND         Use built-in button
LED1               GPIO12 → 220Ω → GND  Visual feedback
Power              USB cable            That's it!
```

This gives you:
- ✅ Working switch input
- ✅ LED visual feedback  
- ✅ Can test Part 1 and Part 2 functions
- ✅ DHT sensor optional (will show error but code runs)

### Add Sensor (2 more minutes)
```
DHT11 VCC  → 3V3
DHT11 GND  → GND
DHT11 DATA → GPIO14 (D5)
```

Now you can test actual sensor readings!

## Testing Without Full Hardware

### Test Part 1 (Sensor Logging)
1. Press built-in button (GPIO0/FLASH)
2. Serial Monitor shows:
   ```
   [SWITCH 1] Pressed -> Part 1 Triggered
   [1/5] Getting timestamp from NTP...
   ✓ Time: 2025-11-03T14:30:45-08:00
   ```

Without DHT11, you'll see:
```
✗ Failed to read sensor
```
But NTP and transmission logic still work!

### Test Part 2 (Status Check)
Since GPIO16 isn't easily accessible as a button, modify code temporarily:

In `main.cpp`, add this to the loop to simulate Switch 2:
```cpp
static unsigned long lastTest = 0;
if (millis() - lastTest > 30000) {  // Every 30 seconds
    Serial.println("\n[TEST] Simulating Switch 2 press");
    // Manually trigger Part 2 code
    lastTest = millis();
}
```

## Cloud Services Setup (Optional)

### Slack Notifications (5 minutes)
1. Go to https://api.slack.com/apps
2. Create New App → "ESP8266 Notifier"
3. Incoming Webhooks → Activate
4. Add to Workspace → Copy webhook URL
5. Edit `messaging.cpp` line 46:
   ```cpp
   const char* SLACK_WEBHOOK = "your-webhook-url-here";
   ```

### Google Sheets (Already setup if you have Part 1)
Your existing `sensor_dashboard.php` works as-is!

## Common First-Time Issues

### ❌ "WiFi connect failed"
**Fix**: Double-check SSID and password in `config.h`
- ESP8266 only supports 2.4GHz WiFi
- Password is case-sensitive

### ❌ "Upload failed" or "COM port not found"
**Fix**: Install CH340 or CP2102 USB driver
- Windows: Search "CH340 driver download"
- Mac: Usually works without driver
- Linux: Add user to dialout group

### ❌ "Failed to read sensor"
**Fix**: Normal if DHT11 not connected yet
- Code runs fine without sensor
- Just can't log actual temperature/humidity

### ❌ "HTTP error: -1"
**Fix**: Check server URLs in `config.h`
- Must be accessible from ESP8266
- Try HTTP instead of HTTPS for testing

## Next Steps

Once you have basic functionality working:

1. **Add more hardware** (See SETUP_GUIDE.md for full wiring)
   - Second switch (GPIO16)
   - Second LED (GPIO13)
   - RGB LED (GPIO15, 4, 5)

2. **Configure backend** (See SETUP_GUIDE.md)
   - Upload PHP files to server
   - Test LED control web interface
   - Configure Slack webhook

3. **Deploy** (Battery operation)
   - Disconnect USB
   - Connect USB battery pack
   - System runs independently!

## Full Documentation

For complete setup with all hardware:
- **SETUP_GUIDE.md** - Complete step-by-step (35 pages)
- **CIRCUIT_DIAGRAM.md** - Detailed wiring diagrams
- **README.md** - Feature overview and usage
- **PROJECT_SUMMARY.md** - Technical details

## Emergency Troubleshooting

### If absolutely nothing works:

1. **Test ESP8266 with blink example:**
   ```cpp
   void setup() {
     pinMode(LED_BUILTIN, OUTPUT);
   }
   void loop() {
     digitalWrite(LED_BUILTIN, LOW);
     delay(1000);
     digitalWrite(LED_BUILTIN, HIGH);
     delay(1000);
   }
   ```
   If this doesn't work, hardware issue (bad ESP8266 or cable).

2. **Check Serial Monitor baud rate:**
   - Must be exactly 9600 baud
   - Wrong baud = garbled text

3. **Verify board selection:**
   - Must be "NodeMCU 1.0" or "ESP-12E Module"
   - Wrong board = upload fails or wrong pins

4. **Power cycle:**
   - Unplug USB
   - Wait 10 seconds
   - Plug back in
   - Open Serial Monitor

## Success Indicators

You know it's working when you see:

✅ **WiFi Connected:**
```
[WiFi] Connected! IP: 192.168.1.xxx
```

✅ **System Ready:**
```
[READY] Waiting for switch events...
```

✅ **Time Sync Working:**
```
✓ Time: 2025-11-03T14:30:45-08:00
```

✅ **LED Blinking:**
Physical LED blinks when you press button

## Time Investment

| Setup Level          | Time    | Hardware Needed    |
|---------------------|---------|-------------------|
| Software only       | 5 min   | ESP8266 + USB     |
| Minimal (1 LED)     | 10 min  | + LED + resistor  |
| Basic (sensor+LED)  | 20 min  | + DHT11           |
| Complete system     | 1 hour  | All components    |

## Support

- Check Serial Monitor output first
- Review error messages (they're descriptive)
- See SETUP_GUIDE.md Troubleshooting section
- Verify wiring against CIRCUIT_DIAGRAM.md

## Bottom Line

**Minimum to get started:**
1. ESP8266 NodeMCU ($3-5)
2. USB cable (you probably have one)
3. 5 minutes to configure and upload

**That's it!** You can add hardware incrementally as you test each feature.

---

**Ready to start? Begin with Step 1 above!**
