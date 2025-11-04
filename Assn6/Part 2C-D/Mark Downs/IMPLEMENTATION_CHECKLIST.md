# Implementation Checklist - ESP8266 Integrated System

Use this checklist to track your progress through the complete setup.

## ☐ Phase 1: Preparation (15 minutes)

### Hardware Acquisition
- [ ] ESP8266 NodeMCU board
- [ ] DHT11 temperature/humidity sensor
- [ ] 2× Standard LEDs (5mm, any color)
- [ ] 1× RGB LED (common cathode)
- [ ] 2× Momentary push button switches
- [ ] 2× 10kΩ resistors (pull-ups)
- [ ] 5× 220-330Ω resistors (current limiting)
- [ ] Breadboard (full size recommended)
- [ ] 20+ Male-to-male jumper wires
- [ ] USB Micro cable
- [ ] Optional: USB battery pack (5V output)

### Software Tools
- [ ] Arduino IDE installed OR PlatformIO installed
- [ ] ESP8266 board support added
- [ ] USB drivers installed (CH340/CP2102 if needed)
- [ ] Serial monitor tested (9600 baud)

### Network Information
- [ ] WiFi SSID (network name) - must be 2.4GHz
- [ ] WiFi password
- [ ] Web server URL (if using cloud features)
- [ ] Slack webhook URL (optional)

---

## ☐ Phase 2: Hardware Assembly (30-45 minutes)

### Power Rails
- [ ] Connected ESP8266 3V3 to breadboard + rail (red)
- [ ] Connected ESP8266 GND to breadboard - rail (blue)
- [ ] Verified 3.3V on + rail with multimeter
- [ ] Verified 0V on - rail with multimeter

### Switch 1 (GPIO0) - Part 1
- [ ] Button positioned across breadboard gap
- [ ] One leg connected to GPIO0 (D3)
- [ ] Same leg has 10kΩ pull-up to 3V3
- [ ] Other leg connected to GND
- [ ] Tested: GPIO0 = 3.3V when not pressed
- [ ] Tested: GPIO0 = 0V when pressed

### Switch 2 (GPIO16) - Part 2
- [ ] Button positioned across breadboard gap
- [ ] One leg connected to GPIO16 (D0)
- [ ] Same leg has 10kΩ pull-up to 3V3
- [ ] Other leg connected to GND
- [ ] Tested: GPIO16 = 3.3V when not pressed
- [ ] Tested: GPIO16 = 0V when pressed

### DHT11 Sensor
- [ ] VCC pin connected to 3V3
- [ ] GND pin connected to GND
- [ ] DATA pin connected to GPIO14 (D5)
- [ ] Power LED on sensor lights up (if present)
- [ ] Tested: VCC = 3.3V, GND = 0V

### LED1 (GPIO12)
- [ ] LED oriented correctly (long leg = anode)
- [ ] GPIO12 → 220Ω resistor → LED anode
- [ ] LED cathode → GND
- [ ] Tested with 3V3: LED lights up

### LED2 (GPIO13)
- [ ] LED oriented correctly (long leg = anode)
- [ ] GPIO13 → 220Ω resistor → LED anode
- [ ] LED cathode → GND
- [ ] Tested with 3V3: LED lights up

### RGB LED
- [ ] Identified correct pinout (pin 2 = cathode, longest)
- [ ] GPIO15 → 330Ω resistor → Red pin (pin 1)
- [ ] GPIO4 → 330Ω resistor → Green pin (pin 3)
- [ ] GPIO5 → 330Ω resistor → Blue pin (pin 4)
- [ ] Cathode (pin 2, longest) → GND
- [ ] Tested: Each color works individually

### Final Hardware Verification
- [ ] All connections secure (no loose wires)
- [ ] No short circuits (verified with continuity test)
- [ ] All grounds connected to common rail
- [ ] All LEDs have current-limiting resistors
- [ ] Power rails still show correct voltages
- [ ] Visual inspection: matches circuit diagram

---

## ☐ Phase 3: Software Configuration (15-20 minutes)

### Download and Extract
- [ ] Downloaded `ESP8266_Integrated_System.zip`
- [ ] Extracted to working directory
- [ ] All files present (verified with file list below)

**File List:**
```
✓ main.cpp
✓ config.h
✓ platformio.ini
✓ switches.h/cpp
✓ sensors.h/cpp
✓ leds.h/cpp
✓ control.h/cpp
✓ messaging.h/cpp
✓ time_client.h/cpp
✓ tx.h/cpp
✓ net.h/cpp
✓ README.md
✓ SETUP_GUIDE.md
✓ CIRCUIT_DIAGRAM.md
✓ QUICK_START.md
✓ web_interface/led_control.php
✓ web_interface/rgb_proxy.php
```

### Configure WiFi
Edit `config.h`:
- [ ] Line 14: Set `WIFI_SSID` to your network name
- [ ] Line 15: Set `WIFI_PASS` to your password
- [ ] Verified: No typos, exact match to router settings
- [ ] Verified: Network is 2.4GHz (ESP8266 requirement)

### Configure Server URLs (Optional)
Edit `config.h` if using cloud features:
- [ ] Line 40: `DB_BASE_URL` points to sensor logging endpoint
- [ ] Line 44: `LED_CONTROL_URL` points to LED control API
- [ ] Line 45: `RGB_CONTROL_URL` points to RGB value provider

### Configure Slack (Optional)
Edit `messaging.cpp`:
- [ ] Line 46: Set `SLACK_WEBHOOK` to your webhook URL
- [ ] Tested webhook with curl command

### Development Environment Setup
**PlatformIO:**
- [ ] Opened project folder in VSCode
- [ ] PlatformIO recognized project (platformio.ini found)
- [ ] Dependencies downloaded automatically

**Arduino IDE:**
- [ ] Opened main.cpp
- [ ] Board selected: "NodeMCU 1.0 (ESP-12E Module)"
- [ ] Port selected: Correct COM port
- [ ] Upload speed: 115200
- [ ] Monitor speed: 9600

---

## ☐ Phase 4: First Upload and Test (15 minutes)

### Upload Firmware
- [ ] ESP8266 connected via USB
- [ ] Upload started successfully
- [ ] Upload completed without errors
- [ ] Upload time: ~30-60 seconds

### Initial Boot Test
- [ ] Opened Serial Monitor at 9600 baud
- [ ] System banner appears
- [ ] MAC address displayed
- [ ] All modules initialize:
  - [ ] [SWITCHES] Initialized
  - [ ] [SENSORS] DHT11 initialized
  - [ ] [TIME] Timezone loaded
  - [ ] [LEDS] Initialized
  - [ ] [CONTROL] Remote control module initialized
  - [ ] [MESSAGING] Module initialized
- [ ] "SYSTEM READY" message appears
- [ ] No error messages in red

### WiFi Connection Test
- [ ] WiFi connection message appears
- [ ] IP address assigned and displayed
- [ ] Connection time: < 15 seconds
- [ ] No "WiFi connect failed" errors

### Visual Startup Test
- [ ] LED1 and LED2 briefly flash at startup
- [ ] RGB LED briefly lights (dim white then off)
- [ ] All LEDs return to OFF state

---

## ☐ Phase 5: Functional Testing (15-30 minutes)

### Test 1: NTP Time Synchronization
- [ ] Serial command 'T' opens timezone menu
- [ ] Entered timezone (e.g., America/Los_Angeles)
- [ ] Timezone saved successfully
- [ ] Current time displayed in correct format
- [ ] Time includes timezone offset

### Test 2: Part 1 - Sensor Logging (Switch 1)
- [ ] Pressed Switch 1 briefly (<1 second)
- [ ] Serial shows "SWITCH 1 Pressed -> Part 1 Triggered"
- [ ] [1/5] Timestamp retrieved from NTP
- [ ] [2/5] DHT11 sensor read successfully
  - Temperature displayed (reasonable value)
  - Humidity displayed (reasonable value)
- [ ] [3/5] Data transmitted to server
- [ ] [4/5] Notification queued for Slack
- [ ] [5/5] LED1 blinks for ~2 seconds
- [ ] Activity count increments
- [ ] Total time: 5-30 seconds (first time includes NTP sync)

### Test 3: Part 2 - Status Check (Switch 2)
- [ ] Set LED states via web interface first:
  - Visited led_control.php in browser
  - Set LED1 to ON
  - Set LED2 to OFF
  - Verified JSON response
- [ ] Set RGB values via mySlider.php:
  - Adjusted R, G, B sliders
  - Noted the values set
- [ ] Pressed Switch 2 briefly (<1 second)
- [ ] Serial shows "SWITCH 2 Pressed -> Part 2 Triggered"
- [ ] [1/4] LED status polled from server
- [ ] [2/4] RGB values polled from server
- [ ] [3/4] Physical LEDs match web settings:
  - LED1 is ON (lit)
  - LED2 is OFF (dark)
  - RGB LED shows correct color
- [ ] [4/4] Status notification queued
- [ ] LED2 blinks for ~2 seconds
- [ ] Activity count increments
- [ ] Total time: 3-10 seconds

### Test 4: Simultaneous Switch Press
- [ ] Pressed Switch 1
- [ ] Immediately pressed Switch 2 (within 1 second)
- [ ] Both operations completed successfully
- [ ] No errors or hangs
- [ ] Both LED1 and LED2 blinked
- [ ] All messages queued properly
- [ ] Serial output shows both complete sequences

### Test 5: Rapid Repeated Presses
- [ ] Pressed Switch 1 five times (1-second intervals)
- [ ] Each press detected and logged
- [ ] Activity counter increments correctly (1→5)
- [ ] No buffer overflow
- [ ] Message queue handles all events
- [ ] System remains responsive

---

## ☐ Phase 6: Web Integration (Optional, 10-15 minutes)

### Server Setup
- [ ] PHP files uploaded to web server:
  - led_control.php
  - rgb_proxy.php
  - sensor_dashboard.php (from Part 1)
- [ ] File permissions set:
  - PHP files: 644
  - Data files: 666 (result.txt, rgb_value.txt)
- [ ] Server accessible from ESP8266

### LED Control Test
- [ ] Opened LED control page in browser
- [ ] GET request returns current states
- [ ] PUT request changes LED1 state
- [ ] PUT request changes LED2 state
- [ ] ESP8266 reflects changes when Switch 2 pressed
- [ ] Response time: < 3 seconds

### RGB Control Test
- [ ] Opened mySlider.php in browser
- [ ] Red slider adjusts red channel
- [ ] Green slider adjusts green channel
- [ ] Blue slider adjusts blue channel
- [ ] Values saved to rgb_value.txt
- [ ] ESP8266 displays color when Switch 2 pressed

### Slack Integration Test
- [ ] Pressed Switch 1
- [ ] Slack notification received
- [ ] Message contains temperature data
- [ ] Message contains humidity data
- [ ] Message contains timestamp
- [ ] Message readable and formatted

---

## ☐ Phase 7: Battery Operation (Optional, 5 minutes)

### Battery Setup
- [ ] USB battery pack fully charged
- [ ] Battery capacity: _____mAh (recommended 5000+)
- [ ] Battery has 5V USB output
- [ ] Disconnected ESP8266 from computer

### Battery Connection
- [ ] Connected ESP8266 to battery pack via USB
- [ ] Power LED on ESP8266 lights up
- [ ] System boots normally (observed LED blinks)
- [ ] Waited 30 seconds for full boot

### Standalone Operation Test
- [ ] Pressed Switch 1 (no Serial Monitor visible)
- [ ] LED1 blinks (visual confirmation)
- [ ] Checked Google Sheets for new entry
- [ ] Checked Slack for notification
- [ ] Data logged successfully

- [ ] Pressed Switch 2
- [ ] LED2 blinks (visual confirmation)
- [ ] LEDs change to match web settings
- [ ] RGB LED shows correct color
- [ ] Status notification received in Slack

### Battery Life Monitoring
- [ ] Noted start time: _________
- [ ] Battery capacity: _____mAh
- [ ] Expected runtime: _____ hours
- [ ] Actual runtime: _____ hours (update later)

---

## ☐ Phase 8: Advanced Testing (Optional, 30+ minutes)

### Stress Testing
- [ ] Left system running for 1 hour
- [ ] No crashes or hangs
- [ ] Memory leaks checked (free heap reported)
- [ ] System responsive throughout

### Network Failure Recovery
- [ ] Disconnected WiFi router
- [ ] Pressed Switch 1
- [ ] Observed error handling
- [ ] Reconnected WiFi
- [ ] System recovered automatically
- [ ] Next press succeeds

### Rapid Event Handling
- [ ] Pressed switches 20 times rapidly
- [ ] All events logged
- [ ] No data loss
- [ ] Message queue handled overflow gracefully

### Long-Term Reliability
- [ ] System running for: _____ hours/days
- [ ] Total events processed: _____
- [ ] Crashes: _____ (should be 0)
- [ ] Successful transmissions: _____%

---

## ☐ Phase 9: Optimization (Optional)

### Performance Tuning
- [ ] Reviewed Serial output for bottlenecks
- [ ] Adjusted timing constants if needed
- [ ] Optimized polling intervals
- [ ] Reduced unnecessary delays

### Power Optimization
- [ ] Measured actual current draw: _____mA
- [ ] Compared to expected: 100-380mA
- [ ] Reduced polling frequency if needed
- [ ] Considered deep sleep implementation

### Code Customization
- [ ] Modified LED blink patterns
- [ ] Adjusted message formats
- [ ] Changed notification content
- [ ] Added custom features

---

## ☐ Phase 10: Documentation and Deployment

### Documentation Complete
- [ ] Recorded all customizations made
- [ ] Noted any wiring changes
- [ ] Documented server URLs used
- [ ] Saved Slack webhook URL securely

### Configuration Backup
- [ ] Backed up modified config.h
- [ ] Backed up modified messaging.cpp
- [ ] Noted all changes from default
- [ ] Saved backup to safe location

### Deployment Checklist
- [ ] All hardware connections secure
- [ ] Breadboard mounted to stable base
- [ ] USB cable strain relief added
- [ ] Components won't shift during transport
- [ ] Battery pack secured if using
- [ ] Access to switches convenient
- [ ] Visual feedback (LEDs) visible

### Maintenance Plan
- [ ] Weekly: Check breadboard connections
- [ ] Weekly: Verify server connectivity
- [ ] Monthly: Clean dust from components
- [ ] Monthly: Check battery health
- [ ] Monthly: Review log files
- [ ] As needed: Update firmware

---

## Troubleshooting Record

If you encounter issues, record them here for future reference:

**Issue 1:**
- Symptom: _________________________________
- Solution: _________________________________
- Time to fix: ________ minutes
- Preventable? [ ] Yes [ ] No

**Issue 2:**
- Symptom: _________________________________
- Solution: _________________________________
- Time to fix: ________ minutes
- Preventable? [ ] Yes [ ] No

**Issue 3:**
- Symptom: _________________________________
- Solution: _________________________________
- Time to fix: ________ minutes
- Preventable? [ ] Yes [ ] No

---

## Completion Summary

### Time Investment
- Phase 1 (Preparation): _____ minutes
- Phase 2 (Hardware): _____ minutes
- Phase 3 (Software): _____ minutes
- Phase 4 (Upload): _____ minutes
- Phase 5 (Testing): _____ minutes
- Phase 6 (Web): _____ minutes
- Phase 7 (Battery): _____ minutes
- Phase 8 (Advanced): _____ minutes
- **Total Time: _____ hours _____minutes**

### Success Criteria (Check all that apply)
- [ ] System boots successfully
- [ ] WiFi connects automatically
- [ ] Switch 1 logs sensor data
- [ ] Switch 2 polls and updates status
- [ ] Both switches work simultaneously
- [ ] LED visual feedback works
- [ ] RGB LED shows correct colors
- [ ] Slack notifications received
- [ ] Google Sheets receives data
- [ ] Battery operation works
- [ ] No crashes or hangs
- [ ] System runs independently

### Overall Rating
**How well does it work?**
- [ ] 🌟🌟🌟🌟🌟 Perfect - Everything works flawlessly
- [ ] 🌟🌟🌟🌟 Great - Minor issues, fully functional
- [ ] 🌟🌟🌟 Good - Some issues, mostly functional
- [ ] 🌟🌟 Fair - Significant issues, needs work
- [ ] 🌟 Poor - Major problems, needs troubleshooting

### Project Complete!
Date Completed: ____________
Total Time: _____ hours
Components Used: $_____
Working Status: [ ] Fully operational [ ] Needs fixes

---

**Notes and Observations:**
_________________________________________
_________________________________________
_________________________________________
_________________________________________

---

**Congratulations on completing your ESP8266 Integrated System!**

For ongoing support, refer to:
- SETUP_GUIDE.md (troubleshooting)
- CIRCUIT_DIAGRAM.md (wiring verification)
- PROJECT_SUMMARY.md (technical details)

Document Version: 1.0
Last Updated: November 3, 2025
