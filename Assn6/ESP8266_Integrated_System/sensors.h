// ============================================================================
// sensors.h
// ============================================================================
// Purpose: DHT11 temperature/humidity sensor interface declarations
// Functions: Sensor initialization and data reading
// Hardware: DHT11 on GPIO14
// ============================================================================

#pragma once
#include <Arduino.h>

bool sensorsBegin();
bool readDHT(float& tC, float& h);