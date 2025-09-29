//----------------------
// Title: Smart_Light_Sensor_RGB_Buzzer_Controller_PlatformIO
//----------------------
// Program Detail:
//----------------------
// Purpose: Monitors ambient light intensity with three distinct modes:
//          - Dark: Buzzer activation
//          - Normal: White LED  
//          - Very Bright: White-to-Red gradient based on intensity
//          Designed for PlatformIO with Visual Studio Code
// Inputs: - Light sensor (HW-486) on analog pin A0 (INVERTED logic)
//         - Serial commands: 'B' or 'b' for manual buzzer activation
// Outputs: - RGB LED: White (normal), White→Red gradient (very bright)
//          - Active buzzer: Activated for dark conditions
//          - Serial monitor: Real-time ADC values and system status
// Date: September 26, 2025
// Compiler: PlatformIO with Visual Studio Code
// Author: Huy Nguyen
// Version: 1.0 - Original code for ARduino IDE
//          2.0 - PlatformIO Edition with inverted sensor logic
//			3.0 - Enhanced light sensor code with filtering and improved precision
//			4.0 - Calibrated using linear regression against reference lux meter
// 				  Calibration equation: Lux = 1678.25 - 2500 × V
//                Based on measured data with R² = 0.735
// Hardware Configuration:
// - ESP8266 (NodeMCU)
// - HW-486 Light Sensor Module → A0 (INVERTED: Lower ADC = Brighter light)
// - HW-479 RGB LED Module → D5(Red), D6(Green), D7(Blue)
// - HW-512 Active Buzzer Module → D8
//
//----------------------
// PlatformIO Configuration (platformio.ini):
//----------------------
// [env:nodemcuv2]
// platform = espressif8266
// board = nodemcuv2
// framework = arduino
// monitor_speed = 9600
// upload_port = COM13 
// monitor_port = COM13
//
//----------------------
// File Dependencies:
//----------------------
// #include <Arduino.h>  // Required for PlatformIO (auto-included)
// No external libraries required - uses built-in Arduino functions
//
//----------------------
// Circuit Connections:
//----------------------
// HW-486 Light Sensor:
//   VCC → 3.3V
//   GND → GND
//   S   → A0 (analog input)
//
// HW-479 RGB LED Module:
//   VCC → 3.3V or 5V
//   GND → GND
//   R   → D5 (GPIO14)
//   G   → D6 (GPIO12)
//   B   → D7 (GPIO13)
//
// HW-512 Active Buzzer:
//   VCC → 3.3V
//   GND → GND
//   I/O → D8 (GPIO15)
//
//----------------------
// Main Program:
//----------------------
//----------------------
// Calibrated using linear regression against reference lux meter
// Calibration equation: Lux = 1678.25 - 2500 × V
// Based on measured data with R² = 0.735
//----------------------
#include <Arduino.h>  // Required for PlatformIO

// Pin definitions for ESP8266
#define LIGHT_SENSOR A0        // HW-486 light sensor (inverted logic)
#define RED_LED   14           // GPIO14 (D5)- Red LED
#define GREEN_LED 12           // GPIO12 (D6)- Green LED  
#define BLUE_LED  13           // GPIO13 (D7)- Blue LED
#define BUZZER    15           // GPIO15 (D8)- Active buzzer

// Calibration constants from linear regression
// From graph: V = -0.0004 × Lux + 0.6713
// Solving for Lux: Lux = (0.6713 - V) / 0.0004
const float CALIB_INTERCEPT = 0.6713;    // V-intercept from graph
const float CALIB_SLOPE = 0.0004;        // Absolute value of slope
// To solve: Lux = (Intercept - V) / Slope

// Lighting thresholds (in calibrated Lux)
const float DARK_THRESHOLD = 300.0;      // Below 300 lux = dark
const float NORMAL_MIN = 300.0;          // 300-800 lux = normal
const float NORMAL_MAX = 800.0;
const float BRIGHT_THRESHOLD = 800.0;    // Above 800 lux = bright

const int BUZZER_DURATION = 5000;

// Filtering
const int FILTER_SIZE = 10;
const int OVERSAMPLE_COUNT = 5;

float luxReadings[FILTER_SIZE];
int filterIndex = 0;
bool filterFilled = false;

// Function declarations
void setRGBColor(int red, int green, int blue);
void displaySystemStatus(float voltage, float rawLux, float filteredLux, float trueLux, String condition);
void handleSerialCommands();
void printWelcomeMessage();
float getCalibratedLux(float voltage);
float getFilteredVoltage();
float getMovingAverageLux(float newLux);

// Get filtered voltage reading with oversampling
float getFilteredVoltage() {
  long adcSum = 0;
  
  for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
    adcSum += analogRead(LIGHT_SENSOR);
    delayMicroseconds(100);
  }
  
  float avgADC = (float)adcSum / OVERSAMPLE_COUNT;
  float voltage = (avgADC / 1023.0) * 1.0;  // ESP8266: 0-1023 = 0-1.0V
  
  return voltage;
}

// Convert voltage to calibrated Lux using linear regression
float getCalibratedLux(float voltage) {
  // From graph: V = -0.0004 × Lux + 0.6713
  // Solving for Lux: Lux = (0.6713 - V) / 0.0004
  
  float lux = (CALIB_INTERCEPT - voltage) / CALIB_SLOPE;
  
  // Debug output for troubleshooting
  if (lux < 0) {
    Serial.print(" [Negative lux: ");
    Serial.print(lux);
    Serial.print("] ");
    lux = 0;
  }
  
  // Allow up to 5000 lux
  if (lux > 5000) {
    lux = 5000;
  }
  
  return lux;
}

// Moving average filter
float getMovingAverageLux(float newLux) {
  luxReadings[filterIndex] = newLux;
  filterIndex = (filterIndex + 1) % FILTER_SIZE;
  
  if (!filterFilled && filterIndex == 0) {
    filterFilled = true;
  }
  
  float sum = 0;
  int count = filterFilled ? FILTER_SIZE : filterIndex;
  
  for (int i = 0; i < count; i++) {
    sum += luxReadings[i];
  }
  
  return sum / count;
}

void setRGBColor(int red, int green, int blue) {
  red = constrain(red, 0, 255);
  green = constrain(green, 0, 255);
  blue = constrain(blue, 0, 255);
  
  analogWrite(RED_LED, red);
  analogWrite(GREEN_LED, green);
  analogWrite(BLUE_LED, blue);
}

void displaySystemStatus(float voltage, float rawLux, float filteredLux, float trueLux, String condition) {
  Serial.print("V: ");
  Serial.print(voltage, 4);
  Serial.print("V | Raw: ");
  Serial.print(rawLux, 1);
  Serial.print(" | Filtered: ");
  Serial.print(filteredLux, 1);
  Serial.print(" | Calibrated: ");
  Serial.print(trueLux, 1);
  Serial.print(" lux | Status: ");
  Serial.println(condition);
}

void handleSerialCommands() {
  if (Serial.available()) {
    char command = Serial.read();
    while (Serial.available()) Serial.read();
    
    if (command == 'B' || command == 'b') {
      Serial.println("Manual buzzer activation (5 seconds)...");
      digitalWrite(BUZZER, HIGH);
      delay(BUZZER_DURATION);
      digitalWrite(BUZZER, LOW);
      Serial.println("Manual buzzer deactivated.");
    } else if (command == 'C' || command == 'c') {
      Serial.println();
      Serial.println("Calibration Info:");
      Serial.print("  Equation: Lux = (");
      Serial.print(CALIB_INTERCEPT);
      Serial.print(" - V) / ");
      Serial.println(CALIB_SLOPE);
      Serial.println("  Based on linear regression (R² = 0.735)");
      Serial.println();
    }
  }
}

void printWelcomeMessage() {
  Serial.println();
  Serial.println("//================================================");
  Serial.println("// Calibrated Light Sensor System");
  Serial.println("// Using Linear Regression Calibration");
  Serial.println("//================================================");
  Serial.println();
  Serial.println("Calibration Equation:");
  Serial.print("  Lux = (");
  Serial.print(CALIB_INTERCEPT);
  Serial.print(" - V) / ");
  Serial.println(CALIB_SLOPE);
  Serial.print("  From: V = -0.0004 × Lux + 0.6713");
  Serial.println();
  Serial.println();
  Serial.println("Thresholds:");
  Serial.print("  Dark: < ");
  Serial.print(DARK_THRESHOLD);
  Serial.println(" lux → Buzzer ON");
  Serial.print("  Normal: ");
  Serial.print(NORMAL_MIN);
  Serial.print("-");
  Serial.print(NORMAL_MAX);
  Serial.println(" lux → White LED");
  Serial.print("  Bright: > ");
  Serial.print(BRIGHT_THRESHOLD);
  Serial.println(" lux → White→Red gradient");
  Serial.println();
  Serial.println("Commands: 'B' = buzzer test, 'C' = show calibration");
  Serial.println();
  Serial.println("Output: Voltage | Raw | Filtered | Calibrated Lux");
  Serial.println("==================================================");
}

void setup() {
  Serial.begin(9600);
  delay(100);
  
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  digitalWrite(BUZZER, LOW);
  setRGBColor(0, 0, 0);
  
  for (int i = 0; i < FILTER_SIZE; i++) {
    luxReadings[i] = 0;
  }
  
  printWelcomeMessage();
}

void loop() {
  // Get filtered voltage reading
  float voltage = getFilteredVoltage();
  
  // Convert to calibrated Lux
  float calibratedLux = getCalibratedLux(voltage);
  
  // Apply moving average filter
  float filteredLux = getMovingAverageLux(calibratedLux);
  
  // For display, also show raw calculation (legacy)
  float rawLux = calibratedLux; // Same as calibrated now
  
  // Use calibrated filtered value for decisions
  digitalWrite(BUZZER, LOW);
  
  if (filteredLux < DARK_THRESHOLD) {
    // Dark condition
    setRGBColor(0, 0, 0);
    digitalWrite(BUZZER, HIGH);
    displaySystemStatus(voltage, rawLux, filteredLux, filteredLux, "DARK - Buzzer ON");
    
  } else if (filteredLux <= NORMAL_MAX) {
    // Normal lighting
    setRGBColor(255, 255, 255);
    displaySystemStatus(voltage, rawLux, filteredLux, filteredLux, "NORMAL - White LED ON");
    
  } else {
    // Bright light - white to red gradient
    // Map lux 800-2000 to intensity 0-255
    int intensity = map(filteredLux, BRIGHT_THRESHOLD, 2000, 0, 255);
    intensity = constrain(intensity, 0, 255);
    
    int redLevel = 255;
    int greenLevel = 255 - intensity;
    int blueLevel = 255 - intensity;
    
    setRGBColor(redLevel, greenLevel, blueLevel);
    
    String gradientInfo = "BRIGHT - White→Red (";
    gradientInfo += (int)filteredLux;
    gradientInfo += " lux)";
    displaySystemStatus(voltage, rawLux, filteredLux, filteredLux, gradientInfo);
  }
  
  handleSerialCommands();
  delay(1000);
}
