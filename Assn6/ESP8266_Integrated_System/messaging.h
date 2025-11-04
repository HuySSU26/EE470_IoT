// ============================================================================
// messaging.h - Slack and SMS Notification Interface
// ============================================================================
// Purpose: Send status notifications via Slack and SMS
// Features: Message buffering, retry logic, formatted status messages
// ============================================================================

#pragma once
#include <Arduino.h>

/**
 * Initialize messaging module
 */
void messagingBegin();

/**
 * Send sensor data notification to Slack
 * @param node Node identifier (1 or 2)
 * @param timestamp ISO8601 timestamp
 * @param tempC Temperature in Celsius
 * @param humidity Humidity percentage
 * @param count Activity count
 * @return true if message sent successfully
 */
bool sendSensorNotification(uint8_t node, const String& timestamp, 
                            float tempC, float humidity, uint32_t count);

/**
 * Send LED/RGB status notification to Slack
 * @param ledStatus LED status string (e.g., "LED1:ON, LED2:OFF")
 * @param rgbStatus RGB status string (e.g., "RGB(255,0,0)")
 * @return true if message sent successfully
 */
bool sendStatusNotification(const String& ledStatus, const String& rgbStatus);

/**
 * Process any pending messages in the queue
 * Call regularly from main loop
 */
void messagingPoll();

/**
 * Check if there are pending messages
 */
bool hasPendingMessages();
