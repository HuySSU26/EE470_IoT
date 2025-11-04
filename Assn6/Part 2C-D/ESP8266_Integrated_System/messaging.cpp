// ============================================================================
// messaging.cpp - Slack and SMS Notification Implementation
// ============================================================================

#include "messaging.h"
#include "config.h"
#include "net.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// Message queue structure
struct Message {
  String content;
  uint32_t timestamp;
  uint8_t retries;
  bool pending;
};

static Message messageQueue[MAX_MESSAGE_QUEUE];
static uint8_t queueHead = 0;
static uint8_t queueTail = 0;
static uint8_t queueSize = 0;

/**
 * Initialize messaging module
 */
void messagingBegin() {
  // Clear message queue
  for (int i = 0; i < MAX_MESSAGE_QUEUE; i++) {
    messageQueue[i].pending = false;
    messageQueue[i].retries = 0;
  }
  Serial.println("[MESSAGING] Module initialized");
}

/**
 * Add message to queue
 */
static bool enqueueMessage(const String& message) {
  if (queueSize >= MAX_MESSAGE_QUEUE) {
    Serial.println("[MESSAGING] Queue full - message dropped");
    return false;
  }

  messageQueue[queueTail].content = message;
  messageQueue[queueTail].timestamp = millis();
  messageQueue[queueTail].retries = 0;
  messageQueue[queueTail].pending = true;

  queueTail = (queueTail + 1) % MAX_MESSAGE_QUEUE;
  queueSize++;

  Serial.print("[MESSAGING] Message queued (");
  Serial.print(queueSize);
  Serial.println(" pending)");
  return true;
}

/**
 * Remove message from queue
 */
static void dequeueMessage() {
  if (queueSize == 0) return;

  messageQueue[queueHead].pending = false;
  queueHead = (queueHead + 1) % MAX_MESSAGE_QUEUE;
  queueSize--;
}

/**
 * Send a message via Slack webhook
 * NOTE: You need to configure your Slack webhook URL
 * This is a placeholder - replace with actual Slack integration
 */
static bool sendSlackMessage(const String& message) {
  if (!ensureWiFi()) {
    Serial.println("[MESSAGING] No WiFi for Slack");
    return false;
  }

  // TODO: Replace with your actual Slack webhook URL
  const char* SLACK_WEBHOOK = "YOUR_SLACK_WEBHOOK_URL";
  
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(10000);

  if (!http.begin(client, SLACK_WEBHOOK)) {
    Serial.println("[MESSAGING] Slack HTTP begin failed");
    return false;
  }

  http.addHeader("Content-Type", "application/json");

  // Format Slack payload
  String payload = "{\"text\":\"";
  payload += message;
  payload += "\"}";

  int code = http.POST(payload);
  bool success = (code == HTTP_CODE_OK || code == 200);

  if (success) {
    Serial.println("[MESSAGING] ✓ Slack message sent");
  } else {
    Serial.print("[MESSAGING] ✗ Slack error: ");
    Serial.println(code);
  }

  http.end();
  return success;
}

/**
 * Send sensor data notification
 */
bool sendSensorNotification(uint8_t node, const String& timestamp,
                            float tempC, float humidity, uint32_t count) {
  String message = "🌡️ Sensor Reading - Node ";
  message += String(node);
  message += "\\n";
  message += "Time: " + timestamp + "\\n";
  message += "Temperature: " + String(tempC, 1) + "°C\\n";
  message += "Humidity: " + String(humidity, 1) + "%\\n";
  message += "Activity Count: " + String(count);

  Serial.println("[MESSAGING] Sensor notification:");
  Serial.println(message);

  return enqueueMessage(message);
}

/**
 * Send LED/RGB status notification
 */
bool sendStatusNotification(const String& ledStatus, const String& rgbStatus) {
  String message = "💡 Status Check\\n";
  message += ledStatus + "\\n";
  message += rgbStatus;

  Serial.println("[MESSAGING] Status notification:");
  Serial.println(message);

  return enqueueMessage(message);
}

/**
 * Process pending messages
 */
void messagingPoll() {
  if (queueSize == 0) return;

  // Get the oldest pending message
  Message& msg = messageQueue[queueHead];
  if (!msg.pending) {
    dequeueMessage();
    return;
  }

  // Attempt to send
  if (sendSlackMessage(msg.content)) {
    Serial.print("[MESSAGING] ✓ Message sent (");
    Serial.print(queueSize - 1);
    Serial.println(" remaining)");
    dequeueMessage();
  } else {
    msg.retries++;
    if (msg.retries >= 3) {
      Serial.println("[MESSAGING] ✗ Message failed after 3 retries - dropping");
      dequeueMessage();
    } else {
      Serial.print("[MESSAGING] Retry ");
      Serial.print(msg.retries);
      Serial.println("/3");
      // Wait before retry
      delay(1000);
    }
  }
}

/**
 * Check for pending messages
 */
bool hasPendingMessages() {
  return (queueSize > 0);
}
