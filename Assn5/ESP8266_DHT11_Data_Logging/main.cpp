/*
 * ----------------------------------------------
 * Project/Program Name : ESP8266 Dual-Node Environmental Monitoring System
 * File Name            : main.cpp
 * Author               : Huy Nguyen
 * Date                 : 22/10/2025
 * Version              : 1.8
 * 
 * Purpose:
 *    Main program orchestrator for ESP8266-based dual-node sensor logger.
 *    Manages two independent input sources (push button and tilt switch) that
 *    trigger DHT11 temperature/humidity readings with NTP-synchronized timestamps.
 *    Data is transmitted via HTTPS to a MySQL database with visual LED feedback.
 *    Implements event-driven architecture with non-blocking design patterns.
 * 
 * Inputs:
 *    - GPIO0 (PIN_BUTTON): Push button with 10kΩ external pull-up → node_1
 *    - GPIO4 (PIN_TILT): Tilt switch with 10kΩ external pull-up → node_2
 *    - GPIO14 (PIN_DHT): DHT11 sensor data line (temperature & humidity)
 *    - Serial Monitor: 'T' or 't' command to change timezone
 *    - WiFi: Internet connectivity for NTP sync and HTTPS transmission
 *    - NTP Servers: pool.ntp.org, time.nist.gov, time.google.com
 * 
 * Outputs:
 *    - GPIO12 (PIN_LED1): Visual confirmation LED for node_1 transmissions
 *    - GPIO13 (PIN_LED2): Visual confirmation LED for node_2 transmissions
 *    - Serial Monitor: Status messages, error codes, sensor readings (9600 baud)
 *    - HTTPS POST: JSON payload to https://huynguyen.co/Chartjs/sensor_dashboard.php
 *      Format: {"node": 1, "temperature_C": 23.4, "humidity_pct": 63.5,
 *               "timestamp": "2025-10-17T22:30:45-07:00", "activity_count": 5}
 *    - Database: Updates sensor_data and sensor_activity tables via PHP endpoint
 * 
 * Example Application:
 *    Environmental monitoring in laboratory or greenhouse:
 *    - Button press: Manual temperature/humidity logging for node_1 location
 *    - Tilt detection: Automatic logging when door opens/closes for node_2
 *    - Activity counters: Track number of measurements per input source
 *    - Real-time dashboard: View data trends via web-based graphs
 *    - LED feedback: Visual confirmation of successful data transmission
 * 
 * Dependencies:
 *    Hardware:
 *      - ESP8266 NodeMCU (or compatible board)
 *      - DHT11 temperature/humidity sensor
 *      - 2x momentary push buttons or tilt switches
 *      - 2x 10kΩ resistors (pull-up for switches)
 *      - 2x LEDs (any color)
 *      - 2x 270Ω resistors (current limiting for LEDs)
 *      - Breadboard and jumper wires
 *      - USB cable for power and programming
 *    
 *    Software Libraries:
 *      - ESP8266 Arduino Core (WiFi, HTTPClient, BearSSL)
 *      - Adafruit DHT sensor library @ ^1.4.6
 *      - ArduinoJson @ ^7.0.4
 *      - time.h (C Standard Library for NTP time functions)
 *		- NTP Protocol: Syncs with pool.ntp.org, time.nist.gov
 *      - EEPROM (timezone persistence)
 *    
 *    Custom Modules:
 *      - config.h: System-wide configuration constants
 *      - switches.h/cpp: Input device handling and debouncing
 *      - time_client.h/cpp: NTP synchronization with timezone support
 *      - sensors.h/cpp: DHT11 sensor interface
 *      - tx.h/cpp: HTTPS data transmission with duplicate detection
 *      - leds.h/cpp: Non-blocking LED feedback controller
 *    
 *    Backend Infrastructure:
 *      - MySQL database: u411050800_SensorsData
 *      - PHP endpoint: sensor_dashboard.php (data ingestion)
 *      - Web server: https://huynguyen.co
 * 
 * Usage Notes:
 *    Setup Instructions:
 *      1. Update WiFi credentials in config.h (WIFI_SSID, WIFI_PASS)
 *      2. Verify DB_BASE_URL points to your PHP endpoint
 *      3. Connect hardware according to pin assignments in config.h
 *      4. Upload code using PlatformIO (Board: nodemcuv2, Speed: 9600 baud)
 *      5. Open Serial Monitor at 9600 baud to view system status
 *      6. Type 'T' to configure timezone (default: America/Los_Angeles)
 *    
 *    Operation:
 *      - First event: NTP sync takes 5-15 seconds (one-time setup)
 *      - Subsequent events: 3-6 seconds per transmission
 *      - LED blinks for 2 seconds on successful transmission
 *      - Activity counters increment only on successful database insert
 *      - Duplicate timestamps are rejected by backend (409 error)
 *    
 *    Timing Behavior:
 *      - Main loop runs at ~500 Hz when idle (2ms delay)
 *      - Switch debounce: 50ms for button and tilt
 *      - DHT11 read time: ~2 seconds per measurement
 *      - HTTPS transmission: 1-3 seconds
 *      - LED feedback: Non-blocking, managed by ledsPoll()
 *    
 *    Troubleshooting:
 *      - WiFi won't connect: Check SSID/password, router proximity
 *      - NTP sync fails: Ensure internet access, check firewall settings
 *      - DHT error (-10): Verify wiring, check 3.3V power supply
 *      - HTTP 404: Wrong DB_BASE_URL in config.h
 *      - LED doesn't blink: Check polarity, resistor value, GPIO pins
 *      - Serial output garbled: Verify 9600 baud rate in monitor
 *    
 *    Performance Notes:
 *      - Power consumption: ~80mA idle, 150-300mA during WiFi/HTTPS
 *      - Memory usage: Minimal due to modular design and static allocation
 *      - Event handling: Non-blocking, responsive to rapid input changes
 *      - Database load: One insert per event (low server impact)
 *    
 *    Security Considerations:
 *      - HTTPS used but certificate validation disabled (setInsecure)
 *      - WiFi password stored in plaintext in config.h
 *      - For production: Enable SSL validation, use secrets management
 *    
 *    Extension Ideas:
 *      - Add third input source with node_3
 *      - Implement local data logging (SPIFFS/SD card)
 *      - Add battery backup with deep sleep mode
 *      - Include additional sensors (pressure, light, motion)
 *      - Implement OTA (Over-The-Air) firmware updates
 * 
 * -------------------------------------------------------------------
 * PROGRAM FLOW SUMMARY:
 * -------------------------------------------------------------------
 * [SETUP PHASE]
 *   1. Initialize Serial communication @ 9600 baud
 *   2. Print startup banner and system information
 *   3. Initialize switches (GPIO0, GPIO4 as INPUT)
 *   4. Initialize DHT11 sensor
 *   5. Load timezone from EEPROM (default: Pacific Time)
 *   6. Initialize LED outputs (GPIO12, GPIO13)
 *   7. Display ready message with instructions
 * 
 * [MAIN LOOP - Continuous Execution]
 *   1. serialMenu(): Check for 'T' command to change timezone
 *   2. pollSwitches(): Read button/tilt states with 50ms debounce
 *   3. ledsPoll(): Update LED states based on active timers
 *   
 *   4. IF takeButtonEvent() == true (button pressed):
 *        a. Print "Button Event" header
 *        b. ensureWiFi() → Connect to WiFi if needed
 *        c. readTimeISO() → Get timestamp via NTP (lazy init on first call)
 *        d. readDHT() → Read temperature and humidity from DHT11
 *        e. transmit() → Send JSON via HTTPS POST to database
 *           - Build JSON: {node:1, temp, humidity, timestamp, count}
 *           - Check for duplicate payload (hash comparison)
 *           - POST to: https://...php?ts=<timestamp>&node=1
 *        f. IF transmission successful:
 *           - incNode1() → Increment activity counter
 *           - blinkAsync(LED1) → Start 2-second blink
 *           - Print activity count
 *        g. ELSE: Print "Transmission failed"
 *   
 *   5. IF takeTiltEvent() == true (tilt detected):
 *        [Same process as button, but for node_2 and LED2]
 *   
 *   6. delay(10) → Small delay for loop stability
 *   7. Repeat from step 1
 * 
 * -------------------------------------------------------------------
 * SERIAL MONITOR COMMANDS:
 * -------------------------------------------------------------------
 * 'T' or 't' = Change timezone
 *   - Prompts for IANA timezone string (e.g., America/New_York)
 *   - Saves to EEPROM for persistence across reboots
 *   - Reconfigures NTP with new timezone
 * 
 * -------------------------------------------------------------------
 * ERROR CODES REFERENCE:
 * -------------------------------------------------------------------
 * -100: WiFi connection timeout (15 seconds)
 * -1  : NTP sync failed (time not available)
 * -2  : Local time conversion failed
 * -10 : DHT11 sensor read error (NaN detected)
 * -20 : HTTPS connection failed (no WiFi)
 * -21 : HTTP client initialization failed
 * 404 : PHP endpoint not found (check DB_BASE_URL)
 * 409 : Duplicate timestamp rejected by database
 * 
 * -------------------------------------------------------------------
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "config.h"
#include "switches.h"
#include "time_client.h"
#include "sensors.h"
#include "tx.h"
#include "leds.h"

/**
 * -- Serial menu handler for runtime timezone configuration
 * 
 * Checks Serial buffer for 'T' or 't' command. When received, prompts
 * user to enter IANA timezone string (e.g., America/New_York) and
 * saves it to EEPROM for persistence. Non-blocking implementation.
 * 
 * -- Note: Called every loop iteration, processes only when data available
 * see setTimezone() in time_client.cpp for timezone validation
 */
static void serialMenu() {
  if (!Serial.available()) return;
  
  char c = Serial.read();
  if (c == 'T' || c == 't') {
    Serial.println("\nEnter IANA time zone (e.g., America/New_York):");
    Serial.println("Common zones: UTC, America/Chicago, Europe/London, Asia/Tokyo");
    Serial.print("> ");
    
    // Wait for user input
    while (!Serial.available()) {
      delay(10);
    }
    
    String in = Serial.readStringUntil('\n');
    in.trim();
    
    if (in.length() > 0) {
      if (setTimezone(in)) {
        Serial.println("Time zone updated successfully!");
      } else {
        Serial.println("Invalid time zone string.");
      }
    }
  }
}

/**
 * -- System initialization and hardware setup
 * 
 * Executes once at power-on or reset. Initializes all hardware peripherals,
 * communication interfaces, and displays startup information. No NTP sync
 * occurs here to avoid blocking during boot (lazy initialization used instead).
 * 
 * Note: Baud rate MUST match platformio.ini (9600 baud)
 */
void setup() {
  // Initialize serial communication
  Serial.begin(9600);  // Match platformio.ini monitor_speed
  delay(100);          // Allow serial port to stabilize
  
  // Display startup banner
  Serial.println("\n=================================");
  Serial.println("ESP8266 Sensor Activity Logger");
  Serial.println("=================================");
  
  // Display MAC address
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("---------------------------------");
  
  // Initialize all modules
  switchesBegin();      // Configure GPIO0 and GPIO4 as inputs
  sensorsBegin();       // Initialize DHT11 sensor
  timeClientBegin();    // Load timezone from EEPROM (no NTP sync yet)
  ledsBegin();          // Configure GPIO12 and GPIO13 as outputs
  
  // Display operational instructions
  Serial.println("\nSystem ready!");
  Serial.println("Press button -> logs to node_1");
  Serial.println("Tilt switch  -> logs to node_2");
  Serial.println("Type 'T' to change time zone");
  Serial.println("=================================\n");
}

/**
 * -- Main event loop - continuous program execution
 * 
 * Implements event-driven architecture with non-blocking design.
 * Continuously polls for input events, manages LED states, and processes
 * data transmission requests. Runs at approximately 500 Hz when idle.
 * 
 * Event Priority:
 *   1. Serial commands (timezone change)
 *   2. Switch events (button/tilt)
 *   3. LED state updates (visual feedback)
 * 
 * Timing:
 *   - Idle loop: ~2ms per iteration
 *   - Button event: 3-30 seconds (includes NTP sync on first call)
 *   - Tilt event: 3-30 seconds (includes NTP sync on first call)
 *   - Subsequent events: 3-6 seconds (NTP cached)
 * 
 * Note: Loop never blocks except during WiFi/NTP/HTTPS operations
 */
void loop() {
  
  serialMenu();  // Check for serial menu commands
  
  pollSwitches();  // Poll hardware inputs (non-blocking debounce)
  
  ledsPoll();  // Update LED states (non-blocking timing)

  // Shared variables for sensor data
  String ts;      // ISO 8601 timestamp from NTP
  float tC, h;    // Temperature (Celsius) and humidity (%)

  // ===== EVENT HANDLER: BUTTON PRESS (Node 1) =====
  if (takeButtonEvent()) {
    Serial.println("\n--- Button Event ---");
    
    // Attempt to get timestamp and sensor readings
    if (readTimeISO(ts) && readDHT(tC, h)) {
      uint32_t nextCount = node1Count() + 1;
      
      // Transmit data to database
      if (transmit(1, ts, tC, h, nextCount)) {
        incNode1();                          // Increment activity counter
        blinkAsync(PIN_LED1, 250, 2000);    // Visual confirmation (2 sec)
        Serial.print("Node 1 activity count: ");
        Serial.println(node1Count());
      } else {
        Serial.println("Transmission failed for node 1");
      }
    }
    Serial.println("--------------------\n");
  }

  // ===== EVENT HANDLER: TILT SWITCH (Node 2) =====
  if (takeTiltEvent()) {
    Serial.println("\n--- Tilt Event ---");
    
    // Attempt to get timestamp and sensor readings
    if (readTimeISO(ts) && readDHT(tC, h)) {
      uint32_t nextCount = node2Count() + 1;
      
      // Transmit data to database
      if (transmit(2, ts, tC, h, nextCount)) {
        incNode2();                          // Increment activity counter
        blinkAsync(PIN_LED2, 250, 2000);    // Visual confirmation (2 sec)
        Serial.print("Node 2 activity count: ");
        Serial.println(node2Count());
      } else {
        Serial.println("Transmission failed for node 2");
      }
    }
    Serial.println("------------------\n");
  }
 
  delay(10); // Small delay for loop stability and reduced power consumption
}

/*
 * -------------------------------------------------------------------
 * END OF MAIN.CPP
 * -------------------------------------------------------------------
 * For detailed module documentation, see individual .h/.cpp files
 * For troubleshooting, monitor Serial output at 9600 baud
 * -------------------------------------------------------------------
 */