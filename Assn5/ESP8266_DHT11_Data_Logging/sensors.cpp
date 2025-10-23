// ============================================================================
// sensors.cpp
// ============================================================================
// Purpose: DHT11 sensor driver implementation
// Library: Adafruit DHT sensor library
// Output: Temperature (Celsius) and relative humidity (percentage)
// Error handling: NaN detection and error reporting
// ============================================================================

#include "sensors.h"
#include "config.h"
#include "errors.h"
#include <DHT.h>

static DHT dht(PIN_DHT, DHTTYPE);

bool sensorsBegin() {
  dht.begin();
  return true;
}

bool readDHT(float& tC, float& h) {
  h = dht.readHumidity();
  tC = dht.readTemperature();
  
  if (isnan(h) || isnan(tC)) {
    reportError("dht/read", -10);
    return false;
  }
  
  Serial.print("[DHT] T=");
  Serial.print(tC);
  Serial.print("°C, H=");
  Serial.print(h);
  Serial.println("%");
  
  return true;
}